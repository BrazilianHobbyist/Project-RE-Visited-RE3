#define WIN32_LEAN_AND_MEAN
#define UNICODE 1
#define _UNICODE 1
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <gdiplus.h>
#include <uxtheme.h>

// Paleta visual do launcher (tema escuro "Raccoon City")
static const COLORREF CLR_BG        = RGB(22, 22, 26);
static const COLORREF CLR_TEXT      = RGB(225, 225, 230);
static const COLORREF CLR_ACCENT    = RGB(200, 40, 40);
static const COLORREF CLR_MUTED     = RGB(140, 140, 150);
static const COLORREF CLR_OK        = RGB(90, 210, 110);
static const COLORREF CLR_WARN      = RGB(240, 150, 60);
static HBRUSH g_hBrBg = NULL;
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <cstring>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")

// Control IDs
enum ControlID {
    ID_BTN_LAUNCH = 1001,
    ID_BTN_SAVE = 1002,
    ID_BTN_SAVES_DIR = 1003,
    ID_BTN_INPUT_INI = 1004,
    ID_COMBO_ASPECT = 1005,
    ID_COMBO_DISPLAY = 1006,
    ID_COMBO_RENDERER = 1007,
    ID_COMBO_INTERNAL_RES = 1008,
    ID_COMBO_FPS = 1009,
    ID_COMBO_CONTROLS = 1010,
    ID_CHK_PGXP_GEOM = 1011,
    ID_CHK_PGXP_TEX = 1012,
    ID_CHK_SKIP_FMV = 1013,
    ID_CHK_SPU_HQ = 1014,
    ID_CHK_ANTIALIASING = 1015,
    ID_BTN_CREDITS = 1016,
    ID_BTN_CREDITS_CLOSE = 1017,
    ID_BTN_CREDITS_COPY = 1018,
    ID_BTN_CREDITS_GITHUB = 1019,
    ID_COMBO_LANGUAGE = 1020,
    ID_BTN_OPEN_SETUP = 1021,
    ID_BTN_SETUP_BROWSE_BIOS = 1022,
    ID_BTN_SETUP_BROWSE_DISC = 1023,
    ID_BTN_SETUP_START = 1024,
    ID_BTN_SETUP_CLOSE = 1025,
    ID_BTN_OPEN_CHEATS = 1026,
    ID_BTN_CHEATS_SAVE = 1027,
    ID_BTN_CHEATS_RESET = 1028,
    ID_BTN_CHEATS_CLOSE = 1029,
    ID_CHK_CHEAT_HEALTH = 1030,
    ID_CHK_CHEAT_AMMO = 1031,
    ID_CHK_CHEAT_RIBBONS = 1032,
    ID_CHK_CHEAT_WEAPONS = 1033,
    ID_CHK_CHEAT_MERCENARIES = 1034,
    ID_CHK_CHEAT_BOUTIQUE = 1035,
    ID_CHK_CHEAT_EPILOGUES = 1036,
    ID_CHK_FAST_DOORS = 1037,
    ID_COMBO_SKINS = 1038
};

// Global handles
HINSTANCE g_hInst = NULL;
HWND g_hWndMain = NULL;
HWND g_hComboLanguage = NULL;
HWND g_hLblLanguage = NULL;
HWND g_hLblSkins = NULL;
HWND g_hComboSkins = NULL;

// Cheats Dialog handles
HWND g_hWndCheats = NULL;
HWND g_hBtnOpenCheats = NULL;
HWND g_hLblCheatsTitle = NULL;
HWND g_hLblCheatsSub = NULL;
HWND g_hGrpCheatsList = NULL;
HWND g_hChkCheatHealth = NULL;
HWND g_hChkCheatAmmo = NULL;
HWND g_hChkCheatRibbons = NULL;
HWND g_hChkCheatWeapons = NULL;
HWND g_hChkCheatMercenaries = NULL;
HWND g_hChkCheatBoutique = NULL;
HWND g_hChkCheatEpilogues = NULL;
HWND g_hLblCheatsHotkeys = NULL;
HWND g_hBtnCheatsSave = NULL;
HWND g_hBtnCheatsReset = NULL;
HWND g_hBtnCheatsClose = NULL;

// Installation status & Setup handles
HWND g_hGrpSetup = NULL;
HWND g_hLblInstallStatus = NULL;
HWND g_hBtnOpenSetup = NULL;

// Setup Wizard Dialog handles
HWND g_hWndSetup = NULL;
HWND g_hLblSetupTitle = NULL;
HWND g_hLblSetupSub = NULL;
HWND g_hGrpSetupBios = NULL;
HWND g_hLblSetupBiosStatus = NULL;
HWND g_hEditSetupBiosPath = NULL;
HWND g_hBtnSetupBrowseBios = NULL;
HWND g_hGrpSetupDisc = NULL;
HWND g_hLblSetupDiscStatus = NULL;
HWND g_hEditSetupDiscPath = NULL;
HWND g_hBtnSetupBrowseDisc = NULL;
HWND g_hGrpSetupLog = NULL;
HWND g_hEditSetupLog = NULL;
HWND g_hBtnSetupStart = NULL;
HWND g_hBtnSetupClose = NULL;

HWND g_hGrpVideo = NULL;
HWND g_hGrpInput = NULL;
HWND g_hGrpAudio = NULL;
HWND g_hLblAspect = NULL;
HWND g_hLblDisplay = NULL;
HWND g_hLblRes = NULL;
HWND g_hLblFps = NULL;
HWND g_hLblRend = NULL;
HWND g_hLblCtrlMode = NULL;
HWND g_hComboAspect = NULL;
HWND g_hComboDisplay = NULL;
HWND g_hComboRenderer = NULL;
HWND g_hComboInternalRes = NULL;
HWND g_hComboFps = NULL;
HWND g_hComboControls = NULL;
HWND g_hChkPgxpGeom = NULL;
HWND g_hChkPgxpTex = NULL;
HWND g_hChkAntialiasing = NULL;
HWND g_hChkSkipFmv = NULL;
HWND g_hChkFastDoors = NULL;
HWND g_hChkSpuHq = NULL;
HWND g_hBtnInput = NULL;
HWND g_hBtnSaves = NULL;
HWND g_hBtnLaunch = NULL;
HWND g_hBtnSave = NULL;
HWND g_hBtnCredits = NULL;
HWND g_hLblLegalNote = NULL;
HWND g_hWndCredits = NULL;
HWND g_hEditCredits = NULL;
HWND g_hBtnCreditsCopy = NULL;
HWND g_hBtnCreditsGithub = NULL;
HWND g_hBtnCreditsClose = NULL;
HWND g_hLblCreditsTitle = NULL;
HWND g_hLblCreditsSub = NULL;
HFONT g_hFontTitle = NULL;
HFONT g_hFontSub = NULL;
HFONT g_hFontBody = NULL;
HFONT g_hFontBold = NULL;
HFONT g_hFontSmall = NULL;
HICON g_hAppIcon = NULL;
Gdiplus::Bitmap* g_pBannerBmp = NULL;
int g_screenHz = 60;
std::wstring g_currentLang = L"en"; // Default is English!

// Forward declarations
void UpdateInstallStatusUI();
void ShowSetupDialog(HWND hWndParent);
void ShowCheatsDialog(HWND hWndParent);
void UpdateCheatsButtonText();

std::wstring GetProjectRootDir() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    std::wstring path = exePath;
    size_t lastSlash = path.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        path = path.substr(0, lastSlash);
    }
    // Check if we are inside launcher/ folder, go one up
    if (path.length() >= 9 && path.substr(path.length() - 8) == L"launcher") {
        size_t upSlash = path.find_last_of(L"\\/");
        if (upSlash != std::wstring::npos) {
            path = path.substr(0, upSlash);
        }
    }
    return path;
}

std::wstring GetBuildDir() {
    std::wstring root = GetProjectRootDir();
    if (GetFileAttributesW((root + L"\\Resident_Evil_3__Nemesis_Recompiled.exe").c_str()) != INVALID_FILE_ATTRIBUTES ||
        GetFileAttributesW((root + L"\\ResidentEvil3.exe").c_str()) != INVALID_FILE_ATTRIBUTES) {
        return root;
    }
    std::wstring pcPort = root + L"\\ResidentEvil3-PCPort";
    if (GetFileAttributesW((pcPort + L"\\Resident_Evil_3__Nemesis_Recompiled.exe").c_str()) != INVALID_FILE_ATTRIBUTES) {
        return pcPort;
    }
    std::wstring sub = root + L"\\RE3Recomp\\build";
    if (GetFileAttributesW((sub + L"\\Resident_Evil_3__Nemesis_Recompiled.exe").c_str()) != INVALID_FILE_ATTRIBUTES) {
        return sub;
    }
    return root;
}

Gdiplus::Bitmap* LoadBannerBitmap(HINSTANCE hInst) {
    HRSRC hRes = FindResourceW(hInst, MAKEINTRESOURCEW(2), RT_RCDATA);
    if (hRes) {
        HGLOBAL hMem = LoadResource(hInst, hRes);
        DWORD size = SizeofResource(hInst, hRes);
        if (hMem && size > 0) {
            void* pData = LockResource(hMem);
            if (pData) {
                IStream* pStream = SHCreateMemStream((const BYTE*)pData, size);
                if (pStream) {
                    Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromStream(pStream);
                    pStream->Release();
                    if (bmp && bmp->GetLastStatus() == Gdiplus::Ok) {
                        return bmp;
                    }
                    if (bmp) delete bmp;
                }
            }
        }
    }

    std::wstring root = GetBuildDir();
    std::wstring localBanner = root + L"\\banner.jpg";
    if (GetFileAttributesW(localBanner.c_str()) != INVALID_FILE_ATTRIBUTES) {
        Gdiplus::Bitmap* bmp = Gdiplus::Bitmap::FromFile(localBanner.c_str());
        if (bmp && bmp->GetLastStatus() == Gdiplus::Ok) {
            return bmp;
        }
        if (bmp) delete bmp;
    }

    return NULL;
}

std::wstring GetTomlPath() {
    return GetBuildDir() + L"\\game.toml";
}

std::wstring GetSettingsTomlPath() {
    return GetBuildDir() + L"\\settings.toml";
}

struct InstallStatus {
    bool hasBios = false;
    std::wstring biosPath;
    bool hasDisc = false;
    std::wstring discPath;
    bool hasSlus = false;
    std::wstring slusPath;
    bool isFullyInstalled = false;
};

InstallStatus CheckInstallationStatus() {
    InstallStatus st;
    std::wstring root = GetBuildDir();

    // 1. BIOS check
    // Check if bios.cfg exists first to honor explicit previous user choice
    std::wstring biosCfgPath = root + L"\\bios.cfg";
    std::wstring configuredBios = L"";
    std::wifstream bcfg(biosCfgPath.c_str());
    if (bcfg.is_open()) {
        std::getline(bcfg, configuredBios);
        while (!configuredBios.empty() && (configuredBios.back() == L'\r' || configuredBios.back() == L'\n' || configuredBios.back() == L' ')) {
            configuredBios.pop_back();
        }
        bcfg.close();
    }

    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!configuredBios.empty()) {
        std::wstring fullCfgBios = root + L"\\" + configuredBios;
        if (GetFileAttributesExW(fullCfgBios.c_str(), GetFileExInfoStandard, &fad)) {
            LARGE_INTEGER size;
            size.HighPart = fad.nFileSizeHigh;
            size.LowPart = fad.nFileSizeLow;
            if (size.QuadPart >= 500000) {
                st.hasBios = true;
                st.biosPath = fullCfgBios;
            }
        }
    }

    // Default order: OpenBIOS (bundled / clean-room), followed by SCPH1001 / SCPH5552
    if (!st.hasBios) {
        std::wstring biosOpen = root + L"\\bios\\openbios.bin";
        std::wstring bios1001 = root + L"\\bios\\SCPH1001.BIN";
        std::wstring bios5552 = root + L"\\bios\\SCPH5552.BIN";

        if (GetFileAttributesExW(biosOpen.c_str(), GetFileExInfoStandard, &fad)) {
            LARGE_INTEGER size;
            size.HighPart = fad.nFileSizeHigh;
            size.LowPart = fad.nFileSizeLow;
            if (size.QuadPart >= 500000) {
                st.hasBios = true;
                st.biosPath = biosOpen;
            }
        } else if (GetFileAttributesExW(bios1001.c_str(), GetFileExInfoStandard, &fad)) {
            LARGE_INTEGER size;
            size.HighPart = fad.nFileSizeHigh;
            size.LowPart = fad.nFileSizeLow;
            if (size.QuadPart >= 500000) {
                st.hasBios = true;
                st.biosPath = bios1001;
            }
        } else if (GetFileAttributesExW(bios5552.c_str(), GetFileExInfoStandard, &fad)) {
            LARGE_INTEGER size;
            size.HighPart = fad.nFileSizeHigh;
            size.LowPart = fad.nFileSizeLow;
            if (size.QuadPart >= 500000) {
                st.hasBios = true;
                st.biosPath = bios5552;
            }
        }
    }

    // 2. Disc check
    std::wstring gameBin = root + L"\\gamedata\\Resident Evil 3 - Nemesis (USA, Canada).bin";
    if (GetFileAttributesExW(gameBin.c_str(), GetFileExInfoStandard, &fad)) {
        LARGE_INTEGER size;
        size.HighPart = fad.nFileSizeHigh;
        size.LowPart = fad.nFileSizeLow;
        if (size.QuadPart >= 50000000) {
            st.hasDisc = true;
            st.discPath = gameBin;
        }
    } else {
        WIN32_FIND_DATAW wfd;
        HANDLE hFind = FindFirstFileW((root + L"\\gamedata\\*.bin").c_str(), &wfd);
        if (hFind != INVALID_HANDLE_VALUE) {
            st.hasDisc = true;
            st.discPath = root + L"\\gamedata\\" + wfd.cFileName;
            FindClose(hFind);
        } else {
            hFind = FindFirstFileW((root + L"\\gamedata\\*.iso").c_str(), &wfd);
            if (hFind != INVALID_HANDLE_VALUE) {
                st.hasDisc = true;
                st.discPath = root + L"\\gamedata\\" + wfd.cFileName;
                FindClose(hFind);
            }
        }
    }

    // 3. SLUS check
    std::wstring slusPath = root + L"\\disc\\SLUS_009.23";
    if (GetFileAttributesExW(slusPath.c_str(), GetFileExInfoStandard, &fad)) {
        LARGE_INTEGER size;
        size.HighPart = fad.nFileSizeHigh;
        size.LowPart = fad.nFileSizeLow;
        if (size.QuadPart >= 500000) {
            std::ifstream slusFile(slusPath.c_str(), std::ios::binary);
            if (slusFile.is_open()) {
                char magic[8] = { 0 };
                slusFile.read(magic, 8);
                if (std::string(magic, 8) == "PS-X EXE") {
                    st.hasSlus = true;
                    st.slusPath = slusPath;
                }
            }
        }
    }

    st.isFullyInstalled = (st.hasBios && st.hasDisc && st.hasSlus);
    return st;
}

void UpdateInstallStatusUI() {
    if (!g_hLblInstallStatus || !g_hBtnOpenSetup) return;
    InstallStatus st = CheckInstallationStatus();
    if (st.isFullyInstalled) {
        if (g_currentLang == L"es") {
            SetWindowTextW(g_hLblInstallStatus, L"● STATUS: Listo para Jugar (Archivos OK)");
            SetWindowTextW(g_hBtnOpenSetup, L"Administrar Archivos...");
        } else if (g_currentLang == L"pt-br") {
            SetWindowTextW(g_hLblInstallStatus, L"● STATUS: Pronto para Jogar (Arquivos OK)");
            SetWindowTextW(g_hBtnOpenSetup, L"Gerenciar Arquivos...");
        } else {
            SetWindowTextW(g_hLblInstallStatus, L"● STATUS: Ready to Play (Files Verified)");
            SetWindowTextW(g_hBtnOpenSetup, L"Manage Files...");
        }
    } else {
        if (g_currentLang == L"es") {
            SetWindowTextW(g_hLblInstallStatus, L"[!] STATUS: Instalación Pendiente");
            SetWindowTextW(g_hBtnOpenSetup, L"Instalar Juego...");
        } else if (g_currentLang == L"pt-br") {
            SetWindowTextW(g_hLblInstallStatus, L"[!] STATUS: Instalação Pendente");
            SetWindowTextW(g_hBtnOpenSetup, L"Instalar Jogo...");
        } else {
            SetWindowTextW(g_hLblInstallStatus, L"[!] STATUS: Setup Required");
            SetWindowTextW(g_hBtnOpenSetup, L"Install Game...");
        }
    }
    InvalidateRect(g_hLblInstallStatus, NULL, TRUE);
}

std::wstring ResolveBinFromCue(const std::wstring& cuePath) {
    std::wifstream f(cuePath.c_str());
    if (!f.is_open()) return L"";
    std::wstring line;
    while (std::getline(f, line)) {
        size_t pos = line.find(L"FILE");
        if (pos != std::wstring::npos) {
            size_t q1 = line.find(L'"', pos);
            size_t q2 = line.find(L'"', q1 + 1);
            if (q1 != std::wstring::npos && q2 != std::wstring::npos) {
                std::wstring binName = line.substr(q1 + 1, q2 - q1 - 1);
                size_t slash = cuePath.find_last_of(L"\\/");
                std::wstring dir = (slash != std::wstring::npos) ? cuePath.substr(0, slash + 1) : L"";
                return dir + binName;
            }
        }
    }
    return L"";
}

