#include "re3_textures.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

void test_default_config() {
    const re3_texture_config_t* cfg = re3_textures_get_config();
    assert(cfg != nullptr);
    assert(std::strcmp(cfg->pack, "SeamlessHD") == 0);
    assert(cfg->hd_backgrounds == true);
    assert(cfg->hd_models == true);
    assert(cfg->hd_ui == true);
    assert(cfg->dump_textures == false);
    std::printf("[PASS] test_default_config\n");
}

void test_init_with_null_and_empty() {
    re3_textures_init(nullptr);
    assert(std::strcmp(re3_textures_get_base_path(), ".") == 0);

    re3_textures_init("");
    assert(std::strcmp(re3_textures_get_base_path(), ".") == 0);
    std::printf("[PASS] test_init_with_null_and_empty\n");
}

void test_pack_resolution() {
    fs::path temp_dir = fs::temp_directory_path() / "re3_test_textures_env";
    fs::remove_all(temp_dir);
    fs::create_directories(temp_dir / "mods" / "textures" / "TestPack" / "backgrounds");
    fs::create_directories(temp_dir / "mods" / "textures" / "TestPack" / "replacements");
    fs::create_directories(temp_dir / "mods" / "textures" / "TestPack" / "ui");

    // Create dummy files
    std::ofstream(temp_dir / "mods" / "textures" / "TestPack" / "backgrounds" / "R100_00.png") << "bg_dummy";
    std::ofstream(temp_dir / "mods" / "textures" / "TestPack" / "replacements" / "1234567890abcdef.png") << "tex_dummy";
    std::ofstream(temp_dir / "mods" / "textures" / "TestPack" / "ui" / "inventory.PNG") << "ui_dummy";

    std::strncpy(g_re3_tex_config.pack, "TestPack", sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.dump_textures = true;

    std::string temp_dir_str = temp_dir.generic_string();
    re3_textures_init(temp_dir_str.c_str());

    assert(re3_textures_is_pack_valid() == true);

    char resolved[1024] = {0};
    bool ok = re3_textures_resolve_path(resolved, sizeof(resolved), "backgrounds", "R100_00.png");
    assert(ok == true);
    assert(fs::exists(resolved));

    // Test case-insensitive extension fallback (.png -> .PNG)
    ok = re3_textures_resolve_path(resolved, sizeof(resolved), "ui", "inventory.png");
    assert(ok == true);
    assert(fs::exists(resolved));

    // Non-existent file
    ok = re3_textures_resolve_path(resolved, sizeof(resolved), "backgrounds", "nonexistent.png");
    assert(ok == false);
    assert(resolved[0] == '\0');

    // Test dumping folder creation
    assert(fs::exists(temp_dir / "dump" / "textures"));

    // Test disabled packs
    const char* disabled_names[] = {"None", "none", "Vanilla", "vanilla", "off", "disabled", "0", ""};
    for (const char* name : disabled_names) {
        std::strncpy(g_re3_tex_config.pack, name, sizeof(g_re3_tex_config.pack) - 1);
        re3_textures_init(temp_dir_str.c_str());
        assert(re3_textures_is_pack_valid() == false);
    }

    re3_textures_shutdown();
    assert(re3_textures_is_pack_valid() == false);
    assert(std::strlen(re3_textures_get_pack_path()) == 0);

    // Clean up
    fs::remove_all(temp_dir);
    std::printf("[PASS] test_pack_resolution\n");
}

int main() {
    std::printf("Running native re3_textures tests...\n");
    test_default_config();
    test_init_with_null_and_empty();
    test_pack_resolution();
    std::printf("All native re3_textures tests passed successfully!\n");
    return 0;
}
