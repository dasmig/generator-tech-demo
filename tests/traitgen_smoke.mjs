import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { mkdtemp, readFile, rm } from 'node:fs/promises';
import http from 'node:http';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { setTimeout as delay } from 'node:timers/promises';

const site = path.resolve(process.argv[2] || '.');
const chrome = process.env.CHROME_PATH || (process.platform === 'win32'
  ? 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe'
  : 'google-chrome');
const profile = await mkdtemp(path.join(tmpdir(), 'traitgen-smoke-'));
const server = http.createServer(async (request, response) => {
  const pathname = new URL(request.url, 'http://localhost').pathname;
  const file = path.resolve(site, '.' + (pathname === '/' ? '/index.html' : pathname));
  if (!file.startsWith(site + path.sep)) {
    response.writeHead(403).end();
    return;
  }
  try {
    const data = await readFile(file);
    const type = { '.html': 'text/html', '.js': 'text/javascript', '.wasm': 'application/wasm' };
    response.writeHead(200, { 'Content-Type': type[path.extname(file)] || 'application/octet-stream' });
    response.end(data);
  } catch (error) {
    if (error.code !== 'ENOENT') console.error(error);
    response.writeHead(error.code === 'ENOENT' ? 404 : 500).end();
  }
});
let browser;
let socket;
const pending = new Map();
let sequence = 0;

async function waitFor(check, description) {
  for (let attempt = 0; attempt < 300; ++attempt) {
    if (await check()) return;
    await delay(100);
  }
  throw new Error('Timed out: ' + description);
}

function send(method, params = {}) {
  const id = ++sequence;
  return new Promise((resolve, reject) => {
    const timeout = setTimeout(() => {
      pending.delete(id);
      reject(new Error('Timed out: ' + method));
    }, 30000);
    pending.set(id, {
      resolve: value => { clearTimeout(timeout); resolve(value); },
      reject: error => { clearTimeout(timeout); reject(error); }
    });
    socket.send(JSON.stringify({ id, method, params }));
  });
}

async function evaluate(expression) {
  const result = await send('Runtime.evaluate', {
    expression, returnByValue: true, awaitPromise: true
  });
  if (result.exceptionDetails) throw new Error(JSON.stringify(result.exceptionDetails));
  return result.result.value;
}

