#include "re3_textures.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <vector>

namespace fs = std::filesystem;

// Exact mirror of the parsing logic in psxrecomp/runtime/src/main.cpp:5692-5778
static void parse_runtime_toml_string(const std::string& toml_content, re3_texture_config_t& cfg) {
    std::istringstream f(toml_content);
    std::string line;
    while (std::getline(f, line)) {
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos || line[first] == '#') continue;
        if (line[first] == '[') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        auto trim = [](std::string& s) {
            size_t p1 = s.find_first_not_of(" \t\r\n\"");
            size_t p2 = s.find_last_not_of(" \t\r\n\"");
            if (p1 == std::string::npos) s.clear();
            else s = s.substr(p1, p2 - p1 + 1);
        };
        trim(key);
        trim(val);
        for (char& c : key) c = (char)std::tolower((unsigned char)c);

        if (key == "pack" || key == "texture_pack") {
            std::strncpy(cfg.pack, val.c_str(), sizeof(cfg.pack) - 1);
            cfg.pack[sizeof(cfg.pack) - 1] = '\0';
        }
        else if (key == "hd_backgrounds") {
            cfg.hd_backgrounds = (val == "true" || val == "1");
        }
        else if (key == "hd_models" || key == "hd_textures") {
            cfg.hd_models = (val == "true" || val == "1");
        }
        else if (key == "hd_ui" || key == "hd_menus") {
            cfg.hd_ui = (val == "true" || val == "1");
        }
        else if (key == "dump_textures") {
            cfg.dump_textures = (val == "true" || val == "1");
        }
    }
}

static void reset_config_to_defaults(re3_texture_config_t& cfg) {
    std::strncpy(cfg.pack, "SeamlessHD", sizeof(cfg.pack) - 1);
    cfg.pack[sizeof(cfg.pack) - 1] = '\0';
    cfg.hd_backgrounds = true;
    cfg.hd_models = true;
    cfg.hd_ui = true;
    cfg.dump_textures = false;
}

// ----------------------------------------------------------------------------
// Test 1: Missing [textures] section (should retain sensible defaults)
// ----------------------------------------------------------------------------
void test_missing_textures_section() {
    std::printf("[TEST] 1. Missing [textures] section\n");
    re3_texture_config_t cfg;
    reset_config_to_defaults(cfg);

    std::string toml_input = 
        "# Global settings without textures section\n"
        "[video]\n"
        "aspect_ratio = \"16:9\"\n"
        "vsync = true\n"
        "\n"
        "[audio]\n"
        "frequency = 44100\n";

    parse_runtime_toml_string(toml_input, cfg);

    if (std::strcmp(cfg.pack, "SeamlessHD") == 0 &&
        cfg.hd_backgrounds == true &&
        cfg.hd_models == true &&
        cfg.hd_ui == true &&
        cfg.dump_textures == false) {
        std::printf("  [PASS] Sensible defaults preserved when [textures] is omitted\n");
    } else {
        std::printf("  [FAIL] Defaults corrupted when [textures] is omitted!\n");
    }
}

// ----------------------------------------------------------------------------
// Test 2: Quote variations
// ----------------------------------------------------------------------------
void test_quote_variations() {
    std::printf("[TEST] 2. Quote variations\n");

    // 2a. Double quotes
    {
        re3_texture_config_t cfg;
        reset_config_to_defaults(cfg);
        parse_runtime_toml_string("[textures]\npack = \"CustomPackHD\"\n", cfg);
        if (std::strcmp(cfg.pack, "CustomPackHD") == 0) {
            std::printf("  [PASS] Double quotes: pack = \"CustomPackHD\" -> \"%s\"\n", cfg.pack);
        } else {
            std::printf("  [FAIL] Double quotes failed: \"%s\"\n", cfg.pack);
        }
    }

    // 2b. Unquoted string
    {
        re3_texture_config_t cfg;
        reset_config_to_defaults(cfg);
        parse_runtime_toml_string("[textures]\npack = CustomPackHD\n", cfg);
        if (std::strcmp(cfg.pack, "CustomPackHD") == 0) {
            std::printf("  [PASS] Unquoted: pack = CustomPackHD -> \"%s\"\n", cfg.pack);
        } else {
            std::printf("  [FAIL] Unquoted failed: \"%s\"\n", cfg.pack);
        }
    }

    // 2c. Single quotes (valid TOML literal strings)
    {
        re3_texture_config_t cfg;
        reset_config_to_defaults(cfg);
        parse_runtime_toml_string("[textures]\npack = 'CustomPackHD'\n", cfg);
        if (std::strcmp(cfg.pack, "CustomPackHD") == 0) {
            std::printf("  [PASS] Single quotes: pack = 'CustomPackHD' -> \"%s\"\n", cfg.pack);
        } else {
            std::printf("  [BUG CONFIRMED] Single quotes NOT stripped: pack = 'CustomPackHD' -> \"%s\"\n", cfg.pack);
        }
    }
}