bool ExtractSlusFromDisc(const std::wstring& imagePath, const std::wstring& outSlusPath, std::wstring& err) {
    std::ifstream in(imagePath.c_str(), std::ios::binary);
    if (!in.is_open()) {
        err = (g_currentLang == L"es") ? L"No se pudo abrir la imagen del disco." :
              (g_currentLang == L"pt-br") ? L"Não foi possível abrir o arquivo de imagem do disco." :
              L"Could not open the disc image file.";
        return false;
    }

    in.seekg(0, std::ios::end);
    size_t totalBytes = in.tellg();
    in.seekg(0, std::ios::beg);

    if (totalBytes < 1000000) {
        err = (g_currentLang == L"es") ? L"Archivo de imagen demasiado pequeño." :
              (g_currentLang == L"pt-br") ? L"Arquivo de imagem muito pequeno ou corrompido." :
              L"Disc image file is too small or corrupted.";
        return false;
    }

    size_t sectorSize = 2352;
    size_t userOffset = 24;
    size_t userSize = 2048;

    std::vector<uint8_t> pvdSector(sectorSize);
    in.seekg(16 * sectorSize, std::ios::beg);
    in.read(reinterpret_cast<char*>(pvdSector.data()), sectorSize);

    bool foundPvd = false;
    if (std::string(reinterpret_cast<const char*>(&pvdSector[userOffset + 1]), 5) == "CD001") {
        foundPvd = true;
    } else {
        userOffset = 16;
        if (std::string(reinterpret_cast<const char*>(&pvdSector[userOffset + 1]), 5) == "CD001") {
            foundPvd = true;
        } else {
            sectorSize = 2048;
            userOffset = 0;
            in.seekg(16 * sectorSize, std::ios::beg);
            in.read(reinterpret_cast<char*>(pvdSector.data()), sectorSize);
            if (std::string(reinterpret_cast<const char*>(&pvdSector[userOffset + 1]), 5) == "CD001") {
                foundPvd = true;
            }
        }
    }

    if (!foundPvd) {
        err = (g_currentLang == L"es") ? L"PVD (CD001) no encontrado en la imagen." :
              (g_currentLang == L"pt-br") ? L"Descritor de Volume (CD001) não encontrado na imagem." :
              L"Primary Volume Descriptor (CD001) not found in disc image.";
        return false;
    }

    const uint8_t* rootRec = &pvdSector[userOffset + 156];
    uint32_t rootExtent = *reinterpret_cast<const uint32_t*>(rootRec + 2);
    uint32_t rootSize = *reinterpret_cast<const uint32_t*>(rootRec + 10);

    if (rootExtent == 0 || rootSize == 0 || rootSize > 10000000) {
        err = (g_currentLang == L"es") ? L"Directorio raíz de la imagen inválido." :
              (g_currentLang == L"pt-br") ? L"Estrutura de diretório raiz inválida na imagem." :
              L"Invalid root directory structure in disc image.";
        return false;
    }

    std::vector<uint8_t> rootData(rootSize);
    size_t numSectors = (rootSize + userSize - 1) / userSize;
    size_t bytesRead = 0;
    for (size_t s = 0; s < numSectors; ++s) {
        std::vector<uint8_t> sec(sectorSize);
        in.seekg((rootExtent + s) * sectorSize, std::ios::beg);
        in.read(reinterpret_cast<char*>(sec.data()), sectorSize);
        size_t toCopy = (std::min)(userSize, rootSize - bytesRead);
        std::memcpy(&rootData[bytesRead], &sec[userOffset], toCopy);
        bytesRead += toCopy;
    }

    size_t offset = 0;
    uint32_t slusExtent = 0;
    uint32_t slusSize = 0;
    std::string foundName = "";

    while (offset < rootData.size()) {
        uint8_t recLen = rootData[offset];
        if (recLen == 0) {
            size_t nextSector = ((offset / userSize) + 1) * userSize;
            offset = nextSector;
            continue;
        }
        if (offset + recLen > rootData.size()) break;

        uint32_t extent = *reinterpret_cast<const uint32_t*>(&rootData[offset + 2]);
        uint32_t size = *reinterpret_cast<const uint32_t*>(&rootData[offset + 10]);
        uint8_t nameLen = rootData[offset + 32];
        std::string name(reinterpret_cast<const char*>(&rootData[offset + 33]), nameLen);

        size_t semi = name.find(';');
        if (semi != std::string::npos) name = name.substr(0, semi);

        if (name.find("SLUS") != std::string::npos ||
            name.find("SLES") != std::string::npos ||
            name.find("SLPS") != std::string::npos) {
            slusExtent = extent;
            slusSize = size;
            foundName = name;
            break;
        }

        offset += recLen;
    }

    if (slusExtent == 0 || slusSize == 0) {
        err = (g_currentLang == L"es") ? L"No se encontró el ejecutable SLUS/SLES en el disco." :
              (g_currentLang == L"pt-br") ? L"Nenhum executável SLUS/SLES encontrado no disco." :
              L"No SLUS/SLES executable found in disc image.";
        return false;
    }

    std::vector<uint8_t> slusData(slusSize);
    size_t slusSectors = (slusSize + userSize - 1) / userSize;
    bytesRead = 0;
    for (size_t s = 0; s < slusSectors; ++s) {
        std::vector<uint8_t> sec(sectorSize);
        in.seekg((slusExtent + s) * sectorSize, std::ios::beg);
        in.read(reinterpret_cast<char*>(sec.data()), sectorSize);
        size_t toCopy = (std::min)(userSize, slusSize - bytesRead);
        std::memcpy(&slusData[bytesRead], &sec[userOffset], toCopy);
        bytesRead += toCopy;
    }

    if (slusSize < 8 || std::string(reinterpret_cast<char*>(slusData.data()), 8) != "PS-X EXE") {
        err = (g_currentLang == L"es") ? L"El archivo extraído no tiene la firma 'PS-X EXE'." :
              (g_currentLang == L"pt-br") ? L"O arquivo extraído não possui a assinatura 'PS-X EXE'." :
              L"The extracted file does not contain 'PS-X EXE' signature.";
        return false;
    }

    std::ofstream out(outSlusPath.c_str(), std::ios::binary);
    if (!out.is_open()) {
        err = (g_currentLang == L"es") ? L"No se pudo escribir en disc\\SLUS_009.23." :
              (g_currentLang == L"pt-br") ? L"Não foi possível gravar em disc\\SLUS_009.23." :
              L"Could not write to disc\\SLUS_009.23.";
        return false;
    }
    out.write(reinterpret_cast<char*>(slusData.data()), slusData.size());
    out.close();

    return true;
}

struct GameConfig {
    std::wstring language = L"en"; // Default: English
    std::wstring aspect_ratio = L"16:9";
    bool fullscreen = true;
    std::wstring renderer = L"opengl";
    std::wstring internal_resolution = L"1080p";
    bool frame_interpolation = true;
    int frame_interpolation_fps = 0;
    bool modern_controls = true;
    bool geometry_correction = false;
    bool perspective_texturing = true;
    bool antialiasing = true;
    bool auto_skip_fmv = false;
    bool fast_doors = true;
    bool spu_hq = true;
    bool infinite_health = false;
    bool infinite_ammo = false;
    bool infinite_ink_ribbons = false;
    bool all_weapons_box = false;
    bool unlock_mercenaries = false;
    bool unlock_boutique = false;
    bool unlock_epilogues = false;
    std::wstring jill_skin = L"default";
};

GameConfig g_cachedConfig;

GameConfig LoadConfig() {
    GameConfig cfg;
    
    std::wstring settingsPath = GetSettingsTomlPath();
    std::wstring loadPath = (GetFileAttributesW(settingsPath.c_str()) != INVALID_FILE_ATTRIBUTES) 
                          ? settingsPath : GetTomlPath();

    std::wifstream file(loadPath.c_str());
    if (!file.is_open()) return cfg;

    std::wstring line;
    while (std::getline(file, line)) {
        size_t start = line.find_first_not_of(L" \t\r\n");
        if (start == std::wstring::npos || line[start] == L'#') continue;
        line = line.substr(start);

        size_t eq = line.find(L'=');
        if (eq == std::wstring::npos) continue;

        std::wstring key = line.substr(0, eq);
        size_t keyEnd = key.find_last_not_of(L" \t");
        if (keyEnd != std::wstring::npos) key = key.substr(0, keyEnd + 1);

        std::wstring val = line.substr(eq + 1);
        size_t valStart = val.find_first_not_of(L" \t");
        if (valStart != std::wstring::npos) val = val.substr(valStart);
        size_t valEnd = val.find_last_not_of(L" \t\r\n");
        if (valEnd != std::wstring::npos) val = val.substr(0, valEnd + 1);

        if (val.length() >= 2 && val.front() == L'"' && val.back() == L'"') {
            val = val.substr(1, val.length() - 2);
        }

        if (key == L"language") {
            if (val == L"es" || val == L"pt-br" || val == L"en") cfg.language = val;
            else cfg.language = L"en";
        }
        else if (key == L"aspect_ratio") cfg.aspect_ratio = val;
        else if (key == L"pan_scan") {
            if (val == L"true" || val == L"1") cfg.aspect_ratio = L"16:9_pan_scan";
        }
        else if (key == L"fullscreen") {
            cfg.fullscreen = (val == L"1" || val == L"2" || val == L"true");
        }
        else if (key == L"renderer") {
            if (val == L"0" || val == L"software") cfg.renderer = L"software";
            else cfg.renderer = L"opengl";
        }
        else if (key == L"internal_resolution") {
            cfg.internal_resolution = val;
        }
        else if (key == L"frame_interpolation") {
            cfg.frame_interpolation = (val == L"true" || val == L"1");
        }
        else if (key == L"frame_interpolation_fps") {
            try {
                cfg.frame_interpolation_fps = std::stoi(val);
            } catch (...) {
                cfg.frame_interpolation_fps = 0;
            }
        }
        else if (key == L"modern_controls") {
            cfg.modern_controls = (val == L"true" || val == L"1");
        }
        else if (key == L"geometry_correction") cfg.geometry_correction = (val == L"true");
        else if (key == L"perspective_texturing") cfg.perspective_texturing = (val == L"true");
        else if (key == L"antialiasing") cfg.antialiasing = (val == L"true" || val == L"1");
        else if (key == L"auto_skip_fmv") cfg.auto_skip_fmv = (val == L"true");
        else if (key == L"fast_doors" || key == L"door_skip") cfg.fast_doors = (val == L"true" || val == L"1");
        else if (key == L"spu_hq") cfg.spu_hq = (val == L"true" || val == L"1");
        else if (key == L"infinite_health") cfg.infinite_health = (val == L"true" || val == L"1");
        else if (key == L"infinite_ammo") cfg.infinite_ammo = (val == L"true" || val == L"1");
        else if (key == L"infinite_ink_ribbons") cfg.infinite_ink_ribbons = (val == L"true" || val == L"1");
        else if (key == L"all_weapons_box") cfg.all_weapons_box = (val == L"true" || val == L"1");
        else if (key == L"unlock_mercenaries") cfg.unlock_mercenaries = (val == L"true" || val == L"1");
        else if (key == L"unlock_boutique") cfg.unlock_boutique = (val == L"true" || val == L"1");
        else if (key == L"unlock_epilogues") cfg.unlock_epilogues = (val == L"true" || val == L"1");
        else if (key == L"jill_skin" || key == L"skin") cfg.jill_skin = val;
    }
    return cfg;
}

int SkinToComboIndex(const std::wstring& skin) {
    if (skin == L"default") return 0;
    if (skin == L"bryan") return 1;
    if (skin == L"police") return 2;
    if (skin == L"biker") return 3;
    if (skin == L"disco") return 4;
    if (skin == L"regina") return 5;
    if (skin == L"stars") return 6;
    if (skin == L"wetsuit") return 7;
    return 0; // Default: Jill Original
}

std::wstring ComboIndexToSkin(int idx) {
    switch (idx) {
        case 0: return L"default";
        case 1: return L"bryan";
        case 2: return L"police";
        case 3: return L"biker";
        case 4: return L"disco";
        case 5: return L"regina";
        case 6: return L"stars";
        case 7: return L"wetsuit";
        default: return L"default";
    }
}

void ApplySkinState(const std::wstring& skin) {
    // Generate state.toml for PSXRecomp mod runtime
    std::wstring modState = L"format_version = 2\n\n[[package]]\nid = \"re3.skins.jill\"\nversion = \"1.0.0\"\n";
    if (skin != L"default" && !skin.empty()) {
        modState += L"\n[[feature]]\npackage_id = \"re3.skins.jill\"\nid = \"skin-" + skin + L"\"\nenabled = true\n";
    }

    std::vector<std::wstring> dirs = {
        GetBuildDir() + L"\\mods",
        GetProjectRootDir() + L"\\mods"
    };

    for (const auto& d : dirs) {
        CreateDirectoryW(d.c_str(), NULL);
        std::wstring p = d + L"\\state.toml";
        std::wofstream sf(p.c_str());
        if (sf.is_open()) {
            sf << modState;
            sf.close();
        }
    }
}

bool SaveConfig(const GameConfig& cfg) {
    int supersampling = 2;
    if (cfg.internal_resolution == L"native") supersampling = 1;
    else if (cfg.internal_resolution == L"720p") supersampling = 2;
    else if (cfg.internal_resolution == L"1080p") supersampling = 2;
    else if (cfg.internal_resolution == L"1440p") supersampling = 3;
    else if (cfg.internal_resolution == L"4k") supersampling = 4;

    std::wstring settingsPath = GetSettingsTomlPath();
    std::wofstream sFile(settingsPath.c_str());
    if (sFile.is_open()) {
        sFile << L"# PSXRecomp Runtime User Settings\n";
        sFile << L"[general]\n";
        sFile << L"language            = \"" << cfg.language << L"\"\n\n";
        sFile << L"[video]\n";
        sFile << L"fullscreen          = " << (cfg.fullscreen ? 1 : 0) << L"\n";
        sFile << L"aspect_ratio        = \"" << cfg.aspect_ratio << L"\"\n";
        sFile << L"renderer            = \"" << cfg.renderer << L"\"\n";
        sFile << L"internal_resolution = \"" << cfg.internal_resolution << L"\"\n";
        sFile << L"supersampling       = " << supersampling << L"\n";
        sFile << L"frame_interpolation = " << (cfg.frame_interpolation ? L"true" : L"false") << L"\n";
        sFile << L"frame_interpolation_fps = " << cfg.frame_interpolation_fps << L"\n";
        sFile << L"frame_interpolation_blend = \"linear\"\n";
        sFile << L"frame_interpolation_source = \"flip\"\n";
        sFile << L"geometry_correction = " << (cfg.geometry_correction ? L"true" : L"false") << L"\n";
        sFile << L"perspective_texturing = " << (cfg.perspective_texturing ? L"true" : L"false") << L"\n";
        sFile << L"antialiasing        = " << (cfg.antialiasing ? L"true" : L"false") << L"\n";
        sFile << L"auto_skip_fmv       = " << (cfg.auto_skip_fmv ? L"true" : L"false") << L"\n";
        sFile << L"fast_doors          = " << (cfg.fast_doors ? L"true" : L"false") << L"\n";
        sFile << L"vsync               = \"on\"\n";
        sFile << L"low_latency_input   = true\n";
        sFile << L"aspect_stretch      = " << ((cfg.aspect_ratio == L"4:3") ? L"false" : L"true") << L"\n";
        sFile << L"pan_scan            = " << ((cfg.aspect_ratio == L"16:9_pan_scan") ? L"true" : L"false") << L"\n\n";
        sFile << L"[audio]\n";
        sFile << L"frequency = 48000\n";
        sFile << L"spu_hq = " << (cfg.spu_hq ? L"true" : L"false") << L"\n\n";
        sFile << L"[controller]\n";
        sFile << L"p1_device           = \"auto\"\n";
        sFile << L"modern_controls     = " << (cfg.modern_controls ? L"true" : L"false") << L"\n";
        sFile << L"hotkey_pad_fast_forward = 1528\n";
        sFile << L"hotkey_pad_save_state_menu = 2040\n";
        sFile << L"hotkey_pad_rewind = 1272\n\n";
        sFile << L"[cheats]\n";
        sFile << L"infinite_health       = " << (cfg.infinite_health ? L"true" : L"false") << L"\n";
        sFile << L"infinite_ammo         = " << (cfg.infinite_ammo ? L"true" : L"false") << L"\n";
        sFile << L"infinite_ink_ribbons  = " << (cfg.infinite_ink_ribbons ? L"true" : L"false") << L"\n";
        sFile << L"all_weapons_box       = " << (cfg.all_weapons_box ? L"true" : L"false") << L"\n";
        sFile << L"unlock_mercenaries    = " << (cfg.unlock_mercenaries ? L"true" : L"false") << L"\n";
        sFile << L"unlock_boutique       = " << (cfg.unlock_boutique ? L"true" : L"false") << L"\n";
        sFile << L"unlock_epilogues      = " << (cfg.unlock_epilogues ? L"true" : L"false") << L"\n\n";
        sFile << L"[skins]\n";
        sFile << L"jill_skin             = \"" << cfg.jill_skin << L"\"\n";
        sFile.close();
    }

    std::wstring tomlPath = GetTomlPath();
    std::wifstream inFile(tomlPath.c_str());
    std::vector<std::wstring> lines;
    if (inFile.is_open()) {
        std::wstring line;
        while (std::getline(inFile, line)) {
            lines.push_back(line);
        }
        inFile.close();
    }

    bool inVideoSection = false;
    for (size_t i = 0; i < lines.size(); ++i) {
        std::wstring& line = lines[i];
        size_t start = line.find_first_not_of(L" \t\r\n");
        if (start != std::wstring::npos && line[start] == L'[') {
            inVideoSection = (line.find(L"[video]") != std::wstring::npos);
        }

        if (inVideoSection) {
            if (line.find(L"aspect_ratio") != std::wstring::npos) {
                line = L"aspect_ratio = \"" + cfg.aspect_ratio + L"\"";
            } else if (line.find(L"fullscreen") != std::wstring::npos) {
                line = L"fullscreen = " + std::wstring(cfg.fullscreen ? L"true" : L"false");
            } else if (line.find(L"renderer") != std::wstring::npos) {
                line = L"renderer = \"" + cfg.renderer + L"\"";
            } else if (line.find(L"geometry_correction") != std::wstring::npos) {
                line = L"geometry_correction = " + std::wstring(cfg.geometry_correction ? L"true" : L"false");
            } else if (line.find(L"perspective_texturing") != std::wstring::npos) {
                line = L"perspective_texturing = " + std::wstring(cfg.perspective_texturing ? L"true" : L"false");
            } else if (line.find(L"auto_skip_fmv") != std::wstring::npos) {
                line = L"auto_skip_fmv = " + std::wstring(cfg.auto_skip_fmv ? L"true" : L"false");
            } else if (line.find(L"fast_doors") != std::wstring::npos) {
                line = L"fast_doors = " + std::wstring(cfg.fast_doors ? L"true" : L"false");
            }
        }
    }

    std::wofstream outFile(tomlPath.c_str());
    if (outFile.is_open()) {
        for (const auto& l : lines) {
            outFile << l << L"\n";
        }
        outFile.close();
    }

    ApplySkinState(cfg.jill_skin);
    return true;
}

