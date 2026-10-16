// dasmig persona generator, Node entry (the package's "node" export
// condition). The module is built for browsers and workers only, so it has no
// Node code for bundlers to trip on; here the .wasm file is read from disk and
// handed to it.
import { readFile } from 'node:fs/promises';
import { availableParallelism } from 'node:os';
import { Worker } from 'node:worker_threads';
import createPersonaModule from './persona.mjs';
import { createGenerator } from './generator.mjs';
import { createPool, poolSize } from './pool.mjs';

export async function createPersonaGenerator(moduleOptions = {}) {
  const wasmBinary = moduleOptions.wasmBinary ?? await readFile(new URL('./persona.wasm', import.meta.url));
  return createGenerator(createPersonaModule, { ...moduleOptions, wasmBinary });
}

// A pool of worker threads (pool-worker-node.mjs, next to this file) that
// generates batches in pieces and joins them in order. Each worker loads its
// own copy of the module, about 60 MB, so the default is at most 4.
export async function createPersonaPool(options = {}) {
  const size = poolSize(options, Math.min(4, availableParallelism()));
  return createPool(() => {
    const worker = new Worker(new URL('./pool-worker-node.mjs', import.meta.url));
    return {
      send: message => worker.postMessage(message),
      onMessage: handler => { worker.on('message', handler); },
      onError: handler => { worker.on('error', handler); },
      terminate: () => { worker.terminate(); }
    };
  }, size);
}
