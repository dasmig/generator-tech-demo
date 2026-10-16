// The generator over the WebAssembly module's C API (src/persona_api.cpp),
// shared by the browser entry (index.mjs) and the Node entry (node.mjs). It
// checks the request, passes it on and parses the JSON that comes back.
import { versions } from './versions.mjs';

const keys = new Set(['country', 'sex', 'ages', 'traits', 'referenceDate', 'seed']);
const sexes = new Set(['male', 'female']);
const maxSeed = (1n << 64n) - 1n;

function malformed(message) {
  throw new TypeError('persona request: ' + message);
}

function outOfRange(message) {
  throw new RangeError('persona request: ' + message);
}

function integer(value, name, min, max) {
  if (typeof value !== 'number' || !Number.isInteger(value)) malformed(`${name} must be an integer`);
  if (value < min || value > max) outOfRange(`${name} must be from ${min} to ${max}, not ${value}`);
  return value;
}

// The seed as the module takes it: 1 to 16 lowercase hexadecimal digits.
function seedText(seed) {
  if (seed === undefined) return '';
  if (typeof seed === 'string') {
    const match = /^(?:0x)?([0-9a-f]{1,16})$/i.exec(seed);
    if (!match) outOfRange(`seed string must be 1 to 16 hexadecimal digits, optionally after 0x, not "${seed}"`);
    return match[1].toLowerCase();
  }
  if (typeof seed === 'bigint') {
    if (seed < 0n || seed > maxSeed) outOfRange('seed must be an unsigned 64-bit integer');
    return seed.toString(16);
  }
  if (typeof seed === 'number') {
    if (!Number.isSafeInteger(seed) || seed < 0)
      outOfRange('seed number must be a non-negative safe integer; use a bigint or a hexadecimal string for larger seeds');
    return seed.toString(16);
  }
  return malformed('seed must be a hexadecimal string, a bigint or a number');
}

function dateText(date) {
  if (date === undefined) return '';
  let year, month, day;
  if (date instanceof Date) {
    if (Number.isNaN(date.getTime())) outOfRange('referenceDate is an invalid Date');
    [year, month, day] = [date.getFullYear(), date.getMonth() + 1, date.getDate()];
  } else if (typeof date === 'string') {
    const match = /^(\d{4})-(\d{2})-(\d{2})$/.exec(date);
    if (!match) outOfRange(`referenceDate must be YYYY-MM-DD, not "${date}"`);
    [year, month, day] = match.slice(1).map(Number);
    const check = new Date(Date.UTC(year, month - 1, day));
    if (check.getUTCMonth() !== month - 1 || check.getUTCDate() !== day) outOfRange(`referenceDate ${date} is not a calendar date`);
  } else {
    return malformed('referenceDate must be a YYYY-MM-DD string or a Date');
  }
  if (year < 1900 || year > 2100) outOfRange('referenceDate must be between 1900 and 2100');
  const pad = n => String(n).padStart(2, '0');
  return `${year}-${pad(month)}-${pad(day)}`;
}

function parse(json) {
  const result = JSON.parse(json);
  if (result.error) throw new Error(result.error);
  return result;
}

/**
 * Wrap an instantiated module.
 * @param {Function} createModule The Emscripten factory (persona.mjs).
 * @param {object} moduleOptions Emscripten module options.
 */
export async function createGenerator(createModule, moduleOptions) {
  const module = await createModule(moduleOptions);
  const generate = module.cwrap('dasmig_persona_generate', 'string',
    ['string', 'string', 'number', 'number', 'number', 'string', 'string']);
  const info = parse(module.cwrap('dasmig_persona_info', 'string', [])());
  const countries = Object.freeze([...info.countries]);
  const supported = new Set(countries);

  function generateJson(request = {}) {
    if (request === null || typeof request !== 'object' || Array.isArray(request)) malformed('must be an object');
    for (const key of Object.keys(request))
      if (!keys.has(key)) malformed(`unknown field "${key}"; the fields are ${[...keys].join(', ')}`);
    const { country, sex, ages, traits, referenceDate, seed } = request;

    let code = '';
    if (country !== undefined) {
      if (typeof country !== 'string') malformed('country must be a string');
      code = country.toUpperCase();
      if (!supported.has(code))
        outOfRange(`country "${country}" is not supported; the supported countries are the generator's countries list`);
    }
    if (sex !== undefined && !sexes.has(sex))
      (typeof sex === 'string' ? outOfRange : malformed)(`sex must be "male" or "female", not ${JSON.stringify(sex)}`);
    let min = -1, max = -1;
    if (ages !== undefined) {
      if (ages === null || typeof ages !== 'object') malformed('ages must be { min, max }');
      min = integer(ages.min, 'ages.min', 0, 120);
      max = integer(ages.max, 'ages.max', 0, 120);
      if (min > max) outOfRange(`ages.min (${min}) must not be greater than ages.max (${max})`);
    }
    const count = traits === undefined ? -1 : integer(traits, 'traits', 0, info.max_traits);
    return generate(code, sex ?? '', min, max, count, dateText(referenceDate), seedText(seed));
  }

  return Object.freeze({
    countries,
    maxTraits: info.max_traits,
    dataHash: info.data,
    versions,
    generateJson(request) {
      const json = generateJson(request);
      parse(json);
      return json;
    },
    generate(request) {
      return parse(generateJson(request));
    }
  });
}