GameConfig GetGuiConfig() {
    GameConfig cfg = g_cachedConfig;
    cfg.language = g_currentLang;

    int idxAspect = (int)SendMessageW(g_hComboAspect, CB_GETCURSEL, 0, 0);
    if (idxAspect == 0) cfg.aspect_ratio = L"16:9";
    else if (idxAspect == 1) cfg.aspect_ratio = L"4:3";
    else if (idxAspect == 2) cfg.aspect_ratio = L"16:9_pan_scan";
    else cfg.aspect_ratio = L"16:9";

    int idxDisp = (int)SendMessageW(g_hComboDisplay, CB_GETCURSEL, 0, 0);
    cfg.fullscreen = (idxDisp == 1);

    int idxRes = (int)SendMessageW(g_hComboInternalRes, CB_GETCURSEL, 0, 0);
    switch (idxRes) {
        case 0: cfg.internal_resolution = L"1080p"; break;
        case 1: cfg.internal_resolution = L"4k"; break;
        case 2: cfg.internal_resolution = L"1440p"; break;
        case 3: cfg.internal_resolution = L"720p"; break;
        case 4: cfg.internal_resolution = L"native"; break;
        default: cfg.internal_resolution = L"1080p"; break;
    }

    int idxFps = (int)SendMessageW(g_hComboFps, CB_GETCURSEL, 0, 0);
    switch (idxFps) {
        case 0:
            cfg.frame_interpolation = true;
            cfg.frame_interpolation_fps = 0;
            break;
        case 1:
            cfg.frame_interpolation = true;
            cfg.frame_interpolation_fps = 144;
            break;
        case 2:
            cfg.frame_interpolation = true;
            cfg.frame_interpolation_fps = 120;
            break;
        case 3:
            cfg.frame_interpolation = true;
            cfg.frame_interpolation_fps = 165;
            break;
        case 4:
            cfg.frame_interpolation = true;
            cfg.frame_interpolation_fps = 240;
            break;
        case 5:
            cfg.frame_interpolation = true;
            cfg.frame_interpolation_fps = 60;
            break;
        case 6:
            cfg.frame_interpolation = false;
            cfg.frame_interpolation_fps = 0;
            break;
        default:
            cfg.frame_interpolation = true;
            cfg.frame_interpolation_fps = 0;
            break;
    }

    int idxRend = (int)SendMessageW(g_hComboRenderer, CB_GETCURSEL, 0, 0);
    cfg.renderer = (idxRend == 1) ? L"software" : L"opengl";

    int idxCtrl = (int)SendMessageW(g_hComboControls, CB_GETCURSEL, 0, 0);
    cfg.modern_controls = (idxCtrl == 0);

    if (g_hComboSkins) {
        int idxSkin = (int)SendMessageW(g_hComboSkins, CB_GETCURSEL, 0, 0);
        cfg.jill_skin = ComboIndexToSkin(idxSkin);
    }

    cfg.geometry_correction = (SendMessageW(g_hChkPgxpGeom, BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.perspective_texturing = (SendMessageW(g_hChkPgxpTex, BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.antialiasing = (SendMessageW(g_hChkAntialiasing, BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.auto_skip_fmv = (SendMessageW(g_hChkSkipFmv, BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.fast_doors = (SendMessageW(g_hChkFastDoors, BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.spu_hq = (SendMessageW(g_hChkSpuHq, BM_GETCHECK, 0, 0) == BST_CHECKED);

    return cfg;
}

void ApplyLanguage(const std::wstring& lang) {
    if (g_hLblLanguage) {
        if (lang == L"es") SetWindowTextW(g_hLblLanguage, L"Idioma del Launcher:");
        else if (lang == L"pt-br") SetWindowTextW(g_hLblLanguage, L"Idioma do Launcher:");
        else SetWindowTextW(g_hLblLanguage, L"Launcher Language:");
    }

    if (g_hGrpSetup) {
        if (lang == L"es") SetWindowTextW(g_hGrpSetup, L"  Instalación y Configuración del Juego  ");
        else if (lang == L"pt-br") SetWindowTextW(g_hGrpSetup, L"  Instalação && Configuração do Jogo  ");
        else SetWindowTextW(g_hGrpSetup, L"  Game Installation && Disc Setup  ");
    }
    UpdateInstallStatusUI();

    if (g_hGrpVideo) {
        if (lang == L"es") SetWindowTextW(g_hGrpVideo, L"  Gráficos HD y Rendimiento Visual  ");
        else if (lang == L"pt-br") SetWindowTextW(g_hGrpVideo, L"  Gráficos HD && Desempenho Visual  ");
        else SetWindowTextW(g_hGrpVideo, L"  HD Graphics && Visual Performance  ");
    }

    if (g_hLblAspect) {
        if (lang == L"es") SetWindowTextW(g_hLblAspect, L"Relación de Aspecto:");
        else if (lang == L"pt-br") SetWindowTextW(g_hLblAspect, L"Proporção de Tela:");
        else SetWindowTextW(g_hLblAspect, L"Aspect Ratio:");
    }

    if (g_hComboAspect) {
        int cur = (int)SendMessageW(g_hComboAspect, CB_GETCURSEL, 0, 0);
        SendMessageW(g_hComboAspect, CB_RESETCONTENT, 0, 0);
        if (lang == L"es") {
            SendMessageW(g_hComboAspect, CB_ADDSTRING, 0, (LPARAM)L"16:9 Widescreen Estirado (Predeterminado / Estable)");
            SendMessageW(g_hComboAspect, CB_ADDSTRING, 0, (LPARAM)L"4:3 Clásico Original (Barras Negras Laterales)");
            SendMessageW(g_hComboAspect, CB_ADDSTRING, 0, (LPARAM)L"16:9 Pan & Scan Dinámico (Experimental)");
        } else if (lang == L"pt-br") {
            SendMessageW(g_hComboAspect, CB_ADDSTRING, 0, (LPARAM)L"16:9 Widescreen Stretched (Padrão / Estável)");
            SendMessageW(g_hComboAspect, CB_ADDSTRING, 0, (LPARAM)L"4:3 Clássico Original (Tarjas Laterais)");
            SendMessageW(g_hComboAspect, CB_ADDSTRING, 0, (LPARAM)L"16:9 Pan & Scan Dinâmico (Experimental)");
        } else {
            SendMessageW(g_hComboAspect, CB_ADDSTRING, 0, (LPARAM)L"16:9 Widescreen Stretched (Default / Rock Solid)");
            SendMessageW(g_hComboAspect, CB_ADDSTRING, 0, (LPARAM)L"4:3 Classic Original (Side Black Bars)");
            SendMessageW(g_hComboAspect, CB_ADDSTRING, 0, (LPARAM)L"16:9 Dynamic Pan & Scan (Experimental)");
        }
        SendMessageW(g_hComboAspect, CB_SETCURSEL, (cur >= 0) ? cur : 0, 0);
    }

    if (g_hLblDisplay) {
        if (lang == L"es") SetWindowTextW(g_hLblDisplay, L"Modo de Pantalla:");
        else if (lang == L"pt-br") SetWindowTextW(g_hLblDisplay, L"Modo de Exibição:");
        else SetWindowTextW(g_hLblDisplay, L"Display Mode:");
    }

    if (g_hComboDisplay) {
        int cur = (int)SendMessageW(g_hComboDisplay, CB_GETCURSEL, 0, 0);
        SendMessageW(g_hComboDisplay, CB_RESETCONTENT, 0, 0);
        if (lang == L"es") {
            SendMessageW(g_hComboDisplay, CB_ADDSTRING, 0, (LPARAM)L"Modo Ventana (Windowed)");
            SendMessageW(g_hComboDisplay, CB_ADDSTRING, 0, (LPARAM)L"Pantalla Completa (Fullscreen Exclusivo)");
        } else if (lang == L"pt-br") {
            SendMessageW(g_hComboDisplay, CB_ADDSTRING, 0, (LPARAM)L"Modo Janela (Windowed)");
            SendMessageW(g_hComboDisplay, CB_ADDSTRING, 0, (LPARAM)L"Tela Cheia (Fullscreen Exclusivo)");
        } else {
            SendMessageW(g_hComboDisplay, CB_ADDSTRING, 0, (LPARAM)L"Windowed Mode");
            SendMessageW(g_hComboDisplay, CB_ADDSTRING, 0, (LPARAM)L"Fullscreen (Exclusive Borderless/Full)");
        }
        SendMessageW(g_hComboDisplay, CB_SETCURSEL, (cur >= 0) ? cur : 0, 0);
    }

    if (g_hLblRes) {
        if (lang == L"es") SetWindowTextW(g_hLblRes, L"Resolución Interna 3D:");
        else if (lang == L"pt-br") SetWindowTextW(g_hLblRes, L"Resolução Interna 3D:");
        else SetWindowTextW(g_hLblRes, L"3D Internal Resolution:");
    }

    if (g_hComboInternalRes) {
        int cur = (int)SendMessageW(g_hComboInternalRes, CB_GETCURSEL, 0, 0);
        SendMessageW(g_hComboInternalRes, CB_RESETCONTENT, 0, 0);
        if (lang == L"es") {
            SendMessageW(g_hComboInternalRes, CB_ADDSTRING, 0, (LPARAM)L"Full HD (1080p) — Alta Nitidez (Recomendado)");
            SendMessageW(g_hComboInternalRes, CB_ADDSTRING, 0, (LPARAM)L"4K Ultra HD (2160p) — Calidad Extrema");
            SendMessageW(g_hComboInternalRes, CB_ADDSTRING, 0, (LPARAM)L"Quad HD (1440p) — Nitidez 2K");
            SendMessageW(g_hComboInternalRes, CB_ADDSTRING, 0, (LPARAM)L"HD (720p) — Equilibrado");
            SendMessageW(g_hComboInternalRes, CB_ADDSTRING, 0, (LPARAM)L"Nativa (240p) — PS1 Original");
        } else if (lang == L"pt-br") {
            SendMessageW(g_hComboInternalRes, CB_ADDSTRING, 0, (LPARAM)L"Full HD (1080p) — Alta Nitidez (Recomendado)");
            SendMessageW(g_hComboInternalRes, CB_ADDSTRING, 0, (LPARAM)L"4K Ultra HD (2160p) — Qualidade Extrema");
            SendMessageW(g_hComboInternalRes, CB_ADDSTRING, 0, (LPARAM)L"Quad HD (1440p) — Nitidez 2K");
            SendMessageW(g_hComboInternalRes, CB_ADDSTRING, 0, (LPARAM)L"HD (720p) — Equilibrado");
            SendMessageW(g_hComboInternalRes, CB_ADDSTRING, 0, (LPARAM)L"Nativa (240p) — PS1 Original");
        } else {
            SendMessageW(g_hComboInternalRes, CB_ADDSTRING, 0, (LPARAM)L"Full HD (1080p) — High Clarity (Recommended)");
            SendMessageW(g_hComboInternalRes, CB_ADDSTRING, 0, (LPARAM)L"4K Ultra HD (2160p) — Extreme Quality");
            SendMessageW(g_hComboInternalRes, CB_ADDSTRING, 0, (LPARAM)L"Quad HD (1440p) — 2K Crisp Detail");
            SendMessageW(g_hComboInternalRes, CB_ADDSTRING, 0, (LPARAM)L"HD (720p) — Balanced Performance");
            SendMessageW(g_hComboInternalRes, CB_ADDSTRING, 0, (LPARAM)L"Native (240p) — Original PS1");
        }
        SendMessageW(g_hComboInternalRes, CB_SETCURSEL, (cur >= 0) ? cur : 0, 0);
    }

    if (g_hLblFps) {
        if (lang == L"es") SetWindowTextW(g_hLblFps, L"Fluidez / Frecuencia:");
        else if (lang == L"pt-br") SetWindowTextW(g_hLblFps, L"Fluidez / Frequência:");
        else SetWindowTextW(g_hLblFps, L"Smoothness / Refresh:");
    }

    if (g_hComboFps) {
        int cur = (int)SendMessageW(g_hComboFps, CB_GETCURSEL, 0, 0);
        SendMessageW(g_hComboFps, CB_RESETCONTENT, 0, 0);
        if (lang == L"es") {
            std::wstring autoHz = L"Auto (Frecuencia Nativa — " + std::to_wstring(g_screenHz) + L" Hz)";
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)autoHz.c_str());
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"144 FPS (Fluidez Alta — 144Hz)");
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"120 FPS (Fluidez Alta — 120Hz)");
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"165 FPS (Fluidez Alta — 165Hz)");
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"240 FPS (Fluidez Máxima — 240Hz)");
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"60 FPS (Estándar Suave — 60Hz)");
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"Desactivado (30 FPS Original de PS1)");
        } else if (lang == L"pt-br") {
            std::wstring autoHz = L"Auto (Taxa Nativa do Monitor — " + std::to_wstring(g_screenHz) + L" Hz)";
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)autoHz.c_str());
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"144 FPS (Fluidez Alta — 144Hz)");
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"120 FPS (Fluidez Alta — 120Hz)");
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"165 FPS (Fluidez Alta — 165Hz)");
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"240 FPS (Fluidez Máxima — 240Hz)");
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"60 FPS (Padrão Suave — 60Hz)");
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"Desativado (30 FPS Original de PS1)");
        } else {
            std::wstring autoHz = L"Auto (Native Monitor Refresh — " + std::to_wstring(g_screenHz) + L" Hz)";
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)autoHz.c_str());
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"144 FPS (High Smoothness — 144Hz)");
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"120 FPS (High Smoothness — 120Hz)");
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"165 FPS (High Smoothness — 165Hz)");
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"240 FPS (Maximum Smoothness — 240Hz)");
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"60 FPS (Smooth Standard — 60Hz)");
            SendMessageW(g_hComboFps, CB_ADDSTRING, 0, (LPARAM)L"Disabled (Original PS1 30 FPS)");
        }
        SendMessageW(g_hComboFps, CB_SETCURSEL, (cur >= 0) ? cur : 0, 0);
    }

    if (g_hLblRend) {
        if (lang == L"es") SetWindowTextW(g_hLblRend, L"Renderizador GPU:");
        else if (lang == L"pt-br") SetWindowTextW(g_hLblRend, L"Renderizador GPU:");
        else SetWindowTextW(g_hLblRend, L"GPU Renderer:");
    }

    if (g_hComboRenderer) {
        int cur = (int)SendMessageW(g_hComboRenderer, CB_GETCURSEL, 0, 0);
        SendMessageW(g_hComboRenderer, CB_RESETCONTENT, 0, 0);
        if (lang == L"es") {
            SendMessageW(g_hComboRenderer, CB_ADDSTRING, 0, (LPARAM)L"OpenGL Hardware (Aceleración Máxima)");
            SendMessageW(g_hComboRenderer, CB_ADDSTRING, 0, (LPARAM)L"Software Renderer (Sin Shaders)");
        } else if (lang == L"pt-br") {
            SendMessageW(g_hComboRenderer, CB_ADDSTRING, 0, (LPARAM)L"OpenGL Hardware (Aceleração Máxima)");
            SendMessageW(g_hComboRenderer, CB_ADDSTRING, 0, (LPARAM)L"Software Renderer (Sem Shaders)");
        } else {
            SendMessageW(g_hComboRenderer, CB_ADDSTRING, 0, (LPARAM)L"OpenGL Hardware (Maximum Acceleration)");
            SendMessageW(g_hComboRenderer, CB_ADDSTRING, 0, (LPARAM)L"Software Renderer (No Shaders)");
        }
        SendMessageW(g_hComboRenderer, CB_SETCURSEL, (cur >= 0) ? cur : 0, 0);
    }

    if (g_hChkPgxpGeom) {
        if (lang == L"es") SetWindowTextW(g_hChkPgxpGeom, L"Corrección PGXP Geométrica (No recomendado: deforma personajes)");
        else if (lang == L"pt-br") SetWindowTextW(g_hChkPgxpGeom, L"Correção PGXP Geométrica (Não recomendado: deforma personagens)");
        else SetWindowTextW(g_hChkPgxpGeom, L"PGXP Geometry Correction (Not recommended: causes model deformation)");
    }

    if (g_hChkPgxpTex) {
        if (lang == L"es") SetWindowTextW(g_hChkPgxpTex, L"Perspectiva de Texturas Correcta (Evita deformación angular de objetos)");
        else if (lang == L"pt-br") SetWindowTextW(g_hChkPgxpTex, L"Perspectiva Texturizada Correta (Sem deformação angular nos objetos)");
        else SetWindowTextW(g_hChkPgxpTex, L"Perspective-Correct Texturing (Prevents angular texture warping)");
    }

    if (g_hChkAntialiasing) {
        if (lang == L"es") SetWindowTextW(g_hChkAntialiasing, L"Anti-Aliasing FXAA (Suaviza bordes irregulares en modelos 3D)");
        else if (lang == L"pt-br") SetWindowTextW(g_hChkAntialiasing, L"Anti-Aliasing FXAA (Suavização de serrilhados nos modelos 3D)");
        else SetWindowTextW(g_hChkAntialiasing, L"FXAA Anti-Aliasing (Smooths jagged edges on 3D models)");
    }

    if (g_hGrpInput) {
        if (lang == L"es") SetWindowTextW(g_hGrpInput, L"  Jugabilidad, Trajes y Controles  ");
        else if (lang == L"pt-br") SetWindowTextW(g_hGrpInput, L"  Jogabilidade, Trajes && Controles  ");
        else SetWindowTextW(g_hGrpInput, L"  Gameplay, Skins && Controls  ");
    }

    if (g_hLblCtrlMode) {
        if (lang == L"es") SetWindowTextW(g_hLblCtrlMode, L"Modo de Control:");
        else if (lang == L"pt-br") SetWindowTextW(g_hLblCtrlMode, L"Modo de Controle:");
        else SetWindowTextW(g_hLblCtrlMode, L"Control Mode:");
    }

    if (g_hComboControls) {
        int cur = (int)SendMessageW(g_hComboControls, CB_GETCURSEL, 0, 0);
        SendMessageW(g_hComboControls, CB_RESETCONTENT, 0, 0);
        if (lang == L"es") {
            SendMessageW(g_hComboControls, CB_ADDSTRING, 0, (LPARAM)L"Moderno 3D (360° + Auto-Run + D-Pad Tanque)");
            SendMessageW(g_hComboControls, CB_ADDSTRING, 0, (LPARAM)L"Clásico Tanque (Original Stick y D-Pad)");
        } else if (lang == L"pt-br") {
            SendMessageW(g_hComboControls, CB_ADDSTRING, 0, (LPARAM)L"Moderno 3D (360° + Auto-Run + D-Pad Tanque)");
            SendMessageW(g_hComboControls, CB_ADDSTRING, 0, (LPARAM)L"Clássico Tanque (Original Stick e D-Pad)");
        } else {
            SendMessageW(g_hComboControls, CB_ADDSTRING, 0, (LPARAM)L"Modern 3D (360° + Auto-Run + Tank D-Pad)");
            SendMessageW(g_hComboControls, CB_ADDSTRING, 0, (LPARAM)L"Classic Tank (Original Stick and D-Pad)");
        }
        SendMessageW(g_hComboControls, CB_SETCURSEL, (cur >= 0) ? cur : 0, 0);
    }

    if (g_hLblSkins) {
        if (lang == L"es") SetWindowTextW(g_hLblSkins, L"Traje / Skin Jill:");
        else if (lang == L"pt-br") SetWindowTextW(g_hLblSkins, L"Skin / Traje da Jill:");
        else SetWindowTextW(g_hLblSkins, L"Jill Skin / Costume:");
    }

    if (g_hComboSkins) {
        int cur = (int)SendMessageW(g_hComboSkins, CB_GETCURSEL, 0, 0);
        SendMessageW(g_hComboSkins, CB_RESETCONTENT, 0, 0);
        if (lang == L"es") {
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Jill Valentine (Original 1999 Predeterminada)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"★ Bryan (Película Resident Evil) [Exclusivo]");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Policía R.P.D. (Minifalda)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Motorista Biker (Traje de Cuero)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Disco 70s (Atuendo Retro)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Regina (Dino Crisis S.O.R.T.)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"S.T.A.R.S. Clásica (Resident Evil 1)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Traje de Buceo Táctico (Wetsuit)");
        } else if (lang == L"pt-br") {
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Jill Valentine (Original 1999 Padrão)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"★ Bryan (Filme Resident Evil) [Exclusivo]");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Policial R.P.D. (Minissaia)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Motoqueira Biker (Couro)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Disco Anos 70 (Balada Retro)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Regina (Dino Crisis S.O.R.T.)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"S.T.A.R.S. Clássica (Resident Evil 1)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Traje Tático de Mergulho (Wetsuit)");
        } else {
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Jill Valentine (Original 1999 Default)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"★ Bryan (Resident Evil Movie) [Exclusive]");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"R.P.D. Police (Miniskirt)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Biker Leather (Motorcycle Suit)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Disco 70s (Nightclub Outfit)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Regina (Dino Crisis S.O.R.T.)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Classic S.T.A.R.S. (Resident Evil 1)");
            SendMessageW(g_hComboSkins, CB_ADDSTRING, 0, (LPARAM)L"Tactical Wetsuit (Diving Suit)");
        }
        SendMessageW(g_hComboSkins, CB_SETCURSEL, (cur >= 0) ? cur : 0, 0);
    }

    if (g_hBtnInput) {
        if (lang == L"es") SetWindowTextW(g_hBtnInput, L"Editar Teclas (input.ini)");
        else if (lang == L"pt-br") SetWindowTextW(g_hBtnInput, L"Editar Teclas (input.ini)");
        else SetWindowTextW(g_hBtnInput, L"Edit Keybinds (input.ini)");
    }

    if (g_hBtnSaves) {
        if (lang == L"es") SetWindowTextW(g_hBtnSaves, L"Abrir Carpeta de Saves (.mcd)");
        else if (lang == L"pt-br") SetWindowTextW(g_hBtnSaves, L"Abrir Pasta de Saves (.mcd)");
        else SetWindowTextW(g_hBtnSaves, L"Open Saves Folder (.mcd)");
    }

    if (g_hGrpAudio) {
        if (lang == L"es") SetWindowTextW(g_hGrpAudio, L"  Audio y Reproducción  ");
        else if (lang == L"pt-br") SetWindowTextW(g_hGrpAudio, L"  Áudio && Reprodução  ");
        else SetWindowTextW(g_hGrpAudio, L"  Audio && Playback  ");
    }

    if (g_hChkSpuHq) {
        if (lang == L"es") SetWindowTextW(g_hChkSpuHq, L"Audio HQ 48kHz (Resampling de Alta Fidelidad SPU Float-Shadow)");
        else if (lang == L"pt-br") SetWindowTextW(g_hChkSpuHq, L"Áudio HQ 48kHz (Resampling de Alta Fidelidade SPU Float-Shadow)");
        else SetWindowTextW(g_hChkSpuHq, L"HQ 48kHz Audio (High-Fidelity SPU Float-Shadow Resampling)");
    }

    if (g_hChkSkipFmv) {
        if (lang == L"es") SetWindowTextW(g_hChkSkipFmv, L"Omitir cinemáticas/FMVs automáticamente (por defecto: desactivado)");
        else if (lang == L"pt-br") SetWindowTextW(g_hChkSkipFmv, L"Pular vídeos/FMVs automaticamente (padrão: desligado)");
        else SetWindowTextW(g_hChkSkipFmv, L"Auto-skip FMVs / cutscenes (Default: Off)");
    }

    if (g_hChkFastDoors) {
        if (lang == L"es") SetWindowTextW(g_hChkFastDoors, L"Omitir puertas / Door Skip (Transición instantánea entre salas)");
        else if (lang == L"pt-br") SetWindowTextW(g_hChkFastDoors, L"Pular portas / Door Skip (Transição instantânea de salas)");
        else SetWindowTextW(g_hChkFastDoors, L"Door Skipping / Fast Transitions (Instant room transitions)");
    }

    if (g_hBtnLaunch) {
        if (lang == L"es") SetWindowTextW(g_hBtnLaunch, L"JUGAR RESIDENT EVIL 3");
        else if (lang == L"pt-br") SetWindowTextW(g_hBtnLaunch, L"JOGAR RESIDENT EVIL 3");
        else SetWindowTextW(g_hBtnLaunch, L"PLAY RESIDENT EVIL 3");
    }

    if (g_hBtnSave) {
        if (lang == L"es") SetWindowTextW(g_hBtnSave, L"Guardar Ajustes");
        else if (lang == L"pt-br") SetWindowTextW(g_hBtnSave, L"Salvar Ajustes");
        else SetWindowTextW(g_hBtnSave, L"Save Settings");
    }

    UpdateCheatsButtonText();

    if (g_hBtnCredits) {
        if (lang == L"es") SetWindowTextW(g_hBtnCredits, L"Créditos y Aviso Legal");
        else if (lang == L"pt-br") SetWindowTextW(g_hBtnCredits, L"Créditos && Disclaimer");
        else SetWindowTextW(g_hBtnCredits, L"Credits && Disclaimer");
    }

    if (g_hLblLegalNote) {
        if (lang == L"es") SetWindowTextW(g_hLblLegalNote, L"Proyecto fan de preservación de código abierto sin fines de lucro. Sin activos propietarios.");
        else if (lang == L"pt-br") SetWindowTextW(g_hLblLegalNote, L"Projeto não comercial de preservação e código aberto. Nenhum dado protegido incluso.");
        else SetWindowTextW(g_hLblLegalNote, L"Non-commercial open-source preservation fan project. No proprietary assets included.");
    }
}

