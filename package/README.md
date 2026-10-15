# @dasmig/persona

Complete fictional characters from one seeded call: name, country, city, birth date and age, body, personality, religion, hobbies, favourite media and job. It is [persona-generator](https://github.com/dasmig/persona-generator) (C++23) compiled to WebAssembly, with the `lite` data of the eight libraries it composes built in, so nothing is fetched at run time but the module's own `persona.wasm`.

The output is fictional sample data, not a description or prediction of a real person.

```js
import { createPersonaGenerator } from '@dasmig/persona';

createPersonaGenerator().then(personas => {
  const p = personas.generate({ country: 'BR', sex: 'female', ages: { min: 25, max: 40 } });
  console.log(p.name, p.age, p.city.name, p.occupation.job?.label);

  // The same seed, request and data give the same persona, on every platform.
  const again = personas.generate({ country: 'BR', sex: 'female', ages: { min: 25, max: 40 }, seed: p.signature.seed });
});
```

The package is an ES module only (no CommonJS build), for browsers, Web Workers and Node 20+.

## Loading

`createPersonaGenerator()` instantiates the module and loads the data of all eight libraries. That takes **a second or two** (about 1.4 s on a desktop, more on a phone) and blocks the thread it runs on, and a generator uses about 90 MB of memory. So:

- **Create one generator and reuse it.** Each persona then takes about a third of a millisecond.
- **In a browser, create it in a Web Worker** so the page stays responsive, and post the personas (plain JSON) back:

```js
// persona-worker.js, started with new Worker(new URL('./persona-worker.js', import.meta.url), { type: 'module' })
import { createPersonaGenerator } from '@dasmig/persona';

const ready = createPersonaGenerator();
onmessage = async ({ data: request }) => {
  const personas = await ready;
  try {
    postMessage({ persona: personas.generate(request) });
  } catch (error) {
    postMessage({ error: error.message });
  }
};
```

In a browser the module finds `persona.wasm` next to itself (`new URL('persona.wasm', import.meta.url)`). Vite and webpack 5 follow that and emit the file as an asset; webpack warns that it is over its recommended asset size, which is expected. To serve the file from elsewhere, pass Emscripten's `locateFile`, or pass its bytes as `wasmBinary`:

```js
const personas = await createPersonaGenerator({ locateFile: file => '/assets/' + file });
```

In Node, the package's `node` export reads `persona.wasm` from disk.

`persona.wasm` is 8.3 MB, most of it data (city and name lists): 2.8 MB with gzip, 2.2 MB with brotli. Serve it compressed.

## Request

Every field is optional. What is left out is drawn; what is given conditions the draws that depend on it. The country sets the city, the name culture, the body and the job; the age and sex set the rest.

| Field | Type | Default |
| --- | --- | --- |
| `country` | An ISO 3166-1 alpha-2 code from `personas.countries` (79 countries), any case | One of `personas.countries`, weighted by population |
| `sex` | `'male'` or `'female'` | Drawn with the birth |
| `ages` | `{ min, max }`, completed years, 0-120 | The country's age pyramid |
| `traits` | Number of personality traits, 0 to `personas.maxTraits` | 3 |
| `referenceDate` | `'YYYY-MM-DD'` or a `Date` (its local date), 1900-2100 | `'2023-07-01'` |
| `seed` | A `bigint`, a non-negative safe integer, or a **hexadecimal** string | Random |

- **Countries.** Only the countries in `personas.countries` have data in all eight libraries, so another code is an error rather than a persona with a name from the wrong culture.
- **Traits.** Fewer traits than asked are returned when no other trait is compatible with those already chosen (traits in one group exclude each other).
- **Dates.** The generator never reads the clock. Pass `referenceDate: new Date()` for ages as of today, and keep the date with the seed if the persona must replay.
- **Seeds.** A seed is a 64-bit unsigned integer. A string is read as hexadecimal, optionally after `0x`, which is the form of `signature.seed`: `seed: '42'` is 0x42 (66), not 42. Use a `bigint` or a hexadecimal string for seeds above `Number.MAX_SAFE_INTEGER`.

An invalid request throws before anything is generated: a `TypeError` for a field of the wrong type or an unknown field (a misspelt `contry` is an error, not a random country), and a `RangeError` for a value out of range. The message says which field and why. A failed call leaves the generator usable.

## Result

`generate()` returns a `Persona` (see `index.d.ts`). It has `name`, `sex`, `age`, `admin1` (the state or province) and `signature`, and each library's own result under `country`, `city`, `birth`, `biodata`, `personality`, `interest` and `occupation`. `generateJson()` returns the same as JSON text, exactly as the C++ library writes it.

Each part draws from its own random stream, split from the persona's seed. Forcing one choice does not shift the unrelated parts: the same seed with the other sex keeps the city and the personality.

## Replaying personas

To recreate a persona later (for example, from a saved game), store its `signature.seed` and the request it was made with, including `referenceDate`.

The same seed and request give the same persona **only with the same data**. `personas.dataHash` identifies the data of all eight libraries, and `personas.versions` lists the library versions the module was built from. Store `dataHash` with the seed: if a package update changes it, the saved seed gives a different persona.

Each part of a persona also has a `signature`, with its own seed and its library's data hash.

## Data

The data comes from open datasets (GeoNames, mledoze/countries, UN World Population Prospects, NCD-RisC, the CIA World Factbook, ILOSTAT and others) and from catalogues written for the libraries. `DATA_LICENSES.txt` lists each source and its licence. The names come from [name-dataset](https://github.com/philipperemy/name-dataset); see name-generator's [data notes](https://github.com/dasmig/name-generator/blob/HEAD/doc/data.md) for its provenance.

Each library documents its data's limits in its own `doc/data.md`.

## Licence

MIT for the code; see `DATA_LICENSES.txt` for the data.
