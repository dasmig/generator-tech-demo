# generator-tech-demo

Live WebAssembly demos for the [name-generator](https://github.com/dasmig/name-generator), [nickname-generator](https://github.com/dasmig/nickname-generator), [entity-generator](https://github.com/dasmig/entity-generator), [birth-generator](https://github.com/dasmig/birth-generator), [biodata-generator](https://github.com/dasmig/biodata-generator), [city-generator](https://github.com/dasmig/city-generator), and [country-generator](https://github.com/dasmig/country-generator) C++23 libraries.

Everything runs client-side in the browser — no server required.

## Prerequisites

- [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) 3.1.50+
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

The CMake file expects the generator libraries as sibling directories:

```
Repo/
├── name-generator/
├── nickname-generator/
├── entity-generator/
├── birth-generator/
├── biodata-generator/
├── city-generator/
├── country-generator/
└── generator-tech-demo/    ← this repo
```

```bash
cd generator-tech-demo
emcmake cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This produces in `build/`:
- `namegen_demo.js` + `namegen_demo.wasm` + `namegen_demo.data`
- `nickgen_demo.js` + `nickgen_demo.wasm` + `nickgen_demo.data`
- `entitygen_demo.js` + `entitygen_demo.wasm`
- `birthgen_demo.js` + `birthgen_demo.wasm` + `birthgen_demo.data`
- `biodatagen_demo.js` + `biodatagen_demo.wasm` + `biodatagen_demo.data`
- `citygen_demo.js` + `citygen_demo.wasm` + `citygen_demo.data`
- `countrygen_demo.js` + `countrygen_demo.wasm` + `countrygen_demo.data`
- `personagen_demo.js` + `personagen_demo.wasm` + `personagen_demo.data`

The **persona** demo bundles resources from all six data-driven libraries into a
single Wasm module (~80 countries supported across all libraries).

## Run locally

Copy the build artifacts next to `index.html` and serve with any HTTP server:

```bash
cp build/*.js build/*.wasm build/*.data .
python3 -m http.server 8080
```

Then open `http://localhost:8080`.

## License

MIT
