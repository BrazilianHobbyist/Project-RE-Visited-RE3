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

#define CHAL_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        std::printf("  [FAIL] %s (Line %d: %s)\n", msg, __LINE__, #cond); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define CHAL_RUN_TEST(fn) do { \
    int last_failed = g_tests_failed; \
    g_tests_run++; \
    std::printf("[CHALLENGE TEST %d] %s...\n", g_tests_run, #fn); \
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

static fs::path create_sandbox(const std::string& prefix) {
    fs::path p = fs::temp_directory_path() / (prefix + "_" + std::to_string(std::rand()) + "_" + std::to_string(std::rand()));
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

static size_t get_process_memory_kb() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / 1024;
    }
#endif
    return 0;
}

static bool wait_for_current_cut(int timeout_ms = 500) {
    int elapsed = 0;
    while (elapsed < timeout_ms) {
        if (re3_hd_bg_has_hd_current()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        elapsed += 5;
    }
    return false;
}

// ============================================================================
// TEST 1: True LRU Recency Promotion Oracle
// Verifies that accessed entries are promoted to MRU, so older NON-accessed
// entries are evicted first.
// ============================================================================
void test_chal_lru_recency_promotion_oracle() {
    fs::path sb = create_sandbox("chal_lru_prom");
    fs::path bg_dir = sb / "mods" / "textures" / "PromoPack" / "backgrounds";

    // Create 40 cuts in Stage 1, Room 0: R100_00.png to R100_27.png (hex)
    for (int ct = 0; ct < 40; ++ct) {
        char fname[64];
        std::snprintf(fname, sizeof(fname), "R100_%02X.png", ct);
        write_png_file(bg_dir / fname, kValid1x1Png, sizeof(kValid1x1Png));
    }

    std::string pack_str = (sb / "mods" / "textures" / "PromoPack").generic_string();
    std::strncpy(g_re3_tex_config.pack, "PromoPack", sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.hd_backgrounds = true;

    SDL_Window* window = psx_sdl_create_window("LruPromoTest", 0, 0, 320, 240, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    SDL_GLContext gl_ctx = window ? SDL_GL_CreateContext(window) : nullptr;
    if (window && gl_ctx) psx_sdl_gl_make_current(window, gl_ctx);

    re3_textures_init(sb.generic_string().c_str());
    re3_hd_bg_init(pack_str.c_str());

    // 1. Load Cut 0 (initially at front of LRU)
    re3_hd_bg_on_room_change(1, 0, 0);
    CHAL_ASSERT(wait_for_current_cut(), "Cut 0 must decode");
    uint32_t tex0_initial = re3_hd_bg_get_current_gl_tex();
    CHAL_ASSERT(tex0_initial != 0, "Cut 0 initial GL upload must succeed");

    // 2. Load Cut 1
    re3_hd_bg_on_room_change(1, 0, 1);
    CHAL_ASSERT(wait_for_current_cut(), "Cut 1 must decode");
    uint32_t tex1 = re3_hd_bg_get_current_gl_tex();
    CHAL_ASSERT(tex1 != 0, "Cut 1 GL upload must succeed");

    // 3. Re-visit Cut 0 and query GL tex to trigger LRU promotion to MRU!
    // Now LRU order (MRU to LRU) should be: [Cut 0, Cut 1]
    re3_hd_bg_on_room_change(1, 0, 0);
    CHAL_ASSERT(wait_for_current_cut(), "Cut 0 must be ready");
    uint32_t tex0_promoted = re3_hd_bg_get_current_gl_tex();
    CHAL_ASSERT(tex0_promoted == tex0_initial, "Cut 0 should still be same GL texture");

    // 4. Load 31 more cuts: Cut 2 to Cut 32 (total 33 textures resident if none evicted)
    // Cache capacity is 32.
    // Uploading the 33rd texture triggers 1 eviction.
    // If LRU is correct: Cut 1 is least recently used, so Cut 1 is evicted! Cut 0 remains resident!
    // If LRU promotion failed (FIFO): Cut 0 would have been evicted!
    for (int ct = 2; ct <= 32; ++ct) {
        re3_hd_bg_on_room_change(1, 0, static_cast<uint8_t>(ct));
        CHAL_ASSERT(wait_for_current_cut(), "Sequential cut must decode");
        uint32_t tex = re3_hd_bg_get_current_gl_tex();
        CHAL_ASSERT(tex != 0, "Sequential cut GL upload must succeed");
    }

    re3_hd_bg_metrics_t m;
    re3_hd_bg_get_metrics(&m);
    CHAL_ASSERT(m.evictions == 1, "Exactly 1 eviction must have occurred at cut 32");

    // 5. Test oracle assertion:
    // Switch to Cut 0: it should STILL be in cache without needing worker decode!
    re3_hd_bg_on_room_change(1, 0, 0);
    // Since Cut 0 was promoted to MRU, it was NOT evicted!
    CHAL_ASSERT(re3_hd_bg_has_hd_current() == true, "Oracle: Cut 0 was promoted to MRU, so it MUST NOT be evicted!");
    uint32_t tex0_after = re3_hd_bg_get_current_gl_tex();
    CHAL_ASSERT(tex0_after == tex0_initial, "Oracle: Cut 0 GL texture ID must remain identical (cache hit)");

    // Switch to Cut 1: Cut 1 was the victim and was evicted!
    re3_hd_bg_on_room_change(1, 0, 1);
    // Immediately after on_room_change, Cut 1 is NOT ready in GL (it was evicted and needs decoding)
    // Wait for it to be re-acquired
    CHAL_ASSERT(wait_for_current_cut(), "Cut 1 must be re-acquired after its LRU eviction");
    uint32_t tex1_new = re3_hd_bg_get_current_gl_tex();
    CHAL_ASSERT(tex1_new != 0, "Cut 1 must obtain a new valid GL texture");

    re3_hd_bg_shutdown();
    re3_textures_shutdown();

    if (gl_ctx) SDL_GL_DestroyContext(gl_ctx);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();

    std::error_code ec;
    fs::remove_all(sb, ec);
}

// ============================================================================
// TEST 2: Multi-Cycle LRU Eviction & Re-Acquisition Thrashing (5 Full Cycles)
// ============================================================================
void test_chal_multicycle_thrashing_and_memory() {
    fs::path sb = create_sandbox("chal_thrash");
    fs::path bg_dir = sb / "mods" / "textures" / "ThrashPack" / "backgrounds";

    // 50 cuts
    for (int ct = 0; ct < 50; ++ct) {
        char fname[64];
        std::snprintf(fname, sizeof(fname), "R100_%02X.png", ct);
        write_png_file(bg_dir / fname, kValid1x1Png, sizeof(kValid1x1Png));
    }

    std::string pack_str = (sb / "mods" / "textures" / "ThrashPack").generic_string();
    std::strncpy(g_re3_tex_config.pack, "ThrashPack", sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.hd_backgrounds = true;

    SDL_Window* window = psx_sdl_create_window("ThrashTest", 0, 0, 320, 240, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    SDL_GLContext gl_ctx = window ? SDL_GL_CreateContext(window) : nullptr;
    if (window && gl_ctx) psx_sdl_gl_make_current(window, gl_ctx);

    re3_textures_init(sb.generic_string().c_str());
    re3_hd_bg_init(pack_str.c_str());

    // Warm up first 50 cuts
    for (int ct = 0; ct < 50; ++ct) {
        re3_hd_bg_on_room_change(1, 0, static_cast<uint8_t>(ct));
        CHAL_ASSERT(wait_for_current_cut(), "Cut must decode and be ready");
        uint32_t tex = re3_hd_bg_get_current_gl_tex();
        CHAL_ASSERT(tex != 0, "Uploaded texture must have valid GL ID");
    }

    size_t mem_post_warmup = get_process_memory_kb();

    // Now run 10 cycles (500 uploads)
    for (int cycle = 0; cycle < 10; ++cycle) {
        for (int ct = 0; ct < 50; ++ct) {
            re3_hd_bg_on_room_change(1, 0, static_cast<uint8_t>(ct));
            CHAL_ASSERT(wait_for_current_cut(), "Cut must decode and be ready");
            uint32_t tex = re3_hd_bg_get_current_gl_tex();
            CHAL_ASSERT(tex != 0, "Uploaded texture must have valid GL ID");
        }
        size_t mem_cycle = get_process_memory_kb();
        std::printf("      Cycle %d/10: Memory = %zu KB (from warmup: %+lld KB)\n",
                    cycle + 1, mem_cycle, (long long)mem_cycle - (long long)mem_post_warmup);
    }

    re3_hd_bg_metrics_t m;
    re3_hd_bg_get_metrics(&m);
    std::printf("    Thrashing metrics: prefetches=%llu, evictions=%llu, active_entries=%u\n",
                (unsigned long long)m.prefetches,
                (unsigned long long)m.evictions,
                m.active_entries);

    size_t mem_end = get_process_memory_kb();
    long long post_warmup_diff = (long long)mem_end - (long long)mem_post_warmup;
    std::printf("    Post-warmup Diff over 500 uploads: %+lld KB\n", post_warmup_diff);
    CHAL_ASSERT(post_warmup_diff < 4096, "Memory must remain strictly bounded across 500 thrashing uploads");

    re3_hd_bg_shutdown();
    re3_textures_shutdown();

    if (gl_ctx) SDL_GL_DestroyContext(gl_ctx);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();

    std::error_code ec;
    fs::remove_all(sb, ec);
}

// ============================================================================
// TEST 3: Key 0 (Stage 0, Room 0, Cut 0) Eviction & Re-Acquisition
// ============================================================================
void test_chal_key0_eviction_and_reacquisition() {
    fs::path sb = create_sandbox("chal_key0");
    fs::path bg_dir = sb / "mods" / "textures" / "Key0Pack" / "backgrounds";

    // Create R000_00.png (key 0) and 35 other files R100_00..34
    write_png_file(bg_dir / "R000_00.png", kValid1x1Png, sizeof(kValid1x1Png));
    for (int ct = 0; ct < 35; ++ct) {
        char fname[64];
        std::snprintf(fname, sizeof(fname), "R100_%02X.png", ct);
        write_png_file(bg_dir / fname, kValid1x1Png, sizeof(kValid1x1Png));
    }

    std::string pack_str = (sb / "mods" / "textures" / "Key0Pack").generic_string();
    std::strncpy(g_re3_tex_config.pack, "Key0Pack", sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.hd_backgrounds = true;

    SDL_Window* window = psx_sdl_create_window("Key0Test", 0, 0, 320, 240, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    SDL_GLContext gl_ctx = window ? SDL_GL_CreateContext(window) : nullptr;
    if (window && gl_ctx) psx_sdl_gl_make_current(window, gl_ctx);

    re3_textures_init(sb.generic_string().c_str());
    re3_hd_bg_init(pack_str.c_str());

    // 1. Load Stage 0, Room 0, Cut 0 (Key = 0)
    re3_hd_bg_on_room_change(0, 0, 0);
    CHAL_ASSERT(wait_for_current_cut(), "Key 0 (0,0,0) must decode initially");
    uint32_t tex0 = re3_hd_bg_get_current_gl_tex();
    CHAL_ASSERT(tex0 != 0, "Key 0 must upload to non-zero GL texture");

    // 2. Evict Key 0 by loading 33 textures
    for (int ct = 0; ct < 33; ++ct) {
        re3_hd_bg_on_room_change(1, 0, static_cast<uint8_t>(ct));
        CHAL_ASSERT(wait_for_current_cut(), "Interim cut must decode");
        uint32_t tex = re3_hd_bg_get_current_gl_tex();
        CHAL_ASSERT(tex != 0, "Interim cut GL tex must be non-zero");
    }

    re3_hd_bg_metrics_t m;
    re3_hd_bg_get_metrics(&m);
    CHAL_ASSERT(m.evictions >= 1, "At least 1 eviction must have occurred");

    // 3. Return to Stage 0, Room 0, Cut 0 (Key 0)
    re3_hd_bg_on_room_change(0, 0, 0);
    CHAL_ASSERT(wait_for_current_cut(), "Key 0 (0,0,0) must successfully re-acquire after eviction!");
    uint32_t tex0_re = re3_hd_bg_get_current_gl_tex();
    CHAL_ASSERT(tex0_re != 0, "Key 0 re-acquisition must yield valid GL texture");

    re3_hd_bg_shutdown();
    re3_textures_shutdown();

    if (gl_ctx) SDL_GL_DestroyContext(gl_ctx);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();

    std::error_code ec;
    fs::remove_all(sb, ec);
}

// ============================================================================
// TEST 4: Extreme Concurrent Room Flooding vs Active GL Render
// 10 threads flooding room changes while main thread queries & uploads GL textures
// ============================================================================
void test_chal_extreme_flood_concurrency() {
    fs::path sb = create_sandbox("chal_flood");
    fs::path bg_dir = sb / "mods" / "textures" / "FloodPack" / "backgrounds";

    for (int st = 1; st <= 2; ++st) {
        for (int rm = 0; rm < 10; ++rm) {
            for (int ct = 0; ct < 4; ++ct) {
                char fname[64];
                std::snprintf(fname, sizeof(fname), "R%1X%02X_%02X.png", st, rm, ct);
                write_png_file(bg_dir / fname, kValid1x1Png, sizeof(kValid1x1Png));
            }
        }
    }

    std::string pack_str = (sb / "mods" / "textures" / "FloodPack").generic_string();
    std::strncpy(g_re3_tex_config.pack, "FloodPack", sizeof(g_re3_tex_config.pack) - 1);
    g_re3_tex_config.hd_backgrounds = true;

    SDL_Window* window = psx_sdl_create_window("FloodTest", 0, 0, 320, 240, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
    SDL_GLContext gl_ctx = window ? SDL_GL_CreateContext(window) : nullptr;
    if (window && gl_ctx) psx_sdl_gl_make_current(window, gl_ctx);

    re3_textures_init(sb.generic_string().c_str());
    re3_hd_bg_init(pack_str.c_str());

    std::atomic<bool> run_flag{true};
    std::atomic<int> flood_operations{0};

    // 8 flood threads
    std::vector<std::thread> workers;
    for (int t = 0; t < 8; ++t) {
        workers.emplace_back([&run_flag, &flood_operations, t]() {
            uint8_t st = 1 + (t % 2);
            uint8_t rm = 0;
            uint8_t ct = 0;
            while (run_flag.load(std::memory_order_relaxed)) {
                rm = (rm + 1) % 10;
                ct = (ct + 1) % 4;
                re3_hd_bg_on_room_change(st, rm, ct);
                re3_hd_bg_has_hd_current();
                flood_operations.fetch_add(1, std::memory_order_relaxed);
                std::this_thread::yield();
            }
        });
    }

    // Main thread queries GL textures and renders for 500 ms
    auto start_time = std::chrono::steady_clock::now();
    int render_queries = 0;
    while (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_time).count() < 500) {
        uint32_t tex = re3_hd_bg_get_current_gl_tex();
        (void)tex;
        render_queries++;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    run_flag.store(false, std::memory_order_relaxed);
    for (auto& w : workers) {
        w.join();
    }

    std::printf("    Completed %d flood room changes and %d GL render queries concurrently without crash/deadlock\n",
                flood_operations.load(), render_queries);

    re3_hd_bg_shutdown();
    re3_textures_shutdown();

    if (gl_ctx) SDL_GL_DestroyContext(gl_ctx);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();

    std::error_code ec;
    fs::remove_all(sb, ec);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    std::printf("============================================================\n");
    std::printf(" RE3Recomp M2 R2: Independent Challenger Verification Suite \n");
    std::printf("============================================================\n");

    CHAL_RUN_TEST(test_chal_lru_recency_promotion_oracle);
    CHAL_RUN_TEST(test_chal_multicycle_thrashing_and_memory);
    CHAL_RUN_TEST(test_chal_key0_eviction_and_reacquisition);
    CHAL_RUN_TEST(test_chal_extreme_flood_concurrency);

    std::printf("============================================================\n");
    std::printf(" Challenger Results: %d executed, %d passed, %d failed\n",
                g_tests_run, g_tests_passed, g_tests_failed);
    std::printf("============================================================\n");

    return (g_tests_failed == 0) ? 0 : 1;
}
