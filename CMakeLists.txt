cmake_minimum_required(VERSION 3.20)
project(generator-tech-demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Surface library input errors to the browser instead of aborting the module.
# Native WebAssembly exceptions: JavaScript-based ones (-fexceptions) route
# every call that may throw through JavaScript, which made loading the data
# about 1.6 times slower.
add_compile_options(-fwasm-exceptions)
add_link_options(-fwasm-exceptions)

# ── Library paths (sibling repos by default) ──
set(NAME_GEN_DIR "${CMAKE_SOURCE_DIR}/../name-generator" CACHE PATH "Path to name-generator repo")
set(NICK_GEN_DIR "${CMAKE_SOURCE_DIR}/../nickname-generator" CACHE PATH "Path to nickname-generator repo")
set(ENTITY_GEN_DIR "${CMAKE_SOURCE_DIR}/../entity-generator" CACHE PATH "Path to entity-generator repo")
set(COUNTRY_GEN_DIR "${CMAKE_SOURCE_DIR}/../country-generator" CACHE PATH "Path to country-generator repo")
set(CITY_GEN_DIR "${CMAKE_SOURCE_DIR}/../city-generator" CACHE PATH "Path to city-generator repo")
set(BIRTH_GEN_DIR "${CMAKE_SOURCE_DIR}/../birth-generator" CACHE PATH "Path to birth-generator repo")
set(BIODATA_GEN_DIR "${CMAKE_SOURCE_DIR}/../biodata-generator" CACHE PATH "Path to biodata-generator repo")
set(INTEREST_GEN_DIR "${CMAKE_SOURCE_DIR}/../interest-generator" CACHE PATH "Path to interest-generator repo")
set(TRAIT_GEN_DIR "${CMAKE_SOURCE_DIR}/../trait-generator" CACHE PATH "Path to trait-generator repo")
set(OCCUPATION_GEN_DIR "${CMAKE_SOURCE_DIR}/../occupation-generator" CACHE PATH "Path to occupation-generator repo")
set(PHONE_GEN_DIR "${CMAKE_SOURCE_DIR}/../phone-generator" CACHE PATH "Path to phone-generator repo")
set(EMAIL_GEN_DIR "${CMAKE_SOURCE_DIR}/../email-generator" CACHE PATH "Path to email-generator repo")
set(PERSONA_GEN_DIR "${CMAKE_SOURCE_DIR}/../persona-generator" CACHE PATH "Path to persona-generator repo")

# ── Wasm modules ──
#
# The libraries' lite data is compiled in (DASMIG_*_EMBED), so the modules
# need no preloaded files; only city's admin1 names are still a file.

# name-generator demo
add_executable(namegen_demo src/namegen_demo.cpp)
target_include_directories(namegen_demo PRIVATE "${NAME_GEN_DIR}")
target_compile_definitions(namegen_demo PRIVATE DASMIG_NAME_EMBED)
target_link_options(namegen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createNamegenModule
  -sENVIRONMENT=web
)

# nickname-generator demo
add_executable(nickgen_demo src/nickgen_demo.cpp)
target_include_directories(nickgen_demo PRIVATE "${NICK_GEN_DIR}")
target_compile_definitions(nickgen_demo PRIVATE DASMIG_NICKNAME_EMBED)
target_link_options(nickgen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createNickgenModule
  -sENVIRONMENT=web
)

# entity-generator demo (no resource files needed)
add_executable(entitygen_demo src/entitygen_demo.cpp)
target_include_directories(entitygen_demo PRIVATE "${ENTITY_GEN_DIR}")
target_link_options(entitygen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createEntitygenModule
  -sENVIRONMENT=web
)

# country-generator demo
add_executable(countrygen_demo src/countrygen_demo.cpp)
target_include_directories(countrygen_demo PRIVATE "${COUNTRY_GEN_DIR}")
target_compile_definitions(countrygen_demo PRIVATE DASMIG_COUNTRY_EMBED)
target_link_options(countrygen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createCountrygenModule
  -sENVIRONMENT=web
)

# city-generator demo
add_executable(citygen_demo src/citygen_demo.cpp)
target_include_directories(citygen_demo PRIVATE "${CITY_GEN_DIR}")
target_compile_definitions(citygen_demo PRIVATE DASMIG_CITY_EMBED)
target_link_options(citygen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createCitygenModule
  -sENVIRONMENT=web
)

# birth-generator demo
add_executable(birthgen_demo src/birthgen_demo.cpp)
target_include_directories(birthgen_demo PRIVATE "${BIRTH_GEN_DIR}")
target_compile_definitions(birthgen_demo PRIVATE DASMIG_BIRTH_EMBED)
target_link_options(birthgen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createBirthgenModule
  -sENVIRONMENT=web
)

# biodata-generator demo
add_executable(biodatagen_demo src/biodatagen_demo.cpp)
target_include_directories(biodatagen_demo PRIVATE "${BIODATA_GEN_DIR}")
target_compile_definitions(biodatagen_demo PRIVATE DASMIG_BIODATA_EMBED)
target_link_options(biodatagen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createBiodatagenModule
  -sENVIRONMENT=web
)

# interest-generator demo
add_executable(interestgen_demo src/interestgen_demo.cpp)
target_include_directories(interestgen_demo PRIVATE "${INTEREST_GEN_DIR}")
target_compile_definitions(interestgen_demo PRIVATE DASMIG_INTEREST_EMBED)
target_link_options(interestgen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createInterestgenModule
  -sENVIRONMENT=web
)

# phone-generator demo
add_executable(phonegen_demo src/phonegen_demo.cpp)
target_include_directories(phonegen_demo PRIVATE "${PHONE_GEN_DIR}")
target_compile_definitions(phonegen_demo PRIVATE DASMIG_PHONE_EMBED)
target_link_options(phonegen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createPhonegenModule
  -sENVIRONMENT=web
)

# email-generator demo
add_executable(emailgen_demo src/emailgen_demo.cpp)
target_include_directories(emailgen_demo PRIVATE "${EMAIL_GEN_DIR}")
target_compile_definitions(emailgen_demo PRIVATE DASMIG_EMAIL_EMBED)
target_link_options(emailgen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createEmailgenModule
  -sENVIRONMENT=web
)

# trait-generator demo
add_executable(traitgen_demo src/traitgen_demo.cpp)
target_include_directories(traitgen_demo PRIVATE "${TRAIT_GEN_DIR}")
target_compile_definitions(traitgen_demo PRIVATE DASMIG_TRAIT_EMBED)
target_link_options(traitgen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createTraitgenModule
  -sENVIRONMENT=web
)

# occupation-generator demo
add_executable(occupationgen_demo src/occupationgen_demo.cpp)
target_include_directories(occupationgen_demo PRIVATE "${OCCUPATION_GEN_DIR}")
target_compile_definitions(occupationgen_demo PRIVATE DASMIG_OCCUPATION_EMBED)
target_link_options(occupationgen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createOccupationgenModule
  -sENVIRONMENT=web
)

# persona-generator demo (persona-generator composes the eight libraries)
set(PERSONA_INCLUDE_DIRS
  "${PERSONA_GEN_DIR}"
  "${ENTITY_GEN_DIR}"
  "${COUNTRY_GEN_DIR}"
  "${CITY_GEN_DIR}"
  "${BIRTH_GEN_DIR}"
  "${BIODATA_GEN_DIR}"
  "${NAME_GEN_DIR}"
  "${INTEREST_GEN_DIR}"
  "${OCCUPATION_GEN_DIR}"
  "${TRAIT_GEN_DIR}"
)
set(PERSONA_EMBED DASMIG_COUNTRY_EMBED DASMIG_CITY_EMBED DASMIG_BIRTH_EMBED DASMIG_BIODATA_EMBED
  DASMIG_NAME_EMBED DASMIG_INTEREST_EMBED DASMIG_OCCUPATION_EMBED DASMIG_TRAIT_EMBED)
add_executable(personagen_demo src/personagen_demo.cpp)
target_include_directories(personagen_demo PRIVATE ${PERSONA_INCLUDE_DIRS})
target_compile_definitions(personagen_demo PRIVATE ${PERSONA_EMBED})
target_link_options(personagen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createPersonagenModule
  -sENVIRONMENT=web
  "SHELL:--preload-file ${CITY_GEN_DIR}/resources/admin1CodesASCII.txt@/resources/city/admin1CodesASCII.txt"
)

# ── Production persona module: the npm package in package/ ──
#
# A small C API (src/persona_api.cpp) built as an ES module. The module is
# built for browsers and workers only, so bundlers find no Node code in it;
# the package's Node entry (package/node.mjs) reads the .wasm file and passes
# it in. city's admin1 names are embedded in the module instead of preloaded,
# so nothing is fetched but persona.wasm. The build assembles the package in
# build/package (npm pack ./build/package).
set(PACKAGE_DIR "${CMAKE_BINARY_DIR}/package")
add_executable(persona_api src/persona_api.cpp)
set_target_properties(persona_api PROPERTIES
  OUTPUT_NAME persona
  SUFFIX ".mjs"
  RUNTIME_OUTPUT_DIRECTORY "${PACKAGE_DIR}"
)
target_include_directories(persona_api PRIVATE ${PERSONA_INCLUDE_DIRS})
target_compile_definitions(persona_api PRIVATE ${PERSONA_EMBED})
target_link_options(persona_api PRIVATE
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sMODULARIZE=1
  -sEXPORT_ES6=1
  -sEXPORT_NAME=createPersonaModule
  -sENVIRONMENT=web,worker
  -sEXPORTED_FUNCTIONS=['_dasmig_persona_generate','_dasmig_persona_generate_batch','_dasmig_persona_info']
  -sEXPORTED_RUNTIME_METHODS=['cwrap']
  "SHELL:--embed-file ${CITY_GEN_DIR}/resources/admin1CodesASCII.txt@/resources/city/admin1CodesASCII.txt"
)
set(PACKAGE_SOURCES package.json index.mjs node.mjs generator.mjs batch.mjs pool.mjs pool-worker.mjs pool-worker-node.mjs pool-worker-core.mjs index.d.ts README.md)
foreach(file IN LISTS PACKAGE_SOURCES)
  add_custom_command(TARGET persona_api POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/package/${file}" "${PACKAGE_DIR}/${file}")
endforeach()
add_custom_command(TARGET persona_api POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/LICENSE" "${PACKAGE_DIR}/LICENSE")

# The package ships the eight libraries' compiled-in data, so it carries their
# data licences, gathered from each checkout.
set(DATA_LICENSES "The persona module compiles in the lite data of eight dasmig libraries.\nTheir data sources and licences follow, as each library states them.\n")
foreach(lib COUNTRY CITY BIRTH BIODATA NAME INTEREST OCCUPATION TRAIT)
  string(TOLOWER "${lib}-generator" lib_name)
  set(lib_file "${${lib}_GEN_DIR}/LICENSE_DATA.txt")
  string(APPEND DATA_LICENSES "\n== ${lib_name} ==\n\n")
  if(EXISTS "${lib_file}")
    file(READ "${lib_file}" lib_text)
    string(APPEND DATA_LICENSES "${lib_text}")
  else()
    string(APPEND DATA_LICENSES "See https://github.com/dasmig/${lib_name}/blob/HEAD/doc/data.md (Source).\n")
  endif()
endforeach()
file(WRITE "${PACKAGE_DIR}/DATA_LICENSES.txt" "${DATA_LICENSES}")

# The library versions compiled in (their git tags), for generator.versions:
# a persona replays only with the same data.
set(PACKAGE_VERSIONS "// Generated by CMakeLists.txt: the library versions this module was built from.\nexport const versions = Object.freeze({\n")
foreach(lib PERSONA ENTITY COUNTRY CITY BIRTH BIODATA NAME INTEREST OCCUPATION TRAIT)
  string(TOLOWER "${lib}-generator" lib_name)
  execute_process(COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY "${${lib}_GEN_DIR}" OUTPUT_VARIABLE lib_version
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET RESULT_VARIABLE lib_result)
  if(NOT lib_result EQUAL 0 OR lib_version STREQUAL "")
    set(lib_version "unknown")
  endif()
  string(APPEND PACKAGE_VERSIONS "  '${lib_name}': '${lib_version}',\n")
endforeach()
string(APPEND PACKAGE_VERSIONS "});\n")
file(WRITE "${PACKAGE_DIR}/versions.mjs" "${PACKAGE_VERSIONS}")