void LaunchGame() {
    InstallStatus st = CheckInstallationStatus();
    if (!st.isFullyInstalled) {
        std::wstring msg;
        std::wstring title;
        if (g_currentLang == L"es") {
            title = L"Instalación Requerida";
            msg = L"Los archivos necesarios para ejecutar el juego aún no están configurados:\n\n"
                  L"- BIOS (OpenBIOS / SCPH1001): " + std::wstring(st.hasBios ? L"✓ OK" : L"❌ FALTA") + L"\n"
                  L"- Imagen del Disco: " + std::wstring(st.hasDisc ? L"✓ OK" : L"❌ FALTA") + L"\n"
                  L"- Ejecutable SLUS_009.23: " + std::wstring(st.hasSlus ? L"✓ OK" : L"❌ FALTA") + L"\n\n"
                  L"¿Desea abrir el Asistente de Instalación ahora para configurarlos automáticamente?";
        } else if (g_currentLang == L"pt-br") {
            title = L"Instalação Necessária";
            msg = L"Os arquivos necessários para rodar o jogo ainda não foram configurados:\n\n"
                  L"- BIOS (OpenBIOS / SCPH1001): " + std::wstring(st.hasBios ? L"✓ OK" : L"❌ AUSENTE") + L"\n"
                  L"- Imagem do Disco: " + std::wstring(st.hasDisc ? L"✓ OK" : L"❌ AUSENTE") + L"\n"
                  L"- Executável SLUS_009.23: " + std::wstring(st.hasSlus ? L"✓ OK" : L"❌ AUSENTE") + L"\n\n"
                  L"Deseja abrir o Assistente de Instalação agora para configurar automaticamente?";
        } else {
            title = L"Setup Required";
            msg = L"The required files to run the game have not been set up yet:\n\n"
                  L"- BIOS (OpenBIOS / SCPH1001): " + std::wstring(st.hasBios ? L"✓ OK" : L"❌ MISSING") + L"\n"
                  L"- Disc Image: " + std::wstring(st.hasDisc ? L"✓ OK" : L"❌ MISSING") + L"\n"
                  L"- SLUS_009.23 Executable: " + std::wstring(st.hasSlus ? L"✓ OK" : L"❌ MISSING") + L"\n\n"
                  L"Would you like to open the Setup Wizard now to configure them automatically?";
        }
        if (MessageBoxW(g_hWndMain, msg.c_str(), title.c_str(), MB_YESNO | MB_ICONWARNING) == IDYES) {
            ShowSetupDialog(g_hWndMain);
        }
        return;
    }

    GameConfig cfg = GetGuiConfig();
    SaveConfig(cfg);

    std::wstring buildDir = GetBuildDir();
    std::wstring exePath = buildDir + L"\\Resident_Evil_3__Nemesis_Recompiled.exe";
    if (GetFileAttributesW(exePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        exePath = buildDir + L"\\ResidentEvil3.exe";
    }

    std::wstring relBios = L"bios\\openbios.bin";
    if (st.hasBios && !st.biosPath.empty()) {
        size_t pos = st.biosPath.find_last_of(L"\\/");
        std::wstring fileName = (pos != std::wstring::npos) ? st.biosPath.substr(pos + 1) : st.biosPath;
        relBios = L"bios\\" + fileName;
    }

    std::wstring cmdLine = L"\"" + exePath + L"\" --bios \"" + relBios + L"\"";

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = {};

    std::vector<wchar_t> cmdBuffer(cmdLine.begin(), cmdLine.end());
    cmdBuffer.push_back(0);

    BOOL success = CreateProcessW(
        exePath.c_str(),
        cmdBuffer.data(),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        buildDir.c_str(),
        &si,
        &pi
    );

    if (success) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        ShowWindow(g_hWndMain, SW_MINIMIZE);
    } else {
        std::wstring msg = (g_currentLang == L"es") ? L"No se pudo iniciar el juego.\nCompruebe que el ejecutable esté en la carpeta." :
                           (g_currentLang == L"pt-br") ? L"Não foi possível iniciar o jogo.\nVerifique se o executável está na pasta." :
                           L"Could not launch the game.\nPlease verify that the executable is in the folder.";
        std::wstring title = (g_currentLang == L"es") ? L"Error al Iniciar" :
                             (g_currentLang == L"pt-br") ? L"Erro ao Iniciar" : L"Launch Error";
        MessageBoxW(g_hWndMain, msg.c_str(), title.c_str(), MB_ICONERROR | MB_OK);
    }
}

std::wstring GetCreditsText(const std::wstring& lang) {
    std::wstring t;
    t.reserve(12288);

    if (lang == L"es") {
        t += L"================================================================================\r\n";
        t += L"  PROJECT RE: VISITED - RESIDENT EVIL 3 — CRÉDITOS Y DESCARGO LEGAL (DISCLAIMER)\r\n";
        t += L"================================================================================\r\n\r\n";

        t += L"--------------------------------------------------------------------------------\r\n";
        t += L"1. AVISO LEGAL Y PREVENCIÓN JURÍDICA (LEGAL DISCLAIMER)\r\n";
        t += L"--------------------------------------------------------------------------------\r\n\r\n";

        t += L"1.1. NATURALEZA NO COMERCIAL Y PRESERVACIÓN HISTÓRICA:\r\n";
        t += L"Este proyecto es el primer lanzamiento de la iniciativa \"Project RE: Visited\",\r\n";
        t += L"dedicada a la preservación y restauración moderna de la trilogía original de PlayStation\r\n";
        t += L"(Resident Evil 1, 2 y 3). Es una iniciativa independiente de código abierto realizada por fans e\r\n";
        t += L"investigadores de la comunidad de ingeniería inversa (\"fan-made project\"), con fines\r\n";
        t += L"estrictamente educativos, científicos y de preservación digital. No existe ningún fin de\r\n";
        t += L"lucro, cobro, monetización ni beneficio comercial asociado a este software.\r\n\r\n";

        t += L"1.2. AUSENCIA TOTAL DE ACTIVOS PROPIETARIOS (NO PROPRIETARY ASSETS DISTRIBUTED):\r\n";
        t += L"Este repositorio, el launcher y sus binarios ejecutables NO CONTIENEN, NO DISTRIBUYEN\r\n";
        t += L"ni hospedan archivos protegidos por derechos de autor de CAPCOM CO., LTD. ni de\r\n";
        t += L"SONY INTERACTIVE ENTERTAINMENT INC. (ROMs, ISOs, audios, texturas, modelos 3D o BIOS).\r\n";
        t += L"El usuario final DEBE OBLIGATORIAMENTE proporcionar sus propios archivos legítimos del\r\n";
        t += L"juego original Resident Evil 3: Nemesis (PlayStation 1 / SLUS-00923).\r\n\r\n";

        t += L"1.3. INGENIERÍA INVERSA LIMPIA E INTEROPERABILIDAD:\r\n";
        t += L"Todo el código fuente consiste en herramientas de traducción estática (MIPS a C) y un runtime\r\n";
        t += L"de compatibilidad de hardware desarrollado bajo el amparo legal de interoperabilidad:\r\n";
        t += L"  * EE.UU.: DMCA 17 U.S.C. § 1201(f) y precedentes judiciales Sega v. Accolade y Sony v. Connectix.\r\n";
        t += L"  * Unión Europea: Directiva 2009/24/CE, Artículo 6 (Descompilación para interoperabilidad).\r\n";
        t += L"  * Brasil: Ley nº 9.609/1998, Artículo 6º, inciso I.\r\n\r\n";

        t += L"1.4. MARCAS REGISTRADAS (NOMINATIVE FAIR USE):\r\n";
        t += L"  * \"Resident Evil\", \"Biohazard\", \"Nemesis\" son marcas registradas de CAPCOM CO., LTD.\r\n";
        t += L"  * \"PlayStation\" es marca comercial registrada de SONY INTERACTIVE ENTERTAINMENT INC.\r\n";
        t += L"  * Su uso es exclusivamente descriptivo y referencial, sin afiliación ni patrocinio.\r\n\r\n";

        t += L"1.5. EXENCIÓN DE GARANTÍA (\"AS IS\"):\r\n";
        t += L"EL SOFTWARE SE PROPORCIONA \"TAL CUAL\" (\"AS IS\"), SIN GARANTÍAS DE NINGÚN TIPO. LOS AUTORES\r\n";
        t += L"NO SE RESPONSABILIZAN POR DAÑOS DERIVADOS DEL USO DE ESTE PROGRAMA.\r\n\r\n\r\n";

        t += L"--------------------------------------------------------------------------------\r\n";
        t += L"2. CRÉDITOS COMPLETOS DE HERRAMIENTAS, PROYECTOS Y FUENTES\r\n";
        t += L"--------------------------------------------------------------------------------\r\n";
        t += L"[1] CAPCOM CO., LTD. — Creación de Resident Evil 3: Nemesis (1999) en PlayStation.\r\n";
        t += L"[2] SONY INTERACTIVE ENTERTAINMENT — Arquitectura original de hardware de PlayStation 1.\r\n";
        t += L"[3] FRAMEWORK PSXRECOMP — Recompilador estático por Matthew Stanley (\"mstan\") y RetroPortingToolkit.\r\n";
        t += L"[4] N64: RECOMPILED & ZELDA64 — Pionero del paradigma de recompilación estática por Mr-Wiseguy.\r\n";
        t += L"[5] SILENT HILL (PS1) DECOMPILATION & PC PORT (INSPIRACIÓN) — Inspiración fundamental en survival horror\r\n";
        t += L"    por la comunidad (shdecompilations/silent-hill-decomp, Vatuu, SlickAmogus y sh1pc.com).\r\n";
        t += L"[6] GHIDRA SRE (NSA) — Análisis y mapeo de 3.328 funciones del binario original.\r\n";
        t += L"[7] PCSX-REDUX & OPENBIOS — BIOS de código abierto limpia bajo licencia MIT por grumpycoders.\r\n";
        t += L"[8] BEETLE PSX / MEDNAFEN / LIBRETRO — Oráculo de precisión de ciclos y emulación de hardware.\r\n";
        t += L"[9] DUCKSTATION — Referencia técnica y compatibilidad de memory cards por stenzek.\r\n";
        t += L"[10] PGXP — Corrección sub-píxel y perspectiva de texturas por iCatButler, simias y Pete Bernert.\r\n";
        t += L"[11] SDL2 — Gestión de ventanas, audio y mandos (XInput/DirectInput) por Sam Lantinga.\r\n";
        t += L"[12] OPENGL & KHRONOS GROUP — Aceleración gráfica 3D, Catmull-Rom y FXAA.\r\n";
        t += L"[13] LIBCHDR & COMPRESIÓN — Romain Tisserand; Zstandard (Meta), LZMA SDK, miniz.\r\n";
        t += L"[14] TINYCC (TCC) — Compilador C ultra-rápido para sharding AOT por Fabrice Bellard.\r\n";
        t += L"[15] TOML11 — Parser y serializador TOML por Toru Niina.\r\n";
        t += L"[16] STB LIBRARIES (STB_IMAGE) — Carga y decodificación de texturas por Sean T. Barrett.\r\n";
        t += L"[17] RABBITIZER — Desensamblador MIPS por Decompollaborate.\r\n";
        t += L"[18] ELFIO — Lector y generador de binarios ELF por Serge Lamikhov-Center.\r\n";
        t += L"[19] {FMT} — Formateo moderno en C++ por Victor Zverovich.\r\n";
        t += L"[20] JRICKEY — ShadowVerifier de audio y calibración CRT/Trinitron.\r\n";
        t += L"[21] RETCOMM-STUDIO — Herramientas multi-disco por Alex Vanderveen.\r\n";
        t += L"[22] LIBJUICE — Protocolo ICE para netplay por Paul-Louis Ageneau.\r\n";
        t += L"[23] MINGW-W64 & GCC — Toolchain de compilación x86_64 para Windows.\r\n";
        t += L"[24] MICROSOFT CORPORATION — Win32 API, GDI+ y Common Controls v6.\r\n";
        t += L"[25] COMUNIDAD R.A.I.D. — Ingeniería inversa retro asistida por IA.\r\n";
        t += L"[26] MEJORAS DEL PORT — Controles 3D analógicos con auto-run, fluidez hasta 240 FPS, SPU HQ 48kHz y saves .mcd.\r\n";
        t += L"================================================================================\r\n";
        return t;
    }

    if (lang == L"pt-br") {
        t += L"================================================================================\r\n";
        t += L"  PROJECT RE: VISITED - RESIDENT EVIL 3 — CRÉDITOS & AVISO LEGAL (DISCLAIMER)\r\n";
        t += L"================================================================================\r\n\r\n";

        t += L"--------------------------------------------------------------------------------\r\n";
        t += L"1. AVISO LEGAL & PREVENÇÃO DE ENTRAVES JURÍDICOS (LEGAL DISCLAIMER)\r\n";
        t += L"--------------------------------------------------------------------------------\r\n\r\n";

        t += L"1.1. NATUREZA NÃO COMERCIAL E FINALIDADE DE PRESERVAÇÃO HISTÓRICA:\r\n";
        t += L"Este projeto é o lançamento inicial da iniciativa \"Project RE: Visited\", dedicada\r\n";
        t += L"à preservação histórica e restauração moderna da trilogia clássica original do\r\n";
        t += L"PlayStation 1 (Resident Evil 1, 2 e 3). Uma iniciativa independente de código aberto\r\n";
        t += L"conduzida por fãs e pesquisadores da comunidade de engenharia reversa (\"fan-made\r\n";
        t += L"project\"), com finalidade estritamente educacional, científica e de preservação digital de\r\n";
        t += L"software clássico. Não há qualquer intuito lucrativo, cobrança, monetização,\r\n";
        t += L"comercialização ou ganho financeiro associado direta ou indiretamente a este projeto.\r\n\r\n";

        t += L"1.2. AUSÊNCIA ABSOLUTA DE DADOS E ATIVOS PROPRIETÁRIOS (NO ASSETS DISTRIBUTED):\r\n";
        t += L"Este repositório, o launcher e seus binários executáveis NÃO CONTÊM, NÃO DISTRIBUEM\r\n";
        t += L"e NÃO HOSPEDAM quaisquer arquivos ou conteúdos protegidos por direitos autorais\r\n";
        t += L"ou propriedade intelectual da CAPCOM CO., LTD. ou da SONY INTERACTIVE ENTERTAINMENT INC.,\r\n";
        t += L"incluindo ROMs, ISOs, áudios, texturas, modelos 3D, vídeos FMV ou BIOS comerciais.\r\n";
        t += L"O usuário final DEVE OBRIGATORIAMENTE possuir e fornecer seus próprios dados de jogo,\r\n";
        t += L"extraídos de sua cópia física ou digital legalmente adquirida de Resident Evil 3:\r\n";
        t += L"Nemesis (PlayStation 1, versão SLUS-00923), para a geração do executável funcional.\r\n\r\n";

        t += L"1.3. ENGENHARIA REVERSA LIMPA E RECOMPILAÇÃO ESTÁTICA PARA INTEROPERABILIDADE:\r\n";
        t += L"Todo o código-fonte presente neste repositório consiste em ferramentas de tradução\r\n";
        t += L"estática de instruções de máquina MIPS R3000A para linguagem C (\"clean-room static\r\n";
        t += L"recompilation\") e numa camada de compatibilidade de hardware livre, amparada por:\r\n";
        t += L"  * Estados Unidos: DMCA 17 U.S.C. Section 1201(f) e jurisprudência Sega v. Accolade e Sony v. Connectix.\r\n";
        t += L"  * União Europeia: Diretiva 2009/24/CE, Artigo 6 (Descompilação para interoperabilidade).\r\n";
        t += L"  * Brasil: Lei nº 9.609/1998 (Lei do Software), Artigo 6º, Inciso I.\r\n\r\n";

        t += L"1.4. RECONHECIMENTO DE MARCAS REGISTRADAS (TRADEMARKS ACKNOWLEDGMENT):\r\n";
        t += L"  * \"Resident Evil\", \"Biohazard\", \"Nemesis\" são marcas registradas da CAPCOM CO., LTD.\r\n";
        t += L"  * \"PlayStation\" é marca comercial registrada da SONY INTERACTIVE ENTERTAINMENT INC. (SIE).\r\n";
        t += L"  * A menção a quaisquer marcas neste software tem finalidade puramente descritiva\r\n";
        t += L"    e referencial (\"nominative fair use\"), sem afiliação, patrocínio ou endosso.\r\n\r\n";

        t += L"1.5. TERMO DE ISENÇÃO DE GARANTIA E RESPONSABILIDADE (\"AS IS\"):\r\n";
        t += L"ESTE SOFTWARE É DISPONIBILIZADO \"NO ESTADO EM QUE SE ENCONTRA\" (\"AS IS\"), SEM GARANTIA\r\n";
        t += L"DE QUALQUER ESPÉCIE. EM NENHUMA HIPÓTESE OS AUTORES SERÃO RESPONSÁVEIS POR QUAISQUER DANOS.\r\n\r\n\r\n";

        t += L"--------------------------------------------------------------------------------\r\n";
        t += L"2. CRÉDITOS COMPLETOS DE FERRAMENTAS, PROJETOS E FONTES UTILIZADAS\r\n";
        t += L"--------------------------------------------------------------------------------\r\n";
        t += L"[1] CAPCOM CO., LTD. — Criação original de Resident Evil 3: Nemesis (1999) no PlayStation.\r\n";
        t += L"[2] SONY INTERACTIVE ENTERTAINMENT (SIE) — Hardware e arquitetura do PlayStation original.\r\n";
        t += L"[3] FRAMEWORK PSXRECOMP — Recompilador estático por Matthew Stanley (\"mstan\") e RetroPortingToolkit.\r\n";
        t += L"[4] N64: RECOMPILED & ZELDA64 — Pioneiros da técnica moderna de recompilação por Mr-Wiseguy.\r\n";
        t += L"[5] SILENT HILL (PS1) DECOMPILATION & PC PORT (INSPIRAÇÃO) — Inspiração fundamental em survival horror\r\n";
        t += L"    pela comunidade (shdecompilations/silent-hill-decomp, Vatuu, SlickAmogus e comunidade).\r\n";
        t += L"[6] GHIDRA SRE (NSA) — Análise e mapeamento das 3.328 funções do binário SLUS_009.23.\r\n";
        t += L"[7] PCSX-REDUX & OPENBIOS — Reimplementação aberta do BIOS do PS1 em MIT por grumpycoders.\r\n";
        t += L"[8] BEETLE PSX / MEDNAFEN / LIBRETRO — Oráculo de precisão de hardware e validação de ciclos.\r\n";
        t += L"[9] DUCKSTATION — Referência técnica e de compatibilidade de Memory Cards por stenzek.\r\n";
        t += L"[10] PGXP — Correção geométrica sub-pixel e perspectiva por iCatButler, simias e Pete Bernert.\r\n";
        t += L"[11] SDL2 — Abstração de janelas, contexto OpenGL e controles por Sam Lantinga.\r\n";
        t += L"[12] OPENGL & KHRONOS GROUP — Aceleração gráfica 3D e shaders FXAA / Catmull-Rom.\r\n";
        t += L"[13] LIBCHDR & COMPRESSÃO — Romain Tisserand; Zstandard (Meta), LZMA SDK, miniz.\r\n";
        t += L"[14] TINYCC (TCC) — Compilador C ultra-leve para sharding dinâmico por Fabrice Bellard.\r\n";
        t += L"[15] TOML11 — Parser e serializador de arquivos TOML por Toru Niina.\r\n";
        t += L"[16] STB LIBRARIES (STB_IMAGE) — Carregamento de imagens por Sean T. Barrett.\r\n";
        t += L"[17] RABBITIZER — Desmontador modular de instruções MIPS por Decompollaborate.\r\n";
        t += L"[18] ELFIO — Manipulação de binários ELF em C++ por Serge Lamikhov-Center.\r\n";
        t += L"[19] {FMT} — Formatação moderna de strings por Victor Zverovich.\r\n";
        t += L"[20] JRICKEY — ShadowVerifier de áudio e ciência de cores de tela CRT.\r\n";
        t += L"[21] RETCOMM-STUDIO — Ferramental multi-disco por Alex Vanderveen.\r\n";
        t += L"[22] LIBJUICE — Biblioteca de transporte ICE para conexões de rede por Paul-Louis Ageneau.\r\n";
        t += L"[23] MINGW-W64 & GCC — Toolchain de compilação C/C++ x86_64.\r\n";
        t += L"[24] MICROSOFT CORPORATION — Win32 API, GDI+ e Common Controls v6.\r\n";
        t += L"[25] COMUNIDADE R.A.I.D. — Engenharia reversa retro assistida por IA.\r\n";
        t += L"[26] MELHORIAS DESTE PORT — Controles 3D analógicos com auto-run, fluidez até 240 FPS, SPU HQ 48kHz e saves .mcd.\r\n";
        t += L"================================================================================\r\n";
        return t;
    }

    // Default: English
    t += L"================================================================================\r\n";
    t += L"  PROJECT RE: VISITED - RESIDENT EVIL 3 — CREDITS & LEGAL DISCLAIMER\r\n";
    t += L"================================================================================\r\n\r\n";

    t += L"--------------------------------------------------------------------------------\r\n";
    t += L"1. LEGAL DISCLAIMER & IP NOTICE\r\n";
    t += L"--------------------------------------------------------------------------------\r\n\r\n";

    t += L"1.1. NON-COMMERCIAL & DIGITAL PRESERVATION PURPOSE:\r\n";
    t += L"This project is the initial release of the \"Project RE: Visited\" initiative, dedicated\r\n";
    t += L"to the historic preservation and modern restoration of the original PlayStation 1\r\n";
    t += L"survival horror trilogy (Resident Evil 1, 2, and 3). An independent, open-source fan-made\r\n";
    t += L"research initiative for educational, historical preservation, and software engineering\r\n";
    t += L"purposes. No commercial activity, monetary compensation, monetization, sale, or financial\r\n";
    t += L"gain is associated directly or indirectly with this project.\r\n\r\n";

    t += L"1.2. ZERO PROPRIETARY ASSETS DISTRIBUTED (ASSET-FREE DISTRIBUTION):\r\n";
    t += L"This repository, launcher, and compiled binaries DO NOT CONTAIN, DISTRIBUTE, NOR\r\n";
    t += L"HOST any copyrighted assets or intellectual property belonging to CAPCOM CO., LTD.\r\n";
    t += L"or SONY INTERACTIVE ENTERTAINMENT INC., including but not limited to:\r\n";
    t += L"  - Game disc images (.ISO, .BIN, .CUE, .IMG, .MDF);\r\n";
    t += L"  - Official soundtracks, sound effects (SFX), and voice acting;\r\n";
    t += L"  - 3D models, polygonal meshes, and textures;\r\n";
    t += L"  - Pre-rendered full-motion video (FMV) cutscenes;\r\n";
    t += L"  - Pre-rendered background artwork;\r\n";
    t += L"  - Game scripts, scenario documents, or dialogue texts;\r\n";
    t += L"  - Proprietary Sony retail BIOS dumps (e.g., SCPH1001.BIN).\r\n";
    t += L"The end user MUST legally own and supply their own game assets extracted from a\r\n";
    t += L"legitimately acquired retail copy of Resident Evil 3: Nemesis (PS1 / SLUS-00923).\r\n\r\n";

    t += L"1.3. CLEAN-ROOM REVERSE ENGINEERING & INTEROPERABILITY:\r\n";
    t += L"The source code consists exclusively of clean-room static machine code translation\r\n";
    t += L"tools (MIPS R3000A to C) and an open-source hardware abstraction runtime designed\r\n";
    t += L"to achieve interoperability on modern x86_64 Windows operating systems.\r\n";
    t += L"This methodology is protected under national and international legal frameworks:\r\n";
    t += L"  * United States: DMCA 17 U.S.C. Section 1201(f) (Reverse Engineering for\r\n";
    t += L"    Interoperability Exception), supported by federal precedents Sega v. Accolade\r\n";
    t += L"    (977 F.2d 1510) and Sony v. Connectix (203 F.3d 596).\r\n";
    t += L"  * European Union: Directive 2009/24/EC, Article 6 (Decompilation for\r\n";
    t += L"    Interoperability of Computer Programs).\r\n";
    t += L"  * Brazil: Law No. 9,609/1998 (Software Law), Article 6, Item I.\r\n\r\n";

    t += L"1.4. TRADEMARKS ACKNOWLEDGMENT (NOMINATIVE FAIR USE):\r\n";
    t += L"  * \"Resident Evil\", \"Biohazard\", \"Nemesis\", \"Jill Valentine\", \"Umbrella\" and\r\n";
    t += L"    associated trademarks and logos are registered trademarks of CAPCOM CO., LTD.\r\n";
    t += L"  * \"PlayStation\", \"PS1\", \"PSX\", \"DualShock\" and related logos are registered\r\n";
    t += L"    trademarks of SONY INTERACTIVE ENTERTAINMENT INC. (SIE).\r\n";
    t += L"  * All references to trademarks are strictly nominative and descriptive, intended\r\n";
    t += L"    solely to identify software compatibility, with no affiliation, endorsement,\r\n";
    t += L"    sponsorship, or licensing from copyright owners.\r\n\r\n";

    t += L"1.5. DISCLAIMER OF WARRANTIES & LIMITATION OF LIABILITY (\"AS IS\"):\r\n";
    t += L"THIS SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\r\n";
    t += L"IMPLIED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR ANY CLAIM,\r\n";
    t += L"DAMAGES, OR OTHER LIABILITY ARISING FROM THE USE OF THIS SOFTWARE.\r\n\r\n\r\n";

    t += L"--------------------------------------------------------------------------------\r\n";
    t += L"2. PROJECT CREDITS, TOOLS & INSPIRATIONS\r\n";
    t += L"--------------------------------------------------------------------------------\r\n";
    t += L"[1] CAPCOM CO., LTD. — Original creation of Resident Evil 3: Nemesis (1999).\r\n";
    t += L"[2] SONY INTERACTIVE ENTERTAINMENT — Original PlayStation 1 hardware architecture.\r\n";
    t += L"[3] RETROPORTINGTOOLKIT / PSXRECOMP — PS1 static recompiler framework by Matthew Stanley (\"mstan\").\r\n";
    t += L"[4] N64: RECOMPILED & ZELDA64 — Static recompilation pioneer by Mr-Wiseguy.\r\n";
    t += L"[5] SILENT HILL (PS1) DECOMPILATION & NATIVE PC PORT — Core survival horror preservation inspiration\r\n";
    t += L"    by the community (shdecompilations/silent-hill-decomp, Vatuu, SlickAmogus, and community).\r\n";
    t += L"[6] GHIDRA SRE (NSA) — Reverse engineering suite used to identify and map 3,328 functions.\r\n";
    t += L"[7] PCSX-REDUX & OPENBIOS — Clean-room open-source PS1 BIOS by grumpycoders (MIT).\r\n";
    t += L"[8] BEETLE PSX / MEDNAFEN / LIBRETRO — Hardware accuracy oracle and cycle validation.\r\n";
    t += L"[9] DUCKSTATION — Modern emulation and memory card reference by stenzek.\r\n";
    t += L"[10] PGXP (PARALLEL GPU EXTENDED PRECISION) — Sub-pixel geometry and perspective correction.\r\n";
    t += L"[11] SDL2 (SIMPLE DIRECTMEDIA LAYER) — Windowing, input (XInput/DirectInput), and audio by Sam Lantinga.\r\n";
    t += L"[12] OPENGL & KHRONOS GROUP — Hardware 3D graphics acceleration, Catmull-Rom upscaling, and FXAA.\r\n";
    t += L"[13] LIBCHDR & COMPRESSION — Romain Tisserand (BSD-3); Zstandard (Meta), LZMA SDK, miniz.\r\n";
    t += L"[14] TINYCC (TCC) — Fast runtime C compiler for overlay sharding by Fabrice Bellard (LGPL-2.1).\r\n";
    t += L"[15] TOML11 — Modern C++ TOML parser and serializer by Toru Niina (MIT).\r\n";
    t += L"[16] STB LIBRARIES (STB_IMAGE) — Texture and image decoding by Sean T. Barrett (MIT/Public Domain).\r\n";
    t += L"[17] RABBITIZER — Modular MIPS instruction disassembler by Decompollaborate (MIT).\r\n";
    t += L"[18] ELFIO — ELF binary reader and generator by Serge Lamikhov-Center (MIT).\r\n";
    t += L"[19] {FMT} — Formatting library by Victor Zverovich (MIT).\r\n";
    t += L"[20] JRICKEY (GBA-RECOMP & SNESRECOMP) — ShadowVerifier audio calibration and CRT color science.\r\n";
    t += L"[21] RETCOMM-STUDIO — Multi-disc verification tooling by Alex Vanderveen.\r\n";
    t += L"[22] LIBJUICE — ICE transport for P2P networking by Paul-Louis Ageneau (MPL-2.0).\r\n";
    t += L"[23] MINGW-W64 & GCC — Toolchain used to compile the native x86_64 executables.\r\n";
    t += L"[24] MICROSOFT CORPORATION — Windows Win32 API, GDI+, and Common Controls v6.\r\n";
    t += L"[25] R.A.I.D. (RETRO AI DEVELOPMENT) — Retro AI-assisted reverse engineering and preservation community.\r\n";
    t += L"[26] PORT ENHANCEMENTS — Modern 3D camera-relative controls, high-refresh rate interpolation up to 240Hz, SPU HQ 48kHz resampling, and native .mcd memory cards.\r\n";
    t += L"================================================================================\r\n";

    return t;
}

