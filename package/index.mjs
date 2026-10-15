// dasmig persona generator: complete fictional characters from one call.
//
// A thin wrapper over the WebAssembly module's C API (src/persona_api.cpp):
// it checks the request, passes it on and parses the JSON that comes back.
import createPersonaModule from './persona.mjs';

const sexes = new Set(['male', 'female']);

function fail(message) {
  throw new TypeError('persona request: ' + message);
}

function integer(value, name, min, max) {
  if (!Number.isInteger(value) || value < min || value > max)
    fail(`${name} must be an integer from ${min} to ${max}`);
  return value;
}

function seedText(seed) {
  if (seed === undefined) return '';
  if (typeof seed === 'string') return seed;
  if (typeof seed === 'bigint') {
    if (seed < 0n || seed >= 1n << 64n) fail('seed must be an unsigned 64-bit integer');
    return seed.toString(16);
  }
  if (Number.isSafeInteger(seed) && seed >= 0) return seed.toString(16);
  return fail('seed must be a hexadecimal string, a bigint or a non-negative safe integer');
}

function dateText(date) {
  if (date === undefined) return '';
  if (typeof date === 'string') return date;
  if (date instanceof Date && !Number.isNaN(date.getTime())) {
    const pad = n => String(n).padStart(2, '0');
    return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())}`;
  }
  return fail('referenceDate must be a YYYY-MM-DD string or a valid Date');
}

function parse(json) {
  const result = JSON.parse(json);
  if (result.error) throw new Error(result.error);
  return result;
}

/**
 * Load the module and return a persona generator.
 * @param {object} [moduleOptions] Emscripten module options, for example
 *   `locateFile` when the .wasm file is served from another path.
 */
export async function createPersonaGenerator(moduleOptions = {}) {
  const module = await createPersonaModule(moduleOptions);
  const generate = module.cwrap('dasmig_persona_generate', 'string',
    ['string', 'string', 'number', 'number', 'number', 'string', 'string']);
  const info = parse(module.cwrap('dasmig_persona_info', 'string', [])());

  function generateJson(request = {}) {
    if (request === null || typeof request !== 'object') fail('must be an object');
    const { country, sex, ages, traits, referenceDate, seed } = request;
    if (country !== undefined && typeof country !== 'string') fail('country must be a string');
    if (sex !== undefined && !sexes.has(sex)) fail('sex must be "male" or "female"');
    if (ages !== undefined && (ages === null || typeof ages !== 'object')) fail('ages must be { min, max }');
    const min = ages === undefined ? -1 : integer(ages.min, 'ages.min', 0, 120);
    const max = ages === undefined ? -1 : integer(ages.max, 'ages.max', 0, 120);
    const count = traits === undefined ? -1 : integer(traits, 'traits', 0, 1000);
    return generate(country ?? '', sex ?? '', min, max, count, dateText(referenceDate), seedText(seed));
  }

  return {
    countries: Object.freeze([...info.countries]),
    dataHash: info.data,
    generateJson(request) {
      const json = generateJson(request);
      parse(json);
      return json;
    },
    generate(request) {
      return parse(generateJson(request));
    }
  };
}
