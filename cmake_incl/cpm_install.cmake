# Ensure android didn't fail.

if(NOT DEFINED ENV{CPM_SOURCE_CACHE})
  set(CPM_SOURCE_CACHE "$ENV{HOME}/.cache/CPM")
endif()

# Download CPM.cmake (package manager for cmake)
file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/get_cpm.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)
include(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

cpmaddpackage(URI "gh:cpm-cmake/CPMLicenses.cmake@0.0.7" OPTIONS
              PRINT_DELIMITER ON)

function(setup_legal_check MAIN_TARGET)
  set(LEGAL_FILE "${CMAKE_BINARY_DIR}/LEGAL.txt")
  set(CHECK_SCRIPT "${CMAKE_BINARY_DIR}/internal_legal_checker.cmake")
  # Keywords (lowercase) if found in LEGAL.txt will terminate compilation.
  set(FORBIDDEN_LICENCE_LOWER_CASE "gpl|agpl|warning|error")

  if("${LEGAL_FILE}" MATCHES "[ ]" OR "${CHECK_SCRIPT}" MATCHES "[ ]")
    message(
      FATAL_ERROR
        "Paths for LEGAL_FILE or CHECK_SCRIPT contain spaces. "
        "Spaces are not supported in toolchain paths to ensure stability. "
        "Please move the build directory to a path without spaces.")
  endif()

  if(NOT CPM_PACKAGES)
    message(
      FATAL_ERROR
        "[Legal] No CPM packages found, do not call this function if you don't use CPM. Check the order of cpm use/this function call."
    )
    return()
  endif()

  # 1. Создаем таргет для генерации отчета (из CPMLicenses)
  cpm_licenses_create_disclaimer_target(generate_legal_report "${LEGAL_FILE}"
                                        "${CPM_PACKAGES}")

  # 1. Генерируем внутренний CMake-скрипт для проверки (чтобы не зависеть от
  #   Python/Perl и Ninja)
  file(
    WRITE "${CHECK_SCRIPT}"
    "
         message(STATUS \"[Legal] Starting check of \${LEGAL_FILE_PATH}.\")

         if(NOT EXISTS \"\${LEGAL_FILE_PATH}\")
             message(FATAL_ERROR \"\${LEGAL_FILE_PATH}:1: error: Legal report file not found!\")
         endif()

         file(READ \"\${LEGAL_FILE_PATH}\" CONTENT)

         # CMake не поддерживает (?i).
         # Переводим всё в нижний регистр для надежного сравнения.
         string(TOLOWER \"\${CONTENT}\" LOWERED_CONTENT)

         if(LOWERED_CONTENT MATCHES \"${FORBIDDEN_LICENCE_LOWER_CASE}\")
             message(FATAL_ERROR \"\${LEGAL_FILE_PATH}:1: error: Forbidden license detected (${FORBIDDEN_LICENCE_LOWER_CASE})!\")
         else()
             message(STATUS \"[Legal] Check passed for \${LEGAL_FILE_PATH}.\")
         endif()
     ")

  # 1. Добавляем проверку как POST_BUILD к таргету отчета
  add_custom_command(
    TARGET generate_legal_report
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -DLEGAL_FILE_PATH=${LEGAL_FILE} -P
            "${CHECK_SCRIPT}"
    COMMENT "Running automated legal policy enforcement for ${LEGAL_FILE}..."
    VERBATIM)

  # 1. Привязываем отчет к основному проекту, чтобы он собирался автоматически
  add_dependencies(${MAIN_TARGET} generate_legal_report)

  message(STATUS "[Legal] Automation enabled for target: ${MAIN_TARGET}")
endfunction()
