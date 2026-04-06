# generator-tech-demo

Live WebAssembly demos for the [name-generator](https://github.com/dasmig/name-generator), [nickname-generator](https://github.com/dasmig/nickname-generator), and [entity-generator](https://github.com/dasmig/entity-generator) C++23 libraries.

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

## Run locally

Copy the build artifacts next to `index.html` and serve with any HTTP server:

```bash
cp build/*.js build/*.wasm build/*.data .
python3 -m http.server 8080
```

Then open `http://localhost:8080`.

## License

MIT
