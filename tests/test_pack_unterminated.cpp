#include "re3_textures.h"
#include <cstdio>
#include <cstring>

int main() {
    // Fill g_re3_tex_config.pack with 128 non-null chars
    std::memset(g_re3_tex_config.pack, 'A', sizeof(g_re3_tex_config.pack));
    g_re3_tex_config.hd_backgrounds = true;
    g_re3_tex_config.hd_models = true;
    g_re3_tex_config.hd_ui = true;
    g_re3_tex_config.dump_textures = false;

    // Call init
    re3_textures_init(".");
    std::printf("Pack length after init with 128 non-null chars: %zu\n", std::strlen(g_re3_tex_config.pack));
    std::printf("Last char (index 127): %d\n", (int)g_re3_tex_config.pack[127]);

    re3_textures_shutdown();
    return 0;
}
