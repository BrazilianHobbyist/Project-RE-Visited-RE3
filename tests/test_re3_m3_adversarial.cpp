#include "re3_hd_textures.h"
#include "re3_textures.h"
#include "xxhash.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define ADV_TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        std::printf("  [FAIL] %s (Line %d: %s)\n", msg, __LINE__, #cond); \
        g_tests_failed++; \
        return; \
    } \
} while(0)

#define RUN_ADV_TEST(fn) do { \
    int last_failed = g_tests_failed; \
    g_tests_run++; \
    std::printf("[ADVERSARIAL %02d] %s...\n", g_tests_run, #fn); \
    fn(); \
    if (g_tests_failed == last_failed) { \
        g_tests_passed++; \
        std::printf("  [PASS] %s\n", #fn); \
    } \
} while(0)

// ----------------------------------------------------------------------------
// Test 1: Golden Fixture Bit-Exact Parity
// ----------------------------------------------------------------------------
void test_golden_hashes_parity() {
    uint8_t lim_full[4] = {0, 0, 255, 255};

    // Jill
    uint16_t pal_jill[16];
    for (int i = 0; i < 16; i++) pal_jill[i] = 0x1234;
    const char jill_bytes[] = "JILL_ACTOR_TEXEL_DATA_MOCK_BYTES";
    uint64_t h_jill = re3_hd_tex_compute_hash(0x0005, 0x7C00, lim_full, pal_jill, 16,
                                              reinterpret_cast<const uint16_t*>(jill_bytes),
                                              (int)(sizeof(jill_bytes) - 1) / 2);
    ADV_TEST_ASSERT(h_jill == 0x2a8d3a5fc58a1180ULL, "Jill hash mismatch");

    // Nemesis
    uint16_t pal_nemesis[16];
    for (int i = 0; i < 16; i++) pal_nemesis[i] = 0x5678;
    const char nemesis_bytes[] = "NEMESIS_ACTOR_TEXEL_DATA_BYTES";
    uint64_t h_nemesis = re3_hd_tex_compute_hash(0x0006, 0x7C10, lim_full, pal_nemesis, 16,
                                                 reinterpret_cast<const uint16_t*>(nemesis_bytes),
                                                 (int)(sizeof(nemesis_bytes) - 1) / 2);
    ADV_TEST_ASSERT(h_nemesis == 0x5eb27155676517dbULL, "Nemesis hash mismatch");

    // Weapon
    uint8_t lim_weapon[4] = {0, 0, 127, 127};
    uint16_t pal_weapon[16];
    for (int i = 0; i < 16; i++) pal_weapon[i] = 0x9ABC;
    const char weapon_bytes[] = "WEAPON_TEXEL_DATA_MOCK_BYTES";
    uint64_t h_weapon = re3_hd_tex_compute_hash(0x0007, 0x7C20, lim_weapon, pal_weapon, 16,
                                                reinterpret_cast<const uint16_t*>(weapon_bytes),
                                                (int)(sizeof(weapon_bytes) - 1) / 2);
    ADV_TEST_ASSERT(h_weapon == 0x80c806206d596532ULL, "Weapon hash mismatch");

    // UI Herb
    uint8_t lim_herb[4] = {0, 0, 31, 31};
    uint16_t pal_herb[16];
    for (int i = 0; i < 16; i++) pal_herb[i] = 0x00FF;
    const char herb_bytes[] = "UI_HERB_ICON_DATA";
    uint64_t h_herb = re3_hd_tex_compute_hash_bytes(0x0008, 0x7C30, lim_herb, pal_herb, 16,
                                                    herb_bytes, sizeof(herb_bytes) - 1);
    ADV_TEST_ASSERT(h_herb == 0x2662656daac1131bULL, "UI Herb hash mismatch");
}

// ----------------------------------------------------------------------------
// Test 2: Extreme Boundary UV Coordinates & Inverted Limits
// ----------------------------------------------------------------------------
void test_extreme_uv_boundaries() {
    uint16_t pal[16] = {0x1111};
    uint16_t texels[64] = {0xAAAA};

    // 2.1 Single-pixel at (0, 0, 0, 0)
    uint8_t lim_0000[4] = {0, 0, 0, 0};
    uint64_t h_0000 = re3_hd_tex_compute_hash(0x0005, 0x7C00, lim_0000, pal, 16, texels, 1);
    ADV_TEST_ASSERT(h_0000 != 0, "Single pixel at origin must produce non-zero hash");

    // 2.2 Single-pixel at (255, 255, 255, 255)
    uint8_t lim_255[4] = {255, 255, 255, 255};
    uint64_t h_255 = re3_hd_tex_compute_hash(0x0005, 0x7C00, lim_255, pal, 16, texels, 1);
    ADV_TEST_ASSERT(h_255 != 0, "Single pixel at max bounds must produce non-zero hash");
    ADV_TEST_ASSERT(h_255 != h_0000, "UV bounds (0,0,0,0) and (255,255,255,255) must produce distinct hashes");

    // 2.3 Single-pixel box inside page (128, 128, 128, 128)
    uint8_t lim_128[4] = {128, 128, 128, 128};
    uint64_t h_128 = re3_hd_tex_compute_hash(0x0005, 0x7C00, lim_128, pal, 16, texels, 1);
    ADV_TEST_ASSERT(h_128 != h_0000 && h_128 != h_255, "Mid-page single pixel must produce distinct hash");

    // 2.4 Inverted UV limits (255, 255, 0, 0)
    uint8_t lim_inv[4] = {255, 255, 0, 0};
    uint64_t h_inv = re3_hd_tex_compute_hash(0x0005, 0x7C00, lim_inv, pal, 16, texels, 16);
    ADV_TEST_ASSERT(h_inv != 0, "Inverted limits must execute deterministically without error");

    // 2.5 NULL lim defaults to (0, 0, 255, 255)
    uint8_t lim_explicit_full[4] = {0, 0, 255, 255};
    uint64_t h_null_lim = re3_hd_tex_compute_hash(0x0005, 0x7C00, nullptr, pal, 16, texels, 16);
    uint64_t h_full_lim = re3_hd_tex_compute_hash(0x0005, 0x7C00, lim_explicit_full, pal, 16, texels, 16);
    ADV_TEST_ASSERT(h_null_lim == h_full_lim, "NULL lim must be bit-exact identical to (0,0,255,255)");

    // 2.6 Out-of-bounds UV coordinates in re3_hd_tex_compute_from_vram
    std::vector<uint16_t> vram(1024 * 512, 0x5555);
    int lim_oob[4] = {-1000, -500, 9999, 8888};
    uint64_t h_oob = re3_hd_tex_compute_from_vram(0x0005, 0x7C00, lim_oob, vram.data());
    ADV_TEST_ASSERT(h_oob != 0, "OOB limits in VRAM streaming must clamp and complete safely");

    // Clamped OOB should match explicit [0, 0, 255, 255]
    int lim_clamped[4] = {0, 0, 255, 255};
    uint64_t h_clamped = re3_hd_tex_compute_from_vram(0x0005, 0x7C00, lim_clamped, vram.data());
    ADV_TEST_ASSERT(h_oob == h_clamped, "Clamped OOB coordinates must match [0, 0, 255, 255] exactly");

    // 2.7 Inverted limits in VRAM streaming (u_min > u_max)
    int lim_vram_inv[4] = {255, 255, 0, 0};
    uint64_t h_vram_inv = re3_hd_tex_compute_from_vram(0x0005, 0x7C00, lim_vram_inv, vram.data());
    ADV_TEST_ASSERT(h_vram_inv != 0, "Inverted limits in VRAM streaming must not crash or loop");
}

// ----------------------------------------------------------------------------
// Test 3: Depth Modes (4bpp, 8bpp, 15bpp direct, invalid depth 3)
// ----------------------------------------------------------------------------
void test_depth_modes() {
    uint8_t lim[4] = {0, 0, 63, 63};
    uint16_t pal_a[256];
    uint16_t pal_b[256];
    for (int i = 0; i < 256; i++) {
        pal_a[i] = (uint16_t)(i * 0x101);
        pal_b[i] = (uint16_t)(i * 0x202 + 1);
    }
    uint16_t texels[64];
    for (int i = 0; i < 64; i++) texels[i] = (uint16_t)i;

    // 3.1 4bpp mode (depth = 0): tpage bit 7..8 = 0
    uint16_t tpage_4bpp = 0x0005; // depth = 0
    uint64_t h_4bpp_a = re3_hd_tex_compute_hash(tpage_4bpp, 0x7C00, lim, pal_a, 16, texels, 64);
    uint64_t h_4bpp_b = re3_hd_tex_compute_hash(tpage_4bpp, 0x7C00, lim, pal_b, 16, texels, 64);
    ADV_TEST_ASSERT(h_4bpp_a != h_4bpp_b, "Changing palette in 4bpp mode must alter hash");

    // 3.2 8bpp mode (depth = 1): tpage bit 7..8 = 1 (tpage | (1 << 7) = 0x0085)
    uint16_t tpage_8bpp = 0x0085; // depth = 1
    uint64_t h_8bpp_a = re3_hd_tex_compute_hash(tpage_8bpp, 0x7C00, lim, pal_a, 256, texels, 64);
    uint64_t h_8bpp_b = re3_hd_tex_compute_hash(tpage_8bpp, 0x7C00, lim, pal_b, 256, texels, 64);
    ADV_TEST_ASSERT(h_8bpp_a != h_8bpp_b, "Changing palette in 8bpp mode must alter hash");
    ADV_TEST_ASSERT(h_4bpp_a != h_8bpp_a, "4bpp and 8bpp must produce distinct hashes for same data");

    // 3.3 15bpp direct mode (depth = 2): tpage bit 7..8 = 2 (tpage | (2 << 7) = 0x0105)
    uint16_t tpage_15bpp = 0x0105; // depth = 2
    uint64_t h_15bpp_a = re3_hd_tex_compute_hash(tpage_15bpp, 0x7C00, lim, pal_a, 256, texels, 64);
    uint64_t h_15bpp_b = re3_hd_tex_compute_hash(tpage_15bpp, 0x7C00, lim, pal_b, 256, texels, 64);
    ADV_TEST_ASSERT(h_15bpp_a == h_15bpp_b, "15bpp direct mode must completely ignore palette (hash invariant)");

    // 15bpp direct with NULL palette must also produce identical hash
    uint64_t h_15bpp_null = re3_hd_tex_compute_hash(tpage_15bpp, 0x7C00, lim, nullptr, 0, texels, 64);
    ADV_TEST_ASSERT(h_15bpp_a == h_15bpp_null, "15bpp direct mode with NULL palette must match exactly");

    // 3.4 Invalid depth 3 (depth = 3): tpage bit 7..8 = 3 (0x0185)
    uint16_t tpage_depth3 = 0x0185;
    uint64_t h_d3_a = re3_hd_tex_compute_hash(tpage_depth3, 0x7C00, lim, pal_a, 256, texels, 64);
    uint64_t h_d3_b = re3_hd_tex_compute_hash(tpage_depth3, 0x7C00, lim, pal_b, 256, texels, 64);
    ADV_TEST_ASSERT(h_d3_a == h_d3_b, "Invalid depth 3 must safely ignore palette like 15bpp direct");
    ADV_TEST_ASSERT(h_d3_a != 0, "Invalid depth 3 must compute valid non-zero hash without crash");
}

// ----------------------------------------------------------------------------
// Test 4: Palette Zero-Padding Equivalence & Truncation
// ----------------------------------------------------------------------------
void test_palette_zero_padding_and_truncation() {
    uint8_t lim[4] = {0, 0, 15, 15};
    uint16_t texels[16] = {0x1234};

    // 4.1 4bpp: short palette of 2 words vs 16-word palette with 14 trailing zeros
    uint16_t short_pal_4[2] = {0xABCD, 0xEF01};
    uint16_t full_padded_pal_4[16] = {0};
    full_padded_pal_4[0] = 0xABCD;
    full_padded_pal_4[1] = 0xEF01;

    uint64_t h_short_4 = re3_hd_tex_compute_hash(0x0005, 0x7C00, lim, short_pal_4, 2, texels, 16);
    uint64_t h_padded_4 = re3_hd_tex_compute_hash(0x0005, 0x7C00, lim, full_padded_pal_4, 16, texels, 16);
    ADV_TEST_ASSERT(h_short_4 == h_padded_4, "4bpp short palette (2 words) must bit-exactly match 16-word zero-padded palette");

    // 4.2 8bpp: short palette of 5 words vs 256-word palette with 251 trailing zeros
    uint16_t short_pal_8[5] = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555};
    uint16_t full_padded_pal_8[256] = {0};
    for (int i = 0; i < 5; i++) full_padded_pal_8[i] = short_pal_8[i];

    uint64_t h_short_8 = re3_hd_tex_compute_hash(0x0085, 0x7C00, lim, short_pal_8, 5, texels, 16);
    uint64_t h_padded_8 = re3_hd_tex_compute_hash(0x0085, 0x7C00, lim, full_padded_pal_8, 256, texels, 16);
    ADV_TEST_ASSERT(h_short_8 == h_padded_8, "8bpp short palette (5 words) must bit-exactly match 256-word zero-padded palette");

    // 4.3 NULL palette vs explicit all-zero palette
    uint16_t zero_pal_4[16] = {0};
    uint64_t h_null_pal = re3_hd_tex_compute_hash(0x0005, 0x7C00, lim, nullptr, 0, texels, 16);
    uint64_t h_zero_pal = re3_hd_tex_compute_hash(0x0005, 0x7C00, lim, zero_pal_4, 16, texels, 16);
    ADV_TEST_ASSERT(h_null_pal == h_zero_pal, "NULL palette must bit-exactly match all-zero palette");

    // 4.4 Truncation: passing 300 entries for 4bpp (which caps at 16)
    uint16_t oversized_pal[300];
    for (int i = 0; i < 300; i++) oversized_pal[i] = (uint16_t)(i + 1);
    uint16_t first_16_pal[16];
    for (int i = 0; i < 16; i++) first_16_pal[i] = (uint16_t)(i + 1);

    uint64_t h_oversized = re3_hd_tex_compute_hash(0x0005, 0x7C00, lim, oversized_pal, 300, texels, 16);
    uint64_t h_exact_16  = re3_hd_tex_compute_hash(0x0005, 0x7C00, lim, first_16_pal, 16, texels, 16);
    ADV_TEST_ASSERT(h_oversized == h_exact_16, "Oversized palette in 4bpp must strictly truncate to 16 words");
}

