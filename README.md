# generator-tech-demo

Live WebAssembly demos for the [name-generator](https://github.com/dasmig/name-generator), [nickname-generator](https://github.com/dasmig/nickname-generator), [entity-generator](https://github.com/dasmig/entity-generator), [birth-generator](https://github.com/dasmig/birth-generator), [biodata-generator](https://github.com/dasmig/biodata-generator), [city-generator](https://github.com/dasmig/city-generator), [country-generator](https://github.com/dasmig/country-generator), [interest-generator](https://github.com/dasmig/interest-generator), and [trait-generator](https://github.com/dasmig/trait-generator) C++23 libraries.

Everything runs client-side in the browser — no server required.

The demos use the libraries' releases on [dasmig-core 1.0](https://github.com/dasmig/dasmig-core): name-generator v3.0.0, interest-generator and trait-generator v1.0.0 and v2.0.0 of the others. Their `lite` data is compiled into the modules (`DASMIG_*_EMBED`), so the same seed, inputs, date and data give the same result here as in a native build. The standalone birth demo accepts an **Age as of** reference date (1900–2100), defaulting to today's local date. Each draw generates a birthday and its completed-year age relative to that date; changing the reference does not age a previously generated birthday. The persona demo still passes today's date (`src/today.hpp`). The libraries never read the clock themselves.

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

The CMake file expects the generator libraries as sibling directories, checked out at the releases above (the deploy workflow checks out those tags):

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
└── generator-tech-demo/    ← this repo
```

```bash
cd generator-tech-demo
emcmake cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This produces in `build/` a `.js` and a `.wasm` file per demo (`namegen_demo`, `nickgen_demo`, `entitygen_demo`, `birthgen_demo`, `biodatagen_demo`, `citygen_demo`, `countrygen_demo`, `interestgen_demo`, `traitgen_demo`, `personagen_demo`), plus `personagen_demo.data`, which holds city-generator's admin1 names (the only file not compiled in).

## What the demos show

The **country and city** demos retain population-weighted single draws and add side-by-side batches of 100, 500 or 2,000 draws per mode. Both modes use the same filter and paired 32-bit random call seeds. Sampling is with replacement: counts include repeats, and shares divide each count by that mode's draw total. Weighted selection uses `max(population, 1)`; uniform selection gives each eligible catalogue row equal probability. All observed rows are shown, sorted by combined count; unobserved rows are omitted. Unique counts and the full loaded catalogue size are labeled separately. City rows use GeoNames IDs so namesakes are not merged. These are samples from embedded lite data, not exact population proportions or exhaustive world coverage.

The **entity** demo identifies each field's `choice_component`, `range_component` or `weighted_choice_component` supplier. The **persona** demo preserves its profile and expandable location panels while adding a field-by-field provenance table: library, adapter component and upstream values. `entity-generator` orchestrates the components; it does not invent the country/name/biodata/interest fields itself.

The **biodata** comparison varies only sex, keeping country and call seed fixed. This pinned release accepts country and sex, **not age**. The **interest** comparison varies only ages 8, 30 and 72, keeping country, sex input and call seed fixed. An unspecified interest age/sex applies no corresponding factor; it is not a randomly chosen demographic input. “Any” country is resolved once per comparison. Interest religion is country-weighted; age factors and limits apply to hobbies and genres. Matching outputs are possible. All profiles and comparisons are fictional model outputs, not measurements, medical advice or predictions about real people or groups.

The **name** demo offers 1, 5 or 10 alternatives with first-name, surname or first-name-plus-surname modes. Culture applies to all parts; gender filters first names only. The **nickname** demo offers the same alternative counts with an optional name input. Blank input uses wordlists; with a name, the library derives from it in one in four draws on average and otherwise still uses wordlists. There is no forced derivation/style control. The output shows each nickname's pre-transform source. Alternatives may repeat.

The standalone **trait** demo generates fictional traits and all five resolved Big Five scores. Choose a trait count and optionally enter a 64-bit seed as 1–16 hexadecimal digits (without `0x`). The output includes the replay seed; use it with the same count to reproduce a result. The explanation shows the actual random base, summed trait effects and resolved score, plus each selected trait's catalogue loadings. Each base is drawn from a normal distribution with mean 0 and standard deviation 1; base values also influence trait selection, alongside weights, exclusive groups, conflicts and synergies. Resolved scores add trait loadings and are unbounded model values, not percentages, percentiles or a personality assessment. Individual effects are obtained through the library's include-only requests with the same seed, not a duplicated score formula. The catalogue is embedded (`DASMIG_TRAIT_EMBED`), with no additional data file.

The **persona** demo composes seven libraries and compiles in the data of all six data-driven ones
(~80 countries supported across all libraries). Its physical values are explicitly **not age-adjusted**, including for child personas, because the biodata adapter can pass only country and sex.

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
The same test also covers all other demos: distributions against the pinned country/city TSV data, exact trait loadings against the catalogue, comparison invariants, complete component provenance, name/nickname modes and counts, leap/reference-date boundaries, accessible labels, keyboard expansion and 320/375/640px layouts. It reads pinned library fixtures from sibling directories by default; pass their parent directory as the third argument for another layout:

```bash
node tests/traitgen_smoke.mjs . path/to/library-parent
```

The deployment workflow passes its checked-out library directory explicitly. Set `DEMO_SCREENSHOT_DIR` to optionally save desktop/mobile PNGs during the same test.

## License

MIT
