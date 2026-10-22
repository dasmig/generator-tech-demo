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
set(ADDRESS_GEN_DIR "${CMAKE_SOURCE_DIR}/../address-generator" CACHE PATH "Path to address-generator repo")
set(USER_GEN_DIR "${CMAKE_SOURCE_DIR}/../user-generator" CACHE PATH "Path to user-generator repo")
set(RELGEN_GEN_DIR "${CMAKE_SOURCE_DIR}/../relationship-generator" CACHE PATH "Path to relationship-generator repo")
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

# address-generator demo: the data is a pack the page writes into /pack
# (see scripts/make_web_pack.py in address-generator), not compiled in
add_executable(addressgen_demo src/addressgen_demo.cpp)
target_include_directories(addressgen_demo PRIVATE "${ADDRESS_GEN_DIR}")
target_link_options(addressgen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sFORCE_FILESYSTEM=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8','FS']
  -sMODULARIZE=1
  -sEXPORT_NAME=createAddressgenModule
  -sENVIRONMENT=web
)

# user-generator demo: six libraries' lite data is compiled in; address-generator's
# is a pack the page writes into /pack, as for the address demo
set(USER_INCLUDE_DIRS
  "${USER_GEN_DIR}"
  "${COUNTRY_GEN_DIR}"
  "${BIRTH_GEN_DIR}"
  "${NAME_GEN_DIR}"
  "${NICK_GEN_DIR}"
  "${ADDRESS_GEN_DIR}"
  "${PHONE_GEN_DIR}"
  "${EMAIL_GEN_DIR}"
)
set(USER_EMBED DASMIG_COUNTRY_EMBED DASMIG_BIRTH_EMBED DASMIG_NAME_EMBED DASMIG_NICKNAME_EMBED
  DASMIG_PHONE_EMBED DASMIG_EMAIL_EMBED DASMIG_USER_EMBED)
add_executable(usergen_demo src/usergen_demo.cpp)
target_include_directories(usergen_demo PRIVATE ${USER_INCLUDE_DIRS})
target_compile_definitions(usergen_demo PRIVATE ${USER_EMBED})
target_link_options(usergen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sFORCE_FILESYSTEM=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8','FS']
  -sMODULARIZE=1
  -sEXPORT_NAME=createUsergenModule
  -sENVIRONMENT=web
)

# relationship-generator demo: its own lite tier, birth-generator's and name-generator's are compiled in
set(RELGEN_INCLUDE_DIRS
  "${RELGEN_GEN_DIR}"
  "${BIRTH_GEN_DIR}"
  "${NAME_GEN_DIR}"
)
set(RELGEN_EMBED DASMIG_RELATIONSHIP_EMBED DASMIG_BIRTH_EMBED DASMIG_NAME_EMBED)
add_executable(relgen_demo src/relgen_demo.cpp)
target_include_directories(relgen_demo PRIVATE ${RELGEN_INCLUDE_DIRS})
target_compile_definitions(relgen_demo PRIVATE ${RELGEN_EMBED})
target_link_options(relgen_demo PRIVATE
  --bind
  -sWASM=1
  -sALLOW_MEMORY_GROWTH=1
  -sEXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8']
  -sMODULARIZE=1
  -sEXPORT_NAME=createRelgenModule
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
  "${RELGEN_GEN_DIR}" # persona-generator 4.1.0 includes relgen.hpp; its data is not compiled in here
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
