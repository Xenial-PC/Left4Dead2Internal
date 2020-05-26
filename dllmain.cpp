#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <d3d9.h>
#include <d3dx9.h>
#include "mem.h"
#include "proc.h"
#include "Patternscaning.h"
#include "detours.h"
#include "PEB.h"
#include "Interfaces.h"
#include"CBaseEntity.h"

#pragma comment (lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

int iMenuPage{ 1 }, iMenuSubPage{ 0 };

bool toggle_gui = false, initialized = false, init = false;
bool is_sub_menu = false;

bool p_health = false;
bool p_ammo = false;
bool s_ammo = false;

int* pistol_mask = nullptr;
int* pumpshotgun_mask = nullptr;
int* pistol_ammo = nullptr;
int* player_health = nullptr;
int* pumpshotgun_ammo = nullptr;

uintptr_t _serverDllModuleBase = NULL;

const char* windowName = "Left 4 Dead 2";

HINSTANCE g_hinstDLL = nullptr;

typedef HRESULT(__stdcall* f_EndScene)(IDirect3DDevice9* pDevice);
f_EndScene oEndScene;

typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
WNDPROC oWndProc;

ID3DXFont* font;

void solid_rect(int x, int y, int w, int h, D3DCOLOR d3Color, IDirect3DDevice9* pDevice);
void border_rect(int x, int y, int w, int h, int th, D3DCOLOR color, IDirect3DDevice9* pDevice);
void create_font_b(IDirect3DDevice9* pDevice, int size, const std::string& choiceFont);
void write_text(LPCSTR text, int x, int y, int w, int h, D3DCOLOR color);
bool check_box(int x, int y, bool var, const std::string& name, IDirect3DDevice9* pDevice, POINT Mouse);
void draw_menu(IDirect3DDevice9* pDevice);

bool MenuOpen;
bool FirstLoad = true;

int menuX = 15;
int menuY = 485;

c_base_entity* localPlayer = nullptr;

uintptr_t pistolMask = NULL;
uintptr_t pumpshotgunMask = NULL;
uintptr_t pistolAmmo = NULL;
uintptr_t playerHealth = NULL;
uintptr_t pumpshotgunAmmo = NULL;

IDirect3D9* gPD3D = nullptr;
IDirect3DDevice9* gDevice = nullptr;

static bool ScreenTransform(const Vector& in, Vector& out)
{
    auto &w2sMatrix = i::engine->WorldToScreenMatrix();
    out.x = w2sMatrix.m[0][0] * in.x + w2sMatrix.m[0][1] * in.y + w2sMatrix.m[0][2] * in.z + w2sMatrix.m[0][3];
    out.y = w2sMatrix.m[1][0] * in.x + w2sMatrix.m[1][1] * in.y + w2sMatrix.m[1][2] * in.z + w2sMatrix.m[1][3];
    out.z = 0.0f;

    float w = w2sMatrix.m[3][0] * in.x + w2sMatrix.m[3][1] * in.y + w2sMatrix.m[3][2] * in.z + w2sMatrix.m[3][3];

    if (w < 0.001f) {
        out.x *= 100000;
        out.y *= 100000;
        return false;
    }

    out.x /= w;
    out.y /= w;

    return true;
}

bool WorldToScreen(const Vector& in, Vector& out)
{
    static std::int32_t w = 0;
    static std::int32_t h = 0;
    if (!w || !h)
        i::engine->GetScreenSize(w, h);

    if (!ScreenTransform(in, out))
        return false;

    out.x = (w / 2.0f) + (out.x * w) / 2.0f;
    out.y = (h / 2.0f) - (out.y * h) / 2.0f;

    return true;
}

