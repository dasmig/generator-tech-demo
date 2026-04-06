cmake_minimum_required(VERSION 3.20)
project(generator-tech-demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Library paths (sibling repos by default) ──
set(NAME_GEN_DIR "${CMAKE_SOURCE_DIR}/../name-generator" CACHE PATH "Path to name-generator repo")
set(NICK_GEN_DIR "${CMAKE_SOURCE_DIR}/../nickname-generator" CACHE PATH "Path to nickname-generator repo")
set(ENTITY_GEN_DIR "${CMAKE_SOURCE_DIR}/../entity-generator" CACHE PATH "Path to entity-generator repo")

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
  --preload-file "${NAME_GEN_DIR}/resources@/resources"
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
