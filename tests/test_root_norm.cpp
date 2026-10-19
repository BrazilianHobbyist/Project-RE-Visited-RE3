#include "re3_textures.h"
#include <cstdio>
#include <cstring>

int main() {
    re3_textures_init("C:\\");
    std::printf("Base path for 'C:\\': '%s'\n", re3_textures_get_base_path());

    re3_textures_init("C:/");
    std::printf("Base path for 'C:/': '%s'\n", re3_textures_get_base_path());

    re3_textures_init("D:\\Games\\RE3\\");
    std::printf("Base path for 'D:\\Games\\RE3\\': '%s'\n", re3_textures_get_base_path());

    re3_textures_shutdown();
    return 0;
}
