// Tests the assembled npm package: node package/test.mjs build/package
//
// The known answers are persona-generator v1.0.0's (tests/tests.cpp), so a
// pass means the WebAssembly build gives the same personas as a native one.
import assert from 'node:assert/strict';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

const dir = path.resolve(process.argv[2] || 'build/package');
const { createPersonaGenerator } = await import(pathToFileURL(path.join(dir, 'index.mjs')).href);
const personas = await createPersonaGenerator();

// persona::to_string(), from the JSON.
const summary = p => [p.name, p.age, p.sex, p.city.name, p.country.name_common,
  p.occupation.job ? p.occupation.job.label : p.occupation.status].join(', ');
const signature = p => p.signature.seed + '-' + p.signature.data;

assert.equal(personas.countries.length, 79);
assert.ok(!personas.countries.includes('KR'));
assert.equal(personas.dataHash, '2c51b25a966f8fa8');

// Known answers.
const one = personas.generate({ seed: 1 });
assert.equal(summary(one), 'Brandon White, 13, male, Gosport, United Kingdom, student');
assert.equal(signature(one), '0000000000000001-16ccfc2fe06c547b');
const two = personas.generate({ seed: '2' });
assert.equal(summary(two), 'Romana Nováková, 16, female, Mashhad, Iran, student');
assert.equal(signature(two), '0000000000000002-16ccfc2fe06c547b');
const three = personas.generate({ country: 'BR', sex: 'female', ages: { min: 25, max: 35 }, traits: 5, seed: 3n });
assert.equal(summary(three), 'Thayna Brandao, 34, female, São Paulo, Brazil, Farm labourer');
assert.equal(signature(three), '0000000000000003-a4436f388f2880a1');
assert.equal(three.personality.traits.length, 5);
const four = personas.generate({ country: 'jp', referenceDate: '2030-03-09', seed: 4 });
assert.equal(summary(four), 'Toshinori Toyama, 70, male, Edogawe, Japan, Shop assistant');
assert.equal(signature(four), '0000000000000004-b617ce3a636b9167');

// The hash of 2,000 JSON documents. The native test runs without admin1
// names, which this module embeds, so they are blanked before hashing.
let hash = 0xcbf29ce484222325n;
const mask = (1n << 64n) - 1n;
const encoder = new TextEncoder();
for (let s = 0; s < 2000; ++s) {
  const json = personas.generateJson({ seed: s }).replace(/"admin1":"(?:[^"\\]|\\.)*"/, '"admin1":""');
  for (const byte of encoder.encode(json)) hash = ((hash ^ BigInt(byte)) * 0x100000001b3n) & mask;
}
assert.equal(hash.toString(16), 'b9d64e6d46c6a12c');

// Admin1 names are embedded.
assert.equal(personas.generate({ country: 'BR', seed: 1 }).admin1.length > 0, true);

// A random persona replays from its seed.
const random = personas.generate({ country: 'DE', referenceDate: new Date(2026, 9, 15) });
assert.equal(random.birth.date <= '2026-10-15', true);
assert.deepEqual(personas.generate({ country: 'DE', referenceDate: '2026-10-15', seed: random.signature.seed }), random);
assert.notEqual(personas.generate().signature.seed, personas.generate().signature.seed);

// Requests.
for (let s = 0; s < 40; ++s) {
  const p = personas.generate({ ages: { min: 30, max: 35 }, seed: s });
  assert.ok(p.age >= 30 && p.age <= 35);
  assert.equal(p.biodata.age, p.age);
}
const female = personas.generate({ country: 'FR', sex: 'female', seed: 9 });
const male = personas.generate({ country: 'FR', sex: 'male', seed: 9 });
assert.equal(female.sex, 'female');
assert.equal(male.sex, 'male');
assert.deepEqual([male.city, male.personality], [female.city, female.personality]);
assert.equal(personas.generate({ seed: 'ffffffffffffffff' }).signature.seed, 'ffffffffffffffff');
assert.equal(personas.generate({ seed: (1n << 64n) - 1n }).signature.seed, 'ffffffffffffffff');

// Errors.
for (const [request, kind, message] of [
  [{ country: 'XX' }, Error, /XX/],
  [{ country: 'US', referenceDate: '2023-02-30' }, Error, /reference date/i],
  [{ ages: { min: 40, max: 20 } }, Error, /ages/],
  [{ seed: '10000000000000000' }, Error, /hexadecimal/i],
  [{ seed: 'xyz' }, Error, /hexadecimal/i],
  [{ sex: 'other' }, TypeError, /sex/],
  [{ ages: { min: 20 } }, TypeError, /ages.max/],
  [{ ages: { min: -1, max: 20 } }, TypeError, /ages.min/],
  [{ traits: 1.5 }, TypeError, /traits/],
  [{ seed: -1 }, TypeError, /seed/],
  [{ seed: 1n << 64n }, TypeError, /seed/],
  [{ referenceDate: new Date(NaN) }, TypeError, /referenceDate/],
  [null, TypeError, /object/]
]) {
  assert.throws(() => personas.generate(request), error => error instanceof kind && message.test(error.message),
    JSON.stringify(request, (_, v) => typeof v === 'bigint' ? v.toString() : v));
}
// A failed call leaves the generator usable.
assert.equal(signature(personas.generate({ seed: 1 })), '0000000000000001-16ccfc2fe06c547b');

console.log('PASS: @dasmig/persona matches persona-generator v1.0.0 known answers (5 personas, 2,000-document hash), replay, requests and errors.');