// ----------------------------------------------------------------------------
// Test 5: VRAM Row Wrapping & Canary Memory Corruption Protection
// ----------------------------------------------------------------------------
void test_vram_row_wrapping_and_canaries() {
    const size_t CANARY_SIZE = 4096;
    const size_t VRAM_WORDS  = 1024 * 512;
    const uint16_t CANARY_VAL = 0xBEEF;

    std::vector<uint16_t> buffer(CANARY_SIZE + VRAM_WORDS + CANARY_SIZE, CANARY_VAL);
    uint16_t* guard_pre  = buffer.data();
    uint16_t* mock_vram  = buffer.data() + CANARY_SIZE;
    uint16_t* guard_post = buffer.data() + CANARY_SIZE + VRAM_WORDS;

    // Fill VRAM with recognizable test pattern
    for (size_t i = 0; i < VRAM_WORDS; i++) {
        mock_vram[i] = (uint16_t)(i & 0x7FFF);
    }

    // 5.1 Horizontal Row Wrapping Test
    // tpage at right edge of VRAM: tpage & 0xF = 15 -> base_x = 15 * 64 = 960.
    // In 15bpp (depth = 2): word_start_x = 960, word_end_x = 960 + 255 = 1215.
    // row_words = 256. Since 960 + 256 = 1216 > 1024, it wraps around past column 1024 to column 0!
    uint16_t tpage_right_edge = 0x010F; // depth = 2, base_x = 960
    int lim_full[4] = {0, 0, 255, 255};

    uint64_t h_wrap_x = re3_hd_tex_compute_from_vram(tpage_right_edge, 0x7C00, lim_full, mock_vram);
    ADV_TEST_ASSERT(h_wrap_x != 0, "Horizontal row wrapping must produce non-zero hash");

    // Verify canaries remain 100% pristine
    for (size_t i = 0; i < CANARY_SIZE; i++) {
        ADV_TEST_ASSERT(guard_pre[i] == CANARY_VAL, "Memory corruption detected in PRE-guard canary!");
        ADV_TEST_ASSERT(guard_post[i] == CANARY_VAL, "Memory corruption detected in POST-guard canary!");
    }

    // 5.2 Vertical Bottom Wrapping Test
    // base_y = 256, v_max = 255 -> y = 256 + 255 = 511 (last line of VRAM).
    uint16_t tpage_bottom = 0x0115; // base_x = 320, base_y = 256
    uint64_t h_wrap_y = re3_hd_tex_compute_from_vram(tpage_bottom, 0x7C00, lim_full, mock_vram);
    ADV_TEST_ASSERT(h_wrap_y != 0, "Vertical bottom bounds must produce non-zero hash");

    // Verify canaries remain pristine
    for (size_t i = 0; i < CANARY_SIZE; i++) {
        ADV_TEST_ASSERT(guard_pre[i] == CANARY_VAL, "Memory corruption detected in PRE-guard canary after vertical test!");
        ADV_TEST_ASSERT(guard_post[i] == CANARY_VAL, "Memory corruption detected in POST-guard canary after vertical test!");
    }

    // 5.3 CLUT Wrapping at column 1008
    // clut_x = (0x3F) * 16 = 1008. In 8bpp mode, 256 words will wrap past 1024 to column 0..239.
    uint16_t clut_wrap = 0x7C3F; // clut_x = 1008, clut_y = 496
    uint16_t tpage_8bpp = 0x0085;
    uint64_t h_clut_wrap = re3_hd_tex_compute_from_vram(tpage_8bpp, clut_wrap, lim_full, mock_vram);
    ADV_TEST_ASSERT(h_clut_wrap != 0, "CLUT horizontal row wrap must succeed deterministically");

    // Verify canaries
    for (size_t i = 0; i < CANARY_SIZE; i++) {
        ADV_TEST_ASSERT(guard_pre[i] == CANARY_VAL, "Memory corruption in PRE-guard canary after CLUT wrap!");
        ADV_TEST_ASSERT(guard_post[i] == CANARY_VAL, "Memory corruption in POST-guard canary after CLUT wrap!");
    }

    // 5.4 Null pointer check
    uint64_t h_null = re3_hd_tex_compute_from_vram(0x0005, 0x7C00, lim_full, nullptr);
    ADV_TEST_ASSERT(h_null == 0, "NULL vram pointer must safely return 0 without crash");
}