// ----------------------------------------------------------------------------
// Test 3: Spaces around '=' and extra whitespace
// ----------------------------------------------------------------------------
void test_whitespace_and_spaces() {
    std::printf("[TEST] 3. Spaces around '=' and extra whitespace\n");
    re3_texture_config_t cfg;
    reset_config_to_defaults(cfg);

    std::string toml_input =
        "[textures]\n"
        "   pack    =    \"SpacedPack\"   \n"
        "\thd_backgrounds\t=\ttrue\t\n"
        "  dump_textures  =  1  \n";

    parse_runtime_toml_string(toml_input, cfg);

    bool ok = true;
    if (std::strcmp(cfg.pack, "SpacedPack") != 0) {
        std::printf("  [FAIL] Spaced pack name incorrect: \"%s\"\n", cfg.pack);
        ok = false;
    }
    if (!cfg.hd_backgrounds) {
        std::printf("  [FAIL] Spaced hd_backgrounds failed\n");
        ok = false;
    }
    if (!cfg.dump_textures) {
        std::printf("  [FAIL] Spaced dump_textures failed\n");
        ok = false;
    }
    if (ok) {
        std::printf("  [PASS] Handles irregular spaces and tabs around '=' and values\n");
    }
}

// ----------------------------------------------------------------------------
// Test 4: Case-insensitivity of boolean values
// ----------------------------------------------------------------------------
void test_boolean_case_insensitivity() {
    std::printf("[TEST] 4. Case-insensitivity of boolean values\n");

    const char* truthy_variants[] = {"true", "True", "TRUE", "1"};
    for (const char* v : truthy_variants) {
        re3_texture_config_t cfg;
        reset_config_to_defaults(cfg);
        cfg.dump_textures = false; // default is false

        std::string toml_input = std::string("[textures]\ndump_textures = ") + v + "\n";
        parse_runtime_toml_string(toml_input, cfg);

        if (cfg.dump_textures == true) {
            std::printf("  [PASS] dump_textures = %-5s -> parsed as TRUE\n", v);
        } else {
            std::printf("  [BUG CONFIRMED] dump_textures = %-5s -> parsed as FALSE! (val is case-sensitive)\n", v);
        }
    }

    const char* falsy_variants[] = {"false", "False", "FALSE", "0"};
    for (const char* v : falsy_variants) {
        re3_texture_config_t cfg;
        reset_config_to_defaults(cfg);
        cfg.hd_backgrounds = true; // default is true

        std::string toml_input = std::string("[textures]\nhd_backgrounds = ") + v + "\n";
        parse_runtime_toml_string(toml_input, cfg);

        if (cfg.hd_backgrounds == false) {
            std::printf("  [PASS] hd_backgrounds = %-5s -> parsed as FALSE\n", v);
        } else {
            std::printf("  [BUG CONFIRMED] hd_backgrounds = %-5s -> parsed as TRUE!\n", v);
        }
    }
}

// ----------------------------------------------------------------------------
// Test 5: Empty pack string, commented out lines, and inline comments
// ----------------------------------------------------------------------------
void test_empty_pack_and_comments() {
    std::printf("[TEST] 5. Empty pack string, commented out lines, and inline comments\n");

    // 5a. Commented out lines
    {
        re3_texture_config_t cfg;
        reset_config_to_defaults(cfg);
        std::string toml_input = 
            "[textures]\n"
            "# pack = \"ShouldNotApply\"\n"
            "# hd_backgrounds = false\n";
        parse_runtime_toml_string(toml_input, cfg);
        if (std::strcmp(cfg.pack, "SeamlessHD") == 0 && cfg.hd_backgrounds == true) {
            std::printf("  [PASS] Full line comments are correctly skipped\n");
        } else {
            std::printf("  [FAIL] Full line comments altered config!\n");
        }
    }

    // 5b. Empty pack strings
    {
        re3_texture_config_t cfg;
        reset_config_to_defaults(cfg);
        parse_runtime_toml_string("[textures]\npack = \"\"\n", cfg);
        if (std::strcmp(cfg.pack, "") == 0) {
            std::printf("  [PASS] Explicit empty string pack = \"\" gives empty pack\n");
        } else {
            std::printf("  [FAIL] Explicit empty string pack gave \"%s\"\n", cfg.pack);
        }

        reset_config_to_defaults(cfg);
        parse_runtime_toml_string("[textures]\npack = \n", cfg);
        if (std::strcmp(cfg.pack, "") == 0) {
            std::printf("  [PASS] Empty value pack = gives empty pack\n");
        } else {
            std::printf("  [FAIL] Empty value pack = gave \"%s\"\n", cfg.pack);
        }
    }

    // 5c. Inline comments
    {
        re3_texture_config_t cfg;
        reset_config_to_defaults(cfg);
        std::string toml_input =
            "[textures]\n"
            "pack = \"SeamlessHD\" # Active pack\n"
            "hd_backgrounds = true # Enable high-res backgrounds\n";
        parse_runtime_toml_string(toml_input, cfg);

        if (std::strcmp(cfg.pack, "SeamlessHD") == 0) {
            std::printf("  [PASS] Inline comment on string: pack = \"SeamlessHD\"\n");
        } else {
            std::printf("  [BUG CONFIRMED] Inline comment corrupted string: \"%s\"\n", cfg.pack);
        }

        if (cfg.hd_backgrounds == true) {
            std::printf("  [PASS] Inline comment on boolean: hd_backgrounds = true\n");
        } else {
            std::printf("  [BUG CONFIRMED] Inline comment broke boolean: hd_backgrounds evaluated to FALSE!\n");
        }
    }
}

