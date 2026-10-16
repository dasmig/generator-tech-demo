// dasmig persona generator, browser and worker entry. The module finds
// persona.wasm next to itself (new URL('persona.wasm', import.meta.url)),
// which bundlers follow; pass locateFile or wasmBinary to load it otherwise.
import createPersonaModule from './persona.mjs';
import { createGenerator } from './generator.mjs';
import { createPool, poolSize } from './pool.mjs';

export function createPersonaGenerator(moduleOptions = {}) {
  return createGenerator(createPersonaModule, moduleOptions);
}

// A pool of module Web Workers (pool-worker.mjs, next to this file) that
// generates batches in pieces and joins them in order. Each worker loads its
// own copy of the module, about 60 MB, so the default is at most 4.
export async function createPersonaPool(options = {}) {
  const size = poolSize(options, Math.min(4, globalThis.navigator?.hardwareConcurrency ?? 2));
  return createPool(() => {
    const worker = new Worker(new URL('./pool-worker.mjs', import.meta.url), { type: 'module' });
    return {
      send: message => worker.postMessage(message),
      onMessage: handler => { worker.onmessage = event => handler(event.data); },
      onError: handler => { worker.onerror = event => handler(new Error(event.message || 'pool worker failed')); },
      terminate: () => worker.terminate()
    };
  }, size);
}
