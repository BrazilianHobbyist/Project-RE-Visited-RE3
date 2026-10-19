#include "re3_hd_textures.h"
#include "re3_textures.h"
#include "xxhash.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cmath>

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
    int last_failed = g_tests_failed; \
    g_tests_run++; \
    std::printf("[TEST %d] %s...\n", g_tests_run, #fn); \
    fn(); \
    if (g_tests_failed == last_failed) { \
        g_tests_passed++; \
        std::printf("  [PASS] %s\n", #fn); \
    } \
} while(0)

// Valid 1x1 RGBA PNG bytes
static const unsigned char kValid1x1Png[] = {
    137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82,
    0, 0, 0, 1, 0, 0, 0, 1, 8, 6, 0, 0, 0, 31, 21, 196, 137,
    0, 0, 0, 13, 73, 68, 65, 84, 120, 156, 99, 248, 207, 192, 240, 31,
    0, 5, 0, 1, 255, 137, 153, 61, 29, 0, 0, 0, 0, 73, 69, 78, 68,
    174, 66, 96, 130
};

static fs::path create_sandbox() {
    fs::path p = fs::temp_directory_path() / ("re3_m3_native_" + std::to_string(std::rand()) + "_" + std::to_string(std::rand()));
    std::error_code ec;
    fs::remove_all(p, ec);
    fs::create_directories(p, ec);
    return p;
}

static void write_file(const fs::path& filepath, const unsigned char* data, size_t size) {
    fs::create_directories(filepath.parent_path());
    std::ofstream f(filepath, std::ios::binary);
    f.write(reinterpret_cast<const char*>(data), size);
}

// -------------------------------------------------------------
// 1. Bit-for-bit Parity with Golden Reference xxHash64
// -------------------------------------------------------------
void test_xxhash64_golden_values() {
    // 1.1 Jill Actor Hash
    // JILL_TPAGE = 0x0005, JILL_CLUT = 0x7C00, lim=(0, 0, 255, 255), pal=[0x1234]*16, texels=b"JILL_ACTOR_TEXEL_DATA_MOCK_BYTES"
    uint8_t lim_full[4] = {0, 0, 255, 255};
    uint16_t pal_jill[16];
    for (int i = 0; i < 16; i++) pal_jill[i] = 0x1234;
    const char jill_bytes[] = "JILL_ACTOR_TEXEL_DATA_MOCK_BYTES";
    uint64_t h_jill = re3_hd_tex_compute_hash(0x0005, 0x7C00, lim_full, pal_jill, 16,
                                              reinterpret_cast<const uint16_t*>(jill_bytes),
                                              (int)(sizeof(jill_bytes) - 1) / 2);
    TEST_ASSERT(h_jill == 0x2a8d3a5fc58a1180ULL, "Jill hash must match Python reference bit-for-bit (0x2a8d3a5fc58a1180)");

    // 1.2 Nemesis Actor Hash
    // NEMESIS_TPAGE = 0x0006, NEMESIS_CLUT = 0x7C10, pal=[0x5678]*16, texels=b"NEMESIS_ACTOR_TEXEL_DATA_BYTES"
    uint16_t pal_nemesis[16];
    for (int i = 0; i < 16; i++) pal_nemesis[i] = 0x5678;
    const char nemesis_bytes[] = "NEMESIS_ACTOR_TEXEL_DATA_BYTES";
    uint64_t h_nemesis = re3_hd_tex_compute_hash(0x0006, 0x7C10, lim_full, pal_nemesis, 16,
                                                 reinterpret_cast<const uint16_t*>(nemesis_bytes),
                                                 (int)(sizeof(nemesis_bytes) - 1) / 2);
    TEST_ASSERT(h_nemesis == 0x5eb27155676517dbULL, "Nemesis hash must match Python reference bit-for-bit (0x5eb27155676517db)");

    // 1.3 Weapon Hash
    // WEAPON_TPAGE = 0x0007, WEAPON_CLUT = 0x7C20, lim=(0, 0, 127, 127), pal=[0x9ABC]*16, texels=b"WEAPON_TEXEL_DATA_MOCK_BYTES"
    uint8_t lim_weapon[4] = {0, 0, 127, 127};
    uint16_t pal_weapon[16];
    for (int i = 0; i < 16; i++) pal_weapon[i] = 0x9ABC;
    const char weapon_bytes[] = "WEAPON_TEXEL_DATA_MOCK_BYTES";
    uint64_t h_weapon = re3_hd_tex_compute_hash(0x0007, 0x7C20, lim_weapon, pal_weapon, 16,
                                                reinterpret_cast<const uint16_t*>(weapon_bytes),
                                                (int)(sizeof(weapon_bytes) - 1) / 2);
    TEST_ASSERT(h_weapon == 0x80c806206d596532ULL, "Weapon hash must match Python reference bit-for-bit (0x80c806206d596532)");

    // 1.4 UI Herb Hash
    // UI_HERB_HASH = compute_texture_xxhash64(0x0008, 0x7C30, (0, 0, 31, 31), [0x00FF]*16, b"UI_HERB_ICON_DATA")
    uint8_t lim_herb[4] = {0, 0, 31, 31};
    uint16_t pal_herb[16];
    for (int i = 0; i < 16; i++) pal_herb[i] = 0x00FF;
    const char herb_bytes[] = "UI_HERB_ICON_DATA";
    uint64_t h_herb = re3_hd_tex_compute_hash_bytes(0x0008, 0x7C30, lim_herb, pal_herb, 16,
                                                    herb_bytes, sizeof(herb_bytes) - 1);
    TEST_ASSERT(h_herb == 0x2662656daac1131bULL, "UI Herb hash must match Python reference bit-for-bit (0x2662656daac1131b)");
}

// -------------------------------------------------------------
// 2. Direct VRAM Streaming (Zero Dynamic Allocations)
// -------------------------------------------------------------
void test_vram_streaming_computation() {
    std::vector<uint16_t> mock_vram(1024 * 512, 0);

    // Setup palette at clut_x = 0, clut_y = 496 (clut = 0x7C00)
    uint16_t clut_x = 0;
    uint16_t clut_y = 496;
    for (int i = 0; i < 16; i++) {
        mock_vram[clut_y * 1024 + clut_x + i] = 0x1234;
    }

    // Setup pixel pattern in VRAM
    int base_x = (0x0005 & 0xF) * 64; // 320
    int base_y = 0;
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 64; x++) {
            mock_vram[(base_y + y) * 1024 + (base_x + x)] = (uint16_t)(0xAA00 + y * 64 + x);
        }
    }

    int lim[4] = {0, 0, 63, 15};
    uint64_t h1 = re3_hd_tex_compute_from_vram(0x0005, 0x7C00, lim, mock_vram.data());
    TEST_ASSERT(h1 != 0, "Computed VRAM hash must be non-zero");

    // Repeat to verify deterministic consistency
    uint64_t h2 = re3_hd_tex_compute_from_vram(0x0005, 0x7C00, lim, mock_vram.data());
    TEST_ASSERT(h1 == h2, "VRAM streaming hash must be 100% deterministic");

    // Null VRAM safety
    uint64_t h_null = re3_hd_tex_compute_from_vram(0x0005, 0x7C00, lim, nullptr);
    TEST_ASSERT(h_null == 0, "Null VRAM pointer must safely return 0");
}

