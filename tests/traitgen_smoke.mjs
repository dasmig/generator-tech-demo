import assert from 'node:assert/strict';
import { spawn } from 'node:child_process';
import { mkdir, mkdtemp, readFile, rm, writeFile } from 'node:fs/promises';
import http from 'node:http';
import { tmpdir } from 'node:os';
import path from 'node:path';
import { setTimeout as delay } from 'node:timers/promises';
import { verifyRequestApis } from './request_api_checks.mjs';

const site = path.resolve(process.argv[2] || '.');
const libraries = path.resolve(process.argv[3] || path.join(site, '..'));
const screenshots = process.env.DEMO_SCREENSHOT_DIR;
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
let mockBiodata = false;
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

async function catalogue(library, filename) {
  const text = await readFile(path.join(libraries, library, 'resources', 'lite', filename), 'utf8');
  const lines = text.trim().split(/\r?\n/).filter(line => line && !line.startsWith('#'));
  const headers = lines.shift().split('\t');
  return lines.map(line => Object.fromEntries(line.split('\t').map((value, index) => [headers[index], value])));
}

async function capture(name, target) {
  if (!screenshots) return;
  await mkdir(screenshots, { recursive: true });
  await evaluate(`document.querySelector(${JSON.stringify(target)}).scrollIntoView({behavior:'instant', block:'start'})`);
  const image = await send('Page.captureScreenshot', { format: 'png', captureBeyondViewport: false });
  await writeFile(path.join(screenshots, name + '.png'), Buffer.from(image.data, 'base64'));
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
      const requestId = message.params.requestId;
      const response = mockBiodata
        ? send('Fetch.fulfillRequest', { requestId, responseCode: 200,
          responseHeaders: [{ name: 'Content-Type', value: 'text/javascript' }],
          body: Buffer.from('var createBiodatagenModule = () => Promise.resolve({ init: () => false });').toString('base64') })
        : send('Fetch.failRequest', { requestId, errorReason: 'Failed' });
      response
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
  const loadings = await catalogue('trait-generator', 'loadings.tsv');
  for (const count of [1, 3, 5, 8]) {
    for (const seed of ['0', '1', 'ffffffffffffffff']) {
      const result = await generate(count, seed);
      assert.equal(result.traits.length, count);
      assert.equal(new Set(result.traits.map(trait => trait.key)).size, count);
      const groups = result.traits.map(trait => trait.group).filter(Boolean);
      assert.equal(new Set(groups).size, groups.length);
      assert.deepEqual(Object.keys(result.axes), axes);
      assert.ok(Object.values(result.axes).every(Number.isFinite));
      assert.equal(result.explanation.axes.length, 5);
      assert.equal(result.explanation.traits.length, count);
      for (let axis = 0; axis < axes.length; ++axis) {
        const explanation = result.explanation.axes[axis];
        const effects = result.explanation.traits.map((trait, index) => {
          const expected = Number(loadings.find(row =>
            row.trait === result.traits[index].key && row.axis === axes[axis])?.loading || 0);
          assert.ok(Math.abs(trait.effects[axis] - expected) < 1e-12, 'Trait effect differs from pinned catalogue');
          return trait.effects[axis];
        });
        assert.ok(Math.abs(explanation.effect - effects.reduce((sum, value) => sum + value, 0)) < 1e-12);
        assert.ok(Math.abs(explanation.base + explanation.effect - result.axes[axes[axis]]) < 1e-12);
        assert.equal(explanation.value, result.axes[axes[axis]]);
      }
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
  await waitFor(() => evaluate('document.querySelectorAll("#tg-output > table tbody tr").length === 7'), 'keyboard generation');
  const rows = await evaluate('Array.from(document.querySelectorAll("#tg-output > table tbody tr"), row => Array.from(row.cells, cell => cell.textContent))');
  assert.equal(rows[0][1], (await generate(5, 'ffffffffffffffff')).traits.map(trait => trait.label).join(', '));
  assert.equal(rows[6][1], 'ffffffffffffffff');
  assert.ok(rows.slice(1, 6).every(row => /^-?\d+\.\d{3}$/.test(row[1])));
  await evaluate(`document.getElementById('tg-seed').value = 'xyz';
    document.getElementById('tg-form').dispatchEvent(new Event('submit', { cancelable: true }))`);
  assert.match(await evaluate('document.getElementById("tg-status").textContent'), /Generation failed:.*hexadecimal/);
  await evaluate(`document.getElementById('tg-seed').value = '1';
    document.getElementById('tg-generate').click()`);
  assert.match(await evaluate('document.getElementById("tg-status").className'), /ready/);
  await waitFor(() => evaluate('Array.from(document.querySelectorAll(".status")).every(status => status.classList.contains("ready"))'), 'all modules');
  await verifyRequestApis((module, method, args) =>
    evaluate(`JSON.parse(${module}.${method}(...${JSON.stringify(args)}))`));

  for (const mode of ['full', 'first', 'surname']) {
    const names = await evaluate(`JSON.parse(ngMod.generateNameOptions('female', 'br', '${mode}', 10))`);
    assert.equal(names.length, 10);
    assert.ok(names.every(name => name.name.length > 0 && name.parts.length === (mode === 'full' ? 2 : 1)));
    assert.ok(names.every(name => name.culture === 'brazilian'));
    await evaluate(`byId('ng-mode').value = '${mode}'; byId('ng-mode').dispatchEvent(new Event('change'));
      byId('ng-generate').click()`);
    assert.equal(await evaluate('byId("ng-gender").disabled'), mode === 'surname');
    assert.equal(await evaluate('document.querySelectorAll("#ng-output li").length'), 5);
  }
  for (const count of [0, 11]) {
    assert.match(await evaluate(`JSON.parse(ngMod.generateNameOptions('', '', 'full', ${count})).error`), /alternatives/);
    assert.match(await evaluate(`JSON.parse(nngMod.generateNicknameOptions('', ${count})).error`), /alternatives/);
  }
  assert.match(await evaluate(`JSON.parse(ngMod.generateNameOptions('', '', 'fake-mode', 1)).error`), /name/);
  for (const name of ['', 'José García', '<img src=x onerror=alert(1)>']) {
    const values = await evaluate(`JSON.parse(nngMod.generateNicknameOptions(${JSON.stringify(name)}, 10))`);
    assert.equal(values.length, 10);
    assert.ok(values.every(value => value.nickname && value.source));
    await evaluate(`byId('nng-name').value = ${JSON.stringify(name)}; byId('nng-generate').click()`);
    assert.equal(await evaluate('document.querySelectorAll("#nng-output li").length'), 5);
    assert.equal(await evaluate('document.querySelector("#nng-output img")'), null);
  }

  for (const reference of ['1900-01-01', '2024-02-29', '2023-12-31', '2024-01-01', '2100-12-31']) {
    const births = await evaluate(`Array.from({length: 30}, () => JSON.parse(bgMod.generateBirthAt('US', 'female', '${reference}')))`);
    for (const birth of births) {
      assert.equal(birth['Reference Date'], reference);
      assert.equal(birth.Country, 'US');
      assert.equal(birth.Sex, 'Female');
      const [year, month, day] = birth.Date.split('-').map(Number);
      const [ry, rm, rd] = reference.split('-').map(Number);
      assert.equal(birth.Age, ry - year - Number(month > rm || (month === rm && day > rd)));
      const date = new Date(Date.UTC(year, month - 1, day));
      assert.equal(date.toISOString().slice(0, 10), birth.Date);
    }
  }
  for (const date of ['', '2023-02-29', '2024-02-30', '1899-12-31', '2101-01-01', '2024-13-01', '2024-0x-01']) {
    assert.match(await evaluate(`JSON.parse(bgMod.generateBirthAt('US', '', ${JSON.stringify(date)})).error`), /date/);
  }
  await evaluate(`byId('bg-reference').value = '2024-02-29'; byId('bg-generate').click()`);
  assert.match(await evaluate('byId("bg-output").textContent'), /2024-02-29/);
  await evaluate(`byId('bg-reference').value = ''; byId('bg-generate').click()`);
  assert.match(await evaluate('byId("bg-status").textContent'), /Generation failed/);
  await evaluate(`byId('bg-reference').value = '2024-02-29'; byId('bg-generate').click()`);

  for (const country of ['', 'US', 'BR']) {
    const biodata = await evaluate(`JSON.parse(bdMod.compareBiodata('${country}'))`);
    assert.deepEqual(biodata.map(value => value.sex), ['male', 'female']);
    assert.equal(biodata[0].country_code, biodata[1].country_code);
    if (country) assert.equal(biodata[0].country_code, country);
    assert.equal(biodata[0].signature.seed, biodata[1].signature.seed);
    assert.ok(biodata.every(value => Number.isFinite(value.height_cm) && Number.isFinite(value.weight_kg)));
    for (const sex of ['', 'male', 'female']) {
      const interests = await evaluate(`JSON.parse(igMod.compareAges('${country}', '${sex}'))`);
      assert.deepEqual(interests.map(value => value.age), [8, 30, 72]);
      assert.equal(new Set(interests.map(value => value.interest.country_code)).size, 1);
      if (country) assert.equal(interests[0].interest.country_code, country);
      assert.equal(new Set(interests.map(value => value.interest.signature.seed)).size, 1);
      assert.equal(new Set(interests.map(value => value.interest.religion)).size, 1);
      for (const value of interests) {
        for (const list of ['hobbies', 'films', 'games', 'books', 'music']) {
          assert.equal(new Set(value.interest[list].map(item => item.key)).size, value.interest[list].length);
        }
      }
    }
  }
  for (const country of ['', 'US', 'ET']) {
    for (const sex of ['', 'male', 'female']) {
      const jobs = await evaluate(`JSON.parse(ogMod.compareEducation('${country}', '${sex}'))`);
      assert.deepEqual(jobs.map(value => value.education), ['less_than_basic', 'basic', 'intermediate', 'advanced']);
      for (const field of ['country_code', 'sex']) {
        assert.equal(new Set(jobs.map(value => value.occupation[field])).size, 1);
      }
      if (country) assert.equal(jobs[0].occupation.country_code, country);
      assert.equal(new Set(jobs.map(value => value.occupation.signature.seed)).size, 1);
      assert.ok(jobs.every(value => value.occupation.status === 'employed' && value.occupation.age === 35 && value.occupation.job));
      assert.deepEqual(jobs.map(value => value.occupation.education), ['less_than_basic', 'basic', 'intermediate', 'advanced']);
    }
  }
  const child = await evaluate(`JSON.parse(ogMod.generateOccupation('BR', 'female', 8, ''))`);
  assert.equal(child.Status, 'student');
  assert.equal(child.Job, '-');
  const worker = await evaluate(`JSON.parse(ogMod.generateOccupation('DE', 'male', 42, 'advanced'))`);
  assert.equal(worker.Education, 'advanced');
  assert.equal(worker.Country, 'DE');
  for (const [select, mod] of [['ph-country', 'phMod'], ['em-country', 'emMod']]) {
    const listed = await evaluate(`JSON.parse(${mod}.supportedCountries())`);
    assert.equal(listed.length, 195);
    assert.deepEqual(await evaluate(`Array.from(byId('${select}').options, option => option.value).filter(Boolean).sort()`),
      [...listed].sort(), "The country list must be the library's own");
    assert.equal(await evaluate(`byId('${select}').options[0].value`), '');
    assert.equal(await evaluate(`Array.from(byId('${select}').options).find(option => option.value === 'BR').textContent`), 'Brazil');
  }
  for (const country of ['', 'US', 'GB', 'BR', 'DE']) {
    for (const type of ['', 'mobile', 'fixed']) {
      const phone = await evaluate(`JSON.parse(phMod.generatePhone('${country}', '${type}', false))`);
      assert.equal(phone.error, undefined, phone.error);
      assert.match(phone['E.164'], /^\+\d{8,15}$/);
      assert.equal(phone['International'].replace(/\D/g, ''), phone['E.164'].slice(1));
      assert.equal(phone['Reserved for fiction'], false);
      if (country) assert.equal(phone.Country, country);
      if (type) assert.equal(phone.Type, type === 'mobile' ? 'mobile' : 'fixed line');
    }
  }
  for (const country of ['US', 'CA']) {
    const safe = await evaluate(`JSON.parse(phMod.generatePhone('${country}', '', true))`);
    assert.equal(safe['Reserved for fiction'], true);
    assert.match(safe['E.164'], /^\+1\d{3}55501\d\d$/);
  }
  const pair = await evaluate(`JSON.parse(phMod.compareSafe('US', ''))`);
  assert.deepEqual(pair.map(item => item.safe), [false, true]);
  assert.equal(pair[0].phone.signature.seed, pair[1].phone.signature.seed);
  assert.equal(pair[1].phone.reserved, true);
  const reserved = new Set(['example.com', 'example.net', 'example.org']);
  for (const country of ['', 'US', 'DE', 'JP']) {
    const mail = await evaluate(`JSON.parse(emMod.generateEmail('${country}', 'Zoë', 'Müller', 'zm', 1988, true))`);
    assert.equal(mail.error, undefined, mail.error);
    assert.ok(reserved.has(mail.Domain));
    assert.equal(mail['Reserved for fiction'], true);
    assert.equal(mail['Domain kind'], '-');
    assert.match(mail['Local part'], /^[a-z0-9]([a-z0-9._-]*[a-z0-9])?$/);
    if (country) assert.equal(mail.Country, country);
  }
  const countries = await evaluate(`JSON.parse(emMod.compareCountries('Ana', 'da Silva', '', 1990, false))`);
  assert.deepEqual(countries.map(item => item.country), ['US', 'DE', 'BR', 'JP', 'CN', 'RU', 'FR', 'EG']);
  assert.equal(new Set(countries.map(item => item.email.signature.seed)).size, 1);
  assert.ok(countries.every(item => item.email.country_code === item.country));
  for (const expression of ["phMod.generatePhone('XX', '', false)", "phMod.compareSafe('XX', '')",
    "phMod.generatePhone('US', 'cell', false)", "emMod.generateEmail('XX', '', '', '', 0, false)"]) {
    assert.ok(await evaluate(`JSON.parse(${expression}).error`), 'Invalid phone or email input must report an error');
  }
  // The address data pack: tables at start, a country's streets on demand.
  const addressCountries = await evaluate('JSON.parse(adMod.supportedCountries())');
  assert.ok(addressCountries.length >= 200, 'The address module lists the countries it has data for');
  assert.equal(await evaluate("adMod.streetsLoaded('PT')"), false, 'No street file is read before it is asked for');
  assert.ok((await evaluate('JSON.parse(adMod.generateAddress("PT", "local"))')).error,
    'A country whose street file was not fetched must report an error, not invent streets');
  for (const [country, script] of [['PT', 'local'], ['US', 'local'], ['JP', 'latin'], ['BR', 'local']]) {
    const address = await evaluate(`(async () => { await adEnsure('${country}');
      return JSON.parse(adMod.generateAddress('${country}', '${script}')); })()`);
    assert.equal(address.error, undefined, address.error);
    assert.equal(address.Country, country);
    assert.equal(address['Real street'], true);
    assert.ok(address.Street && address.City && address.Address.includes(address.Street));
  }
  assert.equal(await evaluate('adFetched.size'), 4, 'Only the countries asked for were fetched');
  assert.equal(await evaluate("adMod.streetsLoaded('DE')"), false);
  const scripts = await evaluate(`(async () => { await adEnsure('JP'); return JSON.parse(adMod.compareScripts('JP')); })()`);
  assert.deepEqual(scripts.map(item => item.script), ['local', 'latin']);
  assert.equal(scripts[0].address.signature.seed, scripts[1].address.signature.seed);
  assert.ok((await evaluate('JSON.parse(adMod.generateAddress("XX", "local"))')).error);
  assert.ok((await evaluate('JSON.parse(adMod.generateAddress("PT", "cyrillic"))')).error);
  await evaluate(`byId('ad-country').value = 'PT'; byId('ad-generate').click()`);
  await waitFor(() => evaluate('byId("ad-output").querySelectorAll("tbody tr").length > 0'), 'address table');
  assert.match(await evaluate('byId("ad-status").textContent'), /Generated an address in PT/);
  // The user page: six libraries' data compiled in, addresses from the pack.
  const userCountries = await evaluate('JSON.parse(usMod.supportedCountries())');
  assert.ok(userCountries.length >= 50 && ['PT', 'US', 'JP', 'BR'].every(code => userCountries.includes(code)));
  assert.ok(userCountries.every(code => addressCountries.includes(code)), 'Users are only offered for countries with addresses');
  assert.ok((await evaluate('JSON.parse(usMod.generateUser("PT", "", true, ""))')).error,
    'A country whose street file was not fetched must report an error');
  for (const [country, sex] of [['PT', 'female'], ['US', 'male'], ['JP', ''], ['BR', '']]) {
    const user = await evaluate(`(async () => { await usEnsure('${country}');
      return JSON.parse(usMod.generateUser('${country}', '${sex}', true, '')); })()`);
    assert.equal(user.error, undefined, user.error);
    assert.ok(user.Name && user.Username && user.Password && user.Address && user.Locale);
    assert.match(user.Email, /@example\.(com|net|org)$/, 'Safe users get a reserved email domain');
    if (sex) assert.equal(user.Sex, sex);
  }
  assert.equal(await evaluate('usFetched.size'), 4, 'Only the countries asked for were fetched');
  const seededUser = () => evaluate(`JSON.parse(usMod.generateUserSeeded('PT', 'female', true, 'active', '2a'))`);
  const first = await seededUser();
  assert.deepEqual(await seededUser(), first, 'A seed replays the same user');
  assert.equal(first.account.status, 'active');
  assert.equal(first.address.country_code, 'PT');
  assert.equal(first.country.code, 'PT');
  const safePair = await evaluate(`JSON.parse(usMod.compareSafe('PT', '', ''))`);
  assert.deepEqual(safePair.map(item => item.safe), [false, true]);
  assert.equal(safePair[0].user.name, safePair[1].user.name);
  assert.equal(safePair[0].user.address.formatted, safePair[1].user.address.formatted);
  assert.equal(safePair[0].user.signature.seed, safePair[1].user.signature.seed);
  assert.match(safePair[1].user.email.address, /@example\.(com|net|org)$/);
  for (const expression of ["usMod.generateUser('XX', '', true, '')", "usMod.generateUser('PT', 'robot', true, '')",
    "usMod.generateUser('', '', true, '')", "usMod.generateUser('PT', '', true, 'banned')"]) {
    assert.ok(await evaluate(`JSON.parse(${expression}).error`), 'Invalid user input must report an error: ' + expression);
  }
  await evaluate(`byId('us-country').value = 'PT'; byId('us-generate').click()`);
  await waitFor(() => evaluate('byId("us-output").querySelectorAll("tbody tr").length > 0'), 'user table');
  assert.match(await evaluate('byId("us-status").textContent'), /Generated a user in PT/);
  await evaluate(`byId('em-given').value = 'Zoë'; byId('em-family').value = 'Müller'; byId('em-year').value = '1988';
    byId('ph-country').value = 'GB'; byId('ph-compare').click(); byId('em-compare').click()`);
  assert.match(await evaluate('byId("ph-output").textContent'), /only the safe option varies/);
  assert.match(await evaluate('byId("em-output").textContent'), /only the country varies/);
  await evaluate(`byId('em-year').value = '88'; byId('em-generate').click()`);
  assert.match(await evaluate('byId("em-status").textContent'), /four digits/);
  await evaluate(`byId('em-year').value = ''; byId('em-generate').click(); byId('ph-generate').click()`);
  assert.match(await evaluate('byId("em-output").textContent'), /Local part/);
  assert.match(await evaluate('byId("ph-output").textContent'), /RFC 3966/);
  for (const expression of ["bdMod.compareBiodata('XX')", "igMod.compareAges('XX', '')", "ogMod.compareEducation('XX', '')",
    "ctgMod.sampleCountry('invalid-region', true, 1)", "cgMod.sampleCity('XX', false, 1)"]) {
    assert.ok(await evaluate(`JSON.parse(${expression}).error`), 'Invalid filter must report an error');
  }
  await evaluate(`byId('bd-compare').click(); byId('ig-compare').click(); byId('og-compare').click(); byId('eg-generate').click();
    byId('pg-country').value = 'BR'; byId('pg-generate').click()`);
  assert.match(await evaluate('byId("bd-output").textContent'), /only sex input varies/);
  assert.match(await evaluate('byId("ig-output").textContent'), /only age input varies/);
  assert.match(await evaluate('byId("og-output").textContent'), /only education input varies/);
  assert.equal(await evaluate('document.querySelectorAll("#eg-output tbody tr").length'), 10);
  assert.match(await evaluate('byId("eg-output").textContent'), /weighted_choice_component/);
  const personaKeys = await evaluate(`Object.keys(JSON.parse(pgMod.generatePersona('BR', '')))`);
  assert.deepEqual(await evaluate(`Array.from(byId('pg-country').options, option => option.value).filter(Boolean)`),
    await evaluate('JSON.parse(pgMod.supportedCountries())'), "The persona country list must be persona-generator's");
  assert.deepEqual(await evaluate('Object.keys(personaSources).sort()'), personaKeys.sort());
  assert.equal(await evaluate('document.querySelectorAll("#pg-provenance tbody tr").length'), personaKeys.length);
  assert.match(await evaluate('byId("pg-output").textContent'), /Fictional persona/);
  const countryButton = '#pg-output button[aria-controls="pg-country-details"]';
  await evaluate(`document.querySelector('${countryButton}').focus()`);
  await send('Input.dispatchKeyEvent', { type: 'keyDown', key: 'Enter', code: 'Enter', windowsVirtualKeyCode: 13, text: '\r' });
  await send('Input.dispatchKeyEvent', { type: 'keyUp', key: 'Enter', code: 'Enter', windowsVirtualKeyCode: 13 });
  assert.equal(await evaluate(`document.querySelector('${countryButton}').getAttribute('aria-expanded')`), 'true');
  await evaluate(`byId('pg-provenance').open = true; document.querySelector('#tg-output details').open = true`);

  for (const [mod, method, filter, library, filename, id, filterKey] of [
    ['ctgMod', 'sampleCountry', 'Europe', 'country-generator', 'countries.tsv', 'CCA2', 'region'],
    ['cgMod', 'sampleCity', 'BR', 'city-generator', 'cities.tsv', 'GeoNames ID', 'country_code']
  ]) {
    const eligible = (await catalogue(library, filename)).filter(row => row[filterKey] === filter);
    const n = 6000;
    const counts = await evaluate(`(() => {
      const counts = [Object.create(null), Object.create(null)];
      for (let seed = 0; seed < ${n}; ++seed) {
        for (let mode = 0; mode < 2; ++mode) {
          const value = JSON.parse(${mod}.${method}('${filter}', mode === 0, seed));
          counts[mode][value[${JSON.stringify(id)}]] = (counts[mode][value[${JSON.stringify(id)}]] || 0) + 1;
        }
      }
      return counts;
    })()`);
    const totalWeight = eligible.reduce((sum, row) => sum + Math.max(Number(row.population), 1), 0);
    const keys = new Set(eligible.map(row => id === 'CCA2' ? row.cca2 : row.geonameid));
    for (let mode = 0; mode < 2; ++mode) {
      assert.equal(Object.values(counts[mode]).reduce((sum, value) => sum + value, 0), n);
      assert.ok(Object.keys(counts[mode]).every(key => keys.has(key)));
      for (const row of eligible) {
        const probability = mode === 0 ? Math.max(Number(row.population), 1) / totalWeight : 1 / eligible.length;
        const observed = (counts[mode][id === 'CCA2' ? row.cca2 : row.geonameid] || 0) / n;
        const tolerance = 6 * Math.sqrt(probability * (1 - probability) / n) + .004;
        assert.ok(Math.abs(observed - probability) < tolerance, `${library} distribution differs from pinned model`);
      }
    }
    assert.notDeepEqual(counts[0], counts[1]);
  }
  for (const [prefix, filterId, filter] of [['cg', 'cg-country', 'BR'], ['ctg', 'ctg-region', 'Europe']]) {
    for (const count of [100, 500, 2000]) {
      await evaluate(`byId('${filterId}').value = '${filter}'; byId('${prefix}-count').value = '${count}';
        byId('${prefix}-compare').click()`);
      const distribution = await evaluate(`Array.from(document.querySelectorAll('#${prefix}-output tbody tr'),
        row => Array.from(row.cells, cell => cell.textContent))`);
      assert.deepEqual(distribution.at(-1).slice(2), [String(count), '100%', String(count), '100%']);
      assert.equal(distribution.slice(0, -1).reduce((sum, row) => sum + Number(row[2]), 0), count);
      assert.equal(distribution.slice(0, -1).reduce((sum, row) => sum + Number(row[4]), 0), count);
      assert.ok(distribution.slice(0, -1).every(row => Math.abs(parseFloat(row[3]) - 100 * Number(row[2]) / count) <= .051));
    }
  }
  assert.equal(await evaluate(`Array.from(document.querySelectorAll('select,input')).every(control =>
    document.querySelector('label[for="' + control.id + '"]'))`), true);
  assert.equal(await evaluate(`Array.from(document.querySelectorAll('.status')).every(status =>
    status.getAttribute('role') === 'status')`), true);
  await capture('desktop-persona', '#personagen-card');
  await capture('desktop-trait', '#traitgen-card');
  await capture('desktop-sampling', '#countrygen-card');
  await capture('desktop-occupation', '#occupationgen-card');
  await capture('desktop-persona-occupation', '#pg-provenance');
  for (const width of [375, 320, 640]) {
    await send('Emulation.setDeviceMetricsOverride', {
      width, height: 812, deviceScaleFactor: 1, mobile: true
    });
    assert.ok(await evaluate(`Array.from(document.querySelectorAll('.demo-card, .controls, .table-scroll, #tg-output > table'),
      element => element.getBoundingClientRect()).every(rect => rect.left >= 0 && rect.right <= innerWidth)`),
    `Demo content overflows at ${width}px`);
    assert.ok(await evaluate('document.documentElement.scrollWidth <= innerWidth'), `Page overflows at ${width}px`);
    if (width === 375) {
      await capture('mobile-persona', '#personagen-card');
      await capture('mobile-trait', '#traitgen-card');
      await capture('mobile-sampling', '#countrygen-card');
      await capture('mobile-occupation', '#occupationgen-card');
    }
  }
  // 640 CSS pixels also covers a 1280px viewport at 200% zoom.
  await evaluate(`byId('cg-generate').click(); byId('ctg-generate').click();
    byId('bd-generate').click(); byId('ig-generate').click(); byId('og-generate').click()`);
  for (const prefix of ['cg', 'ctg', 'bd', 'ig', 'og']) {
    assert.match(await evaluate(`byId('${prefix}-status').className`), /ready/);
    assert.ok(await evaluate(`document.querySelectorAll('#${prefix}-output tbody tr').length > 0`));
  }
  await send('Fetch.enable', { patterns: [{ urlPattern: '*traitgen_demo.wasm*' }] });
  await send('Page.reload', { ignoreCache: true });
  await waitFor(() => evaluate('document.getElementById("tg-status")?.classList.contains("error")'), 'module failure status');
  assert.equal(await evaluate('document.getElementById("tg-generate").disabled'), true);
  assert.match(await evaluate('document.getElementById("tg-status").textContent'), /Reload the page/);
  assert.equal(await evaluate('document.getElementById("tg-output").textContent.trim()'), 'Trait generator unavailable.');
  await send('Fetch.disable');
  await send('Fetch.enable', { patterns: [{ urlPattern: '*namegen_demo.js*' }] });
  await send('Page.reload', { ignoreCache: true });
  await waitFor(() => evaluate('document.getElementById("ng-status")?.classList.contains("error")'), 'missing name script');
  assert.equal(await evaluate('byId("ng-generate").disabled'), true);
  assert.match(await evaluate('byId("ng-status").textContent'), /Reload the page/);
  await waitFor(() => evaluate('igMod !== null && pgMod !== null && cgMod !== null'), 'independent modules after script failure');
  await send('Fetch.disable');
  mockBiodata = true;
  await send('Fetch.enable', { patterns: [{ urlPattern: '*biodatagen_demo.js*' }] });
  await send('Page.reload', { ignoreCache: true });
  await waitFor(() => evaluate('document.getElementById("bd-status")?.classList.contains("error")'), 'embedded data initialization failure');
  assert.equal(await evaluate('byId("bd-generate").disabled && byId("bd-compare").disabled'), true);
  assert.match(await evaluate('byId("bd-status").textContent'), /embedded data.*Reload the page/);
  assert.equal(await evaluate('bdMod'), null);
  console.log('PASS: all real Wasm demos, request-first filters/options and 64-bit replay across ten libraries and persona, pinned distributions/loadings, comparisons, provenance, names/nicknames, reference dates, input/load errors, keyboard, labels and responsive layouts.');
} finally {
  if (browser?.pid && browser.exitCode === null) {
    const exited = new Promise(resolve => browser.once('exit', resolve));
    // Chrome may exit before replying to Browser.close, so don't wait for the
    // reply; fall back to killing it if it hasn't exited shortly after.
    if (socket?.readyState === WebSocket.OPEN) send('Browser.close').catch(() => {});
    else browser.kill();
    const killTimer = setTimeout(() => browser.kill(), 5000);
    await exited;
    clearTimeout(killTimer);
  }
  socket?.close();
  server.closeAllConnections();
  await new Promise(resolve => server.close(resolve));
  await rm(profile, { recursive: true, force: true, maxRetries: 5, retryDelay: 200 });
}
