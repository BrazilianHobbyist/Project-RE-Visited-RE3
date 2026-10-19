#include "re3_hd_backgrounds.h"
#include "re3_textures.h"
#include "psx_sdl.h"

#if defined(PSX_SDL3)
#include <SDL3/SDL_opengl.h>
#else
#include <SDL_opengl.h>
#endif

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
#endif

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
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

static fs::path create_sandbox() {
    fs::path p = fs::temp_directory_path() / ("re3_m2_stress_" + std::to_string(std::rand()) + "_" + std::to_string(std::rand()));
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

static size_t get_process_memory_bytes() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
#endif
    return 0;
}

// -------------------------------------------------------------
// 1. High-Frequency Concurrent Room and Cut Changes
// -------------------------------------------------------------
void test_stress_concurrent_room_changes() {
    fs::path sb = create_sandbox();
    fs::path bg_dir = sb / "mods" / "textures" / "StressPack" / "backgrounds";

    // Create 30 rooms across 2 stages with 4 cuts each (240 files)
    for (int st = 1; st <= 2; ++st) {
        for (int rm = 0; rm < 15; ++rm) {
            for (int ct = 0; ct < 4; ++ct) {
                char fname[64];
                std::snprintf(fname, sizeof(fname), "R%1X%02X_%02X.png", st, rm, ct);
                write_png_file(bg_dir / fname, kValid1x1Png, sizeof(kValid1x1Png));
            }
        }
    }

    std::string pack_str = (sb / "mods" / "textures" / "StressPack").generic_string();
    std::strncpy(g_re3_tex_config.pack, "StressPack", sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.hd_backgrounds = true;

    re3_textures_init(sb.generic_string().c_str());
    re3_hd_bg_init(pack_str.c_str());

    constexpr int kNumThreads = 10;
    constexpr int kChangesPerThread = 100;
    std::vector<std::thread> threads;
    std::atomic<bool> start_signal{false};
    std::atomic<int> completed{0};

    for (int t = 0; t < kNumThreads; ++t) {
        threads.emplace_back([t, &start_signal, &completed]() {
            while (!start_signal.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            std::mt19937 rng(1337 + t);
            std::uniform_int_distribution<int> dist_stage(1, 2);
            std::uniform_int_distribution<int> dist_room(0, 14);
            std::uniform_int_distribution<int> dist_cut(0, 5); // 4 and 5 are missing cuts

            for (int i = 0; i < kChangesPerThread; ++i) {
                uint8_t st = static_cast<uint8_t>(dist_stage(rng));
                uint8_t rm = static_cast<uint8_t>(dist_room(rng));
                uint8_t ct = static_cast<uint8_t>(dist_cut(rng));
                re3_hd_bg_on_room_change(st, rm, ct);
            }
            completed.fetch_add(1, std::memory_order_release);
        });
    }

    // Release all 10 threads simultaneously
    start_signal.store(true, std::memory_order_release);

    // Concurrently poll metrics and queries from main thread
    int main_queries = 0;
    while (completed.load(std::memory_order_acquire) < kNumThreads) {
        re3_hd_bg_has_hd_current();
        re3_hd_bg_metrics_t m;
        re3_hd_bg_get_metrics(&m);
        main_queries++;
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }

    for (auto& th : threads) {
        th.join();
    }

    re3_hd_bg_metrics_t final_metrics;
    re3_hd_bg_get_metrics(&final_metrics);
    std::printf("    Completed 1000 rapid concurrent room changes across 10 threads!\n");
    std::printf("    Metrics: hits=%llu, misses=%llu, prefetches=%llu, evictions=%llu, entries=%u\n",
                (unsigned long long)final_metrics.hits,
                (unsigned long long)final_metrics.misses,
                (unsigned long long)final_metrics.prefetches,
                (unsigned long long)final_metrics.evictions,
                final_metrics.active_entries);

    TEST_ASSERT(final_metrics.hits + final_metrics.misses >= 1000, "Must record at least 1000 room queries");

    re3_hd_bg_shutdown();
    re3_textures_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 2. Worker Thread Decoding vs Main Thread Query/Upload Race
// -------------------------------------------------------------
void test_stress_worker_decode_vs_query_race() {
    fs::path sb = create_sandbox();
    fs::path bg_dir = sb / "mods" / "textures" / "RacePack" / "backgrounds";

    // Create 10 cuts in room 0
    for (int ct = 0; ct < 10; ++ct) {
        char fname[64];
        std::snprintf(fname, sizeof(fname), "R100_%02X.png", ct);
        write_png_file(bg_dir / fname, kValid1x1Png, sizeof(kValid1x1Png));
    }

    std::string pack_str = (sb / "mods" / "textures" / "RacePack").generic_string();
    std::strncpy(g_re3_tex_config.pack, "RacePack", sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.hd_backgrounds = true;

    re3_textures_init(sb.generic_string().c_str());
    re3_hd_bg_init(pack_str.c_str());

    // Rapidly alternate cuts and query in tight loop while worker thread decodes in background
    for (int iter = 0; iter < 50; ++iter) {
        uint8_t target_cut = static_cast<uint8_t>(iter % 10);
        re3_hd_bg_on_room_change(1, 0, target_cut);

        // Immediate queries while worker might be mid-decode
        bool ready_now = re3_hd_bg_has_hd_current();
        (void)ready_now;
        uint32_t tex = re3_hd_bg_get_current_gl_tex();
        (void)tex;
        bool rendered = re3_hd_bg_render_current(0, 320, 240);
        (void)rendered;

        float uv[4];
        re3_hd_bg_get_screen_uv(0, 0, 320, 240, uv);
    }

    // Wait 200ms for worker queue to settle
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Verify current state is valid
    re3_hd_bg_on_room_change(1, 0, 0);
    // Give brief time for cut 0
    for (int i = 0; i < 30; ++i) {
        if (re3_hd_bg_has_hd_current()) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    TEST_ASSERT(re3_hd_bg_has_hd_current() == true, "Cut 0 must be ready after settling");

    re3_hd_bg_shutdown();
    re3_textures_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 3. LRU Cache Eviction Boundary & Texture Re-acquisition
// -------------------------------------------------------------
void test_stress_lru_eviction_boundary_and_reacquisition() {
    fs::path sb = create_sandbox();
    fs::path bg_dir = sb / "mods" / "textures" / "LruPack" / "backgrounds";

    // Create 40 distinct cuts: 0x00 to 0x27 (40 total)
    for (int ct = 0; ct < 40; ++ct) {
        char fname[64];
        std::snprintf(fname, sizeof(fname), "R100_%02X.png", ct);
        write_png_file(bg_dir / fname, kValid1x1Png, sizeof(kValid1x1Png));
    }

    // Initialize SDL3 with a real OpenGL context for GL operations
    if (psx_sdl_init(SDL_INIT_VIDEO) != 0) {
        std::printf("    Warning: SDL_Init failed, skipping GL context creation\n");
    }
    SDL_Window* window = psx_sdl_create_window("LruTest", 0, 0, 320, 240, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    SDL_GLContext gl_ctx = window ? SDL_GL_CreateContext(window) : nullptr;
    if (window && gl_ctx) {
        psx_sdl_gl_make_current(window, gl_ctx);
    }

    std::string pack_str = (sb / "mods" / "textures" / "LruPack").generic_string();
    std::strncpy(g_re3_tex_config.pack, "LruPack", sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.hd_backgrounds = true;

    re3_textures_init(sb.generic_string().c_str());
    re3_hd_bg_init(pack_str.c_str());

    // Step 1: Load Cut 0, wait until ready, upload to GL
    re3_hd_bg_on_room_change(1, 0, 0);
    bool cut0_ready = false;
    for (int i = 0; i < 50; ++i) {
        if (re3_hd_bg_has_hd_current()) {
            cut0_ready = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    TEST_ASSERT(cut0_ready, "Cut 0 must decode to ReadyRAM");

    uint32_t tex0 = re3_hd_bg_get_current_gl_tex();
    TEST_ASSERT(tex0 != 0, "Cut 0 must upload to a non-zero GL texture");
    std::printf("    Cut 0 successfully uploaded with GL tex ID %u\n", tex0);

    // Step 2: Now load Cuts 1 to 32 (total 33 textures resident).
    // Capacity is 32 entries (kMaxCachedTextures = 32).
    // Cut 0 was the least recently used, so uploading the 33rd texture MUST evict Cut 0!
    for (int ct = 1; ct <= 32; ++ct) {
        re3_hd_bg_on_room_change(1, 0, static_cast<uint8_t>(ct));
        for (int i = 0; i < 50; ++i) {
            if (re3_hd_bg_has_hd_current()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        uint32_t tex = re3_hd_bg_get_current_gl_tex();
        TEST_ASSERT(tex != 0, "Sequential cut must upload to GL");
    }

    re3_hd_bg_metrics_t metrics_after_33;
    re3_hd_bg_get_metrics(&metrics_after_33);
    std::printf("    After 33 texture uploads: evictions=%llu\n", (unsigned long long)metrics_after_33.evictions);
    TEST_ASSERT(metrics_after_33.evictions >= 1, "Cache must have evicted at least 1 texture after exceeding 32 entries");

    // Verify active texture (Cut 32) is still valid and not evicted
    uint32_t active_tex = re3_hd_bg_get_current_gl_tex();
    TEST_ASSERT(active_tex != 0, "Active cut 32 must NOT be evicted");

    // Step 3: ADVERSARIAL CRITICAL TEST — Return to Cut 0!
    // Cut 0 was evicted earlier.
    // When the player switches back to Cut 0, the engine MUST re-decode Cut 0 and make it available again!
    std::printf("    Switching back to evicted Cut 0...\n");
    re3_hd_bg_on_room_change(1, 0, 0);

    bool cut0_reacquired = false;
    for (int i = 0; i < 50; ++i) {
        if (re3_hd_bg_has_hd_current()) {
            cut0_reacquired = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::printf("    Re-acquisition check: has_hd_current() = %d\n", cut0_reacquired);
    uint32_t tex0_new = re3_hd_bg_get_current_gl_tex();
    std::printf("    Re-acquisition check: get_current_gl_tex() = %u\n", tex0_new);

    // If evicted entries cannot be reloaded, cut0_reacquired will be false and tex0_new will be 0!
    TEST_ASSERT(cut0_reacquired == true, "CRITICAL: Evicted Cut 0 MUST be reloadable upon re-visiting!");
    TEST_ASSERT(tex0_new != 0, "CRITICAL: Evicted Cut 0 MUST obtain a valid GL texture on re-acquisition!");

    re3_hd_bg_shutdown();
    re3_textures_shutdown();

    if (gl_ctx) SDL_GL_DestroyContext(gl_ctx);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();

    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 4. Rapid Multiple Init / Shutdown Cycles
// -------------------------------------------------------------
void test_stress_rapid_init_shutdown_cycles() {
    fs::path sb = create_sandbox();
    fs::path bg_dir = sb / "mods" / "textures" / "CyclePack" / "backgrounds";
    write_png_file(bg_dir / "R100_00.png", kValid1x1Png, sizeof(kValid1x1Png));
    write_png_file(bg_dir / "R100_01.png", kValid1x1Png, sizeof(kValid1x1Png));

    std::string pack_str = (sb / "mods" / "textures" / "CyclePack").generic_string();
    std::strncpy(g_re3_tex_config.pack, "CyclePack", sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.hd_backgrounds = true;
    re3_textures_init(sb.generic_string().c_str());

    constexpr int kCycles = 200;
    for (int i = 0; i < kCycles; ++i) {
        re3_hd_bg_init(pack_str.c_str());

        // Queue cut work immediately
        re3_hd_bg_on_room_change(1, 0, (i % 2));

        // Rapidly shutdown without waiting
        re3_hd_bg_shutdown();
    }
    std::printf("    Successfully executed %d rapid init/shutdown cycles with zero deadlocks!\n", kCycles);

    re3_textures_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// -------------------------------------------------------------
// 5. Memory Boundedness Under Continuous Stream of 1000 Cuts
// -------------------------------------------------------------
void test_stress_memory_boundedness() {
    fs::path sb = create_sandbox();
    fs::path bg_dir = sb / "mods" / "textures" / "MemPack" / "backgrounds";

    for (int ct = 0; ct < 35; ++ct) {
        char fname[64];
        std::snprintf(fname, sizeof(fname), "R100_%02X.png", ct);
        write_png_file(bg_dir / fname, kValid1x1Png, sizeof(kValid1x1Png));
    }

    std::string pack_str = (sb / "mods" / "textures" / "MemPack").generic_string();
    std::strncpy(g_re3_tex_config.pack, "MemPack", sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.hd_backgrounds = true;

    re3_textures_init(sb.generic_string().c_str());
    re3_hd_bg_init(pack_str.c_str());

    size_t mem_start = get_process_memory_bytes();

    for (int iter = 0; iter < 1000; ++iter) {
        uint8_t ct = static_cast<uint8_t>(iter % 35);
        re3_hd_bg_on_room_change(1, 0, ct);
        if (iter % 10 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    // Allow worker to finish processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    size_t mem_end = get_process_memory_bytes();
    long long diff_kb = (mem_end > mem_start) ? static_cast<long long>(mem_end - mem_start) / 1024 : 0;
    std::printf("    Memory Start: %zu KB, End: %zu KB (Diff: +%lld KB)\n",
                mem_start / 1024, mem_end / 1024, diff_kb);

    // RAM growth should be small (< 50MB for metadata/textures)
    TEST_ASSERT(diff_kb < 50 * 1024, "Process working set growth must be strictly bounded (< 50MB)");

    re3_hd_bg_shutdown();
    re3_textures_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

int main(int argc, char* argv[]) {
    std::printf("============================================================\n");
    std::printf(" RE3Recomp M2: HD Backgrounds Concurrency & Stress Harness  \n");
    std::printf("============================================================\n");

    RUN_TEST(test_stress_rapid_init_shutdown_cycles);
    RUN_TEST(test_stress_concurrent_room_changes);
    RUN_TEST(test_stress_worker_decode_vs_query_race);
    RUN_TEST(test_stress_lru_eviction_boundary_and_reacquisition);
    RUN_TEST(test_stress_memory_boundedness);

    std::printf("============================================================\n");
    std::printf(" Stress Results: %d executed, %d passed, %d failed\n",
                g_tests_run, g_tests_passed, g_tests_failed);
    std::printf("============================================================\n");

    return (g_tests_failed == 0) ? 0 : 1;
}