// ----------------------------------------------------------------------------
// Test 6: Collision Resistance (10,000 distinct payloads) & Avalanche Testing
// ----------------------------------------------------------------------------
void test_collision_resistance_10000() {
    std::unordered_set<uint64_t> hash_set;
    hash_set.reserve(10000);

    const int TOTAL_PAYLOADS = 10000;
    uint16_t palette[256];
    uint16_t texels[32];

    for (int i = 0; i < TOTAL_PAYLOADS; i++) {
        uint16_t tpage = (uint16_t)(i & 0x01FF);
        uint16_t clut  = (uint16_t)((i * 17) & 0xFFFF);
        uint8_t lim[4] = {
            (uint8_t)(i & 0x3F),
            (uint8_t)((i >> 2) & 0x3F),
            (uint8_t)(64 + (i & 0x3F)),
            (uint8_t)(64 + ((i >> 2) & 0x3F))
        };
        for (int p = 0; p < 16; p++) {
            palette[p] = (uint16_t)(i ^ (p * 0x0F0F));
        }
        for (int t = 0; t < 32; t++) {
            texels[t] = (uint16_t)((i * 31) + t);
        }

        uint64_t h = re3_hd_tex_compute_hash(tpage, clut, lim, palette, 16, texels, 32);
        ADV_TEST_ASSERT(h != 0, "Hash must be non-zero");

        auto insert_res = hash_set.insert(h);
        ADV_TEST_ASSERT(insert_res.second, "Collision detected in 10,000 payload test!");
    }

    ADV_TEST_ASSERT(hash_set.size() == TOTAL_PAYLOADS, "Hash set must contain exactly 10,000 unique digests");

    // Avalanche effect: single-bit flip across 1,000 payloads
    for (int i = 0; i < 1000; i++) {
        uint16_t tpage = (uint16_t)(i & 0x01FF);
        uint16_t clut  = (uint16_t)((i * 17) & 0xFFFF);
        uint8_t lim[4] = {0, 0, 31, 31};
        uint16_t pal[16] = {0};
        uint16_t tex[16] = {0};

        uint64_t h_orig = re3_hd_tex_compute_hash(tpage, clut, lim, pal, 16, tex, 16);

        // Flip 1 bit in clut
        uint64_t h_flip_clut = re3_hd_tex_compute_hash(tpage, (uint16_t)(clut ^ 1), lim, pal, 16, tex, 16);
        ADV_TEST_ASSERT(h_orig != h_flip_clut, "Single-bit flip in CLUT must change hash");

        // Flip 1 bit in texels
        tex[0] ^= 1;
        uint64_t h_flip_tex = re3_hd_tex_compute_hash(tpage, clut, lim, pal, 16, tex, 16);
        ADV_TEST_ASSERT(h_orig != h_flip_tex, "Single-bit flip in texels must change hash");
        tex[0] ^= 1; // restore

        // Flip 1 bit in palette
        pal[0] ^= 1;
        uint64_t h_flip_pal = re3_hd_tex_compute_hash(tpage, clut, lim, pal, 16, tex, 16);
        int depth = (tpage >> 7) & 3;
        if (depth < 2) {
            ADV_TEST_ASSERT(h_orig != h_flip_pal, "Single-bit flip in palette must change hash for indexed modes");
        } else {
            ADV_TEST_ASSERT(h_orig == h_flip_pal, "Palette flip in 15bpp direct mode must not change hash");
        }
    }
}

