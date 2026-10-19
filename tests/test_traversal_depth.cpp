#include "re3_textures.h"
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

int main() {
    fs::path sb = fs::temp_directory_path() / "re3_traversal_test";
    fs::remove_all(sb);
    fs::create_directories(sb / "mods" / "textures" / "MyPack" / "backgrounds");
    
    // File outside the pack: in sb
    fs::path target_file = sb / "target.png";
    std::ofstream(target_file) << "TARGET";

    std::strncpy(g_re3_tex_config.pack, "MyPack", sizeof(g_re3_tex_config.pack) - 1);
    re3_textures_init(sb.generic_string().c_str());

    char out[512] = {0};
    // MyPack is at sb/mods/textures/MyPack
    // backgrounds is at sb/mods/textures/MyPack/backgrounds
    // To go from backgrounds to sb:
    // 1: MyPack
    // 2: textures
    // 3: mods
    // 4: sb
    // So ../../../../target.png should reach sb/target.png
    bool res = re3_textures_resolve_path(out, sizeof(out), "backgrounds", "../../../../target.png");
    std::printf("Relative traversal test: res=%d, out='%s'\n", res, out);

    re3_textures_shutdown();
    fs::remove_all(sb);
    return 0;
}