try {
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  browser = spawn(chrome, [
    '--headless=new', '--disable-gpu', '--no-sandbox', '--remote-debugging-port=0',
    '--user-data-dir=' + profile, 'about:blank'
  ], { stdio: 'ignore' });
  let launchError;
  browser.once('error', error => { launchError = error; });
  let port;
  await waitFor(async () => {
    if (launchError) throw launchError;
    try {
      port = Number((await readFile(path.join(profile, 'DevToolsActivePort'), 'utf8')).split('\n')[0]);
      return true;
    } catch (error) {
      if (error.code !== 'ENOENT') throw error;
      return false;
    }
  }, 'Chrome startup');
  const targets = await (await fetch(`http://127.0.0.1:${port}/json`)).json();
  socket = new WebSocket(targets.find(target => target.type === 'page').webSocketDebuggerUrl);
  await new Promise((resolve, reject) => {
    socket.addEventListener('open', resolve, { once: true });
    socket.addEventListener('error', reject, { once: true });
  });
  socket.addEventListener('message', event => {
    const message = JSON.parse(event.data);
    const request = pending.get(message.id);
    if (request) {
      pending.delete(message.id);
      if (message.error) request.reject(new Error(JSON.stringify(message.error)));
      else request.resolve(message.result);
    }
    if (message.method === 'Fetch.requestPaused') {
      send('Fetch.failRequest', { requestId: message.params.requestId, errorReason: 'Failed' })
        .catch(error => console.error(error));
    }
  });
  await send('Page.enable');
  await send('Emulation.setDeviceMetricsOverride', {
    width: 1280, height: 900, deviceScaleFactor: 1, mobile: false
  });
  await send('Page.navigate', { url: `http://127.0.0.1:${server.address().port}/` });
  await waitFor(() => evaluate('typeof tgMod !== "undefined" && tgMod !== null'), 'trait module');
  assert.equal(await evaluate('tgMod.init()'), true);
  const generate = (count, seed) => evaluate(`JSON.parse(tgMod.generateTraits(${count}, ${JSON.stringify(seed)}))`);
  const axes = ['openness', 'conscientiousness', 'extraversion', 'agreeableness', 'neuroticism'];
  for (const count of [1, 3, 5, 8]) {
    for (const seed of ['0', '1', 'ffffffffffffffff']) {
      const result = await generate(count, seed);
      assert.equal(result.traits.length, count);
      assert.equal(new Set(result.traits.map(trait => trait.key)).size, count);
      const groups = result.traits.map(trait => trait.group).filter(Boolean);
      assert.equal(new Set(groups).size, groups.length);
      assert.deepEqual(Object.keys(result.axes), axes);
      assert.ok(Object.values(result.axes).every(Number.isFinite));
      assert.match(result.signature.seed, /^[0-9a-f]{16}$/);
      assert.deepEqual(await generate(count, result.signature.seed), result);
    }
    const random = await generate(count, '');
    assert.deepEqual(await generate(count, random.signature.seed), random);
  }
  assert.deepEqual((await generate(3, '1')).traits.map(trait => trait.key), ['calm', 'resilient', 'traditional']);
  assert.equal((await generate(3, 'FFFFFFFFFFFFFFFF')).signature.seed, 'ffffffffffffffff');
  for (const count of [-1, 0, 53]) assert.match((await generate(count, '1')).error, /count/);
  for (const seed of ['xyz', '-1', '0x1', ' 1', '10000000000000000']) {
    assert.match((await generate(3, seed)).error, /hexadecimal/);
  }
  await evaluate(`document.getElementById('tg-count').value = '5';
    document.getElementById('tg-seed').value = 'ffffffffffffffff';
    document.getElementById('tg-seed').focus()`);
  await send('Input.dispatchKeyEvent', {
    type: 'keyDown', key: 'Enter', code: 'Enter', windowsVirtualKeyCode: 13,
    text: '\r', unmodifiedText: '\r'
  });
  await send('Input.dispatchKeyEvent', { type: 'keyUp', key: 'Enter', code: 'Enter', windowsVirtualKeyCode: 13 });
  await waitFor(() => evaluate('document.querySelectorAll("#tg-output tbody tr").length === 7'), 'keyboard generation');
  const rows = await evaluate('Array.from(document.querySelectorAll("#tg-output tbody tr"), row => Array.from(row.cells, cell => cell.textContent))');
  assert.equal(rows[0][1], (await generate(5, 'ffffffffffffffff')).traits.map(trait => trait.label).join(', '));
  assert.equal(rows[6][1], 'ffffffffffffffff');
  assert.ok(rows.slice(1, 6).every(row => /^-?\d+\.\d{3}$/.test(row[1])));
  await evaluate(`document.getElementById('tg-seed').value = 'xyz';
    document.getElementById('tg-form').dispatchEvent(new Event('submit', { cancelable: true }))`);
  assert.match(await evaluate('document.getElementById("tg-status").textContent'), /Generation failed:.*hexadecimal/);
  await evaluate(`document.getElementById('tg-seed').value = '1';
    document.getElementById('tg-generate').click()`);
  assert.match(await evaluate('document.getElementById("tg-status").className'), /ready/);
  for (const width of [375, 320]) {
    await send('Emulation.setDeviceMetricsOverride', {
      width, height: 812, deviceScaleFactor: 1, mobile: true
    });
    assert.ok(await evaluate(`Array.from(document.querySelectorAll('#traitgen-card, #traitgen-card .controls, #tg-output table'),
      element => element.getBoundingClientRect()).every(rect => rect.left >= 0 && rect.right <= innerWidth)`),
    `Trait card overflows at ${width}px`);
  }
  await waitFor(() => evaluate('Array.from(document.querySelectorAll(".status")).every(status => status.classList.contains("ready"))'), 'all existing modules');
  await send('Fetch.enable', { patterns: [{ urlPattern: '*traitgen_demo.wasm*' }] });
  await send('Page.reload', { ignoreCache: true });
  await waitFor(() => evaluate('document.getElementById("tg-status")?.classList.contains("error")'), 'module failure status');
  assert.equal(await evaluate('document.getElementById("tg-generate").disabled'), true);
  assert.match(await evaluate('document.getElementById("tg-status").textContent'), /Reload the page/);
  assert.equal(await evaluate('document.getElementById("tg-output").textContent.trim()'), 'Trait generator unavailable.');
  console.log('PASS: real Wasm data, counts, Big Five, seed replay, errors, keyboard, mobile layout and existing modules.');
} finally {
  if (browser?.pid && browser.exitCode === null) {
    const exited = new Promise(resolve => browser.once('exit', resolve));
    if (socket?.readyState === WebSocket.OPEN) await send('Browser.close');
    else browser.kill();
    await exited;
  }
  socket?.close();
  server.closeAllConnections();
  await new Promise(resolve => server.close(resolve));
  await rm(profile, { recursive: true, force: true, maxRetries: 5, retryDelay: 200 });
}
