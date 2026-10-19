#include "re3_hd_textures.h"
#include "re3_textures.h"
#include "psx_sdl.h"
#include "xxhash.h"

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
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <random>
#include <algorithm>

namespace fs = std::filesystem;

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define STRESS_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        std::printf("  [FAIL] %s (Line %d: %s)\n", msg, __LINE__, #cond); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define RUN_STRESS_TEST(fn) do { \
    int last_failed = g_tests_failed; \
    g_tests_run++; \
    std::printf("[STRESS TEST %d] %s...\n", g_tests_run, #fn); \
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

// Valid 2x2 RGBA PNG
static const unsigned char kValid2x2Png[] = {
    137, 80, 78, 71, 13, 10, 26, 10, 0, 0, 0, 13, 73, 72, 68, 82,
    0, 0, 0, 2, 0, 0, 0, 2, 8, 6, 0, 0, 0, 114, 182, 13, 36,
    0, 0, 0, 17, 73, 68, 65, 84, 120, 156, 99, 248, 207, 192, 240, 31,
    132, 25, 96, 12, 0, 71, 202, 7, 249, 103, 89, 110, 183,
    0, 0, 0, 0, 73, 69, 78, 68, 174, 66, 96, 130
};

static fs::path create_stress_sandbox(const std::string& prefix = "re3_m3_stress_") {
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

static size_t get_process_memory_bytes() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
#endif
    return 0;
}

static SDL_Window* g_gl_window = nullptr;
static SDL_GLContext g_gl_ctx = nullptr;

static void ensure_gl_context() {
    if (!g_gl_ctx) {
        g_gl_window = psx_sdl_create_window("ChallengerM3_2", 0, 0, 320, 240, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        if (g_gl_window) {
            g_gl_ctx = SDL_GL_CreateContext(g_gl_window);
            if (g_gl_ctx) {
                psx_sdl_gl_make_current(g_gl_window, g_gl_ctx);
            }
        }
    }
}

// ---------------------------------------------------------------------
// 1. High-Throughput Cache Querying: 100,000 rapid calls to bind_replacement
// ---------------------------------------------------------------------
void test_high_throughput_cache_querying() {
    ensure_gl_context();
    fs::path sb = create_stress_sandbox("re3_m3_query_");
    fs::path rep_dir = sb / "mods" / "textures" / "QueryPack" / "replacements";

    // Setup 10 valid texture files with distinct 64-bit hashes
    std::vector<uint64_t> valid_hashes;
    for (int i = 0; i < 10; ++i) {
        uint64_t h = 0x2A8D3A5FC58A1180ULL + i * 0x1000ULL;
        valid_hashes.push_back(h);
        char fname[64];
        std::snprintf(fname, sizeof(fname), "%016llx.png", (unsigned long long)h);
        write_binary_file(rep_dir / fname, kValid1x1Png, sizeof(kValid1x1Png));
    }

    // 10 missing hashes
    std::vector<uint64_t> missing_hashes;
    for (int i = 0; i < 10; ++i) {
        uint64_t h = 0x9999000011110000ULL + i * 0x1000ULL;
        missing_hashes.push_back(h);
    }

    g_re3_tex_config.hd_models = true;
    std::string pack_str = (sb / "mods" / "textures" / "QueryPack").generic_string();
    re3_hd_tex_init(pack_str.c_str());

    STRESS_ASSERT(re3_hd_tex_is_active(), "HD texture subsystem must be active");

    // Prime the positive cache
    for (uint64_t h : valid_hashes) {
        uint32_t tex = re3_hd_tex_bind_replacement(h);
        STRESS_ASSERT(tex != 0, "Valid texture must load and return non-zero GL texture ID");
    }

    // Prime the negative cache
    for (uint64_t h : missing_hashes) {
        uint32_t tex = re3_hd_tex_bind_replacement(h);
        STRESS_ASSERT(tex == 0, "Missing texture must return 0");
    }

    re3_hd_tex_metrics_t m_prime;
    re3_hd_tex_get_metrics(&m_prime);
    STRESS_ASSERT(m_prime.disk_loads == 10, "Disk loads must equal 10 after priming");
    STRESS_ASSERT(m_prime.active_textures == 10, "Active textures must equal 10");

    // Execute 100,000 rapid queries (50,000 positive, 50,000 negative)
    const int kIterations = 100000;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < kIterations; ++i) {
        if (i % 2 == 0) {
            uint64_t h = valid_hashes[(i / 2) % valid_hashes.size()];
            uint32_t tex = re3_hd_tex_bind_replacement(h);
            if (tex == 0) {
                STRESS_ASSERT(false, "Positive cache hit must never return 0");
            }
        } else {
            uint64_t h = missing_hashes[(i / 2) % missing_hashes.size()];
            uint32_t tex = re3_hd_tex_bind_replacement(h);
            if (tex != 0) {
                STRESS_ASSERT(false, "Negative cache hit must return 0");
            }
        }
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    double avg_ns = static_cast<double>(total_ns) / kIterations;
    double total_ms = static_cast<double>(total_ns) / 1000000.0;

    std::printf("    [High-Throughput] 100,000 queries completed in %.2f ms (Average: %.2f ns/query)\n",
                total_ms, avg_ns);

    re3_hd_tex_metrics_t m_final;
    re3_hd_tex_get_metrics(&m_final);

    STRESS_ASSERT(m_final.cache_hits == 50000, "Cache hits must be exactly 50,000");
    // 10 initial misses + 50,000 negative cache hits
    STRESS_ASSERT(m_final.cache_misses == 50010, "Cache misses must be exactly 50,010");
    STRESS_ASSERT(m_final.disk_loads == 10, "Disk loads must remain strictly 10 (zero filesystem touches during benchmark)");

    // Average latency per lookup must be sub-microsecond (< 1000 ns)
    STRESS_ASSERT(avg_ns < 1000.0, "Cache lookup average latency must be < 1.0 microsecond");

    re3_hd_tex_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// ---------------------------------------------------------------------
// 2. Negative Cache Effectiveness: Ensure zero disk hits on repeated queries
// ---------------------------------------------------------------------
void test_negative_cache_effectiveness() {
    ensure_gl_context();
    fs::path sb = create_stress_sandbox("re3_m3_neg_");
    fs::path rep_dir = sb / "mods" / "textures" / "NegPack" / "replacements";
    fs::create_directories(rep_dir);

    g_re3_tex_config.hd_models = true;
    std::string pack_str = (sb / "mods" / "textures" / "NegPack").generic_string();
    re3_hd_tex_init(pack_str.c_str());

    uint64_t missing_hash = 0xDEADBEEFCAFE0001ULL;

    // First call: misses and checks disk
    uint32_t res1 = re3_hd_tex_bind_replacement(missing_hash);
    STRESS_ASSERT(res1 == 0, "Missing hash must return 0");

    re3_hd_tex_metrics_t m1;
    re3_hd_tex_get_metrics(&m1);
    STRESS_ASSERT(m1.cache_misses == 1, "Cache misses should be 1 after first query");
    STRESS_ASSERT(m1.disk_loads == 0, "Disk loads should be 0");

    // Perform 20,000 queries to the same missing hash
    const int kNegCount = 20000;
    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kNegCount; ++i) {
        uint32_t r = re3_hd_tex_bind_replacement(missing_hash);
        if (r != 0) {
            STRESS_ASSERT(false, "Negative cache lookup must return 0");
        }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    auto neg_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
    double avg_neg_ns = static_cast<double>(neg_ns) / kNegCount;

    std::printf("    [Negative Cache] 20,000 missing queries took %.2f ms (%.2f ns/query)\n",
                static_cast<double>(neg_ns) / 1000000.0, avg_neg_ns);

    re3_hd_tex_metrics_t m2;
    re3_hd_tex_get_metrics(&m2);
    STRESS_ASSERT(m2.cache_misses == 1 + kNegCount, "Cache misses must reflect exact negative lookups");
    STRESS_ASSERT(m2.disk_loads == 0, "Disk loads must remain 0 (no filesystem touches)");
    STRESS_ASSERT(avg_neg_ns < 500.0, "Negative cache lookup must be < 500 ns (pure in-memory O(1) set lookup)");

    // Test a wide distribution of 500 distinct missing hashes
    for (int i = 0; i < 500; ++i) {
        uint64_t h = 0xBAADF00D00000000ULL + i;
        uint32_t r = re3_hd_tex_bind_replacement(h);
        STRESS_ASSERT(r == 0, "Missing hash must return 0");
    }

    re3_hd_tex_metrics_t m3;
    re3_hd_tex_get_metrics(&m3);
    STRESS_ASSERT(m3.cache_misses == 1 + kNegCount + 500, "All 500 hashes must be registered in negative cache");

    // Re-query all 500 hashes: should all hit negative cache instantly
    for (int i = 0; i < 500; ++i) {
        uint64_t h = 0xBAADF00D00000000ULL + i;
        uint32_t r = re3_hd_tex_bind_replacement(h);
        STRESS_ASSERT(r == 0, "Negative cache hit must return 0");
    }

    re3_hd_tex_metrics_t m4;
    re3_hd_tex_get_metrics(&m4);
    STRESS_ASSERT(m4.cache_misses == 1 + kNegCount + 1000, "Negative cache misses increment on hits");
    STRESS_ASSERT(m4.disk_loads == 0, "Disk loads strictly 0");

    re3_hd_tex_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// ---------------------------------------------------------------------
// 3. Candidate Pattern Resolution Matrix
// ---------------------------------------------------------------------
void test_candidate_pattern_resolution_matrix() {
    ensure_gl_context();

    // We test two categories of hashes:
    // Hash A: full 16-hex digit hash (e.g. 0x2A8D3A5FC58A1180ULL)
    // Hash B: hash with leading zeros (e.g. 0x0000ABCD12345678ULL) -> unpadded has fewer characters!
    uint64_t hash_a = 0x2A8D3A5FC58A1180ULL;
    uint64_t hash_b = 0x0000ABCD12345678ULL;

    struct PatternTestCase {
        uint64_t hash;
        std::string filename;
        std::string description;
    };

    std::vector<PatternTestCase> test_cases = {
        // Hash A patterns
        { hash_a, "2a8d3a5fc58a1180.png", "Hash A: 16-hex padded lowercase .png" },
        { hash_a, "2A8D3A5FC58A1180.png", "Hash A: 16-hex padded uppercase .png" },
        { hash_a, "2a8d3a5fc58a1180.PNG", "Hash A: 16-hex padded lowercase .PNG" },
        { hash_a, "2A8D3A5FC58A1180.PNG", "Hash A: 16-hex padded uppercase .PNG" },

        // Hash B patterns (leading zeros: padded vs unpadded)
        { hash_b, "0000abcd12345678.png", "Hash B: 16-hex zero-padded lowercase .png" },
        { hash_b, "0000ABCD12345678.png", "Hash B: 16-hex zero-padded uppercase .png" },
        { hash_b, "0000abcd12345678.PNG", "Hash B: 16-hex zero-padded lowercase .PNG" },
        { hash_b, "0000ABCD12345678.PNG", "Hash B: 16-hex zero-padded uppercase .PNG" },
        { hash_b, "abcd12345678.png",     "Hash B: unpadded lowercase .png" },
        { hash_b, "ABCD12345678.png",     "Hash B: unpadded uppercase .png" },
        { hash_b, "abcd12345678.PNG",     "Hash B: unpadded lowercase .PNG" },
        { hash_b, "ABCD12345678.PNG",     "Hash B: unpadded uppercase .PNG" },
    };

    for (size_t idx = 0; idx < test_cases.size(); ++idx) {
        const auto& tc = test_cases[idx];
        fs::path sb = create_stress_sandbox("re3_m3_pat_");
        fs::path rep_dir = sb / "mods" / "textures" / "PatternPack" / "replacements";
        fs::create_directories(rep_dir);

        // Write ONLY this specific file
        write_binary_file(rep_dir / tc.filename, kValid1x1Png, sizeof(kValid1x1Png));

        g_re3_tex_config.hd_models = true;
        std::string pack_str = (sb / "mods" / "textures" / "PatternPack").generic_string();
        re3_hd_tex_init(pack_str.c_str());

        uint32_t tex_id = re3_hd_tex_bind_replacement(tc.hash);
        std::printf("    [Pattern %zu/12] %-48s -> GL Tex ID: %u\n",
                    idx + 1, tc.description.c_str(), tex_id);

        STRESS_ASSERT(tex_id != 0, "Pattern must resolve to a valid non-zero OpenGL texture handle");

        re3_hd_tex_metrics_t m;
        re3_hd_tex_get_metrics(&m);
        STRESS_ASSERT(m.disk_loads == 1, "Disk loads must equal 1 on pattern match");

        // Subsequent query must hit positive cache
        uint32_t tex_id2 = re3_hd_tex_bind_replacement(tc.hash);
        STRESS_ASSERT(tex_id2 == tex_id, "Subsequent query must return identical GL texture handle");

        re3_hd_tex_shutdown();
        std::error_code ec;
        fs::remove_all(sb, ec);
    }

    // Adversarial: directory evasion test
    // Create a DIRECTORY with candidate pattern name instead of a regular file
    {
        fs::path sb = create_stress_sandbox("re3_m3_patdir_");
        fs::path rep_dir = sb / "mods" / "textures" / "DirPack" / "replacements";
        fs::path fake_dir = rep_dir / "2a8d3a5fc58a1180.png";
        fs::create_directories(fake_dir); // create as directory!

        g_re3_tex_config.hd_models = true;
        std::string pack_str = (sb / "mods" / "textures" / "DirPack").generic_string();
        re3_hd_tex_init(pack_str.c_str());

        uint32_t tex_id = re3_hd_tex_bind_replacement(hash_a);
        STRESS_ASSERT(tex_id == 0, "Candidate matching a directory must be rejected and return 0");

        re3_hd_tex_shutdown();
        std::error_code ec;
        fs::remove_all(sb, ec);
    }
}

// ---------------------------------------------------------------------
// 4. Malformed and Corrupted PNG Resilience
// ---------------------------------------------------------------------
void test_corrupted_png_resilience() {
    ensure_gl_context();

    struct CorruptedFixture {
        std::string name;
        std::vector<unsigned char> data;
        std::string description;
    };

    // Construct various adversarial payloads
    std::vector<CorruptedFixture> fixtures;

    // 1. Zero byte file
    fixtures.push_back({ "zero_byte.png", {}, "0-byte completely empty file" });

    // 2. 1 byte file
    fixtures.push_back({ "one_byte.png", { 0x89 }, "1-byte truncated file (0x89)" });

    // 3. 4 bytes
    fixtures.push_back({ "four_bytes.png", { 0x89, 'P', 'N', 'G' }, "4-byte partial magic" });

    // 4. 7 bytes
    fixtures.push_back({ "seven_bytes.png", { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A }, "7-byte truncated magic" });

    // 5. 8 bytes exact PNG magic header only
    fixtures.push_back({ "magic_only.png", { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A }, "8-byte PNG signature only without chunks" });

    // 6. 16 bytes: Valid magic + truncated IHDR chunk length
    {
        std::vector<unsigned char> d = {
            0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A,
            0x00, 0x00, 0x00, 0x0D, 'I', 'H', 'D', 'R'
        };
        fixtures.push_back({ "truncated_ihdr.png", d, "PNG magic + partial IHDR chunk without payload" });
    }

    // 7. Truncated IDAT: Valid magic + IHDR, but cut in the middle of IDAT
    {
        std::vector<unsigned char> d(kValid1x1Png, kValid1x1Png + sizeof(kValid1x1Png) - 16);
        fixtures.push_back({ "truncated_idat.png", d, "Valid PNG header and IHDR with truncated IDAT stream" });
    }

    // 8. Corrupted IHDR: Invalid color type (99) violating PNG spec
    {
        std::vector<unsigned char> d(kValid1x1Png, kValid1x1Png + sizeof(kValid1x1Png));
        d[25] = 99; // invalid color type (> 6)
        fixtures.push_back({ "corrupted_ihdr_ctype.png", d, "PNG with invalid color type 99 in IHDR" });
    }

    // 9. Plain text ASCII renamed to .png
    {
        const char text[] = "This is a plain ASCII file masquerading as a PNG replacement.\n";
        std::vector<unsigned char> d(reinterpret_cast<const unsigned char*>(text),
                                    reinterpret_cast<const unsigned char*>(text) + sizeof(text) - 1);
        fixtures.push_back({ "plain_text.png", d, "Plain text file renamed to .png" });
    }

    // 10. HTML file renamed to .png
    {
        const char html[] = "<!DOCTYPE html><html><body><h1>404 Not Found</h1></body></html>";
        std::vector<unsigned char> d(reinterpret_cast<const unsigned char*>(html),
                                    reinterpret_cast<const unsigned char*>(html) + sizeof(html) - 1);
        fixtures.push_back({ "html_error.png", d, "HTML document renamed to .png" });
    }

    // 11. Random garbage noise
    {
        std::vector<unsigned char> d(2048);
        for (size_t i = 0; i < d.size(); ++i) d[i] = static_cast<unsigned char>(std::rand() & 0xFF);
        fixtures.push_back({ "random_noise.png", d, "2048 bytes of pseudorandom entropy" });
    }

    // 12. Valid magic followed by garbage
    {
        std::vector<unsigned char> d = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
        for (int i = 0; i < 512; ++i) d.push_back(static_cast<unsigned char>(std::rand() & 0xFF));
        fixtures.push_back({ "magic_with_garbage.png", d, "Valid 8-byte PNG header followed by 512 bytes of noise" });
    }

    // Execute tests against all corrupted fixtures
    for (size_t i = 0; i < fixtures.size(); ++i) {
        const auto& fix = fixtures[i];
        fs::path sb = create_stress_sandbox("re3_m3_corrupt_");
        fs::path rep_dir = sb / "mods" / "textures" / "CorruptPack" / "replacements";
        fs::create_directories(rep_dir);

        uint64_t hash = 0xC000000000000000ULL + i;
        char fname[64];
        std::snprintf(fname, sizeof(fname), "%016llx.png", (unsigned long long)hash);
        write_binary_file(rep_dir / fname, fix.data.data(), fix.data.size());

        g_re3_tex_config.hd_models = true;
        std::string pack_str = (sb / "mods" / "textures" / "CorruptPack").generic_string();
        re3_hd_tex_init(pack_str.c_str());

        // First call: Should detect corruption, not crash, return 0, and add to negative cache
        uint32_t tex_id = re3_hd_tex_bind_replacement(hash);
        std::printf("    [Corrupt Fixture %zu/%zu] %-52s -> Returned %u (Safe Fallback)\n",
                    i + 1, fixtures.size(), fix.description.c_str(), tex_id);

        STRESS_ASSERT(tex_id == 0, "Corrupted PNG must gracefully return 0");

        re3_hd_tex_metrics_t m1;
        re3_hd_tex_get_metrics(&m1);
        STRESS_ASSERT(m1.cache_misses == 1, "Corrupted file must register as a cache miss in negative cache");
        STRESS_ASSERT(m1.active_textures == 0, "Corrupted file must not create active GL texture");

        // Second call: Must hit negative cache directly
        uint32_t tex_id2 = re3_hd_tex_bind_replacement(hash);
        STRESS_ASSERT(tex_id2 == 0, "Subsequent query must return 0 from negative cache");

        re3_hd_tex_metrics_t m2;
        re3_hd_tex_get_metrics(&m2);
        STRESS_ASSERT(m2.cache_misses == 2, "Second query must hit negative cache directly");

        re3_hd_tex_shutdown();
        std::error_code ec;
        fs::remove_all(sb, ec);
    }
}

// ---------------------------------------------------------------------
// 5. Lifecycle Stress: 50 Iterations of Init -> Queries -> Shutdown
// ---------------------------------------------------------------------
void test_repeated_lifecycle_sweeps() {
    ensure_gl_context();
    fs::path sb = create_stress_sandbox("re3_m3_life_");
    fs::path rep_dir = sb / "mods" / "textures" / "LifePack" / "replacements";
    fs::create_directories(rep_dir);

    // Create 15 valid textures in the replacement pack
    std::vector<uint64_t> valid_hashes;
    for (int i = 0; i < 15; ++i) {
        uint64_t h = 0x1111222233334440ULL + i;
        valid_hashes.push_back(h);
        char fname[64];
        std::snprintf(fname, sizeof(fname), "%016llx.png", (unsigned long long)h);
        write_binary_file(rep_dir / fname, (i % 2 == 0) ? kValid1x1Png : kValid2x2Png,
                          (i % 2 == 0) ? sizeof(kValid1x1Png) : sizeof(kValid2x2Png));
    }

    std::string pack_str = (sb / "mods" / "textures" / "LifePack").generic_string();
    g_re3_tex_config.hd_models = true;

    size_t mem_start = get_process_memory_bytes();
    const int kCycles = 50;

    std::printf("    Executing %d complete lifecycle sweeps (init -> query 15 valid + 15 missing -> shutdown)...\n",
                kCycles);

    for (int cycle = 0; cycle < kCycles; ++cycle) {
        // 1. Initialize
        re3_hd_tex_init(pack_str.c_str());
        if (!re3_hd_tex_is_active()) {
            STRESS_ASSERT(false, "Subsystem must be active after init");
        }

        // 2. Query valid textures
        for (uint64_t h : valid_hashes) {
            uint32_t tex = re3_hd_tex_bind_replacement(h);
            if (tex == 0) {
                STRESS_ASSERT(false, "Valid texture must bind non-zero handle");
            }
        }

        // 3. Query missing textures
        for (int i = 0; i < 15; ++i) {
            uint64_t h = 0xEEFFEEFF00000000ULL + i;
            uint32_t tex = re3_hd_tex_bind_replacement(h);
            if (tex != 0) {
                STRESS_ASSERT(false, "Missing texture must return 0");
            }
        }

        // Check metrics mid-cycle
        re3_hd_tex_metrics_t m_mid;
        re3_hd_tex_get_metrics(&m_mid);
        if (m_mid.active_textures != 15) {
            STRESS_ASSERT(false, "Active textures must equal 15 before shutdown");
        }

        // 4. Shutdown
        re3_hd_tex_shutdown();

        // 5. Verify state post-shutdown
        if (re3_hd_tex_is_active()) {
            STRESS_ASSERT(false, "Subsystem must be inactive after shutdown");
        }

        re3_hd_tex_metrics_t m_post;
        re3_hd_tex_get_metrics(&m_post);
        if (m_post.active_textures != 0 || m_post.cache_hits != 0 || m_post.cache_misses != 0) {
            STRESS_ASSERT(false, "All metrics and cache structures must be cleared after shutdown");
        }

        // Verify bind returns 0 when inactive
        uint32_t tex_inactive = re3_hd_tex_bind_replacement(valid_hashes[0]);
        if (tex_inactive != 0) {
            STRESS_ASSERT(false, "Bind replacement must return 0 after shutdown");
        }
    }

    size_t mem_end = get_process_memory_bytes();
    long long diff_kb = (mem_end > mem_start) ? static_cast<long long>(mem_end - mem_start) / 1024 : 0;
    std::printf("    [Memory Safety] Start: %zu KB, End: %zu KB (Diff: +%lld KB across 50 cycles)\n",
                mem_start / 1024, mem_end / 1024, diff_kb);

    // Working set growth must be bounded (< 20 MB)
    STRESS_ASSERT(diff_kb < 20 * 1024, "Working set memory growth must be bounded (< 20MB)");

    // Test lifecycle edge cases
    // A. Double-shutdown (idempotence)
    re3_hd_tex_shutdown();
    re3_hd_tex_shutdown();
    STRESS_ASSERT(!re3_hd_tex_is_active(), "Multiple shutdowns must be safe and idempotent");

    // B. Init with empty string / NULL
    re3_hd_tex_init("");
    STRESS_ASSERT(!re3_hd_tex_is_active(), "Init with empty string must set inactive");
    re3_hd_tex_init(nullptr);
    STRESS_ASSERT(!re3_hd_tex_is_active(), "Init with NULL must set inactive");

    // C. Init with nonexistent path
    re3_hd_tex_init("nonexistent/path/textures/invalid");
    STRESS_ASSERT(!re3_hd_tex_is_active(), "Init with nonexistent path must set inactive");

    // D. Config toggle: hd_models = false
    g_re3_tex_config.hd_models = false;
    re3_hd_tex_init(pack_str.c_str());
    STRESS_ASSERT(!re3_hd_tex_is_active(), "Init with hd_models=false must remain inactive");
    uint32_t dis_tex = re3_hd_tex_bind_replacement(valid_hashes[0]);
    STRESS_ASSERT(dis_tex == 0, "Bind must return 0 when hd_models is disabled");

    g_re3_tex_config.hd_models = true;
    re3_hd_tex_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

// ---------------------------------------------------------------------
// 6. Multi-Threaded Cache Contention Stress
// ---------------------------------------------------------------------
void test_concurrent_multithreaded_cache_access() {
    ensure_gl_context();
    fs::path sb = create_stress_sandbox("re3_m3_conc_");
    fs::path rep_dir = sb / "mods" / "textures" / "ConcPack" / "replacements";
    fs::create_directories(rep_dir);

    // 20 valid textures
    std::vector<uint64_t> valid_hashes;
    for (int i = 0; i < 20; ++i) {
        uint64_t h = 0x5555666677778880ULL + i;
        valid_hashes.push_back(h);
        char fname[64];
        std::snprintf(fname, sizeof(fname), "%016llx.png", (unsigned long long)h);
        write_binary_file(rep_dir / fname, kValid1x1Png, sizeof(kValid1x1Png));
    }

    std::string pack_str = (sb / "mods" / "textures" / "ConcPack").generic_string();
    g_re3_tex_config.hd_models = true;
    re3_hd_tex_init(pack_str.c_str());

    // Prime the 20 valid textures on the render thread (which owns the active GL context)
    for (uint64_t h : valid_hashes) {
        uint32_t tex = re3_hd_tex_bind_replacement(h);
        STRESS_ASSERT(tex != 0, "Priming valid texture must succeed on render thread");
    }

    const int kNumThreads = 8;
    const int kOpsPerThread = 2000;
    std::vector<std::thread> workers;

    for (int t = 0; t < kNumThreads; ++t) {
        workers.emplace_back([t, &valid_hashes, kOpsPerThread]() {
            for (int i = 0; i < kOpsPerThread; ++i) {
                if (i % 2 == 0) {
                    uint64_t h = valid_hashes[(t * 13 + i) % valid_hashes.size()];
                    uint32_t tex = re3_hd_tex_bind_replacement(h);
                    assert(tex != 0);
                } else {
                    uint64_t h = 0xFAFA000000000000ULL + ((t * 17 + i) % 50);
                    uint32_t tex = re3_hd_tex_bind_replacement(h);
                    assert(tex == 0);
                }
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    re3_hd_tex_metrics_t m;
    re3_hd_tex_get_metrics(&m);
    std::printf("    [Concurrency] 8 threads completed %d total ops: hits=%llu, misses=%llu, disk_loads=%llu, active=%u\n",
                kNumThreads * kOpsPerThread,
                (unsigned long long)m.cache_hits,
                (unsigned long long)m.cache_misses,
                (unsigned long long)m.disk_loads,
                m.active_textures);

    STRESS_ASSERT(m.active_textures == 20, "Active textures must equal 20");
    STRESS_ASSERT(m.disk_loads == 20, "Disk loads must equal 20 (no duplicate loads across threads)");

    re3_hd_tex_shutdown();
    std::error_code ec;
    fs::remove_all(sb, ec);
}

int main(int argc, char* argv[]) {
    std::printf("====================================================================\n");
    std::printf(" RE3Recomp M3: HD Texture Cache & Lifecycle Adversarial Stress Suite\n");
    std::printf("====================================================================\n");

    RUN_STRESS_TEST(test_high_throughput_cache_querying);
    RUN_STRESS_TEST(test_negative_cache_effectiveness);
    RUN_STRESS_TEST(test_candidate_pattern_resolution_matrix);
    RUN_STRESS_TEST(test_corrupted_png_resilience);
    RUN_STRESS_TEST(test_repeated_lifecycle_sweeps);
    RUN_STRESS_TEST(test_concurrent_multithreaded_cache_access);

    std::printf("====================================================================\n");
    std::printf(" Adversarial Results: %d executed, %d passed, %d failed\n",
                g_tests_run, g_tests_passed, g_tests_failed);
    std::printf("====================================================================\n");

    if (g_gl_ctx) {
        SDL_GL_DestroyContext(g_gl_ctx);
    }
    if (g_gl_window) {
        SDL_DestroyWindow(g_gl_window);
    }

    return (g_tests_failed == 0) ? 0 : 1;
}
