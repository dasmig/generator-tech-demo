// The generator over the WebAssembly module's C API (src/persona_api.cpp),
// shared by the browser entry (index.mjs) and the Node entry (node.mjs). It
// checks the request, passes it on and parses the JSON that comes back.
import { versions } from './versions.mjs';
import { batchPlan, integer, malformed, outOfRange, seedText } from './batch.mjs';

const keys = new Set(['country', 'sex', 'ages', 'traits', 'referenceDate', 'seed']);
const sexes = new Set(['male', 'female']);

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
  const generateBatchRaw = module.cwrap('dasmig_persona_generate_batch', 'string',
    ['string', 'string', 'number', 'number', 'number', 'string', 'string', 'number', 'number']);
  const requestKeys = new Set([...keys].filter(key => key !== 'seed'));
  const info = parse(module.cwrap('dasmig_persona_info', 'string', [])());
  const countries = Object.freeze([...info.countries]);
  const supported = new Set(countries);

  // The request's fields as the module's arguments (everything but the seed).
  function requestArgs(request, allowed) {
    if (request === null || typeof request !== 'object' || Array.isArray(request)) malformed('must be an object');
    for (const key of Object.keys(request))
      if (!allowed.has(key)) malformed(`unknown field "${key}"; the fields are ${[...allowed].join(', ')}`);
    const { country, sex, ages, traits, referenceDate } = request;

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
    return [code, sex ?? '', min, max, count, dateText(referenceDate)];
  }

  function generateJson(request = {}) {
    return generate(...requestArgs(request, keys), seedText(request.seed));
  }

  function generateBatchJsonl(options) {
    const plan = batchPlan(options);
    const text = generateBatchRaw(...requestArgs(plan.request, requestKeys), plan.seed, plan.first, plan.count);
    if (text.startsWith('{"error"')) throw new Error(JSON.parse(text).error);
    return text;
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
    },
    generateBatchJsonl,
    generateBatch(options) {
      const text = generateBatchJsonl(options);
      return text === '' ? [] : text.slice(0, -1).split('\n').map(line => JSON.parse(line));
    }
  });
}
