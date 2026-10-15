// dasmig persona generator, browser and worker entry. The module finds
// persona.wasm next to itself (new URL('persona.wasm', import.meta.url)),
// which bundlers follow; pass locateFile or wasmBinary to load it otherwise.
import createPersonaModule from './persona.mjs';
import { createGenerator } from './generator.mjs';

export function createPersonaGenerator(moduleOptions = {}) {
  return createGenerator(createPersonaModule, moduleOptions);
}