void initOffsets()
{
    pistolMask = mem::FindDMAAddy(_serverDllModuleBase + 0x07B13EC, { 0x0, 0x60, 0x164, 0x28, 0x8, 0x1EC, 0x1468 });
    pumpshotgunMask = mem::FindDMAAddy(_serverDllModuleBase + 0x07BB97C, { 0x28, 0xC, 0x60, 0x4, 0x184, 0x208, 0x1468 });
    pistolAmmo = mem::FindDMAAddy(_serverDllModuleBase + 0x0814610, { 0x4, 0x60, 0x4, 0x28, 0x8, 0x3A8, 0x1414 });
    playerHealth = mem::FindDMAAddy(_serverDllModuleBase + 0x07BACBC, { 0x60, 0x8, 0x18, 0x8, 0x28, 0xC, 0xEC });
    pumpshotgunAmmo = mem::FindDMAAddy(_serverDllModuleBase + 0x07B13EC, { 0x20, 0x28, 0xC, 0x60, 0x4, 0x1414 });
    
    pistol_mask = reinterpret_cast<int*>(pistolMask);
    pumpshotgun_mask = reinterpret_cast<int*>(pumpshotgunMask);
    pistol_ammo = reinterpret_cast<int*>(pistolAmmo);
    player_health = reinterpret_cast<int*>(playerHealth);
    pumpshotgun_ammo = reinterpret_cast<int*>(pumpshotgunAmmo);
}

void CleanOffsets()
{
    pistolMask = NULL, pumpshotgunMask = NULL, pistolAmmo = NULL, playerHealth = NULL;
    pistol_mask = nullptr, pumpshotgun_mask = nullptr, pistol_ammo = nullptr, player_health = nullptr;
    pumpshotgun_ammo = nullptr;
}

int Hook(PBYTE result)
{
    IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!pD3D)
    {
        return 0;
    }

    D3DPRESENT_PARAMETERS d3dapp{ 0 };
    d3dapp.hDeviceWindow = FindWindowA(nullptr, windowName), d3dapp.SwapEffect = D3DSWAPEFFECT_DISCARD, d3dapp.Windowed = TRUE;

    IDirect3DDevice9* Device = nullptr;
    if (FAILED(pD3D->CreateDevice(0, D3DDEVTYPE_HAL, d3dapp.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dapp, &Device)) && pD3D->Release())
    {
        return 0;
    }

    auto pVTable = *reinterpret_cast<void***>(Device);
    if (Device)
    {
        Device->Release(),
            Device = nullptr;
    }

    gPD3D = pD3D;
    gDevice = Device;

    oEndScene = reinterpret_cast<f_EndScene>(DetourFunction(static_cast<PBYTE>(pVTable[42]), reinterpret_cast<PBYTE>(result)));
    pVTable = nullptr;
    if (pD3D)
    {
        pD3D->Release();
        pD3D = nullptr;
    }
    if (Device)
    {
        Device->Release();
        Device = nullptr;
    }
    return 0;
}

void CleanUp(PBYTE result)
{
    if (gDevice)
    {
        gDevice->Release(),
            gDevice = nullptr;
        CleanOffsets();
    }
    DetourRemove(reinterpret_cast<PBYTE>(oEndScene), reinterpret_cast<PBYTE>(result));
}

HRESULT __stdcall Hooked_EndScene(IDirect3DDevice9* pDevice)
{
    static auto init = false;
    if (!init)
    {
        init = true;
        create_font_b(pDevice, 18, "Times New Roman");
    }
    if (i::engineTool->IsInGame())
    {
        Vector screen;
        localPlayer = static_cast<c_base_entity*>(i::entityList->GetClientEntity(i::engine->GetLocalPlayer()));
        if (localPlayer && i::engineTool->IsInGame())
        {
            std::string h = std::to_string(reinterpret_cast<DWORD>(localPlayer));
            h += " - ";
            h += std::to_string(localPlayer->GetHealth());
            write_text(h.c_str(), 20, 20, 200, 100, D3DCOLOR_ARGB(255, 108, 240, 183));
        }
        for (int x = 1; x < i::entityList->GetHighestEntityIndex(); x++)
        {
            auto* entity = static_cast<c_base_entity*>(i::entityList->GetClientEntity(x));
            if (entity && entity->GetHealth() > 0 && entity->GetEntityName() != localPlayer->GetEntityName() && WorldToScreen(entity->GetOrigin(), screen))
            {
                Vector EntityOrigin = entity->GetOrigin();
                Vector Origin;
                if (!WorldToScreen(EntityOrigin, Origin)) continue;
                write_text("Entity!", Origin.x - 100, Origin.y, 150, 150, D3DCOLOR_ARGB(255, 108, 240, 183));
                D3DRECT BarRect = {
                    static_cast<long>(Origin.x - 1.65f * 0.5f),
                    static_cast<long>(Origin.z),
                    static_cast<long>(Origin.x),
                    static_cast<long>(Origin.y) };
                pDevice->Clear(1, &BarRect, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 255, 0, 0), 0.0f, 0);
                //std::string str = std::to_string(x);
                //str += ". ";s 
                //str += entity->GetEntityName();
                //str += " | ";
                //str += std::to_string(entity->GetHealth());
                //str += " / ";
                //str += std::to_string(entity->GetOrigin().x) + " " + std::to_string(entity->GetOrigin().y) + " " + std::to_string(entity->GetOrigin().z);
                //std::cout << str << std::endl;
            }
        }
    }
    else
    {
        CleanUp(reinterpret_cast<PBYTE>(Hooked_EndScene));
        CleanOffsets();
        p_ammo = false, p_health = false, s_ammo = false;
        FirstLoad = true;
    }

    draw_menu(pDevice);
    return oEndScene(pDevice);
}

