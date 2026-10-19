#include "re3_hd_ui.h"
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

static uint8_t g_mock_ram[2 * 1024 * 1024] = {0};

extern "C" uint8_t psx_read_byte(uint32_t addr) {
    return g_mock_ram[addr & 0x1FFFFFu];
}

static fs::path create_sandbox() {
    fs::path p = fs::temp_directory_path() / ("re3_m4_native_" + std::to_string(std::rand()) + "_" + std::to_string(std::rand()));
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
// 1. Lifecycle & Activation Tests
// -------------------------------------------------------------
void test_lifecycle_and_activation() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "SeamlessHD" / "ui";
    fs::create_directories(ui_dir);

    std::string pack_str = (sb / "mods" / "textures" / "SeamlessHD").generic_string();
    g_re3_tex_config.hd_ui = true;
    std::strncpy(g_re3_tex_config.pack, "SeamlessHD", sizeof(g_re3_tex_config.pack) - 1);

    re3_hd_ui_init(pack_str.c_str());
    TEST_ASSERT(re3_hd_ui_is_active() == true, "HD UI must be active when enabled with valid folder");

    // Toggle config off
    g_re3_tex_config.hd_ui = false;
    TEST_ASSERT(re3_hd_ui_is_active() == false, "HD UI must be inactive when config flag is false");

    // Toggle config back on
    g_re3_tex_config.hd_ui = true;
    TEST_ASSERT(re3_hd_ui_is_active() == true, "HD UI must be active again");

    re3_hd_ui_shutdown();
    TEST_ASSERT(re3_hd_ui_is_active() == false, "HD UI must be inactive after shutdown");

    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 2. Backdrop Resolution & Candidate Matching
// -------------------------------------------------------------
void test_backdrop_resolution() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "SeamlessHD" / "ui";

    // Create screens
    write_png_file(ui_dir / "inventory.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(ui_dir / "status_fine.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(ui_dir / "status_caution.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(ui_dir / "status_danger.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(ui_dir / "status_poison.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(ui_dir / "map.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(ui_dir / "clock.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(ui_dir / "save.png", kValid1x1Png, sizeof(kValid1x1Png));

    for (int i = 0; i < 8; i++) {
        write_png_file(ui_dir / ("epilogue_" + std::to_string(i) + ".png"), kValid1x1Png, sizeof(kValid1x1Png));
    }

    std::string pack_str = (sb / "mods" / "textures" / "SeamlessHD").generic_string();
    g_re3_tex_config.hd_ui = true;
    re3_hd_ui_init(pack_str.c_str());

    char out_path[1024];

    // 2.1 Inventory
    const char* p = re3_hd_ui_resolve_backdrop_path("inventory", out_path, sizeof(out_path));
    TEST_ASSERT(p != nullptr, "inventory backdrop must resolve");
    TEST_ASSERT(fs::exists(out_path), "resolved inventory must exist");

    // 2.2 Map
    p = re3_hd_ui_resolve_backdrop_path("map", out_path, sizeof(out_path));
    TEST_ASSERT(p != nullptr, "map backdrop must resolve");

    // 2.3 Clock & Save
    p = re3_hd_ui_resolve_backdrop_path("clock", out_path, sizeof(out_path));
    TEST_ASSERT(p != nullptr, "clock backdrop must resolve");
    p = re3_hd_ui_resolve_backdrop_path("save", out_path, sizeof(out_path));
    TEST_ASSERT(p != nullptr, "save backdrop must resolve");

    // 2.4 Epilogues 0..7
    for (int i = 0; i < 8; i++) {
        std::string ep_name = "epilogue_" + std::to_string(i);
        p = re3_hd_ui_resolve_backdrop_path(ep_name.c_str(), out_path, sizeof(out_path));
        TEST_ASSERT(p != nullptr, "epilogue 0..7 must resolve");
    }

    // 2.5 Epilogue 8 (Out-of-bounds boundary condition)
    p = re3_hd_ui_resolve_backdrop_path("epilogue_8", out_path, sizeof(out_path));
    TEST_ASSERT(p == nullptr, "epilogue_8 must return NULL (boundary check)");

    // 2.6 Path traversal security check
    p = re3_hd_ui_resolve_backdrop_path("../../../../etc/passwd", out_path, sizeof(out_path));
    TEST_ASSERT(p == nullptr, "Path traversal must be rejected");

    re3_hd_ui_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 3. Status ECG Condition Mapping Tests
// -------------------------------------------------------------
void test_status_ecg_mapping() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "SeamlessHD" / "ui";

    write_png_file(ui_dir / "status_fine.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(ui_dir / "status_caution.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(ui_dir / "status_danger.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(ui_dir / "status_poison.png", kValid1x1Png, sizeof(kValid1x1Png));

    std::string pack_str = (sb / "mods" / "textures" / "SeamlessHD").generic_string();
    g_re3_tex_config.hd_ui = true;
    re3_hd_ui_init(pack_str.c_str());

    char out_path[1024];

    // 3.1 Condition FINE (0x04)
    const char* p = re3_hd_ui_resolve_status_backdrop(0x04, out_path, sizeof(out_path));
    TEST_ASSERT(p != nullptr, "Condition FINE must resolve");
    TEST_ASSERT(std::string(out_path).find("status_fine.png") != std::string::npos, "0x04 must map to status_fine");

    // 3.2 Condition CAUTION (0x03)
    p = re3_hd_ui_resolve_status_backdrop(0x03, out_path, sizeof(out_path));
    TEST_ASSERT(p != nullptr && std::string(out_path).find("status_caution.png") != std::string::npos, "0x03 must map to status_caution");

    // 3.3 Condition DANGER (0x02)
    p = re3_hd_ui_resolve_status_backdrop(0x02, out_path, sizeof(out_path));
    TEST_ASSERT(p != nullptr && std::string(out_path).find("status_danger.png") != std::string::npos, "0x02 must map to status_danger");

    // 3.4 Condition POISON (0x01)
    p = re3_hd_ui_resolve_status_backdrop(0x01, out_path, sizeof(out_path));
    TEST_ASSERT(p != nullptr && std::string(out_path).find("status_poison.png") != std::string::npos, "0x01 must map to status_poison");

    // 3.5 Unknown condition 0xFF -> defaults to status_fine (Boundary test b30)
    p = re3_hd_ui_resolve_status_backdrop(0xFF, out_path, sizeof(out_path));
    TEST_ASSERT(p != nullptr && std::string(out_path).find("status_fine.png") != std::string::npos, "0xFF must default to status_fine");

    re3_hd_ui_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 4. Backdrop Loading, Caching & Native Fallback
// -------------------------------------------------------------
void test_backdrop_loading_and_fallback() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "SeamlessHD" / "ui";

    write_png_file(ui_dir / "inventory.png", kValid1x1Png, sizeof(kValid1x1Png));

    std::string pack_str = (sb / "mods" / "textures" / "SeamlessHD").generic_string();
    g_re3_tex_config.hd_ui = true;
    re3_hd_ui_init(pack_str.c_str());

    re3_ui_backdrop_info_t info;

    // 4.1 Successful load
    bool ok = re3_hd_ui_load_backdrop("inventory", &info);
    TEST_ASSERT(ok == true, "inventory backdrop must load successfully");
    TEST_ASSERT(info.has_hd == true, "has_hd must be true");
    TEST_ASSERT(info.fallback_native_ui == false, "fallback_native_ui must be false");
    TEST_ASSERT(info.gl_tex_id != 0, "gl_tex_id must be non-zero");
    TEST_ASSERT(info.width == 1 && info.height == 1, "dimensions must be 1x1");
    TEST_ASSERT(re3_hd_ui_has_backdrop("inventory") == true, "has_backdrop must return true");

    // 4.2 Caching check: second load returns identical gl_tex_id
    re3_ui_backdrop_info_t info2;
    ok = re3_hd_ui_load_backdrop("inventory", &info2);
    TEST_ASSERT(ok == true, "cached inventory must load successfully");
    TEST_ASSERT(info.gl_tex_id == info2.gl_tex_id, "cached gl_tex_id must match");

    // 4.3 Missing backdrop fallback
    re3_ui_backdrop_info_t missing_info;
    ok = re3_hd_ui_load_backdrop("non_existent_screen", &missing_info);
    TEST_ASSERT(ok == false, "non_existent_screen must return false");
    TEST_ASSERT(missing_info.has_hd == false, "missing backdrop has_hd must be false");
    TEST_ASSERT(missing_info.fallback_native_ui == true, "missing backdrop fallback_native_ui must be true");

    re3_hd_ui_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 5. UI Sprite Replacement & Caching (Feature 13)
// -------------------------------------------------------------
void test_ui_sprite_replacement() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "SeamlessHD" / "ui";

    // UI Herb Icon Hash = 0x2662656daac1131bULL
    uint64_t herb_hash = 0x2662656daac1131bULL;
    char herb_fname[32];
    std::snprintf(herb_fname, sizeof(herb_fname), "%016llx.png", (unsigned long long)herb_hash);
    write_png_file(ui_dir / herb_fname, kValid1x1Png, sizeof(kValid1x1Png));

    std::string pack_str = (sb / "mods" / "textures" / "SeamlessHD").generic_string();
    g_re3_tex_config.hd_ui = true;
    re3_hd_ui_init(pack_str.c_str());

    // 5.1 Lookup existing sprite
    uint32_t tex1 = re3_hd_ui_bind_sprite(herb_hash);
    TEST_ASSERT(tex1 != 0, "UI Herb sprite must bind non-zero texture handle");

    // 5.2 Cache hit
    uint32_t tex2 = re3_hd_ui_bind_sprite(herb_hash);
    TEST_ASSERT(tex1 == tex2, "UI Herb sprite must return cached texture handle");

    // 5.3 Missing sprite fallback
    uint32_t missing_tex = re3_hd_ui_bind_sprite(0x9999999999999999ULL);
    TEST_ASSERT(missing_tex == 0, "Missing sprite must return 0 for transparent PS1 fallback");

    // 5.4 Disabled config fallback
    g_re3_tex_config.hd_ui = false;
    uint32_t dis_tex = re3_hd_ui_bind_sprite(herb_hash);
    TEST_ASSERT(dis_tex == 0, "Disabled UI must return 0");
    g_re3_tex_config.hd_ui = true;

    // 5.5 Metrics verification
    re3_hd_ui_metrics_t metrics;
    re3_hd_ui_get_metrics(&metrics);
    TEST_ASSERT(metrics.sprite_hits >= 2, "Sprite hits must be recorded");
    TEST_ASSERT(metrics.sprite_misses >= 1, "Sprite misses must be recorded");

    re3_hd_ui_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 6. Memory Screen Detection & Quad Interception
// -------------------------------------------------------------
void test_memory_detection_and_quad_interception() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "SeamlessHD" / "ui";
    write_png_file(ui_dir / "inventory.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(ui_dir / "status_fine.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(ui_dir / "status_danger.png", kValid1x1Png, sizeof(kValid1x1Png));

    std::string pack_str = (sb / "mods" / "textures" / "SeamlessHD").generic_string();
    g_re3_tex_config.hd_ui = true;
    re3_hd_ui_init(pack_str.c_str());

    // 6.1 Screen quad geometry tests
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(0, 0, 320, 240) == true, "Full 320x240 quad must be detected");
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(0, 0, 160, 240) == true, "Split 160x240 tile must be detected");
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(10, 10, 64, 64) == false, "Small sprite rect must NOT be detected as fullscreen quad");

    // 6.2 Memory state detection: FINE (0x04)
    g_mock_ram[0x000CCC97] = 0x04;
    g_mock_ram[0x000D1F3E] = 0;
    const char* s = re3_hd_ui_detect_screen_from_memory();
    TEST_ASSERT(s != nullptr && std::strcmp(s, "status_fine") == 0, "Condition 0x04 must detect status_fine");

    // 6.3 Memory state detection: DANGER (0x02)
    g_mock_ram[0x000CCC97] = 0x02;
    s = re3_hd_ui_detect_screen_from_memory();
    TEST_ASSERT(s != nullptr && std::strcmp(s, "status_danger") == 0, "Condition 0x02 must detect status_danger");

    // 6.4 Intercept menu quad
    bool intercepted = re3_hd_ui_intercept_menu_quad(0, 0, 320, 240, 0, 0, 0, 320, 240);
    TEST_ASSERT(intercepted == true, "Menu quad must be intercepted when backdrop exists");

    re3_hd_ui_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 7. Corrupt PNG Resilience
// -------------------------------------------------------------
void test_corrupt_png_resilience() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "SeamlessHD" / "ui";

    const char corrupt_bytes[] = "NOT_A_VALID_PNG_FILE_CONTENT_TRUNCATED";
    write_png_file(ui_dir / "inventory.png", reinterpret_cast<const unsigned char*>(corrupt_bytes), sizeof(corrupt_bytes));

    std::string pack_str = (sb / "mods" / "textures" / "SeamlessHD").generic_string();
    g_re3_tex_config.hd_ui = true;
    re3_hd_ui_init(pack_str.c_str());

    re3_ui_backdrop_info_t info;
    bool ok = re3_hd_ui_load_backdrop("inventory", &info);
    TEST_ASSERT(ok == false, "Corrupted PNG must fail gracefully");
    TEST_ASSERT(info.has_hd == false, "has_hd must be false for corrupt PNG");
    TEST_ASSERT(info.fallback_native_ui == true, "fallback_native_ui must be true for corrupt PNG");

    re3_hd_ui_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

int main() {
    std::printf("============================================\n");
    std::printf(" Running Native re3_hd_ui (Milestone 4) Tests\n");
    std::printf("============================================\n");

    RUN_TEST(test_lifecycle_and_activation);
    RUN_TEST(test_backdrop_resolution);
    RUN_TEST(test_status_ecg_mapping);
    RUN_TEST(test_backdrop_loading_and_fallback);
    RUN_TEST(test_ui_sprite_replacement);
    RUN_TEST(test_memory_detection_and_quad_interception);
    RUN_TEST(test_corrupt_png_resilience);

    std::printf("============================================\n");
    std::printf(" Results: %d executed, %d passed, %d failed\n",
                g_tests_run, g_tests_passed, g_tests_failed);
    std::printf("============================================\n");

    return g_tests_failed == 0 ? 0 : 1;
}
