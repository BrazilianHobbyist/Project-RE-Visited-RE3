cmake_minimum_required(VERSION 3.20)
project(RE3Recomp VERSION 0.1.0 LANGUAGES C CXX)

# ============================================================
# RE3Recomp — CMakeLists.txt
# Port nativo de Resident Evil 3: Nemesis (PS1) para Windows
# Usa o framework PSXRecomp como submodule
# ============================================================

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

# ------------------------------------------------------------
# PSXRecomp Framework (submodule)
# ------------------------------------------------------------
# O framework deve estar em: psxrecomp/
# Para clonar: git submodule add https://github.com/RetroPortingToolKit/psxrecomp psxrecomp
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/psxrecomp/CMakeLists.txt")
    message(FATAL_ERROR 
        "PSXRecomp submodule não encontrado!\n"
        "Execute: git submodule update --init --recursive")
endif()

add_subdirectory(psxrecomp)

# ------------------------------------------------------------
# Geração de código (recompilador)
# O código C gerado pelo recompilador fica em build/generated/
# O CMake do PSXRecomp cuida de compilar esses arquivos automaticamente
# ------------------------------------------------------------

# ------------------------------------------------------------
# Mods customizados do RE3Recomp
# ------------------------------------------------------------
add_library(re3_mods STATIC
    mods/widescreen/widescreen_patch.c
    mods/saves/saves.c
)

target_include_directories(re3_mods PRIVATE
    psxrecomp/runtime/include
)

# ------------------------------------------------------------
# Configurações de plataforma
# ------------------------------------------------------------
if(WIN32)
    # Linkar com Shlobj.lib para SHGetFolderPath (módulo de saves)
    target_link_libraries(re3_mods PRIVATE Shell32)
    
    # XInput para suporte ao controle Xbox
    target_link_libraries(re3_mods PRIVATE Xinput)
    
    # Evitar o warning C4996 do MSVC para funções POSIX
    target_compile_definitions(re3_mods PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# ------------------------------------------------------------
# Executável principal
# O PSXRecomp define o target "psxrecomp_runtime" que integra tudo
# ------------------------------------------------------------
if(TARGET psxrecomp_runtime)
    target_link_libraries(psxrecomp_runtime PRIVATE re3_mods)
    
    # Definir nome do executável final
    set_target_properties(psxrecomp_runtime PROPERTIES
        OUTPUT_NAME "RE3Recomp"
    )
endif()

# ------------------------------------------------------------
# Mensagens de status no configure
# ------------------------------------------------------------
message(STATUS "RE3Recomp v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Mods: widescreen, saves, xinput")
