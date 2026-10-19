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
#include <random>

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

static const unsigned char kValid1x1Png[] = {
    137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82,
    0, 0, 0, 1, 0, 0, 0, 1, 8, 6, 0, 0, 0, 31, 21, 196, 137,
    0, 0, 0, 13, 73, 68, 65, 84, 120, 156, 99, 248, 207, 192, 240, 31,
    0, 5, 0, 1, 255, 137, 153, 61, 29, 0, 0, 0, 0, 73, 69, 78, 68,
    174, 66, 96, 130
};

static uint8_t g_adv_mock_ram[2 * 1024 * 1024] = {0};

extern "C" uint8_t psx_read_byte(uint32_t addr) {
    return g_adv_mock_ram[addr & 0x1FFFFFu];
}

static fs::path create_sandbox() {
    fs::path p = fs::temp_directory_path() / ("re3_m4_adv_" + std::to_string(std::rand()) + "_" + std::to_string(std::rand()));
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
// 1. Path Traversal & Security Invariance
// -------------------------------------------------------------
void test_security_path_traversal() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "SecurePack" / "ui";
    fs::create_directories(ui_dir);

    write_png_file(ui_dir / "inventory.png", kValid1x1Png, sizeof(kValid1x1Png));

    std::string pack_str = (sb / "mods" / "textures" / "SecurePack").generic_string();
    g_re3_tex_config.hd_ui = true;
    re3_hd_ui_init(pack_str.c_str());

    char out[1024];

    // Traversal vectors
    const char* malicious_screens[] = {
        "../inventory",
        "..\\inventory",
        "../../../../windows/system32/cmd",
        "C:\\Windows\\System32\\cmd.exe",
        "/etc/shadow",
        "inventory/../../secret",
        "status_fine/..",
        "",
        nullptr
    };

    for (int i = 0; malicious_screens[i] != nullptr; i++) {
        const char* res = re3_hd_ui_resolve_backdrop_path(malicious_screens[i], out, sizeof(out));
        TEST_ASSERT(res == nullptr, "Malicious path traversal attempt must be rejected");
        bool has = re3_hd_ui_has_backdrop(malicious_screens[i]);
        TEST_ASSERT(has == false, "Malicious screen has_backdrop must be false");
    }

    // Traversal inside pack path init
    re3_hd_ui_init("../../../../windows");
    TEST_ASSERT(re3_hd_ui_is_active() == false, "Init with traversal pack path must fail");

    re3_hd_ui_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 2. Rapid Backdrop Switching Stress (1000 Iterations)
// -------------------------------------------------------------
void test_rapid_backdrop_switching_stress() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "StressPack" / "ui";

    const char* screen_names[] = {
        "inventory", "status_fine", "status_caution", "status_danger",
        "status_poison", "map", "clock", "save",
        "epilogue_0", "epilogue_1", "epilogue_2", "epilogue_3",
        "epilogue_4", "epilogue_5", "epilogue_6", "epilogue_7"
    };

    for (const char* name : screen_names) {
        write_png_file(ui_dir / (std::string(name) + ".png"), kValid1x1Png, sizeof(kValid1x1Png));
    }

    std::string pack_str = (sb / "mods" / "textures" / "StressPack").generic_string();
    g_re3_tex_config.hd_ui = true;
    re3_hd_ui_init(pack_str.c_str());

    // 1000 rapid switches
    for (int iter = 0; iter < 1000; iter++) {
        int idx = iter % 16;
        const char* name = screen_names[idx];

        re3_ui_backdrop_info_t info;
        bool ok = re3_hd_ui_load_backdrop(name, &info);
        TEST_ASSERT(ok == true, "Rapid switch load must succeed");
        TEST_ASSERT(info.has_hd == true, "info.has_hd must be true");

        bool blit = re3_hd_ui_render_backdrop(name, 0, 1920, 1080);
        TEST_ASSERT(blit == true, "Blit must succeed in headless context");
    }

    re3_hd_ui_metrics_t metrics;
    re3_hd_ui_get_metrics(&metrics);
    TEST_ASSERT(metrics.backdrop_hits >= 1000, "1000 backdrop hits must be recorded");

    re3_hd_ui_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 3. Sprite Cache Thrashing & LRU Eviction (500 Unique Hashes)
// -------------------------------------------------------------
void test_sprite_cache_flooding_and_eviction() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "SpritePack" / "ui";

    std::vector<uint64_t> test_hashes;
    std::mt19937_64 rng(1337);

    // Create 300 unique sprite PNGs (exceeds default LRU cache of 256)
    for (int i = 0; i < 300; i++) {
        uint64_t h = rng() | 1ULL;
        test_hashes.push_back(h);

        char h_fname[64];
        std::snprintf(h_fname, sizeof(h_fname), "%016llx.png", (unsigned long long)h);
        write_png_file(ui_dir / h_fname, kValid1x1Png, sizeof(kValid1x1Png));
    }

    std::string pack_str = (sb / "mods" / "textures" / "SpritePack").generic_string();
    g_re3_tex_config.hd_ui = true;
    re3_hd_ui_init(pack_str.c_str());

    // Bind all 300 sprites (forces LRU evictions)
    for (uint64_t h : test_hashes) {
        uint32_t tex = re3_hd_ui_bind_sprite(h);
        TEST_ASSERT(tex != 0, "Sprite replacement must load and return handle");
    }

    re3_hd_ui_metrics_t m;
    re3_hd_ui_get_metrics(&m);
    TEST_ASSERT(m.sprite_hits >= 300, "All 300 sprite loads must hit");

    // Re-access first 50 (which may have been evicted and must reload cleanly)
    for (int i = 0; i < 50; i++) {
        uint32_t tex = re3_hd_ui_bind_sprite(test_hashes[i]);
        TEST_ASSERT(tex != 0, "Evicted sprite must reload safely without error");
    }

    re3_hd_ui_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 4. Memory State & Extreme Boundary Invariance
// -------------------------------------------------------------
void test_extreme_memory_states() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "MemPack" / "ui";

    write_png_file(ui_dir / "status_fine.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(ui_dir / "inventory.png", kValid1x1Png, sizeof(kValid1x1Png));

    for (int i = 0; i < 8; i++) {
        write_png_file(ui_dir / ("epilogue_" + std::to_string(i) + ".png"), kValid1x1Png, sizeof(kValid1x1Png));
    }

    std::string pack_str = (sb / "mods" / "textures" / "MemPack").generic_string();
    g_re3_tex_config.hd_ui = true;
    re3_hd_ui_init(pack_str.c_str());

    // 4.1 Condition 0x00 (Unset/death) -> defaults to status_fine
    char out[1024];
    const char* p = re3_hd_ui_resolve_status_backdrop(0x00, out, sizeof(out));
    TEST_ASSERT(p != nullptr && std::string(out).find("status_fine.png") != std::string::npos, "0x00 condition must default to status_fine");

    // 4.2 Condition 0xAA (Bogus value) -> defaults to status_fine
    p = re3_hd_ui_resolve_status_backdrop(0xAA, out, sizeof(out));
    TEST_ASSERT(p != nullptr && std::string(out).find("status_fine.png") != std::string::npos, "0xAA condition must default to status_fine");

    // 4.3 Epilogue memory checks
    g_adv_mock_ram[0x000CCC97] = 0x04;
    g_adv_mock_ram[0x000D1F3E] = 1; // 1 epilogue unlocked -> epilogue_0
    const char* ep_str = re3_hd_ui_detect_screen_from_memory();
    TEST_ASSERT(ep_str != nullptr && std::strcmp(ep_str, "epilogue_0") == 0, "1 unlocked epilogue must return epilogue_0");

    g_adv_mock_ram[0x000D1F3E] = 8; // 8 epilogues unlocked -> epilogue_7
    ep_str = re3_hd_ui_detect_screen_from_memory();
    TEST_ASSERT(ep_str != nullptr && std::strcmp(ep_str, "epilogue_7") == 0, "8 unlocked epilogues must return epilogue_7");

    // 4.4 Quad geometry boundary checks
    // Negative dimensions
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(0, 0, -320, 240) == false, "Negative width must be false");
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(0, 0, 320, -240) == false, "Negative height must be false");
    // Off-screen offsets
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(500, 500, 320, 240) == false, "Off-screen quad must be false");
    // Borderline slack: x0 = 10, y0 = 10, w = 312, h = 236
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(4, 4, 316, 236) == true, "Fullscreen quad with slight edge slack must be accepted");

    re3_hd_ui_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 5. Repeated Shutdown & Re-init Resilience (Leak Check)
