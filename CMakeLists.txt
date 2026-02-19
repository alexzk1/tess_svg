cmake_minimum_required(VERSION 3.24)
project(tess_svg LANGUAGES CXX)

# Установка стандарта C++17 (как в вашем CONFIG += c++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Download CPM.cmake (package manager for cmake)
file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/get_cpm.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)
cpmaddpackage(URI "gh:zeux/pugixml@1.15")
cpmaddpackage(URI "gh:nlohmann/json@3.12.0" OPTIONS "JSON_BuildTests OFF")

find_package(Threads REQUIRED)
find_package(Boost 1.87 REQUIRED COMPONENTS program_options filesystem)
if(UNIX AND NOT APPLE)
  find_package(OpenGL REQUIRED)
  find_library(GLU_LIBRARY NAMES GLU GLU32)
endif()

find_package(SFML)

# 1. Список исходных файлов
set(SOURCES
    main.cpp
    tess_svg/Bounds.cpp
    tess_svg/idumper.cpp
    tess_svg/SvgPath.cpp
    tess_svg/SvgProcessor.cpp
    tess_svg/tagdparser.cpp
    tess_svg/Tesselate.cpp
    engine/Vector2.cpp)

# 1. Создание исполняемого файла
add_executable(${PROJECT_NAME} ${SOURCES})
if(NOT SFML_FOUND)
  target_compile_definitions(${PROJECT_NAME} PRIVATE NO_SFML)
endif()

# 1. Инклюды
target_include_directories(
  ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${Boost_INCLUDE_DIRS}
                          ${X11_INCLUDE_DIR})

# 1. Флаги компиляции (Общие)
target_compile_options(
  ${PROJECT_NAME}
  PRIVATE -Wall
          -ffloat-store
          -frtti
          -fexceptions
          -Werror=return-type
          -Wctor-dtor-privacy
          -Werror=delete-non-virtual-dtor
          -fstrict-aliasing
          -Werror=strict-aliasing
          -Wstrict-aliasing=2)

# 1. Специфичные настройки для Debug и Release
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Building the DEBUG Version")
  target_compile_definitions(${PROJECT_NAME} PRIVATE _DEBUG)
  target_compile_options(${PROJECT_NAME} PRIVATE -march=native -O0 -g)

  if(UNIX AND NOT APPLE)
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined
                                                   -fsanitize=vptr)
    target_link_libraries(${PROJECT_NAME} PRIVATE ubsan)
  endif()
else()
  message(STATUS "Building the RELEASE Version")
  target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG)
  target_compile_options(${PROJECT_NAME} PRIVATE -O3 -march=native)
endif()

if(GLU_LIBRARY)
  message(STATUS "Found GLU: ${GLU_LIBRARY}")
else()
  message(FATAL_ERROR "GLU library not found! It's needed for tessellation.")
endif()

# 1. Линковка библиотек
target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE Threads::Threads
          ${GLU_LIBRARY}
          OpenGL::GL
          Boost::program_options
          Boost::filesystem
          pugixml::pugixml
          nlohmann_json::nlohmann_json)

if(UNIX AND NOT APPLE)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES} GLU)
endif()

enable_testing()

set(TEST_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_test.sh")
set(INPUT_SVG "${CMAKE_CURRENT_SOURCE_DIR}/test_data/test.svg")
set(EXPECTED_JSON "${CMAKE_CURRENT_SOURCE_DIR}/test_data/expected.json")

add_test(NAME IntegrationTest_HoleSquare
         COMMAND bash ${TEST_SCRIPT} $<TARGET_FILE:${PROJECT_NAME}>
                 ${INPUT_SVG} ${EXPECTED_JSON})
