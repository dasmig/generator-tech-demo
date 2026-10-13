# generator-tech-demo

Live WebAssembly demos for the [name-generator](https://github.com/dasmig/name-generator), [nickname-generator](https://github.com/dasmig/nickname-generator), [entity-generator](https://github.com/dasmig/entity-generator), [birth-generator](https://github.com/dasmig/birth-generator), [biodata-generator](https://github.com/dasmig/biodata-generator), [city-generator](https://github.com/dasmig/city-generator), [country-generator](https://github.com/dasmig/country-generator), and [interest-generator](https://github.com/dasmig/interest-generator) C++23 libraries.

Everything runs client-side in the browser — no server required.

The demos use the libraries' releases on [dasmig-core 1.0](https://github.com/dasmig/dasmig-core): name-generator v3.0.0, interest-generator v1.0.0 and v2.0.0 of the others. Their `lite` data is compiled into the modules (`DASMIG_*_EMBED`), so a seed gives the same result here as in a native build. Birth dates and ages are computed as of the day the page is opened: the libraries never read the clock, so the demos pass today's date to birth-generator (`src/today.hpp`).

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
└── generator-tech-demo/    ← this repo
```

```bash
cd generator-tech-demo
emcmake cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This produces in `build/` a `.js` and a `.wasm` file per demo (`namegen_demo`, `nickgen_demo`, `entitygen_demo`, `birthgen_demo`, `biodatagen_demo`, `citygen_demo`, `countrygen_demo`, `interestgen_demo`, `personagen_demo`), plus `personagen_demo.data`, which holds city-generator's admin1 names (the only file not compiled in).

The **persona** demo composes seven libraries and compiles in the data of all six data-driven ones
(~80 countries supported across all libraries).

## Run locally

Copy the build artifacts next to `index.html` and serve with any HTTP server:

```bash
cp build/*.js build/*.wasm build/*.data .
python3 -m http.server 8080
```

Then open `http://localhost:8080`.

## License

MIT
