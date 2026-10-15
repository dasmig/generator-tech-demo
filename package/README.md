# @dasmig/persona

Complete fictional characters from one seeded call: name, country, city, birth date and age, body, personality, religion, hobbies, favourite media and job. It is [persona-generator](https://github.com/dasmig/persona-generator) (C++23) compiled to WebAssembly, with the `lite` data of all eight libraries it composes built in, so nothing is fetched at run time except the module's own `.wasm` file.

The output is fictional sample data, not a description or prediction of a real person.

```js
import { createPersonaGenerator } from '@dasmig/persona';

const personas = await createPersonaGenerator();
const p = personas.generate({ country: 'BR', sex: 'female', ages: { min: 25, max: 40 } });
console.log(p.name, p.age, p.city.name, p.occupation.job?.label);

// The same seed, request and data give the same persona, on every platform.
const again = personas.generate({ country: 'BR', sex: 'female', ages: { min: 25, max: 40 }, seed: p.signature.seed });
```

## Request

Every field is optional; what is left out is drawn, and what is given conditions the draws that depend on it. The country sets the city, the name culture, the body and the job. The age and sex set the rest.

| Field | Type | Default |
| --- | --- | --- |
| `country` | ISO 3166-1 alpha-2 code, any case | One of `personas.countries` (79), weighted by population |
| `sex` | `'male'` or `'female'` | Drawn with the birth |
| `ages` | `{ min, max }`, completed years, 0-120 | The country's age pyramid |
| `traits` | Number of personality traits | 3 |
| `referenceDate` | `'YYYY-MM-DD'` or a `Date` (its local date) | `'2023-07-01'` |
| `seed` | 1-16 hex digits, a `bigint` or a safe integer | Random. The result's `signature.seed` replays it |

The generator never reads the clock. Pass `referenceDate: new Date()` for ages as of today, and keep the date with the seed if the persona must replay.

An invalid request throws: a `TypeError` for a malformed field, or an `Error` with the library's message (for example, an unknown country).

## Result

`generate()` returns a `Persona` (see `index.d.ts`). It has `name`, `sex`, `age`, `admin1` (the state or province), and each library's own result under `country`, `city`, `birth`, `biodata`, `personality`, `interest` and `occupation`. Every part has a `signature` with its seed and a hash of the data it used. `generateJson()` returns the same as JSON text, exactly as the C++ library writes it.

Forcing one choice does not shift the unrelated parts. Each part draws from its own stream, split from the persona's seed, so the same seed with another sex keeps the city and the personality.

## Loading

The package is an ES module for browsers, workers and Node 18+. The module finds `persona.wasm` next to itself (`new URL('persona.wasm', import.meta.url)`), which bundlers such as Vite and webpack 5 follow. To serve it from elsewhere, pass Emscripten's `locateFile`:

```js
const personas = await createPersonaGenerator({ locateFile: file => '/assets/' + file });
```

`persona.wasm` is about 8.4 MB, most of it data (city and name lists): 2.9 MB with gzip, 2.2 MB with brotli.

## Data

The data comes from open datasets (GeoNames, mledoze/countries, UN World Population Prospects, NCD-RisC, the CIA World Factbook, ILOSTAT, and others) and from catalogues written for the libraries. `DATA_LICENSES.txt` lists each source and its licence. The names come from [name-dataset](https://github.com/philipperemy/name-dataset); see name-generator's [data notes](https://github.com/dasmig/name-generator/blob/HEAD/doc/data.md) for its provenance.

Each library documents its data's limits in its `doc/data.md`.

## Licence

MIT for the code; see `DATA_LICENSES.txt` for the data.
