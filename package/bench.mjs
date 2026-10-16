// Benchmarks the assembled npm package in Node:
//   node package/bench.mjs build/package [--out FILE] [--quick]
//
// Measures the files' sizes (raw, gzip, brotli), module startup (a fresh
// module each time), the memory one module holds, and generate() and
// generateJson() over fixed seeds. The results use the same TSV columns as
// the libraries' bench/bench.hpp, so they merge into one baseline.
import fs from 'node:fs';
import path from 'node:path';
import zlib from 'node:zlib';
import { pathToFileURL } from 'node:url';

const args = process.argv.slice(2);
const dir = path.resolve(args.find(a => !a.startsWith('--') && args[args.indexOf(a) - 1] !== '--out') || 'build/package');
const outIndex = args.indexOf('--out');
const out = outIndex >= 0 ? args[outIndex + 1] : '';
const quick = args.includes('--quick');
const calls = quick ? 1000 : 10000;
const startups = quick ? 1 : 5;

const { createPersonaGenerator, createPersonaPool } = await import(pathToFileURL(path.join(dir, 'node.mjs')).href);
const build = `node ${process.versions.node}, wasm32`;
const results = [];
const add = (benchmark, metric, value, unit) => {
  console.log(`  ${benchmark.padEnd(34)} ${metric.padEnd(12)} ${value.toFixed(3).padStart(12)} ${unit}`);
  results.push([benchmark, metric, value, unit]);
};
const median = xs => [...xs].sort((a, b) => a - b)[Math.floor(xs.length / 2)];
console.log(`@dasmig/persona  tier lite (embedded)  ${build}`);

// Sizes.
for (const file of ['persona.wasm', 'persona.mjs']) {
  const bytes = fs.readFileSync(path.join(dir, file));
  add(file, 'size_raw', bytes.length / 1048576, 'MiB');
  add(file, 'size_gzip', zlib.gzipSync(bytes, { level: 9 }).length / 1048576, 'MiB');
  add(file, 'size_brotli', zlib.brotliCompressSync(bytes, {
    params: { [zlib.constants.BROTLI_PARAM_QUALITY]: 11 } }).length / 1048576, 'MiB');
}

// Each instance's linear memory, captured as Emscripten instantiates it.
const memories = [];
for (const name of ['instantiate', 'instantiateStreaming']) {
  const original = WebAssembly[name];
  WebAssembly[name] = async (...xs) => {
    const result = await original.apply(WebAssembly, xs);
    const instance = result.instance ?? result;
    for (const value of Object.values(instance.exports)) {
      if (value instanceof WebAssembly.Memory) memories.push(value);
    }
    return result;
  };
}

// Startup: a fresh module each time.
const startup = [];
const kept = [];
for (let i = 0; i < startups; ++i) {
  const t0 = performance.now();
  kept.push(await createPersonaGenerator());
  startup.push(performance.now() - t0);
}
add('startup', 'load_median', median(startup), 'ms');
add('startup', 'load_min', Math.min(...startup), 'ms');
const memoryOf = () => (memories[0]?.buffer.byteLength ?? 0) / 1048576;
add('module', 'memory', memoryOf(), 'MiB');

// Calls, with the same fixed seeds as bench.hpp is not possible from
// JavaScript without the core, so seeds are 0..calls-1.
const personas = kept[0];
const time = (name, call) => {
  for (let s = 0; s < Math.min(calls, 200); ++s) call(s);
  const t0 = performance.now();
  for (let s = 0; s < calls; ++s) call(s);
  const mean = (performance.now() - t0) * 1000 / calls;
  const us = [];
  for (let s = 0; s < calls; ++s) {
    const t = performance.now();
    call(s);
    us.push((performance.now() - t) * 1000);
  }
  us.sort((a, b) => a - b);
  add(name, 'mean', mean, 'us');
  add(name, 'p50', us[Math.floor(us.length / 2)], 'us');
  add(name, 'p99', us[Math.min(us.length - 1, Math.floor(us.length * 99 / 100))], 'us');
  add(name, 'per_second', 1e6 / mean, 'calls');
};
time('generate', s => personas.generate({ seed: s }));
time('generate BR', s => personas.generate({ country: 'BR', seed: s }));
time('generateJson', s => personas.generateJson({ seed: s }));
// Batches: one generator, then pools of 1, 2 and 4 workers (their startup is
// reported apart). The same personas come out of each.
const batchItems = Math.max(calls, 2000);
const batchOptions = { count: batchItems, seed: 1 };
const batchRate = async (name, run) => {
  await run(); // warm up
  const times = [];
  for (let r = 0; r < 3; ++r) {
    const t0 = performance.now();
    await run();
    times.push(performance.now() - t0);
  }
  add(`${name} x${batchItems}`, 'per_second', batchItems / (median(times) / 1000), 'items');
};
await batchRate('generateBatchJsonl', () => personas.generateBatchJsonl(batchOptions));
for (const workers of [1, 2, 4]) {
  const t0 = performance.now();
  const pool = await createPersonaPool({ workers });
  add(`pool of ${workers}`, 'startup', performance.now() - t0, 'ms');
  try {
    await batchRate(`pool of ${workers} generateBatchJsonl`, () => pool.generateBatchJsonl(batchOptions));
  } finally {
    pool.close();
  }
}
add('module', 'memory_end', memoryOf(), 'MiB');
add('process', 'peak_memory', process.resourceUsage().maxRSS / 1024, 'MiB');

if (out) {
  const header = !fs.existsSync(out) || fs.statSync(out).size === 0;
  const lines = results.map(([b, m, v, u]) => ['@dasmig/persona', 'lite', 'embedded', b, m, v, u, build].join('\t'));
  fs.appendFileSync(out, (header ? 'library\ttier\tsource\tbenchmark\tmetric\tvalue\tunit\tbuild\n' : '') +
    lines.join('\n') + '\n');
}
