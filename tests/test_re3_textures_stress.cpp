#include "re3_textures.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        std::printf("  [FAIL] %s (Line %d: %s)\n", msg, __LINE__, #cond); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define RUN_TEST(fn) do { \
    g_tests_run++; \
    std::printf("[TEST %d] %s...\n", g_tests_run, #fn); \
    fn(); \
    if (g_tests_failed == last_failed) { \
        g_tests_passed++; \
        std::printf("  [PASS] %s\n", #fn); \
    } \
    last_failed = g_tests_failed; \
} while(0)

static fs::path create_sandbox() {
    fs::path p = fs::temp_directory_path() / ("re3_m1_stress_" + std::to_string(std::rand()) + "_" + std::to_string(std::rand()));
    std::error_code ec;
    fs::remove_all(p, ec);
    fs::create_directories(p, ec);
    return p;
}

static void cleanup_sandbox(const fs::path& p) {
    std::error_code ec;
    fs::remove_all(p, ec);
}

// -------------------------------------------------------------
// 1. Stress Test: Base Path Variations & Boundary Limits
// -------------------------------------------------------------
void test_stress_base_path() {
    // 1.1 NULL base_path
    re3_textures_init(nullptr);
    TEST_ASSERT(std::strcmp(re3_textures_get_base_path(), ".") == 0, "NULL base_path must default to '.'");

    // 1.2 Empty string base_path
    re3_textures_init("");
    TEST_ASSERT(std::strcmp(re3_textures_get_base_path(), ".") == 0, "Empty base_path must default to '.'");

    // 1.3 Trailing slashes normalization
    re3_textures_init("base/dir///");
    TEST_ASSERT(std::strcmp(re3_textures_get_base_path(), "base/dir") == 0, "Trailing slashes must be stripped");

    // 1.4 Trailing backslashes normalization
    re3_textures_init("base\\dir\\\\\\");
    TEST_ASSERT(std::strcmp(re3_textures_get_base_path(), "base/dir") == 0, "Trailing backslashes must be normalized and stripped");

    // 1.5 Root slash preserved
    re3_textures_init("/");
    TEST_ASSERT(std::strcmp(re3_textures_get_base_path(), "/") == 0, "Root slash '/' should be preserved");

    // 1.6 Drive root normalization observation (Windows "C:\" -> "C:")
    re3_textures_init("C:\\");
    std::printf("    Observation: 'C:\\' normalized to '%s'\n", re3_textures_get_base_path());

    // 1.7 Very long base path (boundary test for s_base_path[1024])
    std::string long_path(2048, 'x');
    re3_textures_init(long_path.c_str());
    const char* bp = re3_textures_get_base_path();
    TEST_ASSERT(bp != nullptr, "Base path pointer must not be null");
    TEST_ASSERT(std::strlen(bp) == 1023, "Base path must be safely truncated to 1023 characters");
    TEST_ASSERT(bp[1023] == '\0', "Base path must be null-terminated");

    re3_textures_shutdown();
}

// -------------------------------------------------------------
// 2. Stress Test: Non-existent, Empty, and Disabled Pack Names
// -------------------------------------------------------------
void test_stress_nonexistent_packs() {
    fs::path sb = create_sandbox();
    std::string sb_str = sb.generic_string();

    // 2.1 Non-existent pack directory
    std::strncpy(g_re3_tex_config.pack, "NonExistentPack_12345", sizeof(g_re3_tex_config.pack) - 1);
    re3_textures_init(sb_str.c_str());
    TEST_ASSERT(re3_textures_is_pack_valid() == false, "Non-existent pack directory must yield is_pack_valid == false");

    char out[256] = {0};
    bool res = re3_textures_resolve_path(out, sizeof(out), "backgrounds", "bg.png");
    TEST_ASSERT(res == false, "resolve_path must return false for non-existent pack");
    TEST_ASSERT(out[0] == '\0', "out_path must be empty string on failure");

    // 2.2 Pack path is a file, not a directory
    fs::path file_pack = sb / "mods" / "textures" / "FileNotDir";
    fs::create_directories(file_pack.parent_path());
    std::ofstream(file_pack) << "dummy file";

    std::strncpy(g_re3_tex_config.pack, "FileNotDir", sizeof(g_re3_tex_config.pack) - 1);
    re3_textures_init(sb_str.c_str());
    TEST_ASSERT(re3_textures_is_pack_valid() == false, "Pack that is a file (not dir) must be invalid");

    // 2.3 Disabled keywords & whitespace variations
    const char* disabled_names[] = {
        "", "   ", "\t\r\n", "None", "none", "Vanilla", "vanilla", "off", "disabled", "0",
        "  \"None\"  ", "\tvanilla\r\n", "\"0\""
    };
    for (const char* name : disabled_names) {
        std::strncpy(g_re3_tex_config.pack, name, sizeof(g_re3_tex_config.pack) - 1);
        re3_textures_init(sb_str.c_str());
        TEST_ASSERT(re3_textures_is_pack_valid() == false, "Disabled keyword pack name must be invalid");
        TEST_ASSERT(std::strlen(re3_textures_get_pack_path()) == 0, "Disabled pack must have empty pack path");
    }

    re3_textures_shutdown();
    cleanup_sandbox(sb);
}

// -------------------------------------------------------------
// 3. Stress Test: Path Traversal Probing
// -------------------------------------------------------------
void test_stress_path_traversal() {
    fs::path sb = create_sandbox();
    std::string sb_str = sb.generic_string();

    // Create a legitimate pack
    fs::path pack_dir = sb / "mods" / "textures" / "ValidPack";
    fs::create_directories(pack_dir / "backgrounds");
    std::ofstream(pack_dir / "backgrounds" / "R100_00.png") << "bg";

    // Create external targets
    fs::path secret_file = sb / "secret.txt";
    std::ofstream(secret_file) << "TOP_SECRET";

    fs::path outside_folder = sb / "outside_dir";
    fs::create_directories(outside_folder);
    std::ofstream(outside_folder / "leak.png") << "LEAK";

    std::strncpy(g_re3_tex_config.pack, "ValidPack", sizeof(g_re3_tex_config.pack) - 1);
    re3_textures_init(sb_str.c_str());
    TEST_ASSERT(re3_textures_is_pack_valid() == true, "ValidPack must be valid");

    char out[512] = {0};

    // Case 3.1: Relative path traversal through filename (depth 4 reaches sb/secret.txt)
    bool res1 = re3_textures_resolve_path(out, sizeof(out), "backgrounds", "../../../../secret.txt");
    std::printf("    Traversal 3.1 (../../../../secret.txt): resolved=%d, path='%s'\n", res1, out);

    // Case 3.2: Relative path traversal through subfolder
    out[0] = '\0';
    bool res2 = re3_textures_resolve_path(out, sizeof(out), "../../../../outside_dir", "leak.png");
    std::printf("    Traversal 3.2 (subfolder ../../../../outside_dir): resolved=%d, path='%s'\n", res2, out);

    // Case 3.3: Absolute path in filename
    out[0] = '\0';
    std::string secret_abs = secret_file.generic_string();
    bool res3 = re3_textures_resolve_path(out, sizeof(out), "backgrounds", secret_abs.c_str());
    std::printf("    Traversal 3.3 (absolute filename %s): resolved=%d, path='%s'\n", secret_abs.c_str(), res3, out);

    // Case 3.4: Pack name directory traversal in re3_textures_init
    std::strncpy(g_re3_tex_config.pack, "../../outside_dir", sizeof(g_re3_tex_config.pack) - 1);
    re3_textures_init(sb_str.c_str());
    std::printf("    Traversal 3.4 (pack = '../../outside_dir'): is_valid=%d, pack_path='%s'\n",
                re3_textures_is_pack_valid(), re3_textures_get_pack_path());

    re3_textures_shutdown();
    cleanup_sandbox(sb);
}

// -------------------------------------------------------------
// 4. Stress Test: Case-Insensitive Extension Resolution
// -------------------------------------------------------------
void test_stress_case_insensitive_extensions() {
    fs::path sb = create_sandbox();
    std::string sb_str = sb.generic_string();

    fs::path pack_dir = sb / "mods" / "textures" / "CasePack";
    fs::create_directories(pack_dir / "backgrounds");
    fs::create_directories(pack_dir / "replacements");
    fs::create_directories(pack_dir / "ui");

    // Case 1: Lowercase .png on disk
    std::ofstream(pack_dir / "backgrounds" / "room_01.png") << "png_data";
    // Case 2: Uppercase .PNG on disk
    std::ofstream(pack_dir / "replacements" / "ACTOR_01.PNG") << "PNG_DATA";
    // Case 3: Other extension .dds on disk
    std::ofstream(pack_dir / "ui" / "font.dds") << "dds_data";

    std::strncpy(g_re3_tex_config.pack, "CasePack", sizeof(g_re3_tex_config.pack) - 1);
    re3_textures_init(sb_str.c_str());
    TEST_ASSERT(re3_textures_is_pack_valid() == true, "CasePack should be valid");

    char out[512] = {0};

    // 4.1 Match existing .png with .png query
    bool res = re3_textures_resolve_path(out, sizeof(out), "backgrounds", "room_01.png");
    TEST_ASSERT(res == true, "Exact .png query must resolve");

    // 4.2 Fallback: Match existing .png with uppercase .PNG query
    out[0] = '\0';
    res = re3_textures_resolve_path(out, sizeof(out), "backgrounds", "room_01.PNG");
    TEST_ASSERT(res == true, "Uppercase .PNG query must fallback and resolve existing .png");

    // 4.3 Match existing .PNG with uppercase .PNG query
    out[0] = '\0';
    res = re3_textures_resolve_path(out, sizeof(out), "replacements", "ACTOR_01.PNG");
    TEST_ASSERT(res == true, "Exact .PNG query must resolve");

    // 4.4 Fallback: Match existing .PNG with lowercase .png query
    out[0] = '\0';
    res = re3_textures_resolve_path(out, sizeof(out), "replacements", "ACTOR_01.png");
    TEST_ASSERT(res == true, "Lowercase .png query must fallback and resolve existing .PNG");

    // 4.5 Non-existent file
    out[0] = '\0';
    res = re3_textures_resolve_path(out, sizeof(out), "backgrounds", "ghost.png");
    TEST_ASSERT(res == false, "Missing file must fail");
    TEST_ASSERT(out[0] == '\0', "out_path must be reset to empty on missing file");

    // 4.6 Non-png extension lookup
    out[0] = '\0';
    res = re3_textures_resolve_path(out, sizeof(out), "ui", "font.dds");
    TEST_ASSERT(res == true, "Directly resolving existing .dds must succeed");

    re3_textures_shutdown();
    cleanup_sandbox(sb);
}

// -------------------------------------------------------------
// 5. Stress Test: Path Separator Normalization
// -------------------------------------------------------------
void test_stress_path_separators() {
    fs::path sb = create_sandbox();
    std::string sb_str = sb.generic_string();

    fs::path pack_dir = sb / "mods" / "textures" / "SepPack";
    fs::create_directories(pack_dir / "sub" / "folder");
    std::ofstream(pack_dir / "sub" / "folder" / "item.png") << "item";

    // 5.1 Test base_path with backslashes
    std::string mixed_base = sb_str;
    for (size_t i = 0; i < mixed_base.size(); ++i) {
        if (mixed_base[i] == '/') mixed_base[i] = '\\';
    }

    std::strncpy(g_re3_tex_config.pack, "SepPack", sizeof(g_re3_tex_config.pack) - 1);
    re3_textures_init(mixed_base.c_str());
    TEST_ASSERT(re3_textures_is_pack_valid() == true, "SepPack should be valid with backslashes in base_path");

    // 5.2 Verify s_base_path has no backslashes
    const char* bp = re3_textures_get_base_path();
    TEST_ASSERT(std::strchr(bp, '\\') == nullptr, "s_base_path must normalize all backslashes to forward slashes");

    // 5.3 Resolve path using backslashes in subfolder
    char out[512] = {0};
    bool res = re3_textures_resolve_path(out, sizeof(out), "sub\\folder", "item.png");
    TEST_ASSERT(res == true, "resolve_path must handle backslashes in subfolder");
    TEST_ASSERT(std::strchr(out, '\\') == nullptr, "out_path must normalize slashes to forward slashes");

    // 5.4 Resolve path using slashes in filename
    out[0] = '\0';
    res = re3_textures_resolve_path(out, sizeof(out), "sub", "folder/item.png");
    TEST_ASSERT(res == true, "resolve_path must handle slashes inside filename");
    TEST_ASSERT(std::strchr(out, '\\') == nullptr, "out_path must not contain backslashes");

    // 5.5 Check resolved string contains no duplicate slashes "//"
    TEST_ASSERT(std::strstr(out, "//") == nullptr, "out_path must not have duplicate consecutive slashes");

    re3_textures_shutdown();
    cleanup_sandbox(sb);
}

// -------------------------------------------------------------
// 6. Stress Test: Automatic Creation of dump/textures
// -------------------------------------------------------------
void test_stress_dump_textures_creation() {
    fs::path sb = create_sandbox();
    std::string sb_str = sb.generic_string();

    fs::path dump_dir = sb / "dump" / "textures";

    // 6.1 dump_textures = false -> directory must NOT be created
    g_re3_tex_config.dump_textures = false;
    std::strncpy(g_re3_tex_config.pack, "Vanilla", sizeof(g_re3_tex_config.pack) - 1);
    re3_textures_init(sb_str.c_str());
    TEST_ASSERT(fs::exists(dump_dir) == false, "dump/textures must NOT be created when dump_textures is false");
    re3_textures_shutdown();

    // 6.2 dump_textures = true -> directory MUST be created
    g_re3_tex_config.dump_textures = true;
    re3_textures_init(sb_str.c_str());
    TEST_ASSERT(fs::exists(dump_dir) == true, "dump/textures MUST be created when dump_textures is true");
    TEST_ASSERT(fs::is_directory(dump_dir) == true, "dump/textures must be a directory");
    re3_textures_shutdown();

    // 6.3 Repeated init with dump_textures = true must be idempotent
    re3_textures_init(sb_str.c_str());
    TEST_ASSERT(fs::exists(dump_dir) == true, "Repeated init with dump_textures = true must remain valid");
    re3_textures_shutdown();

    cleanup_sandbox(sb);
}

// -------------------------------------------------------------
// 7. Stress Test: Memory Safety, Canaries, and Buffer Bounds
// -------------------------------------------------------------
void test_stress_memory_safety() {
    fs::path sb = create_sandbox();
    std::string sb_str = sb.generic_string();

    fs::path pack_dir = sb / "mods" / "textures" / "SafetyPack";
    fs::create_directories(pack_dir / "bg");
    std::ofstream(pack_dir / "bg" / "test.png") << "test";

    // 7.1 Buffer boundary on char pack[128] with 127 chars
    std::string pack_127(127, 'A');
    std::strncpy(g_re3_tex_config.pack, pack_127.c_str(), sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.pack[sizeof(g_re3_tex_config.pack) - 1] = '\0';
    re3_textures_init(sb_str.c_str());
    TEST_ASSERT(std::strlen(g_re3_tex_config.pack) == 127, "pack string of 127 chars must be preserved");

    // 7.2 Boundary on oversized pack (200 chars)
    std::string pack_200(200, 'B');
    std::strncpy(g_re3_tex_config.pack, pack_200.c_str(), sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.pack[sizeof(g_re3_tex_config.pack) - 1] = '\0';
    re3_textures_init(sb_str.c_str());
    TEST_ASSERT(std::strlen(g_re3_tex_config.pack) == 127, "pack string must be safely truncated to 127 chars");
    TEST_ASSERT(g_re3_tex_config.pack[127] == '\0', "pack[127] must be null-terminated");

    // 7.3 Pack array completely non-null (unterminated 128 bytes)
    std::memset(g_re3_tex_config.pack, 'C', sizeof(g_re3_tex_config.pack));
    re3_textures_init(sb_str.c_str());
    TEST_ASSERT(std::strlen(g_re3_tex_config.pack) == 127, "unterminated pack must be safely truncated and null-terminated");
    TEST_ASSERT(g_re3_tex_config.pack[127] == '\0', "pack[127] must be null-terminated");

    // 7.4 Pointer null checks on re3_textures_resolve_path
    std::strncpy(g_re3_tex_config.pack, "SafetyPack", sizeof(g_re3_tex_config.pack) - 1);
    re3_textures_init(sb_str.c_str());
    TEST_ASSERT(re3_textures_is_pack_valid() == true, "SafetyPack must be valid");

    // Null out_path
    bool ok_null = re3_textures_resolve_path(nullptr, 100, "bg", "test.png");
    TEST_ASSERT(ok_null == false, "resolve_path with nullptr out_path must return false");

    // Zero max_len
    char buf[256];
    bool ok_zero = re3_textures_resolve_path(buf, 0, "bg", "test.png");
    TEST_ASSERT(ok_zero == false, "resolve_path with max_len == 0 must return false");

    // Null filename
    bool ok_null_fn = re3_textures_resolve_path(buf, sizeof(buf), "bg", nullptr);
    TEST_ASSERT(ok_null_fn == false, "resolve_path with nullptr filename must return false");
    TEST_ASSERT(buf[0] == '\0', "out_path must be reset to empty");

    // Empty filename
    bool ok_empty_fn = re3_textures_resolve_path(buf, sizeof(buf), "bg", "");
    TEST_ASSERT(ok_empty_fn == false, "resolve_path with empty filename must return false");

    // Null subfolder
    std::ofstream(pack_dir / "root_file.png") << "root";
    bool ok_null_sub = re3_textures_resolve_path(buf, sizeof(buf), nullptr, "root_file.png");
    TEST_ASSERT(ok_null_sub == true, "resolve_path with nullptr subfolder should resolve in pack root");

    // Empty subfolder
    bool ok_empty_sub = re3_textures_resolve_path(buf, sizeof(buf), "", "root_file.png");
    TEST_ASSERT(ok_empty_sub == true, "resolve_path with empty subfolder should resolve in pack root");

    // 7.5 Canary buffer boundary check to detect buffer overruns/underflows
    struct CanaryTest {
        char canary_before[32];
        char target_buf[32];
        char canary_after[32];
    } ct;
    std::memset(ct.canary_before, 0x5A, sizeof(ct.canary_before));
    std::memset(ct.canary_after, 0xA5, sizeof(ct.canary_after));
    std::memset(ct.target_buf, 0, sizeof(ct.target_buf));

    // Target buffer is 32 bytes, resolved path is much longer (~60-80 bytes)
    bool truncated_ok = re3_textures_resolve_path(ct.target_buf, sizeof(ct.target_buf), "bg", "test.png");

    // Verify canaries were NOT corrupted
    for (size_t i = 0; i < sizeof(ct.canary_before); ++i) {
        TEST_ASSERT(ct.canary_before[i] == 0x5A, "canary_before corrupted! Buffer underflow!");
    }
    for (size_t i = 0; i < sizeof(ct.canary_after); ++i) {
        TEST_ASSERT(ct.canary_after[i] == (char)0xA5, "canary_after corrupted! Buffer overflow!");
    }
    TEST_ASSERT(ct.target_buf[31] == '\0', "target_buf must be null-terminated within max_len");
    std::printf("    Observation: Boundary truncation returned=%d, path='%s'\n", truncated_ok, ct.target_buf);

    // 7.6 1-byte buffer limit
    char one_byte_buf[1] = {'Z'};
    bool one_byte_res = re3_textures_resolve_path(one_byte_buf, 1, "bg", "test.png");
    TEST_ASSERT(one_byte_buf[0] == '\0', "1-byte buffer must contain only null terminator");

    re3_textures_shutdown();
    cleanup_sandbox(sb);
}

int main() {
    std::printf("====================================================\n");
    std::printf("RE3Recomp M1: Path Resolution & Memory Safety Stress Harness\n");
    std::printf("====================================================\n");

    int last_failed = 0;
    RUN_TEST(test_stress_base_path);
    RUN_TEST(test_stress_nonexistent_packs);
    RUN_TEST(test_stress_path_traversal);
    RUN_TEST(test_stress_case_insensitive_extensions);
    RUN_TEST(test_stress_path_separators);
    RUN_TEST(test_stress_dump_textures_creation);
    RUN_TEST(test_stress_memory_safety);

    std::printf("====================================================\n");
    std::printf("Results: %d executed, %d passed, %d failed\n",
                g_tests_run, g_tests_passed, g_tests_failed);
    std::printf("====================================================\n");

    return (g_tests_failed == 0) ? 0 : 1;
}