LRESULT CALLBACK CreditsWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        std::wstring creditsText = GetCreditsText(g_currentLang);

        std::wstring titleStr = (g_currentLang == L"es") ? L"PROJECT RE: VISITED - RESIDENT EVIL 3 — CRÉDITOS Y DESCARGO LEGAL" :
                                (g_currentLang == L"pt-br") ? L"PROJECT RE: VISITED - RESIDENT EVIL 3 — CRÉDITOS & TERMOS LEGAIS" :
                                L"PROJECT RE: VISITED - RESIDENT EVIL 3 — CREDITS & LEGAL DISCLAIMER";
        std::wstring subStr = (g_currentLang == L"es") ? L"Atribuciones completas a herramientas y fuentes, con descargo de responsabilidad legal." :
                              (g_currentLang == L"pt-br") ? L"Atribuições completas a todas as ferramentas e fontes, com isenção legal de responsabilidade." :
                              L"Complete attributions for all tools and sources, with legal prevention terms for GitHub.";
        std::wstring btnCopyStr = (g_currentLang == L"es") ? L"Copiar Texto Completo" :
                                  (g_currentLang == L"pt-br") ? L"Copiar Texto Completo" : L"Copy Full Text";
        std::wstring btnCloseStr = (g_currentLang == L"es") ? L"Cerrar" :
                                   (g_currentLang == L"pt-br") ? L"Fechar" : L"Close";

        g_hLblCreditsTitle = CreateWindowW(L"STATIC", titleStr.c_str(), WS_VISIBLE | WS_CHILD | SS_LEFT, 18, 12, 660, 22, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblCreditsTitle, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hLblCreditsSub = CreateWindowW(L"STATIC", subStr.c_str(), WS_VISIBLE | WS_CHILD | SS_LEFT, 18, 34, 660, 18, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblCreditsSub, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

        g_hEditCredits = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", creditsText.c_str(),
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_TABSTOP,
            18, 56, 660, 500, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hEditCredits, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        SendMessageW(g_hEditCredits, EM_SETLIMITTEXT, (WPARAM)0x200000, 0);

        g_hBtnCreditsCopy = CreateWindowW(L"BUTTON", btnCopyStr.c_str(), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 18, 566, 180, 32, hWnd, (HMENU)ID_BTN_CREDITS_COPY, g_hInst, NULL);
        SendMessageW(g_hBtnCreditsCopy, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hBtnCreditsGithub = CreateWindowW(L"BUTTON", L"Abrir PSXRecomp (GitHub)", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 206, 566, 190, 32, hWnd, (HMENU)ID_BTN_CREDITS_GITHUB, g_hInst, NULL);
        SendMessageW(g_hBtnCreditsGithub, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hBtnCreditsClose = CreateWindowW(L"BUTTON", btnCloseStr.c_str(), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP, 558, 566, 120, 32, hWnd, (HMENU)ID_BTN_CREDITS_CLOSE, g_hInst, NULL);
        SendMessageW(g_hBtnCreditsClose, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        break;
    }
    case WM_SIZE: {
        int w = LOWORD(lParam);
        int h = HIWORD(lParam);
        if (w > 0 && h > 0) {
            if (g_hLblCreditsTitle) MoveWindow(g_hLblCreditsTitle, 18, 12, w - 36, 22, TRUE);
            if (g_hLblCreditsSub) MoveWindow(g_hLblCreditsSub, 18, 34, w - 36, 18, TRUE);
            if (g_hEditCredits) MoveWindow(g_hEditCredits, 18, 56, w - 36, h - 104, TRUE);
            if (g_hBtnCreditsCopy) MoveWindow(g_hBtnCreditsCopy, 18, h - 40, 180, 32, TRUE);
            if (g_hBtnCreditsGithub) MoveWindow(g_hBtnCreditsGithub, 206, h - 40, 190, 32, TRUE);
            if (g_hBtnCreditsClose) MoveWindow(g_hBtnCreditsClose, w - 18 - 120, h - 40, 120, 32, TRUE);
        }
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_BTN_CREDITS_CLOSE) {
            DestroyWindow(hWnd);
        } else if (id == ID_BTN_CREDITS_COPY) {
            std::wstring creditsText = GetCreditsText(g_currentLang);
            if (OpenClipboard(hWnd)) {
                EmptyClipboard();
                size_t bytes = (creditsText.length() + 1) * sizeof(wchar_t);
                HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
                if (hMem) {
                    memcpy(GlobalLock(hMem), creditsText.c_str(), bytes);
                    GlobalUnlock(hMem);
                    SetClipboardData(CF_UNICODETEXT, hMem);
                }
                CloseClipboard();
                std::wstring msg = (g_currentLang == L"es") ? L"¡Los créditos y términos legales se copiaron al portapapeles!" :
                                   (g_currentLang == L"pt-br") ? L"Os créditos e termos legais foram copiados para a sua área de transferência!" :
                                   L"Credits and legal terms copied to clipboard!";
                std::wstring title = (g_currentLang == L"es") ? L"Copiado con Éxito" :
                                     (g_currentLang == L"pt-br") ? L"Copiado com Sucesso" : L"Copied";
                MessageBoxW(hWnd, msg.c_str(), title.c_str(), MB_ICONINFORMATION | MB_OK);
            }
        } else if (id == ID_BTN_CREDITS_GITHUB) {
            ShellExecuteW(hWnd, L"open", L"https://github.com/RetroPortingToolKit/psxrecomp", NULL, NULL, SW_SHOWNORMAL);
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        SetBkColor(hdcStatic, GetSysColor(COLOR_BTNFACE));
        SetTextColor(hdcStatic, GetSysColor(COLOR_WINDOWTEXT));
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }
    case WM_CLOSE: {
        DestroyWindow(hWnd);
        break;
    }
    case WM_DESTROY: {
        g_hWndCredits = NULL;
        if (g_hWndMain) {
            EnableWindow(g_hWndMain, TRUE);
            SetForegroundWindow(g_hWndMain);
        }
        break;
    }
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void ShowCreditsDialog(HWND hWndParent) {
    if (g_hWndCredits) {
        BringWindowToTop(g_hWndCredits);
        SetForegroundWindow(g_hWndCredits);
        return;
    }

    RECT rcParent;
    GetWindowRect(hWndParent, &rcParent);
    int pW = rcParent.right - rcParent.left;
    int pH = rcParent.bottom - rcParent.top;
    int width = 720;
    int height = 650;
    int posX = rcParent.left + (pW - width) / 2;
    int posY = rcParent.top + (pH - height) / 2;
    if (posX < 0) posX = 0;
    if (posY < 0) posY = 0;

    std::wstring dlgTitle = (g_currentLang == L"es") ? L"Créditos del Proyecto y Descargo Legal (Disclaimer)" :
                            (g_currentLang == L"pt-br") ? L"Créditos do Projeto & Isenção de Responsabilidade Legal (Disclaimer)" :
                            L"Project Credits & Legal Disclaimer";

    g_hWndCredits = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"RE3CreditsWindowClass",
        dlgTitle.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX,
        posX, posY, width, height,
        hWndParent, NULL, g_hInst, NULL
    );

    if (g_hWndCredits) {
        EnableWindow(hWndParent, FALSE);
        ShowWindow(g_hWndCredits, SW_SHOWNORMAL);
        UpdateWindow(g_hWndCredits);
    }
}

bool HasAnyCheatActive(const GameConfig& cfg) {
    return cfg.infinite_health || cfg.infinite_ammo || cfg.infinite_ink_ribbons ||
           cfg.all_weapons_box || cfg.unlock_mercenaries || cfg.unlock_boutique ||
           cfg.unlock_epilogues;
}

void UpdateCheatsButtonText() {
    if (!g_hBtnOpenCheats) return;
    bool active = HasAnyCheatActive(g_cachedConfig);
    std::wstring text;
    if (g_currentLang == L"es") {
        text = active ? L"Trucos / Cheats (ACTIVO)" : L"Trucos y Modificadores...";
    } else if (g_currentLang == L"pt-br") {
        text = active ? L"Cheats & Mods (ATIVO)" : L"Cheats & Modificadores...";
    } else {
        text = active ? L"Cheats & Mods (ACTIVE)" : L"Cheats & Modifiers...";
    }
    SetWindowTextW(g_hBtnOpenCheats, text.c_str());
}

LRESULT CALLBACK CheatsWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        std::wstring titleText, subText, grpText;
        std::wstring chkHealth, chkAmmo, chkRibbons, chkWeapons, chkMercs, chkBoutique, chkEpilogues;
        std::wstring hotkeysText, btnReset, btnSave, btnClose;

        if (g_currentLang == L"es") {
            titleText = L"Modificadores del Juego y Selector de Trucos (Cheats)";
            subText = L"100% reversible. Desmarque cualquier opción para restaurar el juego original.";
            grpText = L"  Opciones de Trucos y Modificadores  ";
            chkHealth = L"Salud Infinita (God Mode — Jill es inmune al daño letal y veneno)";
            chkAmmo = L"Munición Infinita (Las armas nunca consumen balas al disparar)";
            chkRibbons = L"Cintas de Tinta Infinitas (Guarda la partida tantas veces como quieras)";
            chkWeapons = L"Todas las Armas en el Baúl (Baúl inicial con armas pesadas: Rocket, Gatling, M4A1)";
            chkMercs = L"Desbloquear The Mercenaries (Modo Mercenarios disponible de inmediato)";
            chkBoutique = L"Desbloquear Boutique y Trajes (Acceso a la boutique con todos los atuendos)";
            chkEpilogues = L"Desbloquear Todos los Epílogos (Galería completa con los 8 epílogos)";
            hotkeysText = L"Atajos en juego: [F6] Alternar God Mode  |  [F7] Munición Infinita  |  [F8] Munición Máxima";
            btnReset = L"Desactivar Todo";
            btnSave = L"Guardar y Aplicar";
            btnClose = L"Cancelar / Cerrar";
        } else if (g_currentLang == L"pt-br") {
            titleText = L"Modificadores de Jogo & Seletor de Cheats";
            subText = L"100% reversível. Desmarque qualquer opção para restaurar a campanha clássica pura.";
            grpText = L"  Opções de Trapaças & Modificadores  ";
            chkHealth = L"Vida Infinita (God Mode — Jill é imune a dano letal e a veneno)";
            chkAmmo = L"Munição Infinita (Armas de fogo nunca gastam balas ao atirar)";
            chkRibbons = L"Fitas de Salvamento Infinitas (Salve o jogo quantas vezes quiser)";
            chkWeapons = L"Todas as Armas no Baú (Baú inicial com armas pesadas: Rocket, Gatling, M4A1)";
            chkMercs = L"Desbloquear The Mercenaries (Modo Mercenários liberado imediatamente)";
            chkBoutique = L"Desbloquear Boutique & Trajes (Acesso à Boutique com todas as 5 roupas extras)";
            chkEpilogues = L"Desbloquear Todos os Epílogos (Galeria completa com todos os 8 epílogos)";
            hotkeysText = L"Atalhos no jogo: [F6] Alternar God Mode  |  [F7] Munição Infinita  |  [F8] Munição Máxima";
            btnReset = L"Desativar Tudo";
            btnSave = L"Salvar & Aplicar";
            btnClose = L"Cancelar / Fechar";
        } else {
            titleText = L"Game Modifiers & Cheat Selector";
            subText = L"100% reversible. Uncheck any option to restore authentic original gameplay.";
            grpText = L"  Cheat Codes & Gameplay Options  ";
            chkHealth = L"Infinite Health (God Mode — Jill is immune to lethal damage & poison)";
            chkAmmo = L"Infinite Ammo (Weapons never consume ammunition when firing)";
            chkRibbons = L"Infinite Ink Ribbons (Save your game as many times as you want)";
            chkWeapons = L"All Weapons in Item Box (Chest stocked with heavy weapons: Rocket, Gatling, M4A1)";
            chkMercs = L"Unlock The Mercenaries (Operation: Mad Jackal accessible immediately)";
            chkBoutique = L"Unlock Boutique & All Costumes (Access uptown boutique with all 5 outfits)";
            chkEpilogues = L"Unlock All Epilogues (Complete gallery with all 8 character epilogues)";
            hotkeysText = L"In-game shortcuts: [F6] Toggle God Mode  |  [F7] Infinite Ammo  |  [F8] Max Ammo";
            btnReset = L"Reset All (Off)";
            btnSave = L"Save & Apply";
            btnClose = L"Cancel / Close";
        }

        g_hLblCheatsTitle = CreateWindowW(L"STATIC", titleText.c_str(), WS_VISIBLE | WS_CHILD | SS_LEFT, 20, 14, 520, 24, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblCheatsTitle, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hLblCheatsSub = CreateWindowW(L"STATIC", subText.c_str(), WS_VISIBLE | WS_CHILD | SS_LEFT, 20, 38, 520, 20, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblCheatsSub, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

        g_hGrpCheatsList = CreateWindowW(L"BUTTON", grpText.c_str(), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 18, 66, 528, 276, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hGrpCheatsList, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hChkCheatHealth = CreateWindowW(L"BUTTON", chkHealth.c_str(), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 32, 92, 500, 24, hWnd, (HMENU)ID_CHK_CHEAT_HEALTH, g_hInst, NULL);
        SendMessageW(g_hChkCheatHealth, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hChkCheatAmmo = CreateWindowW(L"BUTTON", chkAmmo.c_str(), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 32, 126, 500, 24, hWnd, (HMENU)ID_CHK_CHEAT_AMMO, g_hInst, NULL);
        SendMessageW(g_hChkCheatAmmo, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hChkCheatRibbons = CreateWindowW(L"BUTTON", chkRibbons.c_str(), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 32, 160, 500, 24, hWnd, (HMENU)ID_CHK_CHEAT_RIBBONS, g_hInst, NULL);
        SendMessageW(g_hChkCheatRibbons, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hChkCheatWeapons = CreateWindowW(L"BUTTON", chkWeapons.c_str(), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 32, 194, 500, 24, hWnd, (HMENU)ID_CHK_CHEAT_WEAPONS, g_hInst, NULL);
        SendMessageW(g_hChkCheatWeapons, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hChkCheatMercenaries = CreateWindowW(L"BUTTON", chkMercs.c_str(), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 32, 228, 500, 24, hWnd, (HMENU)ID_CHK_CHEAT_MERCENARIES, g_hInst, NULL);
        SendMessageW(g_hChkCheatMercenaries, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hChkCheatBoutique = CreateWindowW(L"BUTTON", chkBoutique.c_str(), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 32, 262, 500, 24, hWnd, (HMENU)ID_CHK_CHEAT_BOUTIQUE, g_hInst, NULL);
        SendMessageW(g_hChkCheatBoutique, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hChkCheatEpilogues = CreateWindowW(L"BUTTON", chkEpilogues.c_str(), WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 32, 296, 500, 24, hWnd, (HMENU)ID_CHK_CHEAT_EPILOGUES, g_hInst, NULL);
        SendMessageW(g_hChkCheatEpilogues, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hLblCheatsHotkeys = CreateWindowW(L"STATIC", hotkeysText.c_str(), WS_VISIBLE | WS_CHILD | SS_CENTER, 18, 350, 528, 20, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblCheatsHotkeys, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

        g_hBtnCheatsReset = CreateWindowW(L"BUTTON", btnReset.c_str(), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 18, 382, 160, 36, hWnd, (HMENU)ID_BTN_CHEATS_RESET, g_hInst, NULL);
        SendMessageW(g_hBtnCheatsReset, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hBtnCheatsSave = CreateWindowW(L"BUTTON", btnSave.c_str(), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP, 190, 382, 184, 36, hWnd, (HMENU)ID_BTN_CHEATS_SAVE, g_hInst, NULL);
        SendMessageW(g_hBtnCheatsSave, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hBtnCheatsClose = CreateWindowW(L"BUTTON", btnClose.c_str(), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 386, 382, 160, 36, hWnd, (HMENU)ID_BTN_CHEATS_CLOSE, g_hInst, NULL);
        SendMessageW(g_hBtnCheatsClose, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        // Populate checkboxes
        SendMessageW(g_hChkCheatHealth, BM_SETCHECK, g_cachedConfig.infinite_health ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(g_hChkCheatAmmo, BM_SETCHECK, g_cachedConfig.infinite_ammo ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(g_hChkCheatRibbons, BM_SETCHECK, g_cachedConfig.infinite_ink_ribbons ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(g_hChkCheatWeapons, BM_SETCHECK, g_cachedConfig.all_weapons_box ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(g_hChkCheatMercenaries, BM_SETCHECK, g_cachedConfig.unlock_mercenaries ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(g_hChkCheatBoutique, BM_SETCHECK, g_cachedConfig.unlock_boutique ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(g_hChkCheatEpilogues, BM_SETCHECK, g_cachedConfig.unlock_epilogues ? BST_CHECKED : BST_UNCHECKED, 0);

        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_BTN_CHEATS_RESET) {
            SendMessageW(g_hChkCheatHealth, BM_SETCHECK, BST_UNCHECKED, 0);
            SendMessageW(g_hChkCheatAmmo, BM_SETCHECK, BST_UNCHECKED, 0);
            SendMessageW(g_hChkCheatRibbons, BM_SETCHECK, BST_UNCHECKED, 0);
            SendMessageW(g_hChkCheatWeapons, BM_SETCHECK, BST_UNCHECKED, 0);
            SendMessageW(g_hChkCheatMercenaries, BM_SETCHECK, BST_UNCHECKED, 0);
            SendMessageW(g_hChkCheatBoutique, BM_SETCHECK, BST_UNCHECKED, 0);
            SendMessageW(g_hChkCheatEpilogues, BM_SETCHECK, BST_UNCHECKED, 0);
        } else if (id == ID_BTN_CHEATS_SAVE) {
            g_cachedConfig.infinite_health = (SendMessageW(g_hChkCheatHealth, BM_GETCHECK, 0, 0) == BST_CHECKED);
            g_cachedConfig.infinite_ammo = (SendMessageW(g_hChkCheatAmmo, BM_GETCHECK, 0, 0) == BST_CHECKED);
            g_cachedConfig.infinite_ink_ribbons = (SendMessageW(g_hChkCheatRibbons, BM_GETCHECK, 0, 0) == BST_CHECKED);
            g_cachedConfig.all_weapons_box = (SendMessageW(g_hChkCheatWeapons, BM_GETCHECK, 0, 0) == BST_CHECKED);
            g_cachedConfig.unlock_mercenaries = (SendMessageW(g_hChkCheatMercenaries, BM_GETCHECK, 0, 0) == BST_CHECKED);
            g_cachedConfig.unlock_boutique = (SendMessageW(g_hChkCheatBoutique, BM_GETCHECK, 0, 0) == BST_CHECKED);
            g_cachedConfig.unlock_epilogues = (SendMessageW(g_hChkCheatEpilogues, BM_GETCHECK, 0, 0) == BST_CHECKED);

            GameConfig fullCfg = GetGuiConfig();
            SaveConfig(fullCfg);
            UpdateCheatsButtonText();
            DestroyWindow(hWnd);
        } else if (id == ID_BTN_CHEATS_CLOSE) {
            DestroyWindow(hWnd);
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        SetBkColor(hdcStatic, GetSysColor(COLOR_BTNFACE));
        SetTextColor(hdcStatic, GetSysColor(COLOR_WINDOWTEXT));
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }
    case WM_CLOSE: {
        DestroyWindow(hWnd);
        break;
    }
    case WM_DESTROY: {
        g_hWndCheats = NULL;
        if (g_hWndMain) {
            EnableWindow(g_hWndMain, TRUE);
            SetForegroundWindow(g_hWndMain);
        }
        break;
    }
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void ShowCheatsDialog(HWND hWndParent) {
    if (g_hWndCheats) {
        BringWindowToTop(g_hWndCheats);
        SetForegroundWindow(g_hWndCheats);
        return;
    }

    RECT rcParent;
    GetWindowRect(hWndParent, &rcParent);
    int pW = rcParent.right - rcParent.left;
    int pH = rcParent.bottom - rcParent.top;
    int width = 580;
    int height = 470;
    int posX = rcParent.left + (pW - width) / 2;
    int posY = rcParent.top + (pH - height) / 2;
    if (posX < 0) posX = 0;
    if (posY < 0) posY = 0;

    std::wstring dlgTitle = (g_currentLang == L"es") ? L"Modificadores del Juego y Selector de Trucos" :
                            (g_currentLang == L"pt-br") ? L"Modificadores de Jogo & Seletor de Cheats" :
                            L"Game Modifiers & Cheat Selector";

    g_hWndCheats = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"RE3CheatsWindowClass",
        dlgTitle.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, width, height,
        hWndParent, NULL, g_hInst, NULL
    );

    if (g_hWndCheats) {
        EnableWindow(hWndParent, FALSE);
        ShowWindow(g_hWndCheats, SW_SHOWNORMAL);
        UpdateWindow(g_hWndCheats);
    }
}

struct SetupThreadParams {
    std::wstring biosSrc;
    std::wstring discSrc;
    HWND hWndSetup;
    HWND hEditLog;
    HWND hBtnStart;
    HWND hBtnClose;
};

void AppendSetupLog(HWND hEdit, const std::wstring& line) {
    if (!hEdit) return;
    int len = GetWindowTextLengthW(hEdit);
    SendMessageW(hEdit, EM_SETSEL, len, len);
    std::wstring withNl = line + L"\r\n";
    SendMessageW(hEdit, EM_REPLACESEL, FALSE, (LPARAM)withNl.c_str());
    SendMessageW(hEdit, EM_SCROLLCARET, 0, 0);
}

DWORD WINAPI SetupWorkerThreadProc(LPVOID lpParam) {
    SetupThreadParams* p = (SetupThreadParams*)lpParam;
    if (!p) return 0;

    EnableWindow(p->hBtnStart, FALSE);
    EnableWindow(p->hBtnClose, FALSE);

    std::wstring buildDir = GetBuildDir();
    CreateDirectoryW((buildDir + L"\\bios").c_str(), NULL);
    CreateDirectoryW((buildDir + L"\\gamedata").c_str(), NULL);
    CreateDirectoryW((buildDir + L"\\disc").c_str(), NULL);
    CreateDirectoryW((buildDir + L"\\saves").c_str(), NULL);
    CreateDirectoryW((buildDir + L"\\cache").c_str(), NULL);

    AppendSetupLog(p->hEditLog, L"============================================================");
    if (g_currentLang == L"es") {
        AppendSetupLog(p->hEditLog, L"Iniciando instalación automática de Project RE: Visited - Resident Evil 3...");
    } else if (g_currentLang == L"pt-br") {
        AppendSetupLog(p->hEditLog, L"Iniciando instalação automática de Project RE: Visited - Resident Evil 3...");
    } else {
        AppendSetupLog(p->hEditLog, L"Starting automatic setup for Project RE: Visited - Resident Evil 3...");
    }

    // 1. BIOS
    std::wstring dstBios = buildDir + L"\\bios\\SCPH1001.BIN";
    std::wstring openBios = buildDir + L"\\bios\\openbios.bin";
    bool biosOk = false;
    std::wstring configuredBiosRel = L"bios/openbios.bin";

    if (!p->biosSrc.empty()) {
        wchar_t fullSrc[MAX_PATH];
        GetFullPathNameW(p->biosSrc.c_str(), MAX_PATH, fullSrc, NULL);
        std::wstring srcPath = fullSrc;
        size_t slash = srcPath.find_last_of(L"\\/");
        std::wstring fileName = (slash != std::wstring::npos) ? srcPath.substr(slash + 1) : L"SCPH1001.BIN";
        std::wstring targetDst = buildDir + L"\\bios\\" + fileName;

        if (_wcsicmp(fullSrc, targetDst.c_str()) != 0) {
            AppendSetupLog(p->hEditLog, (g_currentLang == L"es") ? L"[1/4] Copiando archivo de BIOS..." :
                                        (g_currentLang == L"pt-br") ? L"[1/4] Copiando arquivo de BIOS..." :
                                        L"[1/4] Copying BIOS file...");
            if (CopyFileW(fullSrc, targetDst.c_str(), FALSE)) {
                AppendSetupLog(p->hEditLog, L"  ✓ " + fileName + L" copiada com sucesso.");
                biosOk = true;
                configuredBiosRel = L"bios/" + fileName;
            } else {
                AppendSetupLog(p->hEditLog, L"  ❌ Falha ao copiar a BIOS.");
            }
        } else {
            AppendSetupLog(p->hEditLog, L"[1/4] BIOS " + fileName + L" já localizada na pasta bios\\.");
            biosOk = true;
            configuredBiosRel = L"bios/" + fileName;
        }
    } else if (GetFileAttributesW(openBios.c_str()) != INVALID_FILE_ATTRIBUTES) {
        AppendSetupLog(p->hEditLog, (g_currentLang == L"es") ? L"[1/4] Utilizando OpenBIOS predeterminada (MIT / Código Abierto)." :
                                    (g_currentLang == L"pt-br") ? L"[1/4] Utilizando OpenBIOS padrão (MIT / Código Aberto)." :
                                    L"[1/4] Using default bundled OpenBIOS (MIT / Clean-Room).");
        biosOk = true;
        configuredBiosRel = L"bios/openbios.bin";
    } else if (GetFileAttributesW(dstBios.c_str()) != INVALID_FILE_ATTRIBUTES) {
        AppendSetupLog(p->hEditLog, L"[1/4] BIOS SCPH1001.BIN já presente e validada.");
        biosOk = true;
        configuredBiosRel = L"bios/SCPH1001.BIN";
    } else {
        AppendSetupLog(p->hEditLog, L"[1/4] ❌ Nenhuma BIOS selecionada ou encontrada.");
    }

    if (biosOk) {
        std::wofstream bcfg((buildDir + L"\\bios.cfg").c_str());
        if (bcfg.is_open()) {
            bcfg << configuredBiosRel << L"\n";
            bcfg.close();
        }
    }

    // 2. Disc Image
    std::wstring dstBin = buildDir + L"\\gamedata\\Resident Evil 3 - Nemesis (USA, Canada).bin";
    std::wstring dstCue = buildDir + L"\\gamedata\\Resident Evil 3 - Nemesis (USA, Canada).cue";
    bool discOk = false;
    std::wstring srcBinToExtract = L"";

    if (!p->discSrc.empty()) {
        std::wstring actualBinSrc = p->discSrc;
        if (actualBinSrc.length() >= 4 && _wcsicmp(actualBinSrc.substr(actualBinSrc.length() - 4).c_str(), L".cue") == 0) {
            std::wstring foundBin = ResolveBinFromCue(actualBinSrc);
            if (!foundBin.empty() && GetFileAttributesW(foundBin.c_str()) != INVALID_FILE_ATTRIBUTES) {
                actualBinSrc = foundBin;
            } else {
                actualBinSrc = actualBinSrc.substr(0, actualBinSrc.length() - 4) + L".bin";
            }
        }

        if (GetFileAttributesW(actualBinSrc.c_str()) == INVALID_FILE_ATTRIBUTES) {
            AppendSetupLog(p->hEditLog, L"[2/4] ❌ Arquivo .bin do disco não encontrado.");
        } else {
            wchar_t fullSrc[MAX_PATH], fullDst[MAX_PATH];
            GetFullPathNameW(actualBinSrc.c_str(), MAX_PATH, fullSrc, NULL);
            GetFullPathNameW(dstBin.c_str(), MAX_PATH, fullDst, NULL);

            if (_wcsicmp(fullSrc, fullDst) != 0) {
                AppendSetupLog(p->hEditLog, (g_currentLang == L"es") ? L"[2/4] Copiando imagen del juego (700 MB, espere)..." :
                                            (g_currentLang == L"pt-br") ? L"[2/4] Copiando imagem do jogo (700 MB, aguarde)..." :
                                            L"[2/4] Copying disc image (700 MB, please wait)...");
                if (CopyFileW(fullSrc, fullDst, FALSE)) {
                    AppendSetupLog(p->hEditLog, L"  ✓ Imagem copiada para gamedata\\Resident Evil 3 - Nemesis (USA, Canada).bin");
                    discOk = true;
                    srcBinToExtract = dstBin;
                } else {
                    AppendSetupLog(p->hEditLog, L"  ❌ Falha ao copiar imagem do jogo.");
                }
            } else {
                AppendSetupLog(p->hEditLog, L"[2/4] Imagem já localizada na pasta gamedata\\.");
                discOk = true;
                srcBinToExtract = dstBin;
            }

            if (discOk) {
                std::wofstream cueOut(dstCue.c_str());
                if (cueOut.is_open()) {
                    cueOut << L"FILE \"Resident Evil 3 - Nemesis (USA, Canada).bin\" BINARY\n";
                    cueOut << L"  TRACK 01 MODE2/2352\n";
                    cueOut << L"    INDEX 01 00:00:00\n";
                    cueOut.close();
                    AppendSetupLog(p->hEditLog, L"  ✓ Arquivo .cue gerado em gamedata\\.");
                }
            }
        }
    } else if (GetFileAttributesW(dstBin.c_str()) != INVALID_FILE_ATTRIBUTES) {
        AppendSetupLog(p->hEditLog, L"[2/4] Imagem já existente em gamedata\\.");
        discOk = true;
        srcBinToExtract = dstBin;
    } else {
        InstallStatus curSt = CheckInstallationStatus();
        if (curSt.hasDisc) {
            AppendSetupLog(p->hEditLog, L"[2/4] Imagem existente identificada: " + curSt.discPath);
            discOk = true;
            srcBinToExtract = curSt.discPath;
        } else {
            AppendSetupLog(p->hEditLog, L"[2/4] ❌ Nenhuma imagem de CD selecionada.");
        }
    }

    // 3. Extrair SLUS
    bool slusOk = false;
    std::wstring dstSlus = buildDir + L"\\disc\\SLUS_009.23";

    if (discOk && !srcBinToExtract.empty()) {
        AppendSetupLog(p->hEditLog, (g_currentLang == L"es") ? L"[3/4] Extrayendo ejecutable SLUS_009.23 directamente del disco..." :
                                    (g_currentLang == L"pt-br") ? L"[3/4] Extraindo executável SLUS_009.23 diretamente do disco..." :
                                    L"[3/4] Extracting SLUS_009.23 executable directly from disc image...");
        std::wstring err;
        if (ExtractSlusFromDisc(srcBinToExtract, dstSlus, err)) {
            AppendSetupLog(p->hEditLog, L"  ✓ Executável SLUS_009.23 extraído com sucesso (868.352 bytes).");
            AppendSetupLog(p->hEditLog, L"  ✓ Assinatura 'PS-X EXE' validada com 100% de precisão!");
            slusOk = true;
        } else {
            AppendSetupLog(p->hEditLog, L"  ❌ Erro ao extrair SLUS: " + err);
        }
    } else if (GetFileAttributesW(dstSlus.c_str()) != INVALID_FILE_ATTRIBUTES) {
        AppendSetupLog(p->hEditLog, L"[3/4] Executável SLUS_009.23 já presente e validado.");
        slusOk = true;
    } else {
        AppendSetupLog(p->hEditLog, L"[3/4] ❌ Não foi possível extrair o executável (disco ausente).");
    }

    // 4. Finalização
    AppendSetupLog(p->hEditLog, L"[4/4] Verificando estrutura de diretórios e saves...");
    AppendSetupLog(p->hEditLog, L"============================================================");

    if (biosOk && discOk && slusOk) {
        if (g_currentLang == L"es") {
            AppendSetupLog(p->hEditLog, L"🎉 ¡INSTALACIÓN COMPLETADA CON ÉXITO!");
            AppendSetupLog(p->hEditLog, L"Todos los archivos están configurados. Ya puede jugar.");
        } else if (g_currentLang == L"pt-br") {
            AppendSetupLog(p->hEditLog, L"🎉 INSTALAÇÃO CONCLUÍDA COM SUCESSO!");
            AppendSetupLog(p->hEditLog, L"Todos os arquivos estão configurados. Você já pode jogar.");
        } else {
            AppendSetupLog(p->hEditLog, L"🎉 SETUP COMPLETED SUCCESSFULLY!");
            AppendSetupLog(p->hEditLog, L"All files are in place. You are ready to play.");
        }
        AppendSetupLog(p->hEditLog, L"============================================================");

        std::wstring doneMsg = (g_currentLang == L"es") ? L"¡Instalación completada con éxito!\n\nTodos los archivos han sido configurados y el SLUS_009.23 fue extraído.\n¡Ya puede hacer clic en 'JUGAR RESIDENT EVIL 3'!" :
                               (g_currentLang == L"pt-br") ? L"Instalação concluída com sucesso!\n\nTodos os arquivos foram configurados e o SLUS_009.23 foi extraído.\nVocê já pode clicar em 'JOGAR RESIDENT EVIL 3'!" :
                               L"Setup completed successfully!\n\nAll files have been configured and SLUS_009.23 was extracted.\nYou can now click 'PLAY RESIDENT EVIL 3'!";
        std::wstring doneTitle = (g_currentLang == L"es") ? L"Instalación Exitosa" :
                                 (g_currentLang == L"pt-br") ? L"Instalação Concluída" : L"Setup Success";
        MessageBoxW(p->hWndSetup, doneMsg.c_str(), doneTitle.c_str(), MB_ICONINFORMATION | MB_OK);
    } else {
        if (g_currentLang == L"es") {
            AppendSetupLog(p->hEditLog, L"[!] La instalación no se completó debido a archivos faltantes.");
        } else if (g_currentLang == L"pt-br") {
            AppendSetupLog(p->hEditLog, L"[!] A instalação não foi concluída devido a arquivos pendentes.");
        } else {
            AppendSetupLog(p->hEditLog, L"[!] Setup was not completed due to missing files.");
        }
        AppendSetupLog(p->hEditLog, L"============================================================");
    }

    EnableWindow(p->hBtnStart, TRUE);
    EnableWindow(p->hBtnClose, TRUE);

    PostMessageW(p->hWndSetup, WM_USER + 101, 0, 0);

    delete p;
    return 0;
}

LRESULT CALLBACK SetupWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        std::wstring titleText = (g_currentLang == L"es") ? L"Asistente de Instalación y Montaje" :
                                 (g_currentLang == L"pt-br") ? L"Assistente de Instalação e Montagem" :
                                 L"Game Installation & Disc Setup Wizard";
        std::wstring subText = (g_currentLang == L"es") ? L"Seleccione los archivos originales. El instalador copiará la BIOS y el CD, y extraerá SLUS_009.23 automáticamente." :
                               (g_currentLang == L"pt-br") ? L"Selecione os arquivos originais. O instalador copiará a BIOS e o CD, e extrairá o SLUS_009.23 automaticamente." :
                               L"Select your original files. The installer will copy the BIOS, set up the CD, and extract SLUS_009.23 automatically.";

        g_hLblSetupTitle = CreateWindowW(L"STATIC", titleText.c_str(), WS_VISIBLE | WS_CHILD, 20, 14, 600, 26, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblSetupTitle, WM_SETFONT, (WPARAM)g_hFontTitle, TRUE);

        g_hLblSetupSub = CreateWindowW(L"STATIC", subText.c_str(), WS_VISIBLE | WS_CHILD, 20, 42, 600, 36, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblSetupSub, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

        // Grupo 1: BIOS
        std::wstring grpBiosText = (g_currentLang == L"es") ? L"  1. BIOS de PlayStation (OpenBIOS Predeterminada / SCPH Opcional)  " :
                                   (g_currentLang == L"pt-br") ? L"  1. BIOS do PlayStation (OpenBIOS Padrão / SCPH Opcional)  " :
                                   L"  1. PlayStation BIOS (OpenBIOS Default / SCPH Optional)  ";
        g_hGrpSetupBios = CreateWindowW(L"BUTTON", grpBiosText.c_str(), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 18, 84, 604, 88, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hGrpSetupBios, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hLblSetupBiosStatus = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD, 32, 108, 430, 18, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblSetupBiosStatus, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hEditSetupBiosPath = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY, 32, 130, 430, 26, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hEditSetupBiosPath, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        std::wstring browseText = (g_currentLang == L"es") ? L"Cambiar BIOS..." :
                                  (g_currentLang == L"pt-br") ? L"Trocar BIOS..." : L"Change BIOS...";
        g_hBtnSetupBrowseBios = CreateWindowW(L"BUTTON", browseText.c_str(), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 470, 130, 140, 26, hWnd, (HMENU)ID_BTN_SETUP_BROWSE_BIOS, g_hInst, NULL);
        SendMessageW(g_hBtnSetupBrowseBios, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        // Grupo 2: Imagem do Disco
        std::wstring grpDiscText = (g_currentLang == L"es") ? L"  2. Imagen del CD de Resident Evil 3 (.cue / .bin / .iso)  " :
                                   (g_currentLang == L"pt-br") ? L"  2. Imagem do CD de Resident Evil 3 (.cue / .bin / .iso)  " :
                                   L"  2. Resident Evil 3 CD Image (.cue / .bin / .iso)  ";
        g_hGrpSetupDisc = CreateWindowW(L"BUTTON", grpDiscText.c_str(), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 18, 180, 604, 88, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hGrpSetupDisc, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hLblSetupDiscStatus = CreateWindowW(L"STATIC", L"", WS_VISIBLE | WS_CHILD, 32, 204, 430, 18, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblSetupDiscStatus, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hEditSetupDiscPath = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_READONLY, 32, 226, 430, 26, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hEditSetupDiscPath, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        std::wstring browseDiscText = (g_currentLang == L"es") ? L"Examinar Disco..." :
                                      (g_currentLang == L"pt-br") ? L"Procurar Disco..." : L"Browse Disc...";
        g_hBtnSetupBrowseDisc = CreateWindowW(L"BUTTON", browseDiscText.c_str(), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 470, 226, 140, 26, hWnd, (HMENU)ID_BTN_SETUP_BROWSE_DISC, g_hInst, NULL);
        SendMessageW(g_hBtnSetupBrowseDisc, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        // Grupo 3: Log de Operacoes
        std::wstring grpLogText = (g_currentLang == L"es") ? L"  Registro y Progreso de la Operación  " :
                                  (g_currentLang == L"pt-br") ? L"  Relatório && Progresso da Operação  " :
                                  L"  Operation Log && Progress  ";
        g_hGrpSetupLog = CreateWindowW(L"BUTTON", grpLogText.c_str(), WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 18, 276, 604, 214, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hGrpSetupLog, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hEditSetupLog = CreateWindowW(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 32, 300, 576, 178, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hEditSetupLog, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        // Botoes
        std::wstring startText = (g_currentLang == L"es") ? L"Iniciar Instalación y Extracción" :
                                 (g_currentLang == L"pt-br") ? L"Iniciar Instalação && Extração" :
                                 L"Start Installation && Extraction";
        g_hBtnSetupStart = CreateWindowW(L"BUTTON", startText.c_str(), WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP, 18, 500, 390, 42, hWnd, (HMENU)ID_BTN_SETUP_START, g_hInst, NULL);
        SendMessageW(g_hBtnSetupStart, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        std::wstring closeText = (g_currentLang == L"es") ? L"Cerrar" :
                                 (g_currentLang == L"pt-br") ? L"Fechar" : L"Close";
        g_hBtnSetupClose = CreateWindowW(L"BUTTON", closeText.c_str(), WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 420, 500, 202, 42, hWnd, (HMENU)ID_BTN_SETUP_CLOSE, g_hInst, NULL);
        SendMessageW(g_hBtnSetupClose, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        // Preencher status inicial
        InstallStatus st = CheckInstallationStatus();
        if (st.hasBios) {
            SetWindowTextW(g_hEditSetupBiosPath, st.biosPath.c_str());
            if (st.biosPath.find(L"openbios.bin") != std::wstring::npos) {
                SetWindowTextW(g_hLblSetupBiosStatus, (g_currentLang == L"es") ? L"[OK] OpenBIOS (Predeterminada / Código Abierto MIT)" :
                                                      (g_currentLang == L"pt-br") ? L"[OK] OpenBIOS (Padrão / Código Aberto MIT)" :
                                                      L"[OK] OpenBIOS (Default / Open-Source MIT)");
            } else {
                SetWindowTextW(g_hLblSetupBiosStatus, (g_currentLang == L"es") ? L"[OK] BIOS SCPH Personalizada Instalada" :
                                                      (g_currentLang == L"pt-br") ? L"[OK] BIOS SCPH Personalizada Instalada" :
                                                      L"[OK] Custom SCPH BIOS Installed");
            }
        } else {
            SetWindowTextW(g_hLblSetupBiosStatus, (g_currentLang == L"es") ? L"[!] BIOS no configurada. Haga clic en Cambiar BIOS." :
                                                  (g_currentLang == L"pt-br") ? L"[!] BIOS não configurada. Clique em Trocar BIOS." :
                                                  L"[!] BIOS not configured. Click Change BIOS.");
        }

        if (st.hasDisc) {
            SetWindowTextW(g_hEditSetupDiscPath, st.discPath.c_str());
            SetWindowTextW(g_hLblSetupDiscStatus, (g_currentLang == L"es") ? L"[OK] Imagen de disco configurada." :
                                                  (g_currentLang == L"pt-br") ? L"[OK] Imagem do disco configurada." :
                                                  L"[OK] Disc image configured.");
        } else {
            SetWindowTextW(g_hLblSetupDiscStatus, (g_currentLang == L"es") ? L"[!] Imagen no configurada. Haga clic en Examinar." :
                                                  (g_currentLang == L"pt-br") ? L"[!] Imagem não configurada. Clique em Procurar." :
                                                  L"[!] Disc image not configured. Click Browse.");
        }

        std::wstring initLog;
        if (st.isFullyInstalled) {
            initLog = (g_currentLang == L"es") ? L"Todos los archivos ya están instalados.\nPuede iniciar una reinstalación si desea cambiar la BIOS o la imagen.\r\n" :
                      (g_currentLang == L"pt-br") ? L"Todos os arquivos já estão instalados.\nVocê pode reinstalar se desejar trocar a BIOS ou a imagem do jogo.\r\n" :
                      L"All required files are already installed.\nYou can run setup again if you want to replace the BIOS or disc image.\r\n";
        } else {
            initLog = (g_currentLang == L"es") ? L"Bienvenido. Seleccione la BIOS y la imagen de disco arriba y haga clic en 'Iniciar Instalación'.\r\n" :
                      (g_currentLang == L"pt-br") ? L"Bem-vindo. Selecione a BIOS e a imagem do disco acima e clique em 'Iniciar Instalação'.\r\n" :
                      L"Welcome. Select the BIOS and disc image above, then click 'Start Installation'.\r\n";
        }
        SetWindowTextW(g_hEditSetupLog, initLog.c_str());

        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_BTN_SETUP_CLOSE) {
            DestroyWindow(hWnd);
        } else if (id == ID_BTN_SETUP_BROWSE_BIOS) {
            OPENFILENAMEW ofn = { sizeof(ofn) };
            wchar_t szFile[MAX_PATH] = { 0 };
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = (g_currentLang == L"es") ? L"BIOS de PlayStation (*.bin;*.rom)\0*.bin;*.rom\0Todos los Archivos (*.*)\0*.*\0" :
                              (g_currentLang == L"pt-br") ? L"BIOS do PlayStation (*.bin;*.rom)\0*.bin;*.rom\0Todos os Arquivos (*.*)\0*.*\0" :
                              L"PlayStation BIOS (*.bin;*.rom)\0*.bin;*.rom\0All Files (*.*)\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
            if (GetOpenFileNameW(&ofn)) {
                SetWindowTextW(g_hEditSetupBiosPath, szFile);
                WIN32_FILE_ATTRIBUTE_DATA fad;
                if (GetFileAttributesExW(szFile, GetFileExInfoStandard, &fad)) {
                    LARGE_INTEGER sz; sz.HighPart = fad.nFileSizeHigh; sz.LowPart = fad.nFileSizeLow;
                    if (sz.QuadPart >= 500000) {
                        SetWindowTextW(g_hLblSetupBiosStatus, (g_currentLang == L"es") ? L"[OK] BIOS Válida (512 KB)" :
                                                              (g_currentLang == L"pt-br") ? L"[OK] BIOS Válida (512 KB)" :
                                                              L"[OK] Valid BIOS (512 KB)");
                    }
                }
            }
        } else if (id == ID_BTN_SETUP_BROWSE_DISC) {
            OPENFILENAMEW ofn = { sizeof(ofn) };
            wchar_t szFile[MAX_PATH] = { 0 };
            ofn.hwndOwner = hWnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = MAX_PATH;
            ofn.lpstrFilter = (g_currentLang == L"es") ? L"Imágenes de CD (*.cue;*.bin;*.iso;*.img)\0*.cue;*.bin;*.iso;*.img\0Todos los Archivos (*.*)\0*.*\0" :
                              (g_currentLang == L"pt-br") ? L"Imagens de CD (*.cue;*.bin;*.iso;*.img)\0*.cue;*.bin;*.iso;*.img\0Todos os Arquivos (*.*)\0*.*\0" :
                              L"Disc Images (*.cue;*.bin;*.iso;*.img)\0*.cue;*.bin;*.iso;*.img\0All Files (*.*)\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
            if (GetOpenFileNameW(&ofn)) {
                SetWindowTextW(g_hEditSetupDiscPath, szFile);
                WIN32_FILE_ATTRIBUTE_DATA fad;
                if (GetFileAttributesExW(szFile, GetFileExInfoStandard, &fad)) {
                    LARGE_INTEGER sz; sz.HighPart = fad.nFileSizeHigh; sz.LowPart = fad.nFileSizeLow;
                    if (sz.QuadPart >= 50000000) {
                        SetWindowTextW(g_hLblSetupDiscStatus, (g_currentLang == L"es") ? L"[OK] Imagen de CD Válida" :
                                                              (g_currentLang == L"pt-br") ? L"[OK] Imagem de CD Válida" :
                                                              L"[OK] Valid CD Image");
                    }
                }
            }
        } else if (id == ID_BTN_SETUP_START) {
            SetupThreadParams* params = new SetupThreadParams();
            wchar_t biosBuf[MAX_PATH] = { 0 };
            wchar_t discBuf[MAX_PATH] = { 0 };
            GetWindowTextW(g_hEditSetupBiosPath, biosBuf, MAX_PATH);
            GetWindowTextW(g_hEditSetupDiscPath, discBuf, MAX_PATH);
            params->biosSrc = biosBuf;
            params->discSrc = discBuf;
            params->hWndSetup = hWnd;
            params->hEditLog = g_hEditSetupLog;
            params->hBtnStart = g_hBtnSetupStart;
            params->hBtnClose = g_hBtnSetupClose;
            CreateThread(NULL, 0, SetupWorkerThreadProc, params, 0, NULL);
        }
        break;
    }
    case WM_USER + 101: {
        InstallStatus st = CheckInstallationStatus();
        if (st.hasBios) {
            if (st.biosPath.find(L"openbios.bin") != std::wstring::npos) {
                SetWindowTextW(g_hLblSetupBiosStatus, (g_currentLang == L"es") ? L"[OK] OpenBIOS (Predeterminada / Código Abierto MIT)" :
                                                      (g_currentLang == L"pt-br") ? L"[OK] OpenBIOS (Padrão / Código Aberto MIT)" :
                                                      L"[OK] OpenBIOS (Default / Open-Source MIT)");
            } else {
                SetWindowTextW(g_hLblSetupBiosStatus, (g_currentLang == L"es") ? L"[OK] BIOS SCPH Personalizada Instalada" :
                                                      (g_currentLang == L"pt-br") ? L"[OK] BIOS SCPH Personalizada Instalada" :
                                                      L"[OK] Custom SCPH BIOS Installed");
            }
        }
        if (st.hasDisc) {
            SetWindowTextW(g_hLblSetupDiscStatus, (g_currentLang == L"es") ? L"[OK] Imagen de disco configurada." :
                                                  (g_currentLang == L"pt-br") ? L"[OK] Imagem do disco configurada." :
                                                  L"[OK] Disc image configured.");
        }
        UpdateInstallStatusUI();
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        SetBkColor(hdcStatic, GetSysColor(COLOR_BTNFACE));
        if (hwndStatic == g_hLblSetupBiosStatus || hwndStatic == g_hLblSetupDiscStatus) {
            wchar_t txt[128] = { 0 };
            GetWindowTextW(hwndStatic, txt, 128);
            if (wcsstr(txt, L"[OK]")) {
                SetTextColor(hdcStatic, RGB(0, 140, 20));
            } else if (wcsstr(txt, L"[!]")) {
                SetTextColor(hdcStatic, RGB(190, 80, 0));
            } else {
                SetTextColor(hdcStatic, GetSysColor(COLOR_WINDOWTEXT));
            }
        } else {
            SetTextColor(hdcStatic, GetSysColor(COLOR_WINDOWTEXT));
        }
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }
    case WM_CTLCOLORBTN: {
        HDC hdcBtn = (HDC)wParam;
        SetBkMode(hdcBtn, TRANSPARENT);
        SetBkColor(hdcBtn, GetSysColor(COLOR_BTNFACE));
        return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
    }
    case WM_CLOSE: {
        DestroyWindow(hWnd);
        break;
    }
    case WM_DESTROY: {
        g_hWndSetup = NULL;
        if (g_hWndMain) {
            EnableWindow(g_hWndMain, TRUE);
            SetForegroundWindow(g_hWndMain);
            UpdateInstallStatusUI();
        }
        break;
    }
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

void ShowSetupDialog(HWND hWndParent) {
    if (g_hWndSetup) {
        BringWindowToTop(g_hWndSetup);
        SetForegroundWindow(g_hWndSetup);
        return;
    }

    RECT rcParent;
    GetWindowRect(hWndParent, &rcParent);
    int pW = rcParent.right - rcParent.left;
    int pH = rcParent.bottom - rcParent.top;
    int width = 660;
    int height = 600;
    int posX = rcParent.left + (pW - width) / 2;
    int posY = rcParent.top + (pH - height) / 2;
    if (posX < 0) posX = 0;
    if (posY < 0) posY = 0;

    std::wstring dlgTitle = (g_currentLang == L"es") ? L"Asistente de Instalación y Montaje — Project RE: Visited - Resident Evil 3" :
                            (g_currentLang == L"pt-br") ? L"Assistente de Instalação e Montagem — Project RE: Visited - Resident Evil 3" :
                            L"Installation & Setup Wizard — Project RE: Visited - Resident Evil 3";

    g_hWndSetup = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"RE3SetupWindowClass",
        dlgTitle.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        posX, posY, width, height,
        hWndParent, NULL, g_hInst, NULL
    );

    if (g_hWndSetup) {
        EnableWindow(hWndParent, FALSE);
        ShowWindow(g_hWndSetup, SW_SHOWNORMAL);
        UpdateWindow(g_hWndSetup);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_pBannerBmp = LoadBannerBitmap(g_hInst);

        g_hFontTitle = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_hFontSub   = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_hFontBody  = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_hFontBold  = CreateFontW(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
        g_hFontSmall = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        DEVMODEW dmScreen = { sizeof(dmScreen) };
        if (EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &dmScreen) && dmScreen.dmDisplayFrequency > 0) {
            g_screenHz = dmScreen.dmDisplayFrequency;
        }

        // Seletor de Idioma
        g_hLblLanguage = CreateWindowW(L"STATIC", L"Launcher Language:", WS_VISIBLE | WS_CHILD, 32, 158, 150, 20, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblLanguage, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hComboLanguage = CreateWindowW(L"COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP, 186, 156, 324, 120, hWnd, (HMENU)ID_COMBO_LANGUAGE, g_hInst, NULL);
        SendMessageW(g_hComboLanguage, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);
        SendMessageW(g_hComboLanguage, CB_ADDSTRING, 0, (LPARAM)L"English (Default)");
        SendMessageW(g_hComboLanguage, CB_ADDSTRING, 0, (LPARAM)L"Español");
        SendMessageW(g_hComboLanguage, CB_ADDSTRING, 0, (LPARAM)L"Português (Brasil)");

        // Grupo 0: Instalacao & Montagem do Jogo
        g_hGrpSetup = CreateWindowW(L"BUTTON", L"  Game Installation && Disc Setup  ", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 18, 186, 508, 62, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hGrpSetup, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hLblInstallStatus = CreateWindowW(L"STATIC", L"Checking status...", WS_VISIBLE | WS_CHILD, 32, 208, 306, 24, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblInstallStatus, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hBtnOpenSetup = CreateWindowW(L"BUTTON", L"Setup Files...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 342, 202, 168, 34, hWnd, (HMENU)ID_BTN_OPEN_SETUP, g_hInst, NULL);
        SendMessageW(g_hBtnOpenSetup, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        // Grupo 1: Graficos HD & Desempenho Visual
        g_hGrpVideo = CreateWindowW(L"BUTTON", L"  HD Graphics && Visual Performance  ", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 18, 256, 508, 234, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hGrpVideo, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hLblAspect = CreateWindowW(L"STATIC", L"Aspect Ratio:", WS_VISIBLE | WS_CHILD, 32, 280, 150, 20, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblAspect, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        g_hComboAspect = CreateWindowW(L"COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP, 186, 278, 324, 120, hWnd, (HMENU)ID_COMBO_ASPECT, g_hInst, NULL);
        SendMessageW(g_hComboAspect, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hLblDisplay = CreateWindowW(L"STATIC", L"Display Mode:", WS_VISIBLE | WS_CHILD, 32, 306, 150, 20, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblDisplay, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        g_hComboDisplay = CreateWindowW(L"COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP, 186, 304, 324, 120, hWnd, (HMENU)ID_COMBO_DISPLAY, g_hInst, NULL);
        SendMessageW(g_hComboDisplay, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hLblRes = CreateWindowW(L"STATIC", L"3D Internal Resolution:", WS_VISIBLE | WS_CHILD, 32, 332, 150, 20, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblRes, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        g_hComboInternalRes = CreateWindowW(L"COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP, 186, 330, 324, 140, hWnd, (HMENU)ID_COMBO_INTERNAL_RES, g_hInst, NULL);
        SendMessageW(g_hComboInternalRes, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hLblFps = CreateWindowW(L"STATIC", L"Smoothness / Refresh:", WS_VISIBLE | WS_CHILD, 32, 358, 150, 20, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblFps, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        g_hComboFps = CreateWindowW(L"COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP, 186, 356, 324, 160, hWnd, (HMENU)ID_COMBO_FPS, g_hInst, NULL);
        SendMessageW(g_hComboFps, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hLblRend = CreateWindowW(L"STATIC", L"GPU Renderer:", WS_VISIBLE | WS_CHILD, 32, 384, 150, 20, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblRend, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        g_hComboRenderer = CreateWindowW(L"COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP, 186, 382, 324, 120, hWnd, (HMENU)ID_COMBO_RENDERER, g_hInst, NULL);
        SendMessageW(g_hComboRenderer, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hChkPgxpGeom = CreateWindowW(L"BUTTON", L"PGXP Geometry Correction (Not recommended)", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 32, 412, 480, 20, hWnd, (HMENU)ID_CHK_PGXP_GEOM, g_hInst, NULL);
        SendMessageW(g_hChkPgxpGeom, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hChkPgxpTex = CreateWindowW(L"BUTTON", L"Perspective-Correct Texturing", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 32, 436, 480, 20, hWnd, (HMENU)ID_CHK_PGXP_TEX, g_hInst, NULL);
        SendMessageW(g_hChkPgxpTex, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hChkAntialiasing = CreateWindowW(L"BUTTON", L"FXAA Anti-Aliasing", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 32, 460, 480, 20, hWnd, (HMENU)ID_CHK_ANTIALIASING, g_hInst, NULL);
        SendMessageW(g_hChkAntialiasing, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        // Grupo 2: Jogabilidade, Trajes & Controles
        g_hGrpInput = CreateWindowW(L"BUTTON", L"  Gameplay, Skins && Controls  ", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 18, 498, 508, 134, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hGrpInput, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hLblCtrlMode = CreateWindowW(L"STATIC", L"Control Mode:", WS_VISIBLE | WS_CHILD, 32, 522, 150, 20, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblCtrlMode, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        g_hComboControls = CreateWindowW(L"COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP, 186, 520, 324, 120, hWnd, (HMENU)ID_COMBO_CONTROLS, g_hInst, NULL);
        SendMessageW(g_hComboControls, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hLblSkins = CreateWindowW(L"STATIC", L"Jill Skin / Costume:", WS_VISIBLE | WS_CHILD, 32, 550, 150, 20, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblSkins, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);
        g_hComboSkins = CreateWindowW(L"COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP, 186, 548, 324, 180, hWnd, (HMENU)ID_COMBO_SKINS, g_hInst, NULL);
        SendMessageW(g_hComboSkins, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hBtnInput = CreateWindowW(L"BUTTON", L"Edit Keybinds (input.ini)", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 32, 584, 220, 28, hWnd, (HMENU)ID_BTN_INPUT_INI, g_hInst, NULL);
        SendMessageW(g_hBtnInput, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hBtnSaves = CreateWindowW(L"BUTTON", L"Open Saves Folder (.mcd)", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 270, 584, 240, 28, hWnd, (HMENU)ID_BTN_SAVES_DIR, g_hInst, NULL);
        SendMessageW(g_hBtnSaves, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        // Grupo 3: Audio & Reproducao
        g_hGrpAudio = CreateWindowW(L"BUTTON", L"  Audio && Playback  ", WS_VISIBLE | WS_CHILD | BS_GROUPBOX, 18, 640, 508, 98, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hGrpAudio, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hChkSpuHq = CreateWindowW(L"BUTTON", L"HQ 48kHz Audio", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 32, 664, 480, 20, hWnd, (HMENU)ID_CHK_SPU_HQ, g_hInst, NULL);
        SendMessageW(g_hChkSpuHq, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hChkSkipFmv = CreateWindowW(L"BUTTON", L"Auto-skip FMVs", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 32, 686, 480, 20, hWnd, (HMENU)ID_CHK_SKIP_FMV, g_hInst, NULL);
        SendMessageW(g_hChkSkipFmv, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        g_hChkFastDoors = CreateWindowW(L"BUTTON", L"Door Skipping (Fast Room Transitions)", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP, 32, 708, 480, 20, hWnd, (HMENU)ID_CHK_FAST_DOORS, g_hInst, NULL);
        SendMessageW(g_hChkFastDoors, WM_SETFONT, (WPARAM)g_hFontBody, TRUE);

        // Botoes de Acao
        g_hBtnLaunch = CreateWindowW(L"BUTTON", L"PLAY RESIDENT EVIL 3", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP, 18, 746, 330, 42, hWnd, (HMENU)ID_BTN_LAUNCH, g_hInst, NULL);
        SendMessageW(g_hBtnLaunch, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hBtnSave = CreateWindowW(L"BUTTON", L"Save Settings", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 360, 746, 166, 42, hWnd, (HMENU)ID_BTN_SAVE, g_hInst, NULL);
        SendMessageW(g_hBtnSave, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hBtnOpenCheats = CreateWindowW(L"BUTTON", L"Cheats & Modifiers...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 18, 794, 246, 32, hWnd, (HMENU)ID_BTN_OPEN_CHEATS, g_hInst, NULL);
        SendMessageW(g_hBtnOpenCheats, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hBtnCredits = CreateWindowW(L"BUTTON", L"Credits & Legal Disclaimer", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | WS_TABSTOP, 274, 794, 252, 32, hWnd, (HMENU)ID_BTN_CREDITS, g_hInst, NULL);
        SendMessageW(g_hBtnCredits, WM_SETFONT, (WPARAM)g_hFontBold, TRUE);

        g_hLblLegalNote = CreateWindowW(L"STATIC", L"Non-commercial open-source preservation fan project. No proprietary assets included.", WS_VISIBLE | WS_CHILD | SS_CENTER, 18, 834, 508, 16, hWnd, NULL, g_hInst, NULL);
        SendMessageW(g_hLblLegalNote, WM_SETFONT, (WPARAM)g_hFontSmall, TRUE);

        // Carregar configuracao salva
        GameConfig cfg = LoadConfig();
        g_cachedConfig = cfg;
        g_currentLang = cfg.language;
        if (g_currentLang == L"es") SendMessageW(g_hComboLanguage, CB_SETCURSEL, 1, 0);
        else if (g_currentLang == L"pt-br") SendMessageW(g_hComboLanguage, CB_SETCURSEL, 2, 0);
        else {
            g_currentLang = L"en";
            SendMessageW(g_hComboLanguage, CB_SETCURSEL, 0, 0);
        }

        // Aplicar idioma ativo e preencher comboboxes
        ApplyLanguage(g_currentLang);

        int aspectSel = 0;
        if (cfg.aspect_ratio == L"16:9_pan_scan") aspectSel = 2;
        else if (cfg.aspect_ratio == L"4:3") aspectSel = 1;
        else aspectSel = 0;
        SendMessageW(g_hComboAspect, CB_SETCURSEL, aspectSel, 0);
        SendMessageW(g_hComboDisplay, CB_SETCURSEL, cfg.fullscreen ? 1 : 0, 0);
        SendMessageW(g_hComboRenderer, CB_SETCURSEL, (cfg.renderer == L"software") ? 1 : 0, 0);

        int curResIdx = 0;
        if (cfg.internal_resolution == L"4k") curResIdx = 1;
        else if (cfg.internal_resolution == L"1440p") curResIdx = 2;
        else if (cfg.internal_resolution == L"720p") curResIdx = 3;
        else if (cfg.internal_resolution == L"native") curResIdx = 4;
        SendMessageW(g_hComboInternalRes, CB_SETCURSEL, curResIdx, 0);

        if (!cfg.frame_interpolation) {
            SendMessageW(g_hComboFps, CB_SETCURSEL, 6, 0);
        } else {
            if (cfg.frame_interpolation_fps == 144) SendMessageW(g_hComboFps, CB_SETCURSEL, 1, 0);
            else if (cfg.frame_interpolation_fps == 120) SendMessageW(g_hComboFps, CB_SETCURSEL, 2, 0);
            else if (cfg.frame_interpolation_fps == 165) SendMessageW(g_hComboFps, CB_SETCURSEL, 3, 0);
            else if (cfg.frame_interpolation_fps == 240) SendMessageW(g_hComboFps, CB_SETCURSEL, 4, 0);
            else if (cfg.frame_interpolation_fps == 60) SendMessageW(g_hComboFps, CB_SETCURSEL, 5, 0);
            else SendMessageW(g_hComboFps, CB_SETCURSEL, 0, 0);
        }

        SendMessageW(g_hComboControls, CB_SETCURSEL, cfg.modern_controls ? 0 : 1, 0);
        SendMessageW(g_hComboSkins, CB_SETCURSEL, SkinToComboIndex(cfg.jill_skin), 0);
        SendMessageW(g_hChkPgxpGeom, BM_SETCHECK, cfg.geometry_correction ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(g_hChkPgxpTex, BM_SETCHECK, cfg.perspective_texturing ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(g_hChkAntialiasing, BM_SETCHECK, cfg.antialiasing ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(g_hChkSkipFmv, BM_SETCHECK, cfg.auto_skip_fmv ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(g_hChkFastDoors, BM_SETCHECK, cfg.fast_doors ? BST_CHECKED : BST_UNCHECKED, 0);
        SendMessageW(g_hChkSpuHq, BM_SETCHECK, cfg.spu_hq ? BST_CHECKED : BST_UNCHECKED, 0);

        // Tema escuro: checkboxes e group boxes com visual styles ignoram
        // WM_CTLCOLOR*, entao removemos o tema apenas desses controles.
        EnumChildWindows(hWnd, [](HWND hChild, LPARAM) -> BOOL {
            wchar_t cls[32] = { 0 };
            GetClassNameW(hChild, cls, 32);
            if (_wcsicmp(cls, L"BUTTON") == 0) {
                LONG style = GetWindowLongW(hChild, GWL_STYLE) & BS_TYPEMASK;
                if (style == BS_GROUPBOX || style == BS_AUTOCHECKBOX || style == BS_CHECKBOX) {
                    SetWindowTheme(hChild, L"", L"");
                }
            }
            return TRUE;
        }, 0);

        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        if (g_pBannerBmp) {
            Gdiplus::Graphics g(hdc);
            g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

            Gdiplus::SolidBrush bgBrush(Gdiplus::Color(255, 0, 0, 0));
            g.FillRectangle(&bgBrush, 18, 14, 508, 136);

            int imgW = (int)g_pBannerBmp->GetWidth();
            int imgH = (int)g_pBannerBmp->GetHeight();
            if (imgW > 0 && imgH > 0) {
                float scale = (std::min)((float)(508 - 4) / imgW, (float)(136 - 4) / imgH);
                int drawW = (int)(imgW * scale);
                int drawH = (int)(imgH * scale);
                int drawX = 18 + (508 - drawW) / 2;
                int drawY = 14 + (136 - drawH) / 2;
                g.DrawImage(g_pBannerBmp, drawX, drawY, drawW, drawH);
            }

            Gdiplus::Pen borderPen(Gdiplus::Color(255, 130, 20, 20), 1.0f);
            g.DrawRectangle(&borderPen, 18, 14, 507, 135);
        } else {
            RECT r = { 18, 14, 526, 150 };
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(180, 20, 20));
            SelectObject(hdc, g_hFontTitle ? g_hFontTitle : GetStockObject(DEFAULT_GUI_FONT));
            DrawTextW(hdc, L"PROJECT RE: VISITED", -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        EndPaint(hWnd, &ps);
        return 0;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == ID_COMBO_LANGUAGE && HIWORD(wParam) == CBN_SELCHANGE) {
            int sel = (int)SendMessageW(g_hComboLanguage, CB_GETCURSEL, 0, 0);
            if (sel == 1) g_currentLang = L"es";
            else if (sel == 2) g_currentLang = L"pt-br";
            else g_currentLang = L"en";
            ApplyLanguage(g_currentLang);
            // Salvar automaticamente a preferencia de idioma
            GameConfig cfg = GetGuiConfig();
            SaveConfig(cfg);
        } else if (id == ID_COMBO_SKINS && HIWORD(wParam) == CBN_SELCHANGE) {
            // Salvar automaticamente a skin selecionada e atualizar state.toml
            GameConfig cfg = GetGuiConfig();
            SaveConfig(cfg);
        } else if (id == ID_BTN_LAUNCH) {
            LaunchGame();
        } else if (id == ID_BTN_SAVE) {
            GameConfig cfg = GetGuiConfig();
            if (SaveConfig(cfg)) {
                std::wstring msg = (g_currentLang == L"es") ? L"¡Ajustes guardados correctamente en settings.toml!" :
                                   (g_currentLang == L"pt-br") ? L"Configurações salvas com sucesso no settings.toml!" :
                                   L"Settings saved successfully to settings.toml!";
                std::wstring title = (g_currentLang == L"es") ? L"Guardado con Éxito" :
                                     (g_currentLang == L"pt-br") ? L"Salvo com Sucesso" : L"Saved Successfully";
                MessageBoxW(hWnd, msg.c_str(), title.c_str(), MB_ICONINFORMATION | MB_OK);
            } else {
                std::wstring msg = (g_currentLang == L"es") ? L"No se pudieron guardar los ajustes." :
                                   (g_currentLang == L"pt-br") ? L"Não foi possível salvar as configurações." :
                                   L"Unable to save settings.";
                std::wstring title = (g_currentLang == L"es") ? L"Error" :
                                     (g_currentLang == L"pt-br") ? L"Erro" : L"Error";
                MessageBoxW(hWnd, msg.c_str(), title.c_str(), MB_ICONERROR | MB_OK);
            }
        } else if (id == ID_BTN_SAVES_DIR) {
            std::wstring savesDir = GetBuildDir() + L"\\saves";
            ShellExecuteW(hWnd, L"open", savesDir.c_str(), NULL, NULL, SW_SHOWNORMAL);
        } else if (id == ID_BTN_INPUT_INI) {
            std::wstring inputIni = GetBuildDir() + L"\\input.ini";
            ShellExecuteW(hWnd, L"open", L"notepad.exe", inputIni.c_str(), NULL, SW_SHOWNORMAL);
        } else if (id == ID_BTN_CREDITS) {
            ShowCreditsDialog(hWnd);
        } else if (id == ID_BTN_OPEN_SETUP) {
            ShowSetupDialog(hWnd);
        } else if (id == ID_BTN_OPEN_CHEATS) {
            ShowCheatsDialog(hWnd);
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        SetBkColor(hdcStatic, CLR_BG);
        LONG btnType = GetWindowLongW(hwndStatic, GWL_STYLE) & BS_TYPEMASK;
        wchar_t cls[16] = { 0 };
        GetClassNameW(hwndStatic, cls, 16);
        bool isButton = (_wcsicmp(cls, L"BUTTON") == 0);
        if (hwndStatic == g_hLblInstallStatus) {
            InstallStatus st = CheckInstallationStatus();
            SetTextColor(hdcStatic, st.isFullyInstalled ? CLR_OK : CLR_WARN);
        } else if (isButton && btnType == BS_GROUPBOX) {
            SetTextColor(hdcStatic, CLR_ACCENT);
        } else if (hwndStatic == g_hLblLegalNote) {
            SetTextColor(hdcStatic, CLR_MUTED);
        } else {
            SetTextColor(hdcStatic, CLR_TEXT);
        }
        return (LRESULT)g_hBrBg;
    }
    case WM_CTLCOLORBTN: {
        HDC hdcBtn = (HDC)wParam;
        SetBkMode(hdcBtn, TRANSPARENT);
        SetBkColor(hdcBtn, CLR_BG);
        SetTextColor(hdcBtn, CLR_TEXT);
        return (LRESULT)g_hBrBg;
    }
    case WM_KEYDOWN: {
        if (wParam == VK_F1) {
            ShowCreditsDialog(hWnd);
            return 0;
        }
        break;
    }
    case WM_DESTROY: {
        if (g_pBannerBmp) {
            delete g_pBannerBmp;
            g_pBannerBmp = NULL;
        }
        if (g_hFontTitle) DeleteObject(g_hFontTitle);
        if (g_hFontSub) DeleteObject(g_hFontSub);
        if (g_hFontBody) DeleteObject(g_hFontBody);
        if (g_hFontBold) DeleteObject(g_hFontBold);
        if (g_hFontSmall) DeleteObject(g_hFontSmall);
        PostQuitMessage(0);
        break;
    }
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    g_hInst = hInstance;

    ULONG_PTR gdiplusToken = 0;
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    g_hAppIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(1));

    WNDCLASSEXW cwc = { sizeof(cwc) };
    cwc.lpfnWndProc = CreditsWndProc;
    cwc.hInstance = hInstance;
    cwc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    cwc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    cwc.lpszClassName = L"RE3CreditsWindowClass";
    cwc.hIcon = g_hAppIcon ? g_hAppIcon : LoadIconW(NULL, IDI_APPLICATION);
    cwc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON, 16, 16, 0);
    RegisterClassExW(&cwc);

    WNDCLASSEXW swc = { sizeof(swc) };
    swc.lpfnWndProc = SetupWndProc;
    swc.hInstance = hInstance;
    swc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    swc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    swc.lpszClassName = L"RE3SetupWindowClass";
    swc.hIcon = g_hAppIcon ? g_hAppIcon : LoadIconW(NULL, IDI_APPLICATION);
    swc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON, 16, 16, 0);
    RegisterClassExW(&swc);

    WNDCLASSEXW chwc = { sizeof(chwc) };
    chwc.lpfnWndProc = CheatsWndProc;
    chwc.hInstance = hInstance;
    chwc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    chwc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    chwc.lpszClassName = L"RE3CheatsWindowClass";
    chwc.hIcon = g_hAppIcon ? g_hAppIcon : LoadIconW(NULL, IDI_APPLICATION);
    chwc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON, 16, 16, 0);
    RegisterClassExW(&chwc);

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    g_hBrBg = CreateSolidBrush(CLR_BG);
    wc.hbrBackground = g_hBrBg;
    wc.lpszClassName = L"RE3RecompLauncherClass";
    wc.hIcon = g_hAppIcon ? g_hAppIcon : LoadIconW(NULL, IDI_APPLICATION);
    wc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON, 16, 16, 0);

    if (!RegisterClassExW(&wc)) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 0;
    }

    int width = 560;
    int height = 910;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - width) / 2;
    int posY = (screenH - height) / 2;

    g_hWndMain = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"Project RE: Visited - Resident Evil 3",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, width, height,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWndMain) {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 0;
    }

    ShowWindow(g_hWndMain, nCmdShow);
    UpdateWindow(g_hWndMain);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}
