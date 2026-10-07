/*
 * RE3Recomp — Save/Load Module
 *
 * O PSXRecomp virtualiza o Memory Card do PS1.
 * Este módulo garante que:
 *   1. O diretório de saves seja criado automaticamente
 *   2. O formato .mcr seja preservado (compatível com DuckStation/ePSXe)
 *   3. Saves múltiplos slots sejam suportados (RE3 usa até 3 blocos do MC)
 *
 * O PSXRecomp runtime lida com a maior parte da serialização automaticamente.
 * Este arquivo contém apenas as customizações específicas do RE3.
 */

#include "psxrecomp/runtime/memcard.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>     /* SHGetFolderPath — %APPDATA% */
#endif

#define RE3_SAVE_DIR_NAME  "RE3Recomp\\saves"
#define RE3_MC0_FILENAME   "MC0.mcr"
#define RE3_MC1_FILENAME   "MC1.mcr"

/* Memory Card size: 128KB padrão do PS1 */
#define MC_SIZE_BYTES      (128 * 1024)

/*
 * re3_saves_get_dir()
 * Retorna o caminho do diretório de saves para a plataforma atual.
 * Windows: %APPDATA%\RE3Recomp\saves
 * Linux/macOS: ~/.config/RE3Recomp/saves
 */
static void re3_saves_get_dir(char* out_path, size_t max_len) {
#ifdef _WIN32
    char appdata[MAX_PATH];
    SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata);
    snprintf(out_path, max_len, "%s\\%s", appdata, RE3_SAVE_DIR_NAME);
#else
    const char* home = getenv("HOME");
    snprintf(out_path, max_len, "%s/.config/%s", home, RE3_SAVE_DIR_NAME);
#endif
}

/*
 * re3_saves_ensure_dir()
 * Cria o diretório de saves se não existir.
 */
static void re3_saves_ensure_dir(const char* path) {
#ifdef _WIN32
    /* CreateDirectoryA falha silenciosamente se já existir */
    char parent[MAX_PATH];
    strncpy(parent, path, MAX_PATH);
    /* Criar pai primeiro: %APPDATA%\RE3Recomp */
    char* last_sep = strrchr(parent, '\\');
    if (last_sep) {
        *last_sep = '\0';
        CreateDirectoryA(parent, NULL);
    }
    CreateDirectoryA(path, NULL);
#else
    /* mkdir -p equivalente */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "mkdir -p \"%s\"", path);
    system(cmd);
#endif
}

/*
 * re3_saves_init()
 * Chamado pelo runtime PSXRecomp durante a inicialização.
 * Configura os caminhos dos Memory Cards.
 */
void re3_saves_init(void) {
    char save_dir[512];
    char mc0_path[600];
    char mc1_path[600];

    re3_saves_get_dir(save_dir, sizeof(save_dir));
    re3_saves_ensure_dir(save_dir);

    snprintf(mc0_path, sizeof(mc0_path), "%s\\%s", save_dir, RE3_MC0_FILENAME);
    snprintf(mc1_path, sizeof(mc1_path), "%s\\%s", save_dir, RE3_MC1_FILENAME);

    /* Registrar os caminhos no runtime PSXRecomp */
    psxrecomp_memcard_set_path(0, mc0_path);
    psxrecomp_memcard_set_path(1, mc1_path);

    printf("[RE3Recomp] Saves em: %s\n", save_dir);
    printf("[RE3Recomp] Memory Card 0: %s\n", mc0_path);
    printf("[RE3Recomp] Memory Card 1: %s\n", mc1_path);
}

/*
 * re3_saves_import_from_emulator()
 * Utilitário: copia um .mcr de outro emulador para o diretório do RE3Recomp.
 * Uso: re3_saves_import_from_emulator("C:\\DuckStation\\memcards\\duck_mc0.mcr", 0);
 */
int re3_saves_import_from_emulator(const char* src_path, int slot) {
    char save_dir[512];
    char dst_path[600];

    re3_saves_get_dir(save_dir, sizeof(save_dir));
    re3_saves_ensure_dir(save_dir);

    const char* filename = (slot == 0) ? RE3_MC0_FILENAME : RE3_MC1_FILENAME;
    snprintf(dst_path, sizeof(dst_path), "%s\\%s", save_dir, filename);

#ifdef _WIN32
    if (CopyFileA(src_path, dst_path, FALSE)) {
        printf("[RE3Recomp] Save importado com sucesso para slot %d\n", slot);
        return 1;
    } else {
        printf("[RE3Recomp] Erro ao importar save: %lu\n", GetLastError());
        return 0;
    }
#else
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "cp \"%s\" \"%s\"", src_path, dst_path);
    return system(cmd) == 0;
#endif
}
