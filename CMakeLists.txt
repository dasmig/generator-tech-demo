cmake_minimum_required(VERSION 3.20)
project(generator-tech-demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Library paths (sibling repos by default) ──
set(NAME_GEN_DIR "${CMAKE_SOURCE_DIR}/../name-generator" CACHE PATH "Path to name-generator repo")
set(NICK_GEN_DIR "${CMAKE_SOURCE_DIR}/../nickname-generator" CACHE PATH "Path to nickname-generator repo")
set(ENTITY_GEN_DIR "${CMAKE_SOURCE_DIR}/../entity-generator" CACHE PATH "Path to entity-generator repo")
set(COUNTRY_GEN_DIR "${CMAKE_SOURCE_DIR}/../country-generator" CACHE PATH "Path to country-generator repo")
set(CITY_GEN_DIR "${CMAKE_SOURCE_DIR}/../city-generator" CACHE PATH "Path to city-generator repo")
set(BIRTH_GEN_DIR "${CMAKE_SOURCE_DIR}/../birth-generator" CACHE PATH "Path to birth-generator repo")
set(BIODATA_GEN_DIR "${CMAKE_SOURCE_DIR}/../biodata-generator" CACHE PATH "Path to biodata-generator repo")

# ── Wasm modules ──

# name-generator demo
add_executable(namegen_demo src/namegen_demo.cpp)
target_include_directories(namegen_demo PRIVATE "${NAME_GEN_DIR}")
target_link_options(namegen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createNamegenModule
  -sENVIRONMENT=web
  --preload-file "${NAME_GEN_DIR}/resources/lite@/resources/lite"
)

# nickname-generator demo
add_executable(nickgen_demo src/nickgen_demo.cpp)
target_include_directories(nickgen_demo PRIVATE "${NICK_GEN_DIR}")
target_link_options(nickgen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createNickgenModule
  -sENVIRONMENT=web
  --preload-file "${NICK_GEN_DIR}/resources@/resources"
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
target_link_options(countrygen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createCountrygenModule
  -sENVIRONMENT=web
  --preload-file "${COUNTRY_GEN_DIR}/resources/lite@/resources/lite"
)

# city-generator demo
add_executable(citygen_demo src/citygen_demo.cpp)
target_include_directories(citygen_demo PRIVATE "${CITY_GEN_DIR}")
target_link_options(citygen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createCitygenModule
  -sENVIRONMENT=web
  --preload-file "${CITY_GEN_DIR}/resources/lite@/resources/lite"
)

# birth-generator demo
add_executable(birthgen_demo src/birthgen_demo.cpp)
target_include_directories(birthgen_demo PRIVATE "${BIRTH_GEN_DIR}")
target_link_options(birthgen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createBirthgenModule
  -sENVIRONMENT=web
  --preload-file "${BIRTH_GEN_DIR}/resources/lite@/resources/lite"
)

# biodata-generator demo
add_executable(biodatagen_demo src/biodatagen_demo.cpp)
target_include_directories(biodatagen_demo PRIVATE "${BIODATA_GEN_DIR}")
target_link_options(biodatagen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createBiodatagenModule
  -sENVIRONMENT=web
  --preload-file "${BIODATA_GEN_DIR}/resources/lite@/resources/lite"
)

# persona-generator demo (composes all 7 libraries)
add_executable(personagen_demo src/personagen_demo.cpp)
target_include_directories(personagen_demo PRIVATE
  "${ENTITY_GEN_DIR}"
  "${COUNTRY_GEN_DIR}"
  "${CITY_GEN_DIR}"
  "${BIRTH_GEN_DIR}"
  "${BIODATA_GEN_DIR}"
  "${NAME_GEN_DIR}"
  "${NICK_GEN_DIR}"
)
target_link_options(personagen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createPersonagenModule
  -sENVIRONMENT=web
  --preload-file "${COUNTRY_GEN_DIR}/resources/lite@/resources/country/lite"
  --preload-file "${CITY_GEN_DIR}/resources/lite@/resources/city/lite"
  --preload-file "${BIRTH_GEN_DIR}/resources/lite@/resources/birth/lite"
  --preload-file "${BIODATA_GEN_DIR}/resources/lite@/resources/biodata/lite"
  --preload-file "${NAME_GEN_DIR}/resources/lite@/resources/name/lite"
  --preload-file "${NICK_GEN_DIR}/resources@/resources/nickname"
)
