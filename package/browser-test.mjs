// Tests the assembled npm package's browser entry in headless Chrome, in a
// page and in a module Web Worker: node package/browser-test.mjs build/package
//
// The page imports index.mjs, which loads persona.wasm next to itself (no
// bundler), and checks a known answer from persona-generator's tests.
import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { mkdtemp, readFile, rm } from 'node:fs/promises';
import http from 'node:http';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { setTimeout as delay } from 'node:timers/promises';

const dir = path.resolve(process.argv[2] || 'build/package');
const chromePath = process.env.CHROME_PATH || (process.platform === 'win32'
  ? 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe'
  : 'google-chrome');

const check = `
  const p = personas.generate({ seed: 1 });
  const answer = p.name + ' ' + p.signature.seed + '-' + p.signature.data;`;
const pages = {
  '/test.html': `<!doctype html><meta charset="utf-8"><script type="module">
    window.result = (async () => {
      const { createPersonaGenerator } = await import('./index.mjs');
      const personas = await createPersonaGenerator();${check}
      const worker = new Worker('./worker.mjs', { type: 'module' });
      const fromWorker = await new Promise((resolve, reject) => {
        worker.onmessage = event => resolve(event.data);
        worker.onerror = event => reject(new Error(event.message || 'worker failed'));
      });
      return 'page: ' + answer + ' | worker: ' + fromWorker;
    })().catch(error => 'FAIL ' + error.message);
  </script>`,
  '/worker.mjs': `import { createPersonaGenerator } from './index.mjs';
    try {
      const personas = await createPersonaGenerator();${check}
      postMessage(answer);
    } catch (error) { postMessage('FAIL ' + error.message); }`
};
const types = { '.html': 'text/html', '.mjs': 'text/javascript', '.wasm': 'application/wasm' };

const server = http.createServer(async (request, response) => {
  const pathname = new URL(request.url, 'http://localhost').pathname;
  const type = types[path.extname(pathname)] || 'application/octet-stream';
  if (pages[pathname]) return response.writeHead(200, { 'Content-Type': type }).end(pages[pathname]);
  const file = path.resolve(dir, '.' + pathname);
  if (!file.startsWith(dir + path.sep)) return response.writeHead(403).end();
  let data;
  try {
    data = await readFile(file);
  } catch {
    return response.writeHead(404).end();
  }
  response.writeHead(200, { 'Content-Type': type }).end(data);
});
await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));

const profile = await mkdtemp(path.join(tmpdir(), 'persona-browser-'));
const chrome = spawn(chromePath, ['--headless=new', '--disable-gpu', '--no-first-run', '--no-sandbox',
  '--remote-debugging-port=0', `--user-data-dir=${profile}`, 'about:blank'], { stdio: 'ignore' });
let socket;
try {
  let port;
  for (let attempt = 0; !port && attempt < 200; ++attempt) {
    try {
      port = Number((await readFile(path.join(profile, 'DevToolsActivePort'), 'utf8')).split('\n')[0]);
    } catch {
      await delay(100);
    }
  }
  assert.ok(port, 'Chrome did not start');
  const targets = await (await fetch(`http://127.0.0.1:${port}/json`)).json();
  socket = new WebSocket(targets.find(target => target.type === 'page').webSocketDebuggerUrl);
  await new Promise((resolve, reject) => {
    socket.addEventListener('open', resolve, { once: true });
    socket.addEventListener('error', reject, { once: true });
  });
  let sequence = 0;
  const pending = new Map();
  socket.addEventListener('message', event => {
    const message = JSON.parse(event.data);
    pending.get(message.id)?.(message);
    pending.delete(message.id);
  });
  const send = (method, params = {}) => new Promise(resolve => {
    const id = ++sequence;
    pending.set(id, resolve);
    socket.send(JSON.stringify({ id, method, params }));
  });

  await send('Page.navigate', { url: `http://127.0.0.1:${server.address().port}/test.html` });
  let text;
  for (let attempt = 0; text === undefined && attempt < 600; ++attempt) {
    const { result } = await send('Runtime.evaluate', {
      expression: 'window.result', awaitPromise: true, returnByValue: true
    });
    text = result?.result?.value;
    if (text === undefined) await delay(100);
  }
  const answer = 'Brandon White 0000000000000001-16ccfc2fe06c547b';
  assert.equal(text, `page: ${answer} | worker: ${answer}`);
  console.log('PASS: @dasmig/persona browser entry loads and matches the known answer in a page and a module worker.');
} finally {
  socket?.close();
  chrome.kill();
  server.close();
  await delay(500);
  await rm(profile, { recursive: true, force: true }).catch(() => {});
}
