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
#include <climits>
#include <future>

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
    std::printf("[CHALLENGE TEST %d] %s...\n", g_tests_run, #fn); \
    fn(); \
    if (g_tests_failed == last_failed) { \
        g_tests_passed++; \
        std::printf("  [PASS] %s\n", #fn); \
    } \
} while(0)

// Valid 1x1 RGBA PNG
static const unsigned char kValid1x1Png[] = {
    137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82,
    0, 0, 0, 1, 0, 0, 0, 1, 8, 6, 0, 0, 0, 31, 21, 196, 137,
    0, 0, 0, 13, 73, 68, 65, 84, 120, 156, 99, 248, 207, 192, 240, 31,
    0, 5, 0, 1, 255, 137, 153, 61, 29, 0, 0, 0, 0, 73, 69, 78, 68,
    174, 66, 96, 130
};

static uint8_t g_chal_mock_ram[2 * 1024 * 1024] = {0};

extern "C" uint8_t psx_read_byte(uint32_t addr) {
    return g_chal_mock_ram[addr & 0x1FFFFFu];
}

static fs::path create_sandbox() {
    static int s_count = 0;
    fs::path p = fs::temp_directory_path() / ("re3_m4_chal_" + std::to_string(std::rand()) + "_" + std::to_string(++s_count));
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

// -------------------------------------------------------------
// 1. Adversarial Path Traversal Fuzzing
// -------------------------------------------------------------
void test_adversarial_path_traversal_fuzz() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "FuzzPack" / "ui";
    fs::create_directories(ui_dir);

    write_binary_file(ui_dir / "inventory.png", kValid1x1Png, sizeof(kValid1x1Png));

    std::string pack_str = (sb / "mods" / "textures" / "FuzzPack").generic_string();
    g_re3_tex_config.hd_ui = true;
    re3_hd_ui_init(pack_str.c_str());

    char out[2048];

    // Array of aggressive path traversal vectors
    const char* attack_vectors[] = {
        "../inventory",
        "..\\inventory",
        "../../inventory",
        "..\\..\\inventory",
        "inventory/..",
        "inventory\\..",
        "inventory/../../windows/system32/cmd.exe",
        "inventory\\..\\..\\windows\\system32\\cmd.exe",
        "C:\\Windows\\System32\\cmd.exe",
        "D:\\secrets.txt",
        "/etc/passwd",
        "/etc/shadow",
        "\\\\127.0.0.1\\c$\\windows",
        "inventory\0secret",
        "inventory.png\0.png",
        "....//....//windows",
        "....\\\\....\\\\windows",
        "inventory%2e%2e",
        "inventory%2f",
        "inventory%5c",
        "inventory..png",
        "::$DATA",
        "CON",
        "PRN",
        "AUX",
        "NUL",
        "COM1",
        "LPT1",
        nullptr
    };

    for (int i = 0; attack_vectors[i] != nullptr; i++) {
        const char* res = re3_hd_ui_resolve_backdrop_path(attack_vectors[i], out, sizeof(out));
        TEST_ASSERT(res == nullptr, "Malicious or reserved attack vector must be rejected");
        bool has = re3_hd_ui_has_backdrop(attack_vectors[i]);
        TEST_ASSERT(has == false, "Attack vector has_backdrop must return false");
        re3_ui_backdrop_info_t info;
        bool loaded = re3_hd_ui_load_backdrop(attack_vectors[i], &info);
        TEST_ASSERT(loaded == false, "Attack vector load_backdrop must return false");
        TEST_ASSERT(info.fallback_native_ui == true, "Fallback native UI must be active");
    }

    // Huge string attack (10,000 characters)
    std::string huge_string(10000, 'A');
    const char* huge_res = re3_hd_ui_resolve_backdrop_path(huge_string.c_str(), out, sizeof(out));
    TEST_ASSERT(huge_res == nullptr, "10,000 char screen_name must be safely rejected without buffer overflow");

    // Null pointer resilience
    TEST_ASSERT(re3_hd_ui_resolve_backdrop_path(nullptr, out, sizeof(out)) == nullptr, "nullptr screen_name must return nullptr");
    TEST_ASSERT(re3_hd_ui_resolve_backdrop_path("inventory", nullptr, 1024) == nullptr, "nullptr out_path must return nullptr");
    TEST_ASSERT(re3_hd_ui_resolve_backdrop_path("inventory", out, 0) == nullptr, "0 max_len must return nullptr");
    TEST_ASSERT(re3_hd_ui_has_backdrop(nullptr) == false, "nullptr has_backdrop must return false");
    TEST_ASSERT(re3_hd_ui_load_backdrop(nullptr, nullptr) == false, "nullptr load_backdrop must return false");
    TEST_ASSERT(re3_hd_ui_render_backdrop(nullptr, 0, 1920, 1080) == false, "nullptr render_backdrop must return false");

    re3_hd_ui_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 2. Corrupted PNG Fuzzing & Malformed Asset Suite
// -------------------------------------------------------------
void test_corrupted_png_fuzzing() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "CorruptPack" / "ui";
    fs::create_directories(ui_dir);

    std::string pack_str = (sb / "mods" / "textures" / "CorruptPack").generic_string();
    g_re3_tex_config.hd_ui = true;
    re3_hd_ui_init(pack_str.c_str());

    // Fuzz case 1: Zero-byte file
    write_binary_file(ui_dir / "zero.png", nullptr, 0);

    // Fuzz case 2: Truncated 1..7 bytes (sub-PNG header)
    for (size_t len = 1; len <= 7; len++) {
        write_binary_file(ui_dir / ("trunc_" + std::to_string(len) + ".png"), kValid1x1Png, len);
    }

    // Fuzz case 3: 8-byte PNG header followed by zero bytes
    write_binary_file(ui_dir / "head_only.png", kValid1x1Png, 8);

    // Fuzz case 4: Valid header with corrupted IHDR size (0x0, huge, etc.)
    unsigned char bad_ihdr[33];
    std::memcpy(bad_ihdr, kValid1x1Png, 33);
    // Overwrite width with 0
    bad_ihdr[16] = 0; bad_ihdr[17] = 0; bad_ihdr[18] = 0; bad_ihdr[19] = 0;
    write_binary_file(ui_dir / "zero_dim.png", bad_ihdr, sizeof(bad_ihdr));

    // Fuzz case 5: Garbage payload
    unsigned char garbage[1024];
    for (size_t i = 0; i < sizeof(garbage); i++) garbage[i] = static_cast<unsigned char>(i & 0xFF);
    write_binary_file(ui_dir / "garbage.png", garbage, sizeof(garbage));

    // Test each corrupted asset
    const char* corrupt_names[] = {
        "zero", "trunc_1", "trunc_2", "trunc_3", "trunc_4", "trunc_5", "trunc_6", "trunc_7",
        "head_only", "zero_dim", "garbage"
    };

    for (const char* name : corrupt_names) {
        re3_ui_backdrop_info_t info;
        bool ok = re3_hd_ui_load_backdrop(name, &info);
        TEST_ASSERT(ok == false, "Corrupted PNG load must return false");
        TEST_ASSERT(info.has_hd == false, "has_hd must be false");
        TEST_ASSERT(info.fallback_native_ui == true, "fallback_native_ui must be true");
        TEST_ASSERT(info.gl_tex_id == 0, "gl_tex_id must be 0 for corrupt file");

        bool blit = re3_hd_ui_render_backdrop(name, 0, 1920, 1080);
        TEST_ASSERT(blit == false, "Corrupted PNG render_backdrop must return false");
    }

    // Also test corrupt sprite via re3_hd_ui_bind_sprite
    uint64_t corrupt_sprite_hash = 0xDEADBEEFCAFE1234ULL;
    char s_fname[64];
    std::snprintf(s_fname, sizeof(s_fname), "%016llx.png", (unsigned long long)corrupt_sprite_hash);
    write_binary_file(ui_dir / s_fname, garbage, sizeof(garbage));

    uint32_t s_tex = re3_hd_ui_bind_sprite(corrupt_sprite_hash);
    TEST_ASSERT(s_tex == 0, "Corrupt sprite PNG must return 0 texture handle for fallback");

    re3_hd_ui_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 3. Exhaustive Health Condition Domain Search (0x00 to 0xFF)
// -------------------------------------------------------------
void test_exhaustive_health_condition_domain() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "HealthPack" / "ui";
    fs::create_directories(ui_dir);

    write_binary_file(ui_dir / "status_fine.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_binary_file(ui_dir / "status_caution.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_binary_file(ui_dir / "status_danger.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_binary_file(ui_dir / "status_poison.png", kValid1x1Png, sizeof(kValid1x1Png));

    std::string pack_str = (sb / "mods" / "textures" / "HealthPack").generic_string();
    g_re3_tex_config.hd_ui = true;
    re3_hd_ui_init(pack_str.c_str());

    char out[1024];

    // Test every single uint8_t value 0..255
    for (int c = 0; c <= 255; c++) {
        uint8_t cond = static_cast<uint8_t>(c);
        const char* res = re3_hd_ui_resolve_status_backdrop(cond, out, sizeof(out));
        TEST_ASSERT(res != nullptr, "Status condition must resolve for all 256 byte values");

        std::string s_path(out);
        if (cond == 0x04) {
            TEST_ASSERT(s_path.find("status_fine.png") != std::string::npos, "0x04 must map to status_fine");
        } else if (cond == 0x03) {
            TEST_ASSERT(s_path.find("status_caution.png") != std::string::npos, "0x03 must map to status_caution");
        } else if (cond == 0x02) {
            TEST_ASSERT(s_path.find("status_danger.png") != std::string::npos, "0x02 must map to status_danger");
        } else if (cond == 0x01) {
            TEST_ASSERT(s_path.find("status_poison.png") != std::string::npos, "0x01 must map to status_poison");
        } else {
            // Any other condition (0x00, 0x05..0xFF) must default to status_fine
            TEST_ASSERT(s_path.find("status_fine.png") != std::string::npos, "Unknown condition must default to status_fine");
        }
    }

    re3_hd_ui_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 4. Out-of-Bounds Epilogue Indices Boundary Search
// -------------------------------------------------------------
void test_epilogue_boundary_extremes() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "EpiloguePack" / "ui";
    fs::create_directories(ui_dir);

    for (int i = 0; i < 8; i++) {
        write_binary_file(ui_dir / ("epilogue_" + std::to_string(i) + ".png"), kValid1x1Png, sizeof(kValid1x1Png));
    }
    // Also write a malicious/bogus epilogue_8.png and epilogue_99.png to disk
    // to verify that the boundary check rejects it at the API layer even if a rogue file exists
    write_binary_file(ui_dir / "epilogue_8.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_binary_file(ui_dir / "epilogue_9.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_binary_file(ui_dir / "epilogue_99.png", kValid1x1Png, sizeof(kValid1x1Png));

    std::string pack_str = (sb / "mods" / "textures" / "EpiloguePack").generic_string();
    g_re3_tex_config.hd_ui = true;
    re3_hd_ui_init(pack_str.c_str());

    char out[1024];

    // Valid indices 0..7 must resolve
    for (int i = 0; i < 8; i++) {
        std::string ep_name = "epilogue_" + std::to_string(i);
        const char* res = re3_hd_ui_resolve_backdrop_path(ep_name.c_str(), out, sizeof(out));
        TEST_ASSERT(res != nullptr, "epilogue 0..7 must resolve");
    }

    // Invalid index 8 must be rejected even though epilogue_8.png exists on disk!
    const char* res8 = re3_hd_ui_resolve_backdrop_path("epilogue_8", out, sizeof(out));
    TEST_ASSERT(res8 == nullptr, "epilogue_8 must be rejected by boundary check");

    // Invalid index 9 must be rejected
    const char* res9 = re3_hd_ui_resolve_backdrop_path("epilogue_9", out, sizeof(out));
    TEST_ASSERT(res9 == nullptr, "epilogue_9 must be rejected by boundary check");

    // Extreme numeric overflow epilogue
    const char* res_huge = re3_hd_ui_resolve_backdrop_path("epilogue_999999999999999999999999999999", out, sizeof(out));
    TEST_ASSERT(res_huge == nullptr, "Extreme numeric epilogue must be rejected without exception");

    re3_hd_ui_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 5. Multi-Threaded Concurrency & Race Stress
// -------------------------------------------------------------
void test_multithreaded_concurrency_stress() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "ThreadPack" / "ui";
    fs::create_directories(ui_dir);

    const char* screens[] = {
        "inventory", "status_fine", "status_caution", "status_danger", "status_poison", "map", "clock", "save"
    };
    for (const char* name : screens) {
        write_binary_file(ui_dir / (std::string(name) + ".png"), kValid1x1Png, sizeof(kValid1x1Png));
    }

    // 50 sprites
    std::vector<uint64_t> sprite_hashes;
    for (int i = 0; i < 50; i++) {
        uint64_t h = 0x1000000000000000ULL + i;
        sprite_hashes.push_back(h);
        char fname[64];
        std::snprintf(fname, sizeof(fname), "%016llx.png", (unsigned long long)h);
        write_binary_file(ui_dir / fname, kValid1x1Png, sizeof(kValid1x1Png));
    }

    std::string pack_str = (sb / "mods" / "textures" / "ThreadPack").generic_string();
    g_re3_tex_config.hd_ui = true;
    re3_hd_ui_init(pack_str.c_str());

    constexpr int kNumThreads = 8;
    constexpr int kOpsPerThread = 500;
    std::vector<std::thread> workers;
    std::atomic<int> errors{0};

    for (int t = 0; t < kNumThreads; t++) {
        workers.emplace_back([&, t]() {
            char local_out[1024];
            for (int op = 0; op < kOpsPerThread; op++) {
                // Alternating operations
                int pick = (t * kOpsPerThread + op) % 8;
                const char* scr = screens[pick];

                re3_ui_backdrop_info_t info;
                bool ok = re3_hd_ui_load_backdrop(scr, &info);
                if (!ok || !info.has_hd) {
                    errors++;
                }

                uint64_t s_hash = sprite_hashes[(t + op) % sprite_hashes.size()];
                uint32_t s_tex = re3_hd_ui_bind_sprite(s_hash);
                if (s_tex == 0) {
                    errors++;
                }

                re3_hd_ui_metrics_t m;
                re3_hd_ui_get_metrics(&m);

                const char* p = re3_hd_ui_resolve_backdrop_path(scr, local_out, sizeof(local_out));
                if (!p) {
                    errors++;
                }
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    TEST_ASSERT(errors.load() == 0, "Multi-threaded concurrency stress must encounter zero errors/races");

    re3_hd_ui_metrics_t final_m;
    re3_hd_ui_get_metrics(&final_m);
    TEST_ASSERT(final_m.backdrop_hits > 0, "Backdrop hits must be recorded under concurrency");
    TEST_ASSERT(final_m.sprite_hits > 0, "Sprite hits must be recorded under concurrency");

    re3_hd_ui_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 6. Massive Cache Flooding & Eviction Verification
// -------------------------------------------------------------
void test_massive_cache_flooding() {
    fs::path sb = create_sandbox();
    fs::path ui_dir = sb / "mods" / "textures" / "FloodPack" / "ui";
    fs::create_directories(ui_dir);

    std::mt19937_64 rng(42);
    constexpr int kTotalSprites = 600; // Far exceeds 256 cache capacity
    std::vector<uint64_t> hashes;

    for (int i = 0; i < kTotalSprites; i++) {
        uint64_t h = rng() | 1ULL;
        hashes.push_back(h);
        char fname[64];
        std::snprintf(fname, sizeof(fname), "%016llx.png", (unsigned long long)h);
        write_binary_file(ui_dir / fname, kValid1x1Png, sizeof(kValid1x1Png));
    }

    std::string pack_str = (sb / "mods" / "textures" / "FloodPack").generic_string();
    g_re3_tex_config.hd_ui = true;
    re3_hd_ui_init(pack_str.c_str());

    // 1. Flood all 600 sprites
    for (uint64_t h : hashes) {
        uint32_t t = re3_hd_ui_bind_sprite(h);
        TEST_ASSERT(t != 0, "All sprites must load successfully during flood");
    }

    // 2. Query 200 non-existent sprites to populate negative cache
    for (int i = 0; i < 200; i++) {
        uint64_t bad_h = 0xF000000000000000ULL + i;
        uint32_t t = re3_hd_ui_bind_sprite(bad_h);
        TEST_ASSERT(t == 0, "Missing sprites must return 0");
    }

    // 3. Re-query all 600 sprites; evicted ones must reload cleanly
    for (uint64_t h : hashes) {
        uint32_t t = re3_hd_ui_bind_sprite(h);
        TEST_ASSERT(t != 0, "Reloaded sprites after eviction must return valid texture handle");
    }

    re3_hd_ui_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 7. Quad Geometry Interception Extreme Coordinates
// -------------------------------------------------------------
void test_quad_geometry_extremes() {
    // Standard quads
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(0, 0, 320, 240) == true, "320x240 must match");
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(0, 0, 160, 240) == true, "160x240 must match");
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(0, 0, 256, 240) == true, "256x240 must match");

    // Extreme coordinate tests
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(INT_MIN, 0, 320, 240) == false, "INT_MIN x must be false");
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(0, INT_MIN, 320, 240) == false, "INT_MIN y must be false");
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(INT_MAX, 0, 320, 240) == false, "INT_MAX x must be false");
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(0, INT_MAX, 320, 240) == false, "INT_MAX y must be false");
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(0, 0, INT_MAX, 240) == false, "INT_MAX w must be false");
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(0, 0, 320, INT_MAX) == false, "INT_MAX h must be false");
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(0, 0, -1, -1) == false, "Negative dimensions must be false");
    TEST_ASSERT(re3_hd_ui_is_fullscreen_quad(0, 0, 0, 0) == false, "Zero dimensions must be false");
}

int main() {
    std::printf("====================================================================\n");
    std::printf(" Running Empirical Challenger M4-1 Stress & Adversarial Suite\n");
    std::printf("====================================================================\n");

    RUN_TEST(test_adversarial_path_traversal_fuzz);
    RUN_TEST(test_corrupted_png_fuzzing);
    RUN_TEST(test_exhaustive_health_condition_domain);
    RUN_TEST(test_epilogue_boundary_extremes);
    RUN_TEST(test_multithreaded_concurrency_stress);
    RUN_TEST(test_massive_cache_flooding);
    RUN_TEST(test_quad_geometry_extremes);

    std::printf("====================================================================\n");
    std::printf(" Challenger Results: %d executed, %d passed, %d failed\n",
                g_tests_run, g_tests_passed, g_tests_failed);
    std::printf("====================================================================\n");

    return g_tests_failed == 0 ? 0 : 1;
}
