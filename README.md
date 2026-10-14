# generator-tech-demo

Live WebAssembly demos for the [name-generator](https://github.com/dasmig/name-generator), [nickname-generator](https://github.com/dasmig/nickname-generator), [entity-generator](https://github.com/dasmig/entity-generator), [birth-generator](https://github.com/dasmig/birth-generator), [biodata-generator](https://github.com/dasmig/biodata-generator), [city-generator](https://github.com/dasmig/city-generator), [country-generator](https://github.com/dasmig/country-generator), [interest-generator](https://github.com/dasmig/interest-generator), and [trait-generator](https://github.com/dasmig/trait-generator), and [occupation-generator](https://github.com/dasmig/occupation-generator) C++23 libraries.

Everything runs client-side in the browser — no server required.

The demos use the libraries' releases on [dasmig-core 1.0](https://github.com/dasmig/dasmig-core): name-generator v4.0.0, city-generator v4.0.0, nickname-generator, entity-generator, country-generator, birth-generator and biodata-generator v3.0.0, and interest-generator, trait-generator and occupation-generator v2.0.0, all with the **request-first API**. Every generation call supplies the library's request struct; deterministic calls supply the request followed by a call seed, without generator-wide seed or option controls. Their `lite` data is compiled into the modules (`DASMIG_*_EMBED`), so the same seed, inputs, date and data give the same result here as in a native build. The standalone birth demo accepts an **Age as of** reference date (1900–2100), defaulting to today's local date. Each draw generates a birthday and its completed-year age relative to that date; changing the reference does not age a previously generated birthday. The persona demo still passes today's date (`src/today.hpp`). The libraries never read the clock themselves.

`.github/workflows/deploy.yml` pins these exact merged commits for deterministic consumer integration coverage. Advance the pins and smoke expectations together when adopting a newer API or dataset:

| Library | Pinned merged commit |
|---|---|
| country-generator | `a01ce1fd91e082730a24cbfa4f49941e3930a7b6` |
| city-generator | `2429b4394374743f1370c0b733048d7f526960d9` |
| birth-generator | `a093ecb7916868ad9476b326119471768b0ed1a3` |
| biodata-generator | `e7fa38368e4741a6974e2556fdf35564e14116fe` |
| interest-generator | `b48a5e54bcad2d5ff4597effadc83dd33c9c73c7` |
| occupation-generator | `48cd5824c5212efe896b0125bc0268e4107a0652` |
| name-generator | `b7bc6a2f00361724cb28c43c3a8d78d5a2aecbee` |
| nickname-generator | `298fafab391b83ea0d918e3031cc8d1855dd0bdf` |
| entity-generator | `491eb7a839250e3da000bf71f77a0de323bfa71d` |
| trait-generator | `204ca1b65d09c2955b7ac4d4e8845511199794fa` |

## Prerequisites

- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) 3.1.74+ (the deployed site is built with 3.1.74)
- CMake 3.20+

## Setup

```bash
# Install / activate Emscripten
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk
./emsdk install latest
./emsdk activate latest
source ./emsdk_env.sh   # or emsdk_env.bat on Windows
cd ..
```

## Build

The CMake file expects the generator libraries as sibling directories, checked out at the exact commits above (the deploy workflow checks out those SHAs):

```
Repo/
├── name-generator/
├── nickname-generator/
├── entity-generator/
├── birth-generator/
├── biodata-generator/
├── city-generator/
├── country-generator/
├── interest-generator/
├── trait-generator/
├── occupation-generator/
└── generator-tech-demo/    ← this repo
```

