cmake_minimum_required(VERSION 3.20)
set(PSXRECOMP_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/psxrecomp" CACHE PATH
    "Path to psxrecomp")
if(NOT EXISTS "${PSXRECOMP_ROOT}/runtime/runtime.cmake")
    message(FATAL_ERROR
        "PSXRecomp runtime is missing: ${PSXRECOMP_ROOT}/runtime/runtime.cmake\n"
        "This project expects the psxrecomp framework checkout at ${PSXRECOMP_ROOT}.\n"
        "Run: git submodule update --init --recursive\n"
        "If psxrecomp/ is not a submodule, set -DPSXRECOMP_ROOT=<path-to-psxrecomp>.")
endif()

project(RE3Recomp LANGUAGES C CXX)

set(PSX_RECOMP_UI OFF CACHE BOOL
    "No recomp-ui submodule in this build" FORCE)
set(PSX_REWIND OFF CACHE BOOL
    "Disable rewind engine" FORCE)

include("${PSXRECOMP_ROOT}/runtime/runtime.cmake")

set(PSX_PRELOADED_MODS "${CMAKE_CURRENT_SOURCE_DIR}/mods/preloaded")
if(NOT IS_DIRECTORY "${PSX_PRELOADED_MODS}")
    set(PSX_PRELOADED_MODS "NONE")
endif()

psxrecomp_add_game_runtime(RE3Recomp
    PRELOADED_MODS_DIR "${PSX_PRELOADED_MODS}"
    WINDOW_TITLE "Resident Evil 3: Nemesis Recompiled"
    DEFAULT_GAME_CONFIG_PATH "game.toml"
    MAX_PLAYERS 1
    GEN_MARKER "generated/SLUS_009.23_dispatch.c"
    GEN_FULL_GLOB "generated/SLUS_009.23_full_*.c"
    PGXP
)

# Reproducible build: no link-time timestamp in the PE header (and so none in
# the CodeView build id derived from it). __DATE__/__TIME__ come from
# SOURCE_DATE_EPOCH, which build.ps1 sets to the commit time. Two clean builds
# of one commit with the same toolchain are then byte-identical.
if(MINGW OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_link_options(RE3Recomp PRIVATE "-Wl,--no-insert-timestamp")
endif()

# Stage runtime configs next to the exe for direct build/ launches.
foreach(_PSX_RUNTIME_CONFIG game.toml)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_PSX_RUNTIME_CONFIG}")
        add_custom_command(TARGET RE3Recomp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/${_PSX_RUNTIME_CONFIG}"
                "$<TARGET_FILE_DIR:RE3Recomp>/${_PSX_RUNTIME_CONFIG}"
            COMMENT "Staging ${_PSX_RUNTIME_CONFIG}"
            VERBATIM)
    endif()
endforeach()