// ----------------------------------------------------------------------------
// Test 6: Random/unrecognized keys inside [textures]
// ----------------------------------------------------------------------------
void test_unrecognized_keys() {
    std::printf("[TEST] 6. Random/unrecognized keys inside [textures]\n");
    re3_texture_config_t cfg;
    reset_config_to_defaults(cfg);

    std::string toml_input =
        "[textures]\n"
        "random_unrecognized_key = 12345\n"
        "another_bogus_setting = \"hello world\"\n"
        "corrupted_line_without_equal_sign\n"
        "pack = \"SeamlessHD\"\n";

    parse_runtime_toml_string(toml_input, cfg);

    if (std::strcmp(cfg.pack, "SeamlessHD") == 0 &&
        cfg.hd_backgrounds == true &&
        cfg.hd_models == true &&
        cfg.hd_ui == true &&
        cfg.dump_textures == false) {
        std::printf("  [PASS] Unknown keys and malformed lines ignored gracefully without crash or corruption\n");
    } else {
        std::printf("  [FAIL] Unrecognized keys corrupted config!\n");
    }
}

// ----------------------------------------------------------------------------
// Test 7: End-to-end integration with re3_textures_init and path resolution
// ----------------------------------------------------------------------------
void test_re3_textures_lifecycle_integration() {
    std::printf("[TEST] 7. Lifecycle integration with re3_textures_init\n");

    fs::path temp_dir = fs::temp_directory_path() / "re3_adversarial_test_env";
    fs::remove_all(temp_dir);
    fs::create_directories(temp_dir / "mods" / "textures" / "SeamlessHD" / "backgrounds");

    // Case 1: Standard pack
    {
        g_re3_tex_config.hd_backgrounds = true;
        std::strncpy(g_re3_tex_config.pack, "SeamlessHD", sizeof(g_re3_tex_config.pack) - 1);
        re3_textures_init(temp_dir.generic_string().c_str());
        assert(re3_textures_is_pack_valid() == true);
        std::printf("  [PASS] Standard pack SeamlessHD is valid\n");
        re3_textures_shutdown();
    }

    // Case 2: What happens if single quotes leaked into pack name?
    {
        std::strncpy(g_re3_tex_config.pack, "'SeamlessHD'", sizeof(g_re3_tex_config.pack) - 1);
        re3_textures_init(temp_dir.generic_string().c_str());
        if (re3_textures_is_pack_valid() == false) {
            std::printf("  [IMPACT CONFIRMED] Leaked single quotes invalidate pack (folder 'SeamlessHD' not found)!\n");
        } else {
            std::printf("  [UNEXPECTED] Pack with single quotes was found\n");
        }
        re3_textures_shutdown();
    }

    fs::remove_all(temp_dir);
}

int main() {
    std::printf("=================================================================\n");
    std::printf("RE3Recomp M1 Adversarial Stress Test Harness (Settings & Config)\n");
    std::printf("=================================================================\n");

    test_missing_textures_section();
    test_quote_variations();
    test_whitespace_and_spaces();
    test_boolean_case_insensitivity();
    test_empty_pack_and_comments();
    test_unrecognized_keys();
    test_re3_textures_lifecycle_integration();

    std::printf("=================================================================\n");
    std::printf("Adversarial test harness execution completed.\n");
    std::printf("=================================================================\n");
    return 0;
}