// ----------------------------------------------------------------------------
// Test 7: UV Normalization Mathematics & Degenerate Rectangles
// ----------------------------------------------------------------------------
void test_uv_normalization_stress() {
    float u_norm = 0.0f, v_norm = 0.0f;

    // 7.1 Full page edge conditions
    re3_hd_tex_normalize_uv_full_page(-100.0f, 1000.0f, &u_norm, &v_norm);
    ADV_TEST_ASSERT(std::fabs(u_norm - ((-100.0f + 0.5f) / 256.0f)) < 1e-6f, "Full page negative u");
    ADV_TEST_ASSERT(std::fabs(v_norm - ((1000.0f + 0.5f) / 256.0f)) < 1e-6f, "Full page large v");

    // 7.2 Sub-rect zero-width / zero-height protection (u_min == u_max)
    re3_hd_tex_normalize_uv_sub_rect(10.0f, 20.0f, 10, 20, 10, 20, &u_norm, &v_norm);
    ADV_TEST_ASSERT(u_norm == 0.0f, "Zero-width sub-rect u must be 0.0f");
    ADV_TEST_ASSERT(v_norm == 0.0f, "Zero-height sub-rect v must be 0.0f");

    // 7.3 Inverted sub-rect (u_max < u_min)
    re3_hd_tex_normalize_uv_sub_rect(15.0f, 25.0f, 20, 30, 10, 10, &u_norm, &v_norm);
    // w is clamped to 1.0f when u_max <= u_min -> (15 - 20) / 1 = -5.0f
    ADV_TEST_ASSERT(u_norm == -5.0f, "Inverted sub-rect width must fall back to 1.0f without division by zero");

    // 7.4 Null pointers safety
    re3_hd_tex_normalize_uv_full_page(0.0f, 0.0f, nullptr, nullptr);
    re3_hd_tex_normalize_uv_sub_rect(0.0f, 0.0f, 0, 0, 10, 10, nullptr, nullptr);
    // If it reaches here without crash, test passes.
}