DWORD WINAPI HackThread(LPVOID hModule)
{
    try
    {
        AllocConsole();
        FILE* f;
        freopen_s(&f, "CONOUT$", "w", stdout);

        init_interfaces();
        auto moduleBase = reinterpret_cast<uintptr_t>(GetModuleHandle("left4dead2.exe"));
        auto serverDllModuleBase = reinterpret_cast<uintptr_t>(GetModuleHandle("server.dll"));
        _serverDllModuleBase = serverDllModuleBase;

        std::cout << i::engineTool << std::endl;
        std::cout << i::engine << std::endl;
        std::cout << i::entityList << std::endl;

        while (!GetAsyncKeyState(VK_END))
        {
            try
            {
                if (i::engineTool->IsInGame())
                {
                    if (FirstLoad)
                    {
                        FirstLoad = false;
                        Hook(reinterpret_cast<PBYTE>(Hooked_EndScene));
                        std::cout << "Injected" << std::endl;
                    }
                }
                if (GetAsyncKeyState(VK_INSERT) & 1) 
                {
                    toggle_gui = !toggle_gui;
                    if (toggle_gui)
                    {
                        init = false;
                        if (!init)
                        {
                            init = true;
                            CleanOffsets();
                            initOffsets();
                        }
                    }
                }
                if (GetAsyncKeyState(VK_HOME) & 1)
                {
                    CleanUp(reinterpret_cast<PBYTE>(Hooked_EndScene));
                    Hook(reinterpret_cast<PBYTE>(Hooked_EndScene));
                    CleanOffsets();
                    initOffsets();
                }
                if (GetAsyncKeyState(VK_RIGHT) & 1)
                {
                    if (!is_sub_menu)
                    {
                        iMenuPage++;

                        if (iMenuPage >= 3)
                        {
                            iMenuPage = 3;
                        }
                    }
                    else if (is_sub_menu)
                    {
                        iMenuSubPage++;

                        if (iMenuSubPage >= 4)
                        {
                            iMenuSubPage = 4;
                        }
                    }
                }
                if (GetAsyncKeyState(VK_LEFT) & 1)
                {
                    if (!is_sub_menu)
                    {
                        iMenuPage--;

                        if (iMenuPage < 1)
                        {
                            iMenuPage = 1;
                        }
                    }
                    else if (is_sub_menu)
                    {
                        iMenuSubPage--;

                        if (iMenuSubPage < 1)
                        {
                            iMenuPage = 2;
                            is_sub_menu = false;
                        }
                    }
                }
                if (p_ammo)
                {
                    *pistol_ammo = 15;
                }
                if (p_health)
                {
                    *player_health = 115;
                }
                if (s_ammo)
                {
                    *pumpshotgun_ammo = 8;
                }

                Sleep(5);
            }
            catch (...)
            {
                std::cout << "Looping Error!" << std::endl;
            }
        }

        CleanUp(reinterpret_cast<PBYTE>(Hooked_EndScene));
        DetourRemove(reinterpret_cast<PBYTE>(oEndScene), reinterpret_cast<PBYTE>(Hooked_EndScene));
        fclose(f);
        FreeConsole();
        RelinkModuleToPEB(g_hinstDLL);
        FreeLibraryAndExitThread(g_hinstDLL, ERROR_SUCCESS);
    }
    catch (...)
    {
        std::cout << "First Pass Error!" << std::endl;
    }
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            g_hinstDLL = hinstDLL;
            UnlinkModuleFromPEB(hinstDLL);
            CreateThread(nullptr, 0, HackThread, nullptr, 0, nullptr);
        }
        case DLL_PROCESS_DETACH:
            break;
        default:
    	
    	break;
    }
    return 1;
}

