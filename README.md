# generator-tech-demo

Live WebAssembly demos for the [name-generator](https://github.com/dasmig/name-generator), [nickname-generator](https://github.com/dasmig/nickname-generator), [entity-generator](https://github.com/dasmig/entity-generator), [birth-generator](https://github.com/dasmig/birth-generator), [biodata-generator](https://github.com/dasmig/biodata-generator), [city-generator](https://github.com/dasmig/city-generator), [country-generator](https://github.com/dasmig/country-generator), [interest-generator](https://github.com/dasmig/interest-generator), [trait-generator](https://github.com/dasmig/trait-generator), [occupation-generator](https://github.com/dasmig/occupation-generator), [phone-generator](https://github.com/dasmig/phone-generator), [email-generator](https://github.com/dasmig/email-generator), [address-generator](https://github.com/dasmig/address-generator), [device-generator](https://github.com/dasmig/device-generator), [user-generator](https://github.com/dasmig/user-generator), [relationship-generator](https://github.com/dasmig/relationship-generator), [weather-generator](https://github.com/dasmig/weather-generator), [vehicle-generator](https://github.com/dasmig/vehicle-generator) and [persona-generator](https://github.com/dasmig/persona-generator) C++23 libraries.

Everything runs client-side in the browser — no server required.

The demos use the libraries' releases on [dasmig-core 1.3](https://github.com/dasmig/dasmig-core) listed below, all with the **request-first API**. Every generation call supplies the library's request struct; deterministic calls supply the request followed by a call seed, without generator-wide seed or option controls. Their `lite` data is compiled into the modules (`DASMIG_*_EMBED`), so the same seed, inputs, date and data give the same result here as in a native build. The standalone birth demo accepts an **Age as of** reference date (1900–2100), defaulting to today's local date. Each draw generates a birthday and its completed-year age relative to that date; changing the reference does not age a previously generated birthday. The persona demo passes today's date (`src/today.hpp`) as its request's reference date. The libraries never read the clock themselves.

`.github/workflows/deploy.yml` checks out these release tags, so the site changes only when a tag moves. Advance the tags and smoke expectations together when adopting a newer API or dataset:

| Library | Tag |
|---|---|
| name-generator | v4.5.1 |
| nickname-generator | v5.2.1 |
| entity-generator | v4.2.2 |
| country-generator, birth-generator | v3.2.1 |
| city-generator | v4.2.1 |
| biodata-generator | v5.0.1 |
| interest-generator, occupation-generator | v3.0.2 |
| trait-generator | v2.1.3 |
| phone-generator, email-generator | v2.1.2 |
| address-generator | v2.1.1 |
| device-generator | v1.0.3 |
| user-generator | v2.1.1 |
| relationship-generator | v1.2.1 |
| persona-generator | v4.1.2 |
| weather-generator | v2.0.0 |
| vehicle-generator | v2.0.0 |

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

The CMake file expects the generator libraries as sibling directories, checked out at the tags above (the deploy workflow checks out those tags):

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
├── phone-generator/
├── email-generator/
├── address-generator/
├── device-generator/
├── user-generator/
├── relationship-generator/
├── persona-generator/
├── weather-generator/
├── vehicle-generator/
└── generator-tech-demo/    ← this repo
```

```bash
cd generator-tech-demo
emcmake cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This produces in `build/` a `.js` and a `.wasm` file per demo (`namegen_demo`, `nickgen_demo`, `entitygen_demo`, `birthgen_demo`, `biodatagen_demo`, `citygen_demo`, `countrygen_demo`, `interestgen_demo`, `occupationgen_demo`, `phonegen_demo`, `emailgen_demo`, `addressgen_demo`, `devicegen_demo`, `usergen_demo`, `relgen_demo`, `traitgen_demo`, `weathergen_demo`, `vehiclegen_demo`, `personagen_demo`), plus `personagen_demo.data`, which holds city-generator's admin1 names. The address and user demos read address-generator's data pack (`scripts/make_web_pack.py` in that repository) instead of compiled-in data.

## The persona npm package

The production build of the persona generator, `@dasmig/persona` (an ES module for browsers, workers and Node 20+ with TypeScript types), lives in [persona-generator](https://github.com/dasmig/persona-generator/tree/main/package), next to the library it is compiled from; it moved there from this repository in persona-generator 3.0.1. `npm install @dasmig/persona`.

All modules, demos included, use native WebAssembly exceptions (`-fwasm-exceptions`; Chrome 95, Firefox 100, Safari 15.2 and Node 17 or later). Emscripten's JavaScript-based exceptions route every call that may throw through JavaScript, which made loading the persona data about 1.6 times slower.

## What the demos show

The **country and city** demos retain population-weighted single draws and add side-by-side batches of 100, 500 or 2,000 draws per mode. Both modes use the same filter and paired 32-bit random call seeds. Sampling is with replacement: counts include repeats, and shares divide each count by that mode's draw total. Weighted selection uses `max(population, 1)`; uniform selection gives each eligible catalogue row equal probability. All observed rows are shown, sorted by combined count; unobserved rows are omitted. Unique counts and the full loaded catalogue size are labeled separately. City rows use GeoNames IDs so namesakes are not merged. These are samples from embedded lite data, not exact population proportions or exhaustive world coverage.

The **weather** demo draws a year of daily weather (weather-generator's `get_weather_series`, one call seed per year) at the point clicked on the map or typed in, in metric or imperial units, and charts the highs and lows against the cell's normal mean, with the precipitation below; the year's numbers include snow on the ground, fog days, the strongest gust and the solar radiation. Picking a day shows its 24 hours (`weather_request::hourly`), the same day as in the year since a day depends only on the seed, the place and the date. The map tiles come from OpenStreetMap; the weather does not need them. The historical mode (`historical`) is left out: its 23 MB table is not compiled in.

The **entity** demo identifies each field's `choice_component`, `range_component` or `weighted_choice_component` supplier. The **persona** demo shows a profile with expandable location panels and a provenance table: for each key of the persona's JSON, the library and the persona-generator pipeline field that supply it and what that field reads.

The **biodata** comparison varies only sex, keeping country and call seed fixed. The pinned API accepts optional age, but this demo leaves it unset to preserve the unadjusted comparison. The **interest** comparison varies only ages 8, 30 and 72, keeping country, sex input and call seed fixed. An unspecified interest age/sex applies no corresponding factor; it is not a randomly chosen demographic input. “Any” country is resolved once per comparison. Interest religion is country-weighted; age factors and limits apply to hobbies and genres. Matching outputs are possible. All profiles and comparisons are fictional model outputs, not measurements, medical advice or predictions about real people or groups.

The **occupation** comparison varies only the education input (the four levels), keeping country, sex, call seed, age 35 and an employed status fixed. “Any” country and sex are resolved once. Under 15 a person is always a student with no job. Status, education and the occupation groups are measured (ILO labour force statistics; countries without a survey use a regional mean), while the job catalogue with its skills and gear is curated, so a job is a plausible example of its group, not a measured fact. The persona demo adds an Occupation section from the persona’s country, age and sex.

The **name** demo offers 1, 5 or 10 alternatives with first-name, surname or first-name-plus-surname modes. Culture applies to all parts; gender filters first names only. The **nickname** demo offers the same alternative counts with an optional name input. Blank input uses wordlists; with a name, the library derives from it in one in four draws on average and otherwise still uses wordlists. There is no forced derivation/style control. The output shows each nickname's pre-transform source. Alternatives may repeat.

The standalone **trait** demo generates fictional traits and all five resolved Big Five scores. Choose a trait count and optionally enter a 64-bit seed as 1–16 hexadecimal digits (without `0x`). The output includes the replay seed; use it with the same count to reproduce a result. The explanation shows the actual random base, summed trait effects and resolved score, plus each selected trait's catalogue loadings. Each base is drawn from a normal distribution with mean 0 and standard deviation 1; base values also influence trait selection, alongside weights, exclusive groups, conflicts and synergies. Resolved scores add trait loadings and are unbounded model values, not percentages, percentiles or a personality assessment. Individual effects are obtained through the library's include-only requests with the same seed, not a duplicated score formula. The catalogue is embedded (`DASMIG_TRAIT_EMBED`), with no additional data file.

The **one person, everywhere** card runs the persona, address, phone, email, device, vehicle and weather demos' libraries for one country on one call seed: the persona sets the name, age and city, the email is built from its name and birth year, and the weather is its city today. The parts are composed in the page, not by a library (persona-generator does not include vehicles or weather yet); a part the country has no data for is marked rather than hidden. `tests/consistency.mjs` checks that `deploy.yml`, this README, `CMakeLists.txt` and `index.html` list the same libraries.

The **vehicle** demo draws a car or motorcycle from the chosen country, plate region, make, kind, body, fuel and date (today by default, 2000 to 2050), with the odometer in the country's unit, kilometres or miles. The region and make lists are the country's (`plate_regions`, `makes`), and the kind limits the makes and bodies to cars or motorcycles; without a country, the region picks one that has it. Its comparison draws one call seed in Germany, the United Kingdom, Spain, the United States and Japan: each part draws from its own stream, so the same seed meets each country's fleet, plate format and factories. The safe option gives plates no authority issues and VINs whose WMI NHTSA vPIC does not know.

The **device** demo draws a device from the chosen country, platform, browser, IP family, locale and date (today by default; a date after the data's newest release is seen as that date). Its comparison sees one call seed in June 2021, 2022, 2023 and 2024 and today, with the country and platform fixed: the address stays, while the model, OS and browser versions, user agent and client hints are those of each date. The safe option gives documentation addresses only (RFC 5737, RFC 3849).

The **user** demo generates a user in the chosen country, with ages and account times as of today. With *Device* (on by default) it also shows the device of the last sign-in (user-generator's `user_request::device`): the account's platform and locale on a model, OS and browser of the last login's date, with its user agent and `Accept-Language` header; asking for it changes nothing else. *Compare safe* shows one call seed with and without the safe option: the email domain, the phone number and the device's IP address change, the person, username and address stay.

The **persona** demo calls [persona-generator](https://github.com/dasmig/persona-generator): one `psg::get_persona(request, seed)` runs the eight libraries as entity-generator fields in dependency order, and the module returns the persona's `to_json()`. The page sends a `persona_request` with the chosen country and sex and today's date; the country list is persona-generator's `supported_countries()` (79 countries; South Korea needs name-generator's `full` tier). Body, interests and occupation are drawn for the persona's country, age and sex. Each field draws from its own stream split from the persona's seed, so the seed replays the whole persona, and forcing the sex changes only the parts that depend on it. All eight libraries' `lite` data is compiled in.

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
The same test also covers all other demos: distributions against the pinned country/city TSV data, exact trait loadings against the catalogue, comparison invariants, complete component provenance, name/nickname modes and counts, leap/reference-date boundaries, accessible labels, keyboard expansion and 320/375/640px layouts. Additional `*Seeded` bindings return library JSON (persona returns persona-generator's JSON) and accept 1–16 hexadecimal seed digits as strings, preserving all 64 bits across JavaScript/Wasm. They exercise deterministic replay in every library and the full persona, interleaved request filters and weighting, birth year/age ranges, optional biodata age effects, interest list counts, occupation status/education, and entity component selection. These bindings do not change the UI defaults. It reads pinned library fixtures from sibling directories by default; pass their parent directory as the third argument for another layout:

```bash
node tests/traitgen_smoke.mjs . path/to/library-parent
```

The deployment workflow passes its checked-out library directory explicitly. Set `DEMO_SCREENSHOT_DIR` to optionally save desktop/mobile PNGs during the same test.

## License

MIT
