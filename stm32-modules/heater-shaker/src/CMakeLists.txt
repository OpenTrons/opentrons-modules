# This CMakeLists.txt handles compiling all the parts of the heater-shaker
# module that are portable between host and cross compilation as a static
# library. It is included in both host and cross configurations.

find_package(Python)
find_package(Git QUIET)

# This is like version for project but works when there aren't any tags yet
execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --match "heater-shaker*"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  OUTPUT_VARIABLE TAGNAME
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if (NOT TAGNAME)
  set(TAGNAME "@(dev)-")
endif()

string(REGEX MATCH "@.*$" PREPENDED_VERSION ${TAGNAME})
string(SUBSTRING "${PREPENDED_VERSION}" 1 -1 VERSION)
if (NOT VERSION)
  set(VERSION "(dev)")
endif()
execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  OUTPUT_VARIABLE HASH
  OUTPUT_STRIP_TRAILING_WHITESPACE)

set(heater-shaker_VERSION "${VERSION}-${HASH}")

configure_file(./version.cpp.in ./version.cpp)

add_custom_command(
  COMMAND Python::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/generate_thermistor_table.py
    ${CMAKE_CURRENT_SOURCE_DIR}/ntcg104ed104dtdsx.csv
    ${CMAKE_CURRENT_BINARY_DIR}/thermistor_lookups.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/thermistor_lookups.cpp
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/generate_thermistor_table.py
          ${CMAKE_CURRENT_SOURCE_DIR}/ntcg104ed104dtdsx.csv
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/thermistor_lookups.hpp
         ${CMAKE_CURRENT_BINARY_DIR}/thermistor_lookups.cpp
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )

set(CORE_LINTABLE_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/pid.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/errors.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/thermistor_conversion.cpp
  )

set(CORE_NONLINTABLE_SOURCES
  ${CMAKE_CURRENT_BINARY_DIR}/thermistor_lookups.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/version.cpp)

add_library(heater-shaker-core STATIC
  ${CORE_LINTABLE_SOURCES}
  ${CORE_NONLINTABLE_SOURCES}
)

set_target_properties(heater-shaker-core
  PROPERTIES CXX_STANDARD 20
             CXX_STANDARD_REQUIRED TRUE)

target_include_directories(heater-shaker-core
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}  # need to add this to get the generated conversion header
  )

target_compile_options(heater-shaker-core
  PRIVATE
  -Wall
  -Werror
  -Weffc++
  -Wreorder
  -Wsign-promo
  -Wextra-semi
  -Wctor-dtor-privacy
  -fno-rtti
  )
