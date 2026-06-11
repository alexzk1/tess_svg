cmake_minimum_required(VERSION 3.24)
project(tess_svg LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Enable unit tests" OFF)

include(cmake_incl/cpm_install.cmake)
cpmaddpackage(URI "gh:zeux/pugixml@1.15")
cpmaddpackage(URI "gh:nlohmann/json@3.12.0" OPTIONS "JSON_BuildTests OFF")

set(CLIPPER2_EXAMPLES
    OFF
    CACHE BOOL "" FORCE)
set(CLIPPER2_TESTS
    OFF
    CACHE BOOL "" FORCE)
set(CLIPPER2_HI_PRECISION
    ON
    CACHE BOOL "" FORCE)

cpmaddpackage(URI "gh:AngusJohnson/Clipper2#Clipper2_2.0.1")
if(EXISTS "${Clipper2_SOURCE_DIR}/CPP/CMakeLists.txt")
  add_subdirectory(${Clipper2_SOURCE_DIR}/CPP
                   ${Clipper2_BINARY_DIR}/Clipper2_Build})
else()
  message(
    FATAL_ERROR
      "Clipper2 '${Clipper2_SOURCE_DIR}/CPP' directory not found! Check your CPM download."
  )
endif()

find_package(Threads REQUIRED)
find_package(Boost 1.87 REQUIRED COMPONENTS program_options filesystem)
if(UNIX AND NOT APPLE)
  find_package(OpenGL REQUIRED)
  find_library(GLU_LIBRARY NAMES GLU GLU32)
endif()

file(GLOB_RECURSE SOURCES_NO_MAIN CONFIGURE_DEPENDS tess_svg/*.h tess_svg/*.hpp
     tess_svg/*.cpp)

set(SOURCES main.cpp ${SOURCES_NO_MAIN})
add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(
  ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${Boost_INCLUDE_DIRS}
                          ${X11_INCLUDE_DIR})

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

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Building the RELEASE Version")
  target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG)
  target_compile_options(${PROJECT_NAME} PRIVATE -O3 -march=native)

  add_compile_options(-flto=4)
  add_link_options(-flto=4)
  message(STATUS "LTO forced for Release build")
else()
  message(STATUS "Building the DEBUG Version")
  target_compile_definitions(${PROJECT_NAME} PRIVATE _DEBUG)
  target_compile_options(${PROJECT_NAME} PRIVATE -march=native -O0 -g)

  if(UNIX AND NOT APPLE)
    target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=undefined
                                                   -fsanitize=vptr)
    target_link_libraries(${PROJECT_NAME} PRIVATE ubsan)
  endif()
endif()

if(GLU_LIBRARY)
  message(STATUS "Found GLU: ${GLU_LIBRARY}")
else()
  message(FATAL_ERROR "GLU library not found! It's needed for tessellation.")
endif()

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE Threads::Threads
          ${GLU_LIBRARY}
          OpenGL::GL
          Clipper2
          Boost::program_options
          Boost::filesystem
          pugixml::pugixml
          nlohmann_json::nlohmann_json)

if(UNIX AND NOT APPLE)
  target_link_libraries(${PROJECT_NAME} PRIVATE ${X11_LIBRARIES} GLU)
endif()

enable_testing()

# Integration tests.
set(TEST_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/tests/run_test_compact.sh")

# IntegrationTest_HoleSquare
set(INPUT_SVG "${CMAKE_CURRENT_SOURCE_DIR}/test_data/test.svg")
set(EXPECTED_JSON "${CMAKE_CURRENT_SOURCE_DIR}/test_data/expected.json")
add_test(NAME IntegrationTest_HoleSquare
         COMMAND bash ${TEST_SCRIPT} $<TARGET_FILE:${PROJECT_NAME}>
                 ${INPUT_SVG} ${EXPECTED_JSON})

# IntegrationTest_Zeppelin
set(INPUT_SVG "${CMAKE_CURRENT_SOURCE_DIR}/test_data/zeppelin_path.svg")
set(EXPECTED_JSON
    "${CMAKE_CURRENT_SOURCE_DIR}/test_data/zeppelin_expected.json")
add_test(NAME IntegrationTest_Zeppelin
         COMMAND bash ${TEST_SCRIPT} $<TARGET_FILE:${PROJECT_NAME}>
                 ${INPUT_SVG} ${EXPECTED_JSON})

# IntegrationTest_F
set(INPUT_SVG "${CMAKE_CURRENT_SOURCE_DIR}/test_data/F.svg")
set(EXPECTED_JSON "${CMAKE_CURRENT_SOURCE_DIR}/test_data/F.json")
add_test(NAME IntegrationTest_F
         COMMAND bash ${TEST_SCRIPT} $<TARGET_FILE:${PROJECT_NAME}>
                 ${INPUT_SVG} ${EXPECTED_JSON})

# IntegrationTest_Bezier
set(INPUT_SVG "${CMAKE_CURRENT_SOURCE_DIR}/test_data/bezier.svg")
set(EXPECTED_JSON "${CMAKE_CURRENT_SOURCE_DIR}/test_data/bezier.json")
add_test(NAME IntegrationTest_Bezier
         COMMAND bash ${TEST_SCRIPT} $<TARGET_FILE:${PROJECT_NAME}>
                 ${INPUT_SVG} ${EXPECTED_JSON})

# GTests.

if(BUILD_TESTS)
  file(GLOB_RECURSE TESTS_LIST ${CMAKE_CURRENT_LIST_DIR}/tests/*.h
       ${CMAKE_CURRENT_LIST_DIR}/tests/*.hpp
       ${CMAKE_CURRENT_LIST_DIR}/tests/*.cpp)
  list(LENGTH TESTS_LIST TESTS_LIST_FILES_COUNT)

  if(TESTS_LIST_FILES_COUNT GREATER 0)
    cpmaddpackage(
      NAME
      GTest
      GITHUB_REPOSITORY
      google/googletest
      VERSION
      1.15.2
      OPTIONS
      "INSTALL_GTEST OFF")

    source_group("tests" FILES ${TESTS_LIST})
    add_executable(tess_svg_tests ${SOURCES_NO_MAIN} ${TESTS_LIST})
    target_compile_options(
      tess_svg_tests
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

    target_link_libraries(
      tess_svg_tests
      PRIVATE GTest::gtest_main
              pugixml::pugixml
              Threads::Threads
              Clipper2
              ${GLU_LIBRARY}
              OpenGL::GL
              nlohmann_json::nlohmann_json)

    target_include_directories(
      tess_svg_tests PUBLIC ${CMAKE_CURRENT_LIST_DIR}/tests/
                            ${CMAKE_CURRENT_SOURCE_DIR} ${Boost_INCLUDE_DIRS})
    include(GoogleTest)
    gtest_discover_tests(tess_svg_tests)
  endif()
endif()

setup_legal_check(${PROJECT_NAME})