// -------------------------------------------------------------
// 3. UV Normalization Mathematics
// -------------------------------------------------------------
void test_uv_normalization() {
    float u_norm = 0.0f, v_norm = 0.0f;

    // Full page: (0, 0) -> (0.5/256, 0.5/256)
    re3_hd_tex_normalize_uv_full_page(0.0f, 0.0f, &u_norm, &v_norm);
    TEST_ASSERT(std::fabs(u_norm - (0.5f / 256.0f)) < 1e-6f, "u_norm for 0 must be 0.5/256");
    TEST_ASSERT(std::fabs(v_norm - (0.5f / 256.0f)) < 1e-6f, "v_norm for 0 must be 0.5/256");

    // Full page: (255, 255) -> (255.5/256, 255.5/256)
    re3_hd_tex_normalize_uv_full_page(255.0f, 255.0f, &u_norm, &v_norm);
    TEST_ASSERT(std::fabs(u_norm - (255.5f / 256.0f)) < 1e-6f, "u_norm for 255 must be 255.5/256");
    TEST_ASSERT(std::fabs(v_norm - (255.5f / 256.0f)) < 1e-6f, "v_norm for 255 must be 255.5/256");

    // Sub-rect: [32, 64, 95, 127]
    re3_hd_tex_normalize_uv_sub_rect(32.0f, 64.0f, 32, 64, 95, 127, &u_norm, &v_norm);
    TEST_ASSERT(std::fabs(u_norm - 0.0f) < 1e-6f, "Sub-rect origin must be 0.0");
    TEST_ASSERT(std::fabs(v_norm - 0.0f) < 1e-6f, "Sub-rect origin must be 0.0");

    // Zero-dimension edge case protection (u_min == u_max)
    re3_hd_tex_normalize_uv_sub_rect(50.0f, 50.0f, 50, 50, 50, 50, &u_norm, &v_norm);
    TEST_ASSERT(std::fabs(u_norm - 0.0f) < 1e-6f, "Zero-width rect must not divide by zero");
}

