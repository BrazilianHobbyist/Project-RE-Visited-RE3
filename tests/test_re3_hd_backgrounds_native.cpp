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
    fs::path p = fs::temp_directory_path() / ("re3_m2_native_" + std::to_string(std::rand()) + "_" + std::to_string(std::rand()));
    std::error_code ec;
    fs::remove_all(p, ec);
    fs::create_directories(p, ec);
    return p;
}

static void write_png_file(const fs::path& filepath, const unsigned char* data, size_t size) {
    fs::create_directories(filepath.parent_path());
    std::ofstream f(filepath, std::ios::binary);
    f.write(reinterpret_cast<const char*>(data), size);
}

// -------------------------------------------------------------
// 1. RESHDP File Resolution Tests
// -------------------------------------------------------------
void test_file_resolution() {
    fs::path sb = create_sandbox();
    fs::path bg_dir = sb / "mods" / "textures" / "SeamlessHD" / "backgrounds";

    // Create various naming variations
    write_png_file(bg_dir / "R100_00.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(bg_dir / "R100_2.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(bg_dir / "r100_01.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(bg_dir / "r100_3.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(bg_dir / "R100_04.PNG", kValid1x1Png, sizeof(kValid1x1Png));

    std::string pack_str = (sb / "mods" / "textures" / "SeamlessHD").generic_string();
    re3_hd_bg_init(pack_str.c_str());

    char out[1024];

    // 1.1 Standard RESHDP
    const char* res = re3_hd_bg_resolve_file(1, 0, 0, out, sizeof(out));
    TEST_ASSERT(res != nullptr, "Standard R100_00.png must be resolved");
    TEST_ASSERT(fs::exists(out), "Resolved path must exist on disk");

    // 1.2 Single-digit cut
    res = re3_hd_bg_resolve_file(1, 0, 2, out, sizeof(out));
    TEST_ASSERT(res != nullptr, "Single digit R100_2.png must be resolved");
    TEST_ASSERT(fs::exists(out), "Resolved path must exist on disk");

    // 1.3 Lowercase standard
    res = re3_hd_bg_resolve_file(1, 0, 1, out, sizeof(out));
    TEST_ASSERT(res != nullptr, "Lowercase r100_01.png must be resolved");
    TEST_ASSERT(fs::exists(out), "Resolved path must exist on disk");

    // 1.4 Lowercase single digit
    res = re3_hd_bg_resolve_file(1, 0, 3, out, sizeof(out));
    TEST_ASSERT(res != nullptr, "Lowercase single digit r100_3.png must be resolved");
    TEST_ASSERT(fs::exists(out), "Resolved path must exist on disk");

    // 1.5 Uppercase extension .PNG
    res = re3_hd_bg_resolve_file(1, 0, 4, out, sizeof(out));
    TEST_ASSERT(res != nullptr, "Uppercase extension R100_04.PNG must be resolved");
    TEST_ASSERT(fs::exists(out), "Resolved path must exist on disk");

    // 1.6 Missing cut returns NULL
    res = re3_hd_bg_resolve_file(1, 99, 99, out, sizeof(out));
    TEST_ASSERT(res == nullptr, "Missing cut must return nullptr");

    // 1.7 Path traversal rejection
    re3_hd_bg_init("../../../../windows/system32");
    res = re3_hd_bg_resolve_file(1, 0, 0, out, sizeof(out));
    TEST_ASSERT(res == nullptr, "Path traversal attempt must return nullptr");

    // 1.8 Null buffer safe handling
    res = re3_hd_bg_resolve_file(1, 0, 0, nullptr, 0);
    TEST_ASSERT(res == nullptr, "Null buffer must return nullptr without crash");

    re3_hd_bg_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 2. Screen-Space UV Mapping Tests
// -------------------------------------------------------------
void test_screen_space_uv() {
    float uv[4] = {0};

    // 2.1 Full-screen quad
    re3_hd_bg_get_screen_uv(0, 0, 320, 240, uv);
    TEST_ASSERT(std::fabs(uv[0] - 0.0f) < 1e-5f, "u_min must be 0.0");
    TEST_ASSERT(std::fabs(uv[1] - 0.0f) < 1e-5f, "v_min must be 0.0");
    TEST_ASSERT(std::fabs(uv[2] - 1.0f) < 1e-5f, "u_max must be 1.0");
    TEST_ASSERT(std::fabs(uv[3] - 1.0f) < 1e-5f, "v_max must be 1.0");

    // 2.2 Sub-rect pillar mask (50, 40, 60, 120)
    re3_hd_bg_get_screen_uv(50, 40, 60, 120, uv);
    TEST_ASSERT(std::fabs(uv[0] - (50.0f / 320.0f)) < 1e-5f, "u_min math correct");
    TEST_ASSERT(std::fabs(uv[1] - (40.0f / 240.0f)) < 1e-5f, "v_min math correct");
    TEST_ASSERT(std::fabs(uv[2] - (110.0f / 320.0f)) < 1e-5f, "u_max math correct");
    TEST_ASSERT(std::fabs(uv[3] - (160.0f / 240.0f)) < 1e-5f, "v_max math correct");

    // 2.3 Null pointer safety
    re3_hd_bg_get_screen_uv(0, 0, 320, 240, nullptr);
}

// -------------------------------------------------------------
// 3. Asynchronous Pre-fetcher & LRU Cache Tests
// -------------------------------------------------------------
void test_async_prefetch_and_cache() {
    fs::path sb = create_sandbox();
    fs::path bg_dir = sb / "mods" / "textures" / "SeamlessHD" / "backgrounds";

    // Setup room 0 with multiple cuts
    write_png_file(bg_dir / "R100_00.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(bg_dir / "R100_01.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(bg_dir / "R100_02.png", kValid1x1Png, sizeof(kValid1x1Png));

    std::string pack_str = (sb / "mods" / "textures" / "SeamlessHD").generic_string();
    std::strncpy(g_re3_tex_config.pack, "SeamlessHD", sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.hd_backgrounds = true;

    re3_textures_init(sb.generic_string().c_str());
    re3_hd_bg_init(pack_str.c_str());

    // Trigger room change to stage 1, room 0, cut 0
    re3_hd_bg_on_room_change(1, 0, 0);

    // Wait up to 500ms for async worker to decode
    bool decoded = false;
    for (int i = 0; i < 50; ++i) {
        if (re3_hd_bg_has_hd_current()) {
            decoded = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    TEST_ASSERT(decoded, "Async worker must decode active cut into memory");

    // Verify metrics
    re3_hd_bg_metrics_t metrics;
    re3_hd_bg_get_metrics(&metrics);
    TEST_ASSERT(metrics.prefetches >= 1, "Prefetch count must be >= 1");

    // Second access to (1, 0, 0) should register a hit
    re3_hd_bg_on_room_change(1, 0, 0);
    re3_hd_bg_get_metrics(&metrics);
    TEST_ASSERT(metrics.hits >= 1, "Hit count must increment on cached cut");

    // Missing cut should register a miss
    re3_hd_bg_on_room_change(1, 88, 88);
    re3_hd_bg_get_metrics(&metrics);
    TEST_ASSERT(metrics.misses >= 1, "Miss count must increment on missing cut");
    TEST_ASSERT(re3_hd_bg_has_hd_current() == false, "Missing cut must return has_hd_current == false");

    re3_hd_bg_shutdown();
    re3_textures_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 4. Corrupt PNG Resilience
// -------------------------------------------------------------
void test_corrupt_png_resilience() {
    fs::path sb = create_sandbox();
    fs::path bg_dir = sb / "mods" / "textures" / "CorruptPack" / "backgrounds";

    // Corrupted PNG file
    const char corrupt_bytes[] = "THIS_IS_NOT_A_PNG_FILE_AT_ALL_CORRUPTED";
    write_png_file(bg_dir / "R100_00.png", reinterpret_cast<const unsigned char*>(corrupt_bytes), sizeof(corrupt_bytes));

    std::string pack_str = (sb / "mods" / "textures" / "CorruptPack").generic_string();
    std::strncpy(g_re3_tex_config.pack, "CorruptPack", sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.hd_backgrounds = true;

    re3_textures_init(sb.generic_string().c_str());
    re3_hd_bg_init(pack_str.c_str());

    re3_hd_bg_on_room_change(1, 0, 0);

    // Give worker time to process and fail
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Must gracefully fall back without crash
    TEST_ASSERT(re3_hd_bg_has_hd_current() == false, "Corrupted PNG must gracefully fail without crashing");

    re3_hd_bg_shutdown();
    re3_textures_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 5. Transparent Fallback & Lifecycle
// -------------------------------------------------------------
void test_transparent_fallback_and_lifecycle() {
    // When disabled via config
    g_re3_tex_config.hd_backgrounds = false;
    TEST_ASSERT(re3_hd_bg_has_hd_current() == false, "Must return false when hd_backgrounds config is false");
    TEST_ASSERT(re3_hd_bg_get_current_gl_tex() == 0, "Must return 0 GL texture when disabled");
    TEST_ASSERT(re3_hd_bg_render_current(0, 320, 240) == false, "Render current must return false when disabled");

    g_re3_tex_config.hd_backgrounds = true;

    // Shutdown and restart repeatedly to ensure no deadlocks or thread leaks
    for (int i = 0; i < 5; ++i) {
        re3_hd_bg_init("mods/textures/SeamlessHD");
        re3_hd_bg_shutdown();
    }
}

int main() {
    std::printf("============================================\n");
    std::printf(" Running Native re3_hd_backgrounds Tests\n");
    std::printf("============================================\n");

    RUN_TEST(test_file_resolution);
    RUN_TEST(test_screen_space_uv);
    RUN_TEST(test_async_prefetch_and_cache);
    RUN_TEST(test_corrupt_png_resilience);
    RUN_TEST(test_transparent_fallback_and_lifecycle);

    std::printf("============================================\n");
    std::printf(" Results: %d executed, %d passed, %d failed\n",
                g_tests_run, g_tests_passed, g_tests_failed);
    std::printf("============================================\n");

    return g_tests_failed == 0 ? 0 : 1;
}
