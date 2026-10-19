#include "re3_hd_backgrounds.h"
#include "re3_textures.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <cmath>
#include <random>

namespace fs = std::filesystem;

static int g_adv_tests_run = 0;
static int g_adv_tests_passed = 0;
static int g_adv_tests_failed = 0;

#define ADV_TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        std::printf("  [FAIL] %s (Line %d: %s)\n", msg, __LINE__, #cond); \
        g_adv_tests_failed++; \
        return; \
    } \
} while(0)

#define RUN_ADV_TEST(fn) do { \
    int last_failed = g_adv_tests_failed; \
    g_adv_tests_run++; \
    std::printf("[ADVERSARIAL TEST %d] %s...\n", g_adv_tests_run, #fn); \
    fn(); \
    if (g_adv_tests_failed == last_failed) { \
        g_adv_tests_passed++; \
        std::printf("  [PASS] %s\n", #fn); \
    } \
} while(0)

// Minimal valid 1x1 RGBA PNG
static const unsigned char kValid1x1Png[] = {
    137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82,
    0, 0, 0, 1, 0, 0, 0, 1, 8, 6, 0, 0, 0, 31, 21, 196, 137,
    0, 0, 0, 13, 73, 68, 65, 84, 120, 156, 99, 248, 207, 192, 240, 31,
    0, 5, 0, 1, 255, 137, 153, 61, 29, 0, 0, 0, 0, 73, 69, 78, 68,
    174, 66, 96, 130
};

static fs::path create_adv_sandbox(const std::string& prefix = "re3_m2_adv_") {
    fs::path p = fs::temp_directory_path() / (prefix + std::to_string(std::rand()) + "_" + std::to_string(std::rand()));
    std::error_code ec;
    fs::remove_all(p, ec);
    fs::create_directories(p, ec);
    return p;
}

static void write_binary_file(const fs::path& filepath, const unsigned char* data, size_t size) {
    fs::create_directories(filepath.parent_path());
    std::ofstream f(filepath, std::ios::binary);
    f.write(reinterpret_cast<const char*>(data), size);
}

static void write_binary_file(const fs::path& filepath, const std::string& str) {
    write_binary_file(filepath, reinterpret_cast<const unsigned char*>(str.data()), str.size());
}

static bool wait_for_decode(int max_ms = 300) {
    int elapsed = 0;
    while (elapsed < max_ms) {
        if (re3_hd_bg_has_hd_current()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        elapsed += 5;
    }
    return re3_hd_bg_has_hd_current();
}

// ============================================================================
// 1. MALFORMED & CORRUPTED PNG INJECTION
// ============================================================================
void test_adv_malformed_png_injection() {
    fs::path sb = create_adv_sandbox("re3_malform_");
    fs::path bg_dir = sb / "mods" / "textures" / "MalformPack" / "backgrounds";
    fs::create_directories(bg_dir);

    // Case 1.1: Zero-byte file
    write_binary_file(bg_dir / "R100_00.png", nullptr, 0);

    // Case 1.2: 1-byte file
    unsigned char b1[] = { 0x89 };
    write_binary_file(bg_dir / "R100_01.png", b1, sizeof(b1));

    // Case 1.3: 7-byte file (just under PNG magic length)
    unsigned char b7[] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A };
    write_binary_file(bg_dir / "R100_02.png", b7, sizeof(b7));

    // Case 1.4: 8-byte valid signature only (no chunks)
    unsigned char b8[] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    write_binary_file(bg_dir / "R100_03.png", b8, sizeof(b8));

    // Case 1.5: Valid signature + truncated IHDR chunk
    unsigned char b_ihdr[] = {
        0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A,
        0x00, 0x00, 0x00, 0x0D, 'I', 'H', 'D', 'R', 0x00, 0x00
    };
    write_binary_file(bg_dir / "R100_04.png", b_ihdr, sizeof(b_ihdr));

    // Case 1.6: Non-PNG fake headers
    write_binary_file(bg_dir / "R100_05.png", "GIF89a\x01\x00\x01\x00\x80\x00\x00");
    write_binary_file(bg_dir / "R100_06.png", "BM\x36\x00\x00\x00\x00\x00\x00\x00");
    write_binary_file(bg_dir / "R100_07.png", "\xFF\xD8\xFF\xE0\x00\x10JFIF");
    write_binary_file(bg_dir / "R100_08.png", "PLAIN_TEXT_NOT_AN_IMAGE_FILE_DATA");

    // Case 1.7: Valid PNG header & IHDR, but corrupted IDAT payload
    std::vector<unsigned char> bad_idat(sizeof(kValid1x1Png));
    std::memcpy(bad_idat.data(), kValid1x1Png, sizeof(kValid1x1Png));
    // Corrupt IDAT bytes
    for (size_t i = 35; i < 50 && i < bad_idat.size(); ++i) {
        bad_idat[i] = 0xAA;
    }
    write_binary_file(bg_dir / "R100_09.png", bad_idat.data(), bad_idat.size());

    // Case 1.8: Oversized file exceeding 64MB limit
    fs::path huge_file = bg_dir / "R100_0A.png";
    {
        std::ofstream hf(huge_file, std::ios::binary);
        // Write PNG header
        hf.write(reinterpret_cast<const char*>(b8), sizeof(b8));
        // Seek to 65 MB and write 1 byte
        hf.seekp(65 * 1024 * 1024, std::ios::beg);
        char end_byte = 0;
        hf.write(&end_byte, 1);
    }

    // Case 1.9: Valid PNG for isolation check
    write_binary_file(bg_dir / "R100_0B.png", kValid1x1Png, sizeof(kValid1x1Png));

    // Initialize engine
    std::string pack_str = (sb / "mods" / "textures" / "MalformPack").generic_string();
    std::strncpy(g_re3_tex_config.pack, "MalformPack", sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.hd_backgrounds = true;

    re3_textures_init(sb.generic_string().c_str());
    re3_hd_bg_init(pack_str.c_str());

    // Test each corrupted cut: MUST NOT CRASH, must return false for has_hd_current
    uint8_t malformed_cuts[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    for (uint8_t cut : malformed_cuts) {
        re3_hd_bg_on_room_change(1, 0, cut);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        ADV_TEST_ASSERT(re3_hd_bg_has_hd_current() == false,
                        "Malformed PNG cut must never be reported as ready HD");
        ADV_TEST_ASSERT(re3_hd_bg_get_current_gl_tex() == 0,
                        "Malformed PNG cut must return 0 GL texture");
        ADV_TEST_ASSERT(re3_hd_bg_render_current(0, 320, 240) == false,
                        "Malformed PNG render must return false to trigger VRAM fallback");
    }

    // Fault Isolation Check: Cut 11 (0x0B) is valid! Must decode properly despite previous errors!
    re3_hd_bg_on_room_change(1, 0, 0x0B);
    bool ok_valid = wait_for_decode(300);
    ADV_TEST_ASSERT(ok_valid == true, "Engine must successfully decode valid PNG even after multiple corrupted files");
    ADV_TEST_ASSERT(re3_hd_bg_has_hd_current() == true, "Valid cut 0x0B must report ready");

    re3_hd_bg_shutdown();
    re3_textures_shutdown();

    std::error_code ec;
    fs::remove_all(sb, ec);
}

// ============================================================================
// 2. PATH TRAVERSAL INJECTION
// ============================================================================
void test_adv_path_traversal_injection() {
    fs::path sb = create_adv_sandbox("re3_traversal_");
    fs::path legitimate_pack = sb / "mods" / "textures" / "LegitPack";
    fs::create_directories(legitimate_pack / "backgrounds");
    write_binary_file(legitimate_pack / "backgrounds" / "R100_00.png", kValid1x1Png, sizeof(kValid1x1Png));

    // Secret files outside pack
    fs::path secret_dir = sb / "secret_sys";
    fs::create_directories(secret_dir);
    write_binary_file(secret_dir / "secret.png", kValid1x1Png, sizeof(kValid1x1Png));

    char out[1024];

    // Traversal attack vectors
    const char* attack_paths[] = {
        "../../../../secret_sys",
        "..\\..\\..\\..\\secret_sys",
        "mods/textures/../../secret_sys",
        "mods/textures/..\\..\\secret_sys",
        "../../windows/system32",
        "..\\..\\windows\\system32",
        "C:\\Windows\\System32",
        "C:/Windows/System32",
        "//server/share/mods",
        "\\\\server\\share\\mods",
        ".../....//secret",
        "LegitPack/../../../secret_sys"
    };

    for (const char* attack : attack_paths) {
        re3_hd_bg_init(attack);
        const char* res = re3_hd_bg_resolve_file(1, 0, 0, out, sizeof(out));
        ADV_TEST_ASSERT(res == nullptr, "Path traversal in pack_path must be rejected by re3_hd_bg_resolve_file");
        ADV_TEST_ASSERT(re3_hd_bg_has_hd_current() == false, "Path traversal must not produce active HD background");

        re3_hd_bg_on_room_change(1, 0, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ADV_TEST_ASSERT(re3_hd_bg_has_hd_current() == false, "Path traversal on room change must not load anything");
        re3_hd_bg_shutdown();
    }

    // Verify legitimate pack still works after traversal tests
    std::string legit_str = legitimate_pack.generic_string();
    re3_hd_bg_init(legit_str.c_str());
    const char* res_legit = re3_hd_bg_resolve_file(1, 0, 0, out, sizeof(out));
    ADV_TEST_ASSERT(res_legit != nullptr, "Legitimate pack must resolve cleanly");
    re3_hd_bg_shutdown();

    std::error_code ec;
    fs::remove_all(sb, ec);
}

// ============================================================================
// 3. EXTREME OUT-OF-BOUNDS ROOM/CUT NUMBERS
// ============================================================================
void test_adv_extreme_out_of_bounds() {
    fs::path sb = create_adv_sandbox("re3_oob_");
    fs::path bg_dir = sb / "mods" / "textures" / "OobPack" / "backgrounds";
    fs::create_directories(bg_dir);

    // Write background for stage 0, room 0, cut 0
    write_binary_file(bg_dir / "R000_00.png", kValid1x1Png, sizeof(kValid1x1Png));
    // Write background for boundary cut: stage 15 (0xF), room 255 (0xFF), cut 255 (0xFF)
    write_binary_file(bg_dir / "RFFF_FF.png", kValid1x1Png, sizeof(kValid1x1Png));

    std::string pack_str = (sb / "mods" / "textures" / "OobPack").generic_string();
    std::strncpy(g_re3_tex_config.pack, "OobPack", sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.hd_backgrounds = true;

    re3_textures_init(sb.generic_string().c_str());
    re3_hd_bg_init(pack_str.c_str());

    char out[1024];

    // 3.1 Min boundaries: (0, 0, 0)
    const char* res0 = re3_hd_bg_resolve_file(0, 0, 0, out, sizeof(out));
    std::printf("    DEBUG: res0 = %s\n", res0 ? res0 : "NULL");
    std::printf("    DEBUG: is_pack_valid = %d, hd_backgrounds = %d\n",
                re3_textures_is_pack_valid(), g_re3_tex_config.hd_backgrounds);
    ADV_TEST_ASSERT(res0 != nullptr, "Boundary min (0, 0, 0) must resolve to R000_00.png");

    // Test stage 0, room 0, cut 1 (key = 1 != 0) -> Succeeded
    write_binary_file(bg_dir / "R000_01.png", kValid1x1Png, sizeof(kValid1x1Png));
    re3_hd_bg_on_room_change(0, 0, 1);
    bool ok_cut1 = wait_for_decode(500);
    std::printf("    Observation: stage 0, room 0, cut 1 (key = 1): wait_for_decode = %d, has_hd = %d [SUCCESS]\n",
                ok_cut1, re3_hd_bg_has_hd_current());

    // Test stage 0, room 0, cut 0 (key = 0) -> Fails due to 'if (key == 0) continue;' in worker_thread_func
    re3_hd_bg_on_room_change(0, 0, 0);
    bool ok_d = wait_for_decode(500);
    std::printf("    Observation: stage 0, room 0, cut 0 (key = 0): wait_for_decode = %d, has_hd = %d [FAILURE]\n",
                ok_d, re3_hd_bg_has_hd_current());
    if (!ok_d) {
        std::printf("    [BUG CONFIRMED] re3_hd_backgrounds.cpp:301 skips decoding when key == 0 (stage 0, room 0, cut 0).\n");
    }
    ADV_TEST_ASSERT(ok_d == true, "Stage 0, room 0, cut 0 must load (Failed: key == 0 discarded by worker thread)");

    // 3.2 Max uint8_t boundary: (15, 255, 255) -> RFFF_FF.png
    const char* res_max = re3_hd_bg_resolve_file(0xF, 0xFF, 0xFF, out, sizeof(out));
    ADV_TEST_ASSERT(res_max != nullptr, "Boundary (15, 255, 255) must resolve to RFFF_FF.png");

    re3_hd_bg_on_room_change(0xF, 0xFF, 0xFF);
    ADV_TEST_ASSERT(wait_for_decode(300) == true, "Boundary (15, 255, 255) must load");

    // 3.3 Stage 255 (0xFF) -> candidate RFFF_FF.png formatted via %1X
    const char* res_255 = re3_hd_bg_resolve_file(255, 255, 255, out, sizeof(out));
    ADV_TEST_ASSERT(res_255 != nullptr, "Stage 255 with %1X handles lower nibble/hex safely");

    // 3.4 Non-existent extreme bounds: (255, 254, 253) -> missing file
    const char* res_none = re3_hd_bg_resolve_file(255, 254, 253, out, sizeof(out));
    ADV_TEST_ASSERT(res_none == nullptr, "Missing extreme bounds must return nullptr");

    // 3.5 Fuzzing loop: 1,000 rapid arbitrary (stage, room, cut) requests
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 255);

    for (int i = 0; i < 1000; ++i) {
        uint8_t s = static_cast<uint8_t>(dist(rng));
        uint8_t r = static_cast<uint8_t>(dist(rng));
        uint8_t c = static_cast<uint8_t>(dist(rng));

        re3_hd_bg_on_room_change(s, r, c);
        re3_hd_bg_has_hd_current();
        re3_hd_bg_get_current_gl_tex();
    }

    re3_hd_bg_metrics_t metrics;
    re3_hd_bg_get_metrics(&metrics);
    ADV_TEST_ASSERT(metrics.misses > 900, "Fuzzed missing cuts should register as misses");

    re3_hd_bg_shutdown();
    re3_textures_shutdown();

    std::error_code ec;
    fs::remove_all(sb, ec);
}

// ============================================================================
// 4. VARIATIONS OF RESHDP FILENAMES & PRECEDENCE
// ============================================================================
void test_adv_reshdp_variations_and_precedence() {
    fs::path sb = create_adv_sandbox("re3_variations_");
    fs::path bg_dir = sb / "mods" / "textures" / "VarPack" / "backgrounds";
    fs::create_directories(bg_dir);

    // Create different files
    write_binary_file(bg_dir / "R100_00.png", kValid1x1Png, sizeof(kValid1x1Png)); // Canonical
    write_binary_file(bg_dir / "R100_1.png", kValid1x1Png, sizeof(kValid1x1Png));  // Single digit
    write_binary_file(bg_dir / "r100_02.png", kValid1x1Png, sizeof(kValid1x1Png)); // Lowercase standard
    write_binary_file(bg_dir / "r100_3.png", kValid1x1Png, sizeof(kValid1x1Png));  // Lowercase single digit
    write_binary_file(bg_dir / "R100_04.PNG", kValid1x1Png, sizeof(kValid1x1Png)); // Uppercase .PNG
    write_binary_file(bg_dir / "R10A_0F.png", kValid1x1Png, sizeof(kValid1x1Png)); // Hex uppercase
    write_binary_file(bg_dir / "r10b_0e.png", kValid1x1Png, sizeof(kValid1x1Png)); // Hex lowercase

    std::string pack_str = (sb / "mods" / "textures" / "VarPack").generic_string();
    re3_hd_bg_init(pack_str.c_str());

    char out[1024];

    // 4.1 Canonical
    ADV_TEST_ASSERT(re3_hd_bg_resolve_file(1, 0, 0, out, sizeof(out)) != nullptr, "Canonical R100_00.png");
    ADV_TEST_ASSERT(fs::path(out).filename().string() == "R100_00.png", "Matched filename must be exact");

    // 4.2 Single digit
    ADV_TEST_ASSERT(re3_hd_bg_resolve_file(1, 0, 1, out, sizeof(out)) != nullptr, "Single digit R100_1.png");

    // 4.3 Lowercase standard
    ADV_TEST_ASSERT(re3_hd_bg_resolve_file(1, 0, 2, out, sizeof(out)) != nullptr, "Lowercase r100_02.png");

    // 4.4 Lowercase single digit
    ADV_TEST_ASSERT(re3_hd_bg_resolve_file(1, 0, 3, out, sizeof(out)) != nullptr, "Lowercase single digit r100_3.png");

    // 4.5 Uppercase .PNG
    ADV_TEST_ASSERT(re3_hd_bg_resolve_file(1, 0, 4, out, sizeof(out)) != nullptr, "Uppercase R100_04.PNG");

    // 4.6 Hexadecimal room/cut uppercase
    ADV_TEST_ASSERT(re3_hd_bg_resolve_file(1, 0x0A, 0x0F, out, sizeof(out)) != nullptr, "Hex uppercase R10A_0F.png");

    // 4.7 Hexadecimal room/cut lowercase
    ADV_TEST_ASSERT(re3_hd_bg_resolve_file(1, 0x0B, 0x0E, out, sizeof(out)) != nullptr, "Hex lowercase r10b_0e.png");

    // 4.8 Precedence test: Both canonical R100_05.png and single digit R100_5.png exist
    write_binary_file(bg_dir / "R100_05.png", std::string("CANONICAL"));
    write_binary_file(bg_dir / "R100_5.png", std::string("SINGLE_DIGIT"));

    ADV_TEST_ASSERT(re3_hd_bg_resolve_file(1, 0, 5, out, sizeof(out)) != nullptr, "Precedence cut 5");
    std::string resolved_name = fs::path(out).filename().string();
    ADV_TEST_ASSERT(resolved_name == "R100_05.png", "Canonical R100_05.png must take priority over R100_5.png");

    re3_hd_bg_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// ============================================================================
// 5. PARTIAL PACKS & TRANSPARENT FALLBACK
// ============================================================================
void test_adv_partial_packs_transparent_fallback() {
    fs::path sb = create_adv_sandbox("re3_partial_");
    fs::path bg_dir = sb / "mods" / "textures" / "PartialPack" / "backgrounds";
    fs::create_directories(bg_dir);

    // In PartialPack, only cut 0 and cut 2 exist for room 0x10.
    // Cut 1 and cut 3 are missing!
    write_binary_file(bg_dir / "R110_00.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_binary_file(bg_dir / "R110_02.png", kValid1x1Png, sizeof(kValid1x1Png));

    // Also add noise / dummy non-image files in backgrounds folder
    write_binary_file(bg_dir / "README.txt", "Some mod description");
    write_binary_file(bg_dir / "R110_00.bak", "Backup file");
    write_binary_file(bg_dir / "thumbs.db", "Fake thumbnail db");

    std::string pack_str = (sb / "mods" / "textures" / "PartialPack").generic_string();
    std::strncpy(g_re3_tex_config.pack, "PartialPack", sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.hd_backgrounds = true;

    re3_textures_init(sb.generic_string().c_str());
    re3_hd_bg_init(pack_str.c_str());

    // Step 1: Switch to cut 0 -> Present!
    re3_hd_bg_on_room_change(1, 0x10, 0);
    ADV_TEST_ASSERT(wait_for_decode(300) == true, "Cut 0 must decode successfully");
    ADV_TEST_ASSERT(re3_hd_bg_has_hd_current() == true, "Cut 0 has HD");

    // Step 2: Switch to cut 1 -> MISSING!
    re3_hd_bg_on_room_change(1, 0x10, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    ADV_TEST_ASSERT(re3_hd_bg_has_hd_current() == false, "Missing cut 1 must report has_hd_current == false");
    ADV_TEST_ASSERT(re3_hd_bg_get_current_gl_tex() == 0, "Missing cut 1 must return 0 GL texture handle");
    ADV_TEST_ASSERT(re3_hd_bg_render_current(0, 320, 240) == false, "Missing cut 1 render must return false (VRAM fallback)");

    // Step 3: Switch to cut 2 -> Present!
    re3_hd_bg_on_room_change(1, 0x10, 2);
    ADV_TEST_ASSERT(wait_for_decode(300) == true, "Cut 2 must decode successfully");
    ADV_TEST_ASSERT(re3_hd_bg_has_hd_current() == true, "Cut 2 has HD");

    // Step 4: Switch back to cut 0 -> Should be immediately available from cache!
    re3_hd_bg_on_room_change(1, 0x10, 0);
    ADV_TEST_ASSERT(re3_hd_bg_has_hd_current() == true, "Cut 0 re-visit must hit cache immediately");

    // Step 5: Completely empty pack folder
    fs::path empty_dir = sb / "mods" / "textures" / "EmptyPack" / "backgrounds";
    fs::create_directories(empty_dir);
    std::string empty_str = (sb / "mods" / "textures" / "EmptyPack").generic_string();

    re3_hd_bg_init(empty_str.c_str());
    re3_hd_bg_on_room_change(1, 0, 0);
    ADV_TEST_ASSERT(re3_hd_bg_has_hd_current() == false, "Empty pack must report false");
    ADV_TEST_ASSERT(re3_hd_bg_render_current(0, 320, 240) == false, "Empty pack render must return false");

    // Step 6: Missing backgrounds directory completely
    fs::path no_bg_dir = sb / "mods" / "textures" / "NoBgPack";
    fs::create_directories(no_bg_dir);
    std::string no_bg_str = no_bg_dir.generic_string();

    re3_hd_bg_init(no_bg_str.c_str());
    re3_hd_bg_on_room_change(1, 0, 0);
    ADV_TEST_ASSERT(re3_hd_bg_has_hd_current() == false, "Pack without backgrounds dir must report false");

    re3_hd_bg_shutdown();
    re3_textures_shutdown();

    std::error_code ec;
    fs::remove_all(sb, ec);
}

// ============================================================================
// 6. CACHE THRASHING, STRESS & CONCURRENCY
// ============================================================================
void test_adv_cache_thrashing_and_concurrency() {
    fs::path sb = create_adv_sandbox("re3_stress_");
    fs::path bg_dir = sb / "mods" / "textures" / "StressPack" / "backgrounds";
    fs::create_directories(bg_dir);

    // Create 45 cuts to exceed the 32 max cached texture limit
    constexpr int kTotalCuts = 45;
    for (int i = 0; i < kTotalCuts; ++i) {
        char name[64];
        std::snprintf(name, sizeof(name), "R100_%02X.png", i);
        write_binary_file(bg_dir / name, kValid1x1Png, sizeof(kValid1x1Png));
    }

    std::string pack_str = (sb / "mods" / "textures" / "StressPack").generic_string();
    std::strncpy(g_re3_tex_config.pack, "StressPack", sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.hd_backgrounds = true;

    re3_textures_init(sb.generic_string().c_str());
    re3_hd_bg_init(pack_str.c_str());

    // Cycle through all 45 cuts sequentially to trigger decodes and GL uploads
    for (int i = 0; i < kTotalCuts; ++i) {
        re3_hd_bg_on_room_change(1, 0, static_cast<uint8_t>(i));
        // Give worker thread a moment to decode
        wait_for_decode(100);
        // Trigger GL upload
        re3_hd_bg_get_current_gl_tex();
    }

    // Verify metrics: prefetches should be triggered, and evictions should occur if limit exceeded
    re3_hd_bg_metrics_t metrics;
    re3_hd_bg_get_metrics(&metrics);
    ADV_TEST_ASSERT(metrics.prefetches > 0, "Prefetches must be recorded");

    // Rapid concurrent hammering: simulate thread querying while room changes happen
    std::atomic<bool> stop_hammer{false};
    std::thread hammer([&]() {
        while (!stop_hammer.load()) {
            re3_hd_bg_has_hd_current();
            re3_hd_bg_get_current_gl_tex();
            re3_hd_bg_render_current(0, 320, 240);
        }
    });

    for (int i = 0; i < 200; ++i) {
        re3_hd_bg_on_room_change(1, 0, static_cast<uint8_t>(i % kTotalCuts));
    }

    stop_hammer = true;
    hammer.join();

    // Rapid init/shutdown stress (ensure no deadlocks, thread termination issues)
    for (int i = 0; i < 15; ++i) {
        re3_hd_bg_init(pack_str.c_str());
        re3_hd_bg_on_room_change(1, 0, 0);
        re3_hd_bg_shutdown();
    }

    re3_textures_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// ============================================================================
// MAIN RUNNER
// ============================================================================
int main() {
    std::printf("===============================================================\n");
    std::printf(" RE3Recomp M2: Adversarial Fault Injection & Resilience Harness\n");
    std::printf("===============================================================\n");

    RUN_ADV_TEST(test_adv_malformed_png_injection);
    RUN_ADV_TEST(test_adv_path_traversal_injection);
    RUN_ADV_TEST(test_adv_extreme_out_of_bounds);
    RUN_ADV_TEST(test_adv_reshdp_variations_and_precedence);
    RUN_ADV_TEST(test_adv_partial_packs_transparent_fallback);
    RUN_ADV_TEST(test_adv_cache_thrashing_and_concurrency);

    std::printf("===============================================================\n");
    std::printf(" Adversarial Results: %d executed, %d passed, %d failed\n",
                g_adv_tests_run, g_adv_tests_passed, g_adv_tests_failed);
    std::printf("===============================================================\n");

    return (g_adv_tests_failed == 0) ? 0 : 1;
}