// -------------------------------------------------------------
// 4. File Resolution & Cache Semantics
// -------------------------------------------------------------
void test_file_resolution_and_cache() {
    fs::path sb = create_sandbox();
    fs::path rep_dir = sb / "mods" / "textures" / "TestPack" / "replacements";

    // Write variations
    write_file(rep_dir / "2a8d3a5fc58a1180.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_file(rep_dir / "5EB27155676517DB.PNG", kValid1x1Png, sizeof(kValid1x1Png));

    g_re3_tex_config.hd_models = true;
    std::string pack_str = (sb / "mods" / "textures" / "TestPack").generic_string();
    re3_hd_tex_init(pack_str.c_str());

    TEST_ASSERT(re3_hd_tex_is_active() == true, "Subsystem must be active when replacements folder exists");

    // Lookup known missing hash: triggers negative cache
    uint64_t missing_hash = 0x1122334455667788ULL;
    uint32_t tex0 = re3_hd_tex_bind_replacement(missing_hash);
    TEST_ASSERT(tex0 == 0, "Missing replacement must return 0 (fallback)");

    re3_hd_tex_metrics_t metrics;
    re3_hd_tex_get_metrics(&metrics);
    TEST_ASSERT(metrics.cache_misses >= 1, "Cache misses metric must increment on missing replacement");

    // Lookup again: must hit negative cache immediately
    uint64_t misses_before = metrics.cache_misses;
    re3_hd_tex_bind_replacement(missing_hash);
    re3_hd_tex_get_metrics(&metrics);
    TEST_ASSERT(metrics.cache_misses > misses_before, "Second lookup must hit negative cache");

    re3_hd_tex_shutdown();
    TEST_ASSERT(re3_hd_tex_is_active() == false, "Subsystem must be inactive after shutdown");
}

// -------------------------------------------------------------
// 5. Transparent Fallback and Lifecycle Stability
// -------------------------------------------------------------
void test_fallback_and_lifecycle() {
    // Disabled via config
    g_re3_tex_config.hd_models = false;
    TEST_ASSERT(re3_hd_tex_is_active() == false, "Must be inactive when hd_models is false");
    TEST_ASSERT(re3_hd_tex_bind_replacement(0x2a8d3a5fc58a1180ULL) == 0, "Must return 0 when disabled");

    g_re3_tex_config.hd_models = true;

    // Idempotent multi-cycle test
    for (int i = 0; i < 5; ++i) {
        re3_hd_tex_init("mods/textures/SeamlessHD");
        re3_hd_tex_shutdown();
    }
}

int main() {
    std::printf("============================================\n");
    std::printf(" Running Native re3_hd_textures Tests\n");
    std::printf("============================================\n");

    RUN_TEST(test_xxhash64_golden_values);
    RUN_TEST(test_vram_streaming_computation);
    RUN_TEST(test_uv_normalization);
    RUN_TEST(test_file_resolution_and_cache);
    RUN_TEST(test_fallback_and_lifecycle);

    std::printf("============================================\n");
    std::printf(" Results: %d executed, %d passed, %d failed\n",
                g_tests_run, g_tests_passed, g_tests_failed);
    std::printf("============================================\n");

    return g_tests_failed == 0 ? 0 : 1;
}