// ----------------------------------------------------------------------------
// Test 8: Multithreaded Concurrency Stress
// ----------------------------------------------------------------------------
void test_concurrent_hash_stress() {
    const int NUM_THREADS = 4;
    const int OPS_PER_THREAD = 2500;
    std::vector<std::thread> threads;
    std::vector<uint64_t> thread_sums(NUM_THREADS, 0);

    for (int t = 0; t < NUM_THREADS; t++) {
        threads.emplace_back([t, OPS_PER_THREAD, &thread_sums]() {
            uint16_t pal[16] = {0x1234};
            uint16_t tex[16] = {0x5678};
            uint8_t lim[4] = {0, 0, 15, 15};
            uint64_t local_sum = 0;
            for (int i = 0; i < OPS_PER_THREAD; i++) {
                uint16_t tpage = (uint16_t)(0x0005 + (i & 0xF));
                uint16_t clut  = (uint16_t)(0x7C00 + ((t * 1000 + i) & 0x1FF));
                uint64_t h = re3_hd_tex_compute_hash(tpage, clut, lim, pal, 16, tex, 16);
                local_sum ^= h;
            }
            thread_sums[t] = local_sum;
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    for (int t = 0; t < NUM_THREADS; t++) {
        ADV_TEST_ASSERT(thread_sums[t] != 0, "Thread computation sum must be non-zero");
    }
}

// ----------------------------------------------------------------------------
// Parity Vector Exporter for Cross-Language Verification
// ----------------------------------------------------------------------------
static uint32_t s_adv_prng = 0x12345678;
static uint32_t adv_rand() {
    s_adv_prng = s_adv_prng * 1664525u + 1013904223u;
    return s_adv_prng;
}

static void export_parity_vectors(const char* filepath) {
    std::FILE* f = std::fopen(filepath, "wb");
    if (!f) {
        std::fprintf(stderr, "Failed to open %s for writing\n", filepath);
        return;
    }

    const uint32_t num_vectors = 500;
    std::fwrite(&num_vectors, sizeof(uint32_t), 1, f);

    for (uint32_t i = 0; i < num_vectors; i++) {
        uint16_t tpage = (uint16_t)(adv_rand() & 0x01FF);
        uint16_t clut  = (uint16_t)(adv_rand() & 0xFFFF);
        uint8_t has_lim = (uint8_t)((adv_rand() % 4 != 0) ? 1 : 0);
        uint8_t lim[4] = {0, 0, 255, 255};
        if (has_lim) {
            uint32_t style = adv_rand() % 4;
            if (style == 0) {
                // Inverted limits
                lim[0] = 255; lim[1] = 255; lim[2] = 0; lim[3] = 0;
            } else if (style == 1) {
                // Single pixel
                uint8_t p = (uint8_t)(adv_rand() & 0xFF);
                lim[0] = p; lim[1] = p; lim[2] = p; lim[3] = p;
            } else if (style == 2) {
                // Sub-rect
                lim[0] = (uint8_t)(adv_rand() & 0x7F);
                lim[1] = (uint8_t)(adv_rand() & 0x7F);
                lim[2] = (uint8_t)(lim[0] + (adv_rand() & 0x3F));
                lim[3] = (uint8_t)(lim[1] + (adv_rand() & 0x3F));
            } else {
                // Full page
                lim[0] = 0; lim[1] = 0; lim[2] = 255; lim[3] = 255;
            }
        }

        uint16_t palette_len = 0;
        uint32_t pal_mode = adv_rand() % 6;
        if (pal_mode == 0) palette_len = 0;
        else if (pal_mode == 1) palette_len = 2; // short
        else if (pal_mode == 2) palette_len = 16;
        else if (pal_mode == 3) palette_len = 64;
        else if (pal_mode == 4) palette_len = 256;
        else palette_len = 300; // oversized

        std::vector<uint16_t> palette(palette_len);
        for (size_t p = 0; p < palette_len; p++) {
            palette[p] = (uint16_t)adv_rand();
        }

        uint32_t texel_bytes = 0;
        uint32_t tex_mode = adv_rand() % 6;
        if (tex_mode == 0) texel_bytes = 0;
        else if (tex_mode == 1) texel_bytes = 1;
        else if (tex_mode == 2) texel_bytes = 16;
        else if (tex_mode == 3) texel_bytes = 64;
        else if (tex_mode == 4) texel_bytes = 256;
        else texel_bytes = 512;

        std::vector<uint8_t> texels(texel_bytes);
        for (size_t t = 0; t < texel_bytes; t++) {
            texels[t] = (uint8_t)adv_rand();
        }

        uint64_t c_hash = re3_hd_tex_compute_hash_bytes(
            tpage,
            clut,
            has_lim ? lim : nullptr,
            palette_len > 0 ? palette.data() : nullptr,
            (int)palette_len,
            texel_bytes > 0 ? texels.data() : nullptr,
            texel_bytes
        );

        std::fwrite(&tpage, sizeof(uint16_t), 1, f);
        std::fwrite(&clut, sizeof(uint16_t), 1, f);
        std::fwrite(&has_lim, sizeof(uint8_t), 1, f);
        std::fwrite(lim, sizeof(uint8_t), 4, f);
        std::fwrite(&palette_len, sizeof(uint16_t), 1, f);
        if (palette_len > 0) {
            std::fwrite(palette.data(), sizeof(uint16_t), palette_len, f);
        }
        std::fwrite(&texel_bytes, sizeof(uint32_t), 1, f);
        if (texel_bytes > 0) {
            std::fwrite(texels.data(), sizeof(uint8_t), texel_bytes, f);
        }
        std::fwrite(&c_hash, sizeof(uint64_t), 1, f);
    }

    std::fclose(f);
    std::printf("[PARITY] Successfully exported %u test vectors to %s\n", num_vectors, filepath);
}

// ----------------------------------------------------------------------------
// Main Runner
// ----------------------------------------------------------------------------
int main(int argc, char** argv) {
    if (argc > 1 && std::strcmp(argv[1], "--export-parity") == 0) {
        const char* path = (argc > 2) ? argv[2] : "build/parity_vectors.bin";
        export_parity_vectors(path);
        return 0;
    }

    std::printf("========================================================\n");
    std::printf(" Running RE3Recomp M3-1 xxHash64 Adversarial Stress Tests\n");
    std::printf("========================================================\n");

    RUN_ADV_TEST(test_golden_hashes_parity);
    RUN_ADV_TEST(test_extreme_uv_boundaries);
    RUN_ADV_TEST(test_depth_modes);
    RUN_ADV_TEST(test_palette_zero_padding_and_truncation);
    RUN_ADV_TEST(test_vram_row_wrapping_and_canaries);
    RUN_ADV_TEST(test_collision_resistance_10000);
    RUN_ADV_TEST(test_uv_normalization_stress);
    RUN_ADV_TEST(test_concurrent_hash_stress);

    std::printf("========================================================\n");
    std::printf(" Adversarial Results: %d executed, %d passed, %d failed\n",
                g_tests_run, g_tests_passed, g_tests_failed);
    std::printf("========================================================\n");

    return g_tests_failed == 0 ? 0 : 1;
}