void solid_rect(int x, int y, int w, int h, D3DCOLOR d3Color, IDirect3DDevice9* pDevice)
{
    D3DRECT rect = { x, y, x + w, y + h };
    pDevice->Clear(1, &rect, D3DCLEAR_TARGET, d3Color, 0.0f, 0);
}

void border_rect(int x, int y, int w, int h, int th, D3DCOLOR color, IDirect3DDevice9* pDevice)
{
    solid_rect(x, y, w, th, color, pDevice);
    solid_rect(x, y, th, h, color, pDevice);
    solid_rect(x + (w - th), y, th, h, color, pDevice);
    solid_rect(x, y + (h - th), w, th, color, pDevice);
}

void create_font_b(IDirect3DDevice9* pDevice, int size, const std::string& choiceFont)
{
    D3DXCreateFontA(pDevice, size, 0, FW_BOLD, 0, false, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, choiceFont.c_str(), &font);
}

void write_text(LPCSTR text, int x, int y, int w, int h, D3DCOLOR color)
{
    RECT rect = { x, y, x + w, y + h };
    font->DrawTextA(nullptr, text, -1, &rect, DT_CENTER, color);
}

bool check_box(int x, int y, bool var, const std::string& name, IDirect3DDevice9* pDevice, POINT Mouse)
{
    border_rect(x + 1, y + 4, 13, 13, 1, D3DCOLOR_ARGB(255, 108, 240, 183), pDevice);

    if (var)
    {
        solid_rect(x + 1, y + 4, 12, 12, D3DCOLOR_ARGB(255, 108, 240, 183), pDevice);
        write_text(name.c_str(), x + 15, y, 155, 500, D3DCOLOR_ARGB(255, 108, 240, 183));
    }
    else
    {
        write_text(name.c_str(), x + 15, y, 155, 500, D3DCOLOR_ARGB(255, 108, 240, 183));
    }
    return var;
}

