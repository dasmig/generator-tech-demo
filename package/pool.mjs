// A pool of workers, each with its own copy of the module, that generates a
// batch in pieces and joins them in order. The pool is independent of where the
// workers come from: the browser entry spawns Web Workers, the Node entry
// worker threads, and each passes `spawn`, which returns
//
//   { send(message), onMessage(handler), onError(handler), terminate() }
//
// No SharedArrayBuffer is needed: a piece is a string of JSON Lines.
import { batchPlan, outOfRange } from './batch.mjs';

function rebuild(error) {
  const type = { TypeError, RangeError }[error.name] ?? Error;
  return new type(error.message);
}

export function poolSize(options, fallback) {
  if (options === null || typeof options !== 'object') throw new TypeError('pool options must be an object');
  for (const key of Object.keys(options))
    if (key !== 'workers') throw new TypeError(`unknown pool option "${key}"; the option is workers`);
  const { workers = fallback } = options;
  if (!Number.isInteger(workers)) throw new TypeError('workers must be an integer');
  if (workers < 1 || workers > 64) outOfRange(`workers must be from 1 to 64, not ${workers}`);
  return workers;
}

export async function createPool(spawn, size) {
  let sequence = 0;
  let closed = false;
  const pending = new Map();
  const workers = [];
  const started = [];

  for (let i = 0; i < size; ++i) {
    const worker = spawn();
    workers.push(worker);
    started.push(new Promise((resolve, reject) => {
      worker.onMessage(message => {
        if (message.ready) return resolve();
        const entry = pending.get(message.id);
        if (!entry) return;
        pending.delete(message.id);
        if (message.error) entry.reject(rebuild(message.error));
        else entry.resolve(message.jsonl);
      });
      worker.onError(error => {
        reject(error);
        for (const [id, entry] of pending) {
          if (entry.worker !== worker) continue;
          pending.delete(id);
          entry.reject(error);
        }
      });
    }));
  }
  try {
    await Promise.all(started);
  } catch (error) {
    for (const worker of workers) worker.terminate();
    throw error;
  }

  function run(worker, options) {
    return new Promise((resolve, reject) => {
      const id = ++sequence;
      pending.set(id, { resolve, reject, worker });
      worker.send({ id, options });
    });
  }

  async function generateBatchJsonl(options) {
    if (closed) throw new Error('the pool is closed');
    const plan = batchPlan(options);
    const pieces = [];
    for (let c = 0; c < size; ++c) {
      const lo = Math.floor((plan.count * c) / size);
      const hi = Math.floor((plan.count * (c + 1)) / size);
      if (hi === lo) continue;
      pieces.push(run(workers[c], { request: plan.request, seed: plan.seed, first: plan.first + lo, count: hi - lo }));
    }
    return (await Promise.all(pieces)).join('');
  }

  return Object.freeze({
    size,
    generateBatchJsonl,
    async generateBatch(options) {
      const text = await generateBatchJsonl(options);
      return text === '' ? [] : text.slice(0, -1).split('\n').map(line => JSON.parse(line));
    },
    close() {
      closed = true;
      for (const worker of workers) worker.terminate();
    }
  });
}