// -------------------------------------------------------------
void test_reinit_stress() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "ReinitPack" / "ui";
    write_png_file(ui_dir / "inventory.png", kValid1x1Png, sizeof(kValid1x1Png));

    std::string pack_str = (sb / "mods" / "textures" / "ReinitPack").generic_string();

    for (int cycle = 0; cycle < 50; cycle++) {
        g_re3_tex_config.hd_ui = true;
        re3_hd_ui_init(pack_str.c_str());
        TEST_ASSERT(re3_hd_ui_is_active() == true, "Must be active on init cycle");

        re3_ui_backdrop_info_t info;
        re3_hd_ui_load_backdrop("inventory", &info);

        re3_hd_ui_shutdown();
        TEST_ASSERT(re3_hd_ui_is_active() == false, "Must be inactive after shutdown");
    }

    std::error_code ec;
    fs::remove_all(sb, ec);
}

int main() {
    std::printf("=====================================================\n");
    std::printf(" Running Adversarial Stress Tests for Milestone 4 (HD UI)\n");
    std::printf("=====================================================\n");

    RUN_TEST(test_security_path_traversal);
    RUN_TEST(test_rapid_backdrop_switching_stress);
    RUN_TEST(test_sprite_cache_flooding_and_eviction);
    RUN_TEST(test_extreme_memory_states);
    RUN_TEST(test_reinit_stress);

    std::printf("=====================================================\n");
    std::printf(" Adversarial Results: %d executed, %d passed, %d failed\n",
                g_tests_run, g_tests_passed, g_tests_failed);
    std::printf("=====================================================\n");

    return g_tests_failed == 0 ? 0 : 1;
}