```bash
cd generator-tech-demo
emcmake cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This produces in `build/` a `.js` and a `.wasm` file per demo (`namegen_demo`, `nickgen_demo`, `entitygen_demo`, `birthgen_demo`, `biodatagen_demo`, `citygen_demo`, `countrygen_demo`, `interestgen_demo`, `occupationgen_demo`, `traitgen_demo`, `personagen_demo`), plus `personagen_demo.data`, which holds city-generator's admin1 names (the only file not compiled in).

## What the demos show

The **country and city** demos retain population-weighted single draws and add side-by-side batches of 100, 500 or 2,000 draws per mode. Both modes use the same filter and paired 32-bit random call seeds. Sampling is with replacement: counts include repeats, and shares divide each count by that mode's draw total. Weighted selection uses `max(population, 1)`; uniform selection gives each eligible catalogue row equal probability. All observed rows are shown, sorted by combined count; unobserved rows are omitted. Unique counts and the full loaded catalogue size are labeled separately. City rows use GeoNames IDs so namesakes are not merged. These are samples from embedded lite data, not exact population proportions or exhaustive world coverage.

The **entity** demo identifies each field's `choice_component`, `range_component` or `weighted_choice_component` supplier. The **persona** demo preserves its profile and expandable location panels while adding a field-by-field provenance table: library, adapter component and upstream values. `entity-generator` orchestrates the components; it does not invent the country/name/biodata/interest fields itself.

The **biodata** comparison varies only sex, keeping country and call seed fixed. The pinned API accepts optional age, but this demo leaves it unset to preserve the unadjusted comparison. The **interest** comparison varies only ages 8, 30 and 72, keeping country, sex input and call seed fixed. An unspecified interest age/sex applies no corresponding factor; it is not a randomly chosen demographic input. “Any” country is resolved once per comparison. Interest religion is country-weighted; age factors and limits apply to hobbies and genres. Matching outputs are possible. All profiles and comparisons are fictional model outputs, not measurements, medical advice or predictions about real people or groups.

The **occupation** comparison varies only the education input (the four levels), keeping country, sex, call seed, age 35 and an employed status fixed. “Any” country and sex are resolved once. Under 15 a person is always a student with no job. Status, education and the occupation groups are measured (ILO labour force statistics; countries without a survey use a regional mean), while the job catalogue with its skills and gear is curated, so a job is a plausible example of its group, not a measured fact. The persona demo adds an Occupation section from the persona’s country, age and sex.

The **name** demo offers 1, 5 or 10 alternatives with first-name, surname or first-name-plus-surname modes. Culture applies to all parts; gender filters first names only. The **nickname** demo offers the same alternative counts with an optional name input. Blank input uses wordlists; with a name, the library derives from it in one in four draws on average and otherwise still uses wordlists. There is no forced derivation/style control. The output shows each nickname's pre-transform source. Alternatives may repeat.

The standalone **trait** demo generates fictional traits and all five resolved Big Five scores. Choose a trait count and optionally enter a 64-bit seed as 1–16 hexadecimal digits (without `0x`). The output includes the replay seed; use it with the same count to reproduce a result. The explanation shows the actual random base, summed trait effects and resolved score, plus each selected trait's catalogue loadings. Each base is drawn from a normal distribution with mean 0 and standard deviation 1; base values also influence trait selection, alongside weights, exclusive groups, conflicts and synergies. Resolved scores add trait loadings and are unbounded model values, not percentages, percentiles or a personality assessment. Individual effects are obtained through the library's include-only requests with the same seed, not a duplicated score formula. The catalogue is embedded (`DASMIG_TRAIT_EMBED`), with no additional data file.

The **persona** demo composes seven libraries and compiles in the data of all six data-driven ones
(~80 countries supported across all libraries), orchestrated by entity-generator. Its physical values remain explicitly **not age-adjusted**, including for child personas: the biodata adapter intentionally leaves optional age unset. All seven adapters use the entity context's per-component random stream as the upstream call seed, so the entire composition replays with the same entity seed, country, reference date and pinned data.

## Run locally

Copy the build artifacts next to `index.html` and serve with any HTTP server:

```bash
cp build/*.js build/*.wasm build/*.data .
python3 -m http.server 8080
```

Then open `http://localhost:8080`.

## Browser smoke test

After copying the build artifacts next to `index.html`, run with Node.js 22+ and Chrome installed:

```bash
node tests/traitgen_smoke.mjs .
```

Set `CHROME_PATH` if Chrome is not in its usual Windows location or on `PATH`. The test checks embedded data, trait counts, Big Five scores, 64-bit seed replay, input errors, module-load errors, keyboard generation and mobile layout. Pull requests run the Emscripten build and this test without deploying; only `master` deploys to Pages.
The same test also covers all other demos: distributions against the pinned country/city TSV data, exact trait loadings against the catalogue, comparison invariants, complete component provenance, name/nickname modes and counts, leap/reference-date boundaries, accessible labels, keyboard expansion and 320/375/640px layouts. Additional `*Seeded` bindings return library JSON (persona returns its existing field map) and accept 1–16 hexadecimal seed digits as strings, preserving all 64 bits across JavaScript/Wasm. They exercise deterministic replay in all ten libraries and the full persona, interleaved request filters and weighting, birth year/age ranges, optional biodata age effects, interest list counts, occupation status/education, and entity component selection. These bindings do not change the UI defaults. It reads pinned library fixtures from sibling directories by default; pass their parent directory as the third argument for another layout:

```bash
node tests/traitgen_smoke.mjs . path/to/library-parent
```

The deployment workflow passes its checked-out library directory explicitly. Set `DEMO_SCREENSHOT_DIR` to optionally save desktop/mobile PNGs during the same test.

## License

MIT
