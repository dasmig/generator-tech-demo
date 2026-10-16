// dasmig persona generator, Node entry (the package's "node" export
// condition). The module is built for browsers and workers only, so it has no
// Node code for bundlers to trip on; here the .wasm file is read from disk and
// handed to it.
import { readFile } from 'node:fs/promises';
import createPersonaModule from './persona.mjs';
import { createGenerator } from './generator.mjs';

export async function createPersonaGenerator(moduleOptions = {}) {
  const wasmBinary = moduleOptions.wasmBinary ?? await readFile(new URL('./persona.wasm', import.meta.url));
  return createGenerator(createPersonaModule, { ...moduleOptions, wasmBinary });
}