void DrawMaskMenu(int* mask, int defaultGunVal)
{
    if (iMenuSubPage == 1)
    {
        write_text("(F1) Reset Mask", menuX + 20, menuY, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F2) Tank Arms", menuX + 20, menuY + 16, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F3) SMG", menuX + 20, menuY + 33, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F4) Smoker Arms", menuX + 20, menuY + 48, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F5) Drunk Mode", menuX + 20, menuY + 67, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F6) Army sniper", menuX + 20, menuY + 87, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F7) Spitter Arms", menuX + 20, menuY + 107, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F8) Silenced SMG", menuX + 20, menuY + 127, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F9) Spas Shotgun", menuX + 20, menuY + 147, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F10) Pump-Shotgun", menuX + 20, menuY + 167, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F11) LMG", menuX + 20, menuY + 185, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));

        if (GetAsyncKeyState(VK_F1) & 1)
        {
            *mask = defaultGunVal;
        }
        if (GetAsyncKeyState(VK_F2) & 1)
        {
            *mask = 1;
        }
        if (GetAsyncKeyState(VK_F3) & 1)
        {
            *mask = 2;
        }
        if (GetAsyncKeyState(VK_F4) & 1)
        {
            *mask = 3;
        }
        if (GetAsyncKeyState(VK_F5) & 1)
        {
            *mask = 4;
        }
        if (GetAsyncKeyState(VK_F6) & 1)
        {
            *mask = 5;
        }
        if (GetAsyncKeyState(VK_F7) & 1)
        {
            *mask = 6;
        }
        if (GetAsyncKeyState(VK_F8) & 1)
        {
            *mask = 7;
        }
        if (GetAsyncKeyState(VK_F9) & 1)
        {
            *mask = 8;
        }
        if (GetAsyncKeyState(VK_F10) & 1)
        {
            *mask = 9;
        }
        if (GetAsyncKeyState(VK_F11) & 1)
        {
            *mask = 10;
        }
    }
    else if (iMenuSubPage == 2)
    {
        write_text("(F1) Reset Mask", menuX + 20, menuY, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F2) Scar", menuX + 20, menuY + 16, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F3) AK", menuX + 20, menuY + 33, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F4) Lil Pump-Shotgun", menuX + 20, menuY + 48, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F5) Ghost Arms", menuX + 20, menuY + 67, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F6) Desert Eagle", menuX + 20, menuY + 87, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F7) Pistol", menuX + 20, menuY + 107, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F8) Pipe-Bomb", menuX + 20, menuY + 127, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F9) Jockey Arms", menuX + 20, menuY + 147, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F10) Incendiary Box", menuX + 20, menuY + 167, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F11) Explosive Box", menuX + 20, menuY + 185, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));

        if (GetAsyncKeyState(VK_F1) & 1)
        {
            *mask = defaultGunVal;
        }
        if (GetAsyncKeyState(VK_F2) & 1)
        {
            *mask = 11;
        }
        if (GetAsyncKeyState(VK_F3) & 1)
        {
            *mask = 12;
        }
        if (GetAsyncKeyState(VK_F4) & 1)
        {
            *mask = 13;
        }
        if (GetAsyncKeyState(VK_F5) & 1)
        {
            *mask = 14;
        }
        if (GetAsyncKeyState(VK_F6) & 1)
        {
            *mask = 15;
        }
        if (GetAsyncKeyState(VK_F7) & 1)
        {
            *mask = 16;
        }
        if (GetAsyncKeyState(VK_F8) & 1)
        {
            *mask = 17;
        }
        if (GetAsyncKeyState(VK_F9) & 1)
        {
            *mask = 21;
        }
        if (GetAsyncKeyState(VK_F10) & 1)
        {
            *mask = 23;
        }
        if (GetAsyncKeyState(VK_F11) & 1)
        {
            *mask = 24;
        }
    }
    else if (iMenuSubPage == 3)
    {
        write_text("(F1) Reset Mask", menuX + 20, menuY, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F2) Defibrillator", menuX + 20, menuY + 16, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F3) Adrenaline Shot", menuX + 20, menuY + 33, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F4) Hunter Arms", menuX + 20, menuY + 48, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F5) Grenade Launcher", menuX + 20, menuY + 67, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F6) First-Aid Kit", menuX + 20, menuY + 87, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F7) Coke Case", menuX + 20, menuY + 107, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F8) Charger Arm", menuX + 20, menuY + 127, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F9) Chainsaw", menuX + 20, menuY + 147, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F10) Boomer Arms", menuX + 20, menuY + 167, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F11) Semi-Auto Shotgun", menuX + 20, menuY + 185, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));

        if (GetAsyncKeyState(VK_F1) & 1)
        {
            *mask = defaultGunVal;
        }
        if (GetAsyncKeyState(VK_F2) & 1)
        {
            *mask = 25;
        }
        if (GetAsyncKeyState(VK_F3) & 1)
        {
            *mask = 26;
        }
        if (GetAsyncKeyState(VK_F4) & 1)
        {
            *mask = 27;
        }
        if (GetAsyncKeyState(VK_F5) & 1)
        {
            *mask = 28;
        }
        if (GetAsyncKeyState(VK_F6) & 1)
        {
            *mask = 31;
        }
        if (GetAsyncKeyState(VK_F7) & 1)
        {
            *mask = 33;
        }
        if (GetAsyncKeyState(VK_F8) & 1)
        {
            *mask = 34;
        }
        if (GetAsyncKeyState(VK_F9) & 1)
        {
            *mask = 35;
        }
        if (GetAsyncKeyState(VK_F10) & 1)
        {
            *mask = 36;
        }
        if (GetAsyncKeyState(VK_F11) & 1)
        {
            *mask = 37;
        }
    }
    else if (iMenuSubPage == 4)
    {
        write_text("(F1) Reset Mask", menuX + 20, menuY, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F2) Assault-Rifle ", menuX + 20, menuY + 16, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("(F3) CS MP4", menuX + 20, menuY + 33, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));

        if (GetAsyncKeyState(VK_F1) & 1)
        {
            *mask = defaultGunVal;
        }
        if (GetAsyncKeyState(VK_F2) & 1)
        {
            *mask = 38;
        }
        if (GetAsyncKeyState(VK_F3) & 1)
        {
            *mask = 40;
        }
    }
}

