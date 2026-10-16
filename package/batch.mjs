// Batch options, checked the same way by the generator and by the worker pool.
// No WebAssembly here: the pool uses it to split a batch before any worker
// has loaded the module.
//
// A batch is `count` personas from one batch seed; persona `i` is exactly the
// single call with the seed core::item_seed(batchSeed, i), so a range of a
// batch can be generated alone and the pieces joined.

const batchKeys = new Set(['request', 'count', 'seed', 'first']);
const maxSeed = (1n << 64n) - 1n;

export function malformed(message) {
  throw new TypeError('persona request: ' + message);
}

export function outOfRange(message) {
  throw new RangeError('persona request: ' + message);
}

export function integer(value, name, min, max) {
  if (typeof value !== 'number' || !Number.isInteger(value)) malformed(`${name} must be an integer`);
  if (value < min || value > max) outOfRange(`${name} must be from ${min} to ${max}, not ${value}`);
  return value;
}

// The seed as the module takes it: 1 to 16 lowercase hexadecimal digits.
export function seedText(seed) {
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

function randomSeed() {
  const words = globalThis.crypto.getRandomValues(new Uint32Array(2));
  return words[0].toString(16).padStart(8, '0') + words[1].toString(16).padStart(8, '0');
}

/**
 * Check batch options and fill in the defaults.
 * @returns {{ request: object, count: number, first: number, seed: string }} The seed is always set
 *   (drawn when the caller gave none), so every piece of a split batch uses the same one.
 */
export function batchPlan(options) {
  if (options === null || typeof options !== 'object' || Array.isArray(options)) malformed('batch options must be an object');
  for (const key of Object.keys(options))
    if (!batchKeys.has(key)) malformed(`unknown batch field "${key}"; the fields are ${[...batchKeys].join(', ')}`);
  const { request = {}, count, seed, first = 0 } = options;
  if (count === undefined) malformed('count is required');
  if (request === null || typeof request !== 'object' || Array.isArray(request)) malformed('request must be an object');
  if ('seed' in request) malformed('the batch has its own seed; set it on the batch options, not on the request');
  return {
    request,
    count: integer(count, 'count', 0, 2 ** 31 - 1),
    first: integer(first, 'first', 0, Number.MAX_SAFE_INTEGER),
    seed: seed === undefined ? randomSeed() : seedText(seed)
  };
}