void draw_menu(IDirect3DDevice9* pDevice)
{
    static POINT mouse;
    GetCursorPos(&mouse);

    if (toggle_gui)
    {
        solid_rect(menuX, menuY, 200, 200, D3DCOLOR_ARGB(255, 2, 8, 5), pDevice);
        solid_rect(menuX - 2, menuY - 20, 204, 20, D3DCOLOR_ARGB(255, 2, 8, 5), pDevice);
        border_rect(menuX - 2, menuY - 2, 204, 204, 2, D3DCOLOR_ARGB(255, 0, 0, 0), pDevice);
        write_text("Left 4 Hax 2", menuX, menuY - 20, 200, 14, D3DCOLOR_ARGB(255, 108, 240, 183));

        if (iMenuPage == 1) // Player Menu
        {
            iMenuSubPage = 0;
            if (iMenuSubPage == 0)
            {
                if (GetAsyncKeyState(VK_F1) & 1)
                {
                    p_health = !p_health;
                }

                check_box(menuX + 5, menuY + 5, p_health, "(F1) Unlimited Health", pDevice, mouse);
                write_text("1", menuX + 2, menuY + 185, 188, 14, D3DCOLOR_ARGB(255, 148, 75, 211));
            }
        }
        else if (iMenuPage == 2) // Pistol Menu
        {
            if (iMenuSubPage == 0)
            {
                if (GetAsyncKeyState(VK_F1) & 1)
                {
                    p_ammo = !p_ammo;
                }
                if (GetAsyncKeyState(VK_F12) & 1)
                {
                    iMenuSubPage = 1;
                    is_sub_menu = true;
                }

                check_box(menuX + 5, menuY + 5, p_ammo, "(F1) Unlim Pistol Ammo", pDevice, mouse);
                write_text("(F12) ChangeWeapon", menuX + 5, menuY + 160, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
                write_text("2", menuX + 1, menuY + 185, 188, 14, D3DCOLOR_ARGB(255, 148, 75, 211));
            }
            else if (is_sub_menu && iMenuPage == 2)
            {
                DrawMaskMenu(pistol_mask, 16);
            }
        }
        else if (iMenuPage == 3) // Large Pump Shotgun Menu
        {
            if (iMenuSubPage == 0)
            {
                if (GetAsyncKeyState(VK_F1) & 1)
                {
                    s_ammo = !s_ammo;
                }
                if (GetAsyncKeyState(VK_F12) & 1)
                {
                    iMenuSubPage = 1;
                    is_sub_menu = true;
                }

                check_box(menuX + 5, menuY + 5, s_ammo, "(F1) Unlim Shotgun Ammo", pDevice, mouse);
                write_text("(F12) ChangeWeapon", menuX + 5, menuY + 160, 155, 200, D3DCOLOR_ARGB(255, 108, 240, 183));
                write_text("3", menuX + 2, menuY + 185, 188, 14, D3DCOLOR_ARGB(255, 148, 75, 211));
            }
            else if (is_sub_menu && iMenuPage == 3)
            {
                DrawMaskMenu(pumpshotgun_mask, 9);
            }
        }

        write_text("<- Left Arrow |", menuX - 50, menuY + 200, 200, 14, D3DCOLOR_ARGB(255, 108, 240, 183));
        write_text("Right Arrow ->", menuX + 50, menuY + 200, 200, 14, D3DCOLOR_ARGB(255, 108, 240, 183));
    }

    if (mouse.x >= menuX - 2 && mouse.x <= menuX + 204 && mouse.y >= menuY - 20 && mouse.y <= menuY)
    {
        if (GetAsyncKeyState(VK_LBUTTON))
        {
            menuX = mouse.x - 100;
            menuY = mouse.y + 10;
        }
    }
}