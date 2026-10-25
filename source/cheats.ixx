module;

#include <common.hxx>
#include <cwchar>
#include <cstdio>
#include <intrin.h>

export module cheats;

import common;
import comvars;
import natives;
import settings;
import seasonal;

constexpr size_t MAX_CHEAT_LENGTH = 32;
char CheatString[MAX_CHEAT_LENGTH] = { 0 };

auto SnowCheat(bool bPrintHelp = true) -> void
{
    if (!SeasonalManager::IsTimedEventsDisabled())
        SeasonalManager::DisableTimedEvents();

    if (SeasonalManager::GetCurrent() == SeasonalType::Snow)
    {
        SeasonalManager::Set(SeasonalType::None);

        if (bPrintHelp)
            Natives::PrintHelp((char*)"WinterEvent0");
    }
    else
    {
        SeasonalManager::Set(SeasonalType::Snow);

        if (bPrintHelp)
            Natives::PrintHelp((char*)"WinterEvent1");
    }
}

auto HallCheat(bool bPrintHelp = true) -> void
{
    if (!SeasonalManager::IsTimedEventsDisabled())
        SeasonalManager::DisableTimedEvents();

    if (SeasonalManager::GetCurrent() == SeasonalType::Halloween)
    {
        SeasonalManager::Set(SeasonalType::None);

        if (bPrintHelp)
            Natives::PrintHelp((char*)"HalloweenEvent0");
    }
    else
    {
        SeasonalManager::Set(SeasonalType::Halloween);

        if (bPrintHelp)
            Natives::PrintHelp((char*)"HalloweenEvent1");
    }
}

std::vector<std::pair<const char*, std::function<void()>>> ReversedCheats =
{
    { "wonstitel", []() { SnowCheat(); }}, // letitsnow
    { "ecafyracs", []() { HallCheat(); }}, // scaryface
};

char VirtualKeyToChar(int vkCode)
{
    if (vkCode >= 'A' && vkCode <= 'Z')
    {
        return static_cast<char>(vkCode + 32);
    }

    if (vkCode >= '0' && vkCode <= '9')
    {
        return static_cast<char>(vkCode);
    }

    switch (vkCode)
    {
        case VK_SPACE:
            return ' ';
        default:
            return '\0';
    }
}

#pragma pack(push, 8)
struct modelNameId
{
    const char* pszName = nullptr;
    int nModelId = 0;
    modelNameId* pPrev = nullptr;
};
#pragma pack(pop)

namespace ModelNameId
{
    modelNameId Tampa;
    modelNameId DoubleT;
    modelNameId Hakuchou;
    modelNameId Hexer;
    modelNameId Slamvan;
    modelNameId PoliceBike;
    modelNameId PoliceBuffalo;
    modelNameId PoliceStinger;
    modelNameId Brickade;

    modelNameId* (__fastcall* ModelNameId)(modelNameId* _this, void* edx, const char* pszModelName) = nullptr;

    SafetyHookInline shinitialize = {};
    void __fastcall initialize(void* CModelInfoStore, void* edx)
    {
        if (!Tampa.pszName)
            ModelNameId(&Tampa, 0, "tampa");

        if (!DoubleT.pszName)
            ModelNameId(&DoubleT, 0, "double");

        if (!Hakuchou.pszName)
            ModelNameId(&Hakuchou, 0, "hakuchou");

        if (!Hexer.pszName)
            ModelNameId(&Hexer, 0, "hexer");

        if (!Slamvan.pszName)
            ModelNameId(&Slamvan, 0, "slamvan");

        if (!PoliceBike.pszName)
            ModelNameId(&PoliceBike, 0, "policeb");

        if (!PoliceBuffalo.pszName)
            ModelNameId(&PoliceBuffalo, 0, "police3");

        if (!PoliceStinger.pszName)
            ModelNameId(&PoliceStinger, 0, "police4");

        if (!Brickade.pszName)
            ModelNameId(&Brickade, 0, "avan");

        return shinitialize.unsafe_fastcall(CModelInfoStore, edx);
    }
}

void (__cdecl* SpawnCar)(int modelID) = nullptr;

// ---- Beta Niko EXPERIMENT: log every time the game sets Niko's gloves (slot 4) or hair (slot 7) ----
// Writes to "GTAIV.EFLC.FusionFix.asi.BetaNikoHook.log" next to the .asi. Changes nothing, only observes.
static void BetaNikoHookLog(const char* line)
{
    static wchar_t szPath[MAX_PATH] = { 0 };
    if (!szPath[0])
    {
        HMODULE hm = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&CheatString, &hm);
        GetModuleFileNameW(hm, szPath, MAX_PATH);
        wcscat_s(szPath, L".BetaNikoHook.log");
    }
    FILE* f = nullptr;
    if (_wfopen_s(&f, szPath, L"a") == 0 && f)
    {
        fputs(line, f);
        fputc('\n', f);
        fclose(f);
    }
}

SafetyHookInline shSetPedComponent = {};
bool __cdecl SetPedComponentHook(void* ped, void* a2, void* a3, uint8_t flag, int comp, int draw, int tex)
{
    static int nLogged = 0;
    if ((comp == 4 || comp == 7) && nLogged < 800)
    {
        void* player = FindPlayerPed ? FindPlayerPed(0) : nullptr;
        const bool bIsPlayer = player && ped == player;
        if (bIsPlayer || !player) // player's own calls, or any call before a player exists
        {
            ++nLogged;
            char buf[200];
            snprintf(buf, sizeof(buf), "ret=%p ped=%p player=%p isPlayer=%d comp=%d draw=%d tex=%d ep=%d",
                _ReturnAddress(), ped, player, bIsPlayer ? 1 : 0, comp, draw, tex,
                _dwCurrentEpisode ? (int)*_dwCurrentEpisode : -1);
            BetaNikoHookLog(buf);
        }
    }
    return shSetPedComponent.unsafe_ccall<bool>(ped, a2, a3, flag, comp, draw, tex);
}

class Cheats
{
public:
    Cheats()
    {
        FusionFix::onInitEventAsync() += []()
        {
            auto pattern = hook::pattern("51 80 7C 24 ? 00 53 56 8B 74 24 ? 57 0F BF 46 2E 8B 7C 24 ? 8B 04 85");
            if (!pattern.empty())
            {
                shSetPedComponent = safetyhook::create_inline(pattern.get_first(0), SetPedComponentHook);
                BetaNikoHookLog("--- hook installed on the internal set-ped-component function ---");
            }
            else
            {
                BetaNikoHookLog("--- hook pattern NOT found ---");
            }
        };

        FusionFix::onGameInitEvent() += []()
        {
            if (CText::hasViceCityStrings())
            {
                memset(CheatString, 0, sizeof(CheatString));

                FusionFix::onGameProcessEvent() += []()
                {
                    int keyIndex = 0;
                    while (keyIndex < 255)
                    {
                        int pressedKey;
                        if (keyIndex != 8 && Natives::GetAsciiJustPressed(keyIndex, &pressedKey))
                        {
                            memmove(CheatString + 1, CheatString, MAX_CHEAT_LENGTH - 2);
                            CheatString[0] = VirtualKeyToChar(keyIndex);
                            CheatString[MAX_CHEAT_LENGTH - 1] = '\0';
                        }

                        ++keyIndex;
                    }

                    if (CheatString[0] == '\0')
                        return;

                    for (const auto& it : ReversedCheats)
                    {
                        const char* revCheat = it.first;
                        size_t len = strlen(revCheat);
                        if (len > MAX_CHEAT_LENGTH)
                            continue;
                        if (strncmp(CheatString, revCheat, len) == 0)
                        {
                            it.second();
                            CheatString[0] = '\0';
                            break;
                        }
                    }
                };
            }
        };

        FusionFix::onInitEventAsync() += []()
        {
            CIniReader iniReader("");
            bool bSnowCheat = iniReader.ReadInteger("CHEATS", "SnowCheat", 0) != 0;
            bool bHalloweenCheat = iniReader.ReadInteger("CHEATS", "HalloweenCheat", 0) != 0;

            if (bSnowCheat)
                FusionFix::onGameInitEvent() += []() { SnowCheat(); };

            if (bHalloweenCheat)
                FusionFix::onGameInitEvent() += []() { HallCheat(); };

            NativeOverride::RegisterPhoneCheat("7665550100", []
            {
                SnowCheat();
            });

            NativeOverride::RegisterPhoneCheat("2665550100", []
            {
                HallCheat();
            });

            enum ePedComponent
            {
                PED_COMPONENT_HEAD,
                PED_COMPONENT_UPPER,
                PED_COMPONENT_LOWER,
                PED_COMPONENT_SUSE,
                PED_COMPONENT_HAND,
                PED_COMPONENT_FEET,
                PED_COMPONENT_JACKET,
                PED_COMPONENT_HAIR,
                PED_COMPONENT_SUS2,
                PED_COMPONENT_TEEF,
                PED_COMPONENT_FACE,
            };

            // IV Beta Gloves
            NativeOverride::RegisterPhoneCheat("4585550100", []
            {
                if (*_dwCurrentEpisode == 0)
                {
                    Ped PlayerPed = 0;
                    Natives::GetPlayerChar(Natives::ConvertIntToPlayerIndex(Natives::GetPlayerId()), &PlayerPed);
                    if (PlayerPed)
                    {
                        auto Gloves = Natives::GetCharDrawableVariation(PlayerPed, PED_COMPONENT_HAND);

                        if (!Gloves)
                        {
                            Natives::SetCharComponentVariation(PlayerPed, PED_COMPONENT_HAND, 1, 0);
                            Natives::PrintHelp((char*)"CHEAT1");
                        }
                        else
                        {
                            Natives::SetCharComponentVariation(PlayerPed, PED_COMPONENT_HAND, 0, 0);
                            Natives::PrintHelp((char*)"CHEAT2");
                        }
                    }
                }
            });

            // IV Beta Hair
            NativeOverride::RegisterPhoneCheat("2885550100", []
            {
                if (*_dwCurrentEpisode == 0)
                {
                    Ped PlayerPed = 0;
                    Natives::GetPlayerChar(Natives::ConvertIntToPlayerIndex(Natives::GetPlayerId()), &PlayerPed);
                    if (PlayerPed)
                    {
                        auto Hair = Natives::GetCharDrawableVariation(PlayerPed, PED_COMPONENT_HAIR);

                        if (!Hair)
                        {
                            Natives::SetCharComponentVariation(PlayerPed, PED_COMPONENT_HAIR, 1, 0);
                            Natives::PrintHelp((char*)"CHEAT1");
                        }
                        else
                        {
                            Natives::SetCharComponentVariation(PlayerPed, PED_COMPONENT_HAIR, 0, 0);
                            Natives::PrintHelp((char*)"CHEAT2");
                        }
                    }
                }
            });

            // TLAD Pants
            NativeOverride::RegisterPhoneCheat("7265550100", []
            {
                if (*_dwCurrentEpisode == 1)
                {
                    Ped PlayerPed = 0;
                    Natives::GetPlayerChar(Natives::ConvertIntToPlayerIndex(Natives::GetPlayerId()), &PlayerPed);
                    if (PlayerPed)
                    {
                        auto Pants = Natives::GetCharTextureVariation(PlayerPed, PED_COMPONENT_LOWER);

                        if (!Pants)
                        {
                            Natives::SetCharComponentVariation(PlayerPed, PED_COMPONENT_LOWER, 0, 1);
                            Natives::PrintHelp((char*)"CHEAT1");
                        }
                        else
                        {
                            Natives::SetCharComponentVariation(PlayerPed, PED_COMPONENT_LOWER, 0, 0);
                            Natives::PrintHelp((char*)"CHEAT2");
                        }
                    }
                }
            });

            auto pattern = find_pattern("8B 44 24 ? 89 01 C7 41 ? ? ? ? ? A1", "8B C1 8B 4C 24 ? 89 08 C7 40");
            ModelNameId::ModelNameId = (decltype(ModelNameId::ModelNameId))pattern.get_first();

            pattern = find_pattern("56 8B 35 ? ? ? ? 85 F6 74 ? EB 03 8D 49 ? 8D 46", "56 8B 35 ? ? ? ? 85 F6 74 ? EB 03 8D 49 ? 8D 46");
            ModelNameId::shinitialize = safetyhook::create_inline(pattern.get_first(), ModelNameId::initialize);

            pattern = find_pattern("55 8B EC 83 E4 F0 81 EC 58 01 00 00 56 57 8B 7D ? 85 FF", "55 8B EC 83 E4 F0 81 EC 84 01 00 00 53 56 57 8B 7D");
            SpawnCar = (decltype(SpawnCar))pattern.get_first();

            // Spawn Tampa
            NativeOverride::RegisterPhoneCheat("2275558267", []
            {
                if (*_dwCurrentEpisode == 1 || *_dwCurrentEpisode == 2)
                {
                    SpawnCar(ModelNameId::Tampa.nModelId);
                    Natives::PrintHelp((char*)"CHEAT1");
                }
            });

            // Spawn DoubleT
            NativeOverride::RegisterPhoneCheat("2455550125", []
            {
                if (*_dwCurrentEpisode == 2)
                {
                    SpawnCar(ModelNameId::DoubleT.nModelId);
                    Natives::PrintHelp((char*)"CHEAT1");
                }
            });

            // Spawn Hakuchou
            NativeOverride::RegisterPhoneCheat("2455550199", []
            {
                if (*_dwCurrentEpisode == 2)
                {
                    SpawnCar(ModelNameId::Hakuchou.nModelId);
                    Natives::PrintHelp((char*)"CHEAT1");
                }
            });

            // Spawn Hexer
            NativeOverride::RegisterPhoneCheat("2455550150", []
            {
                if (*_dwCurrentEpisode == 2)
                {
                    SpawnCar(ModelNameId::Hexer.nModelId);
                    Natives::PrintHelp((char*)"CHEAT1");
                }
            });

            // Spawn Slamvan
            NativeOverride::RegisterPhoneCheat("8265550100", []
            {
                if (*_dwCurrentEpisode == 2)
                {
                    SpawnCar(ModelNameId::Slamvan.nModelId);
                    Natives::PrintHelp((char*)"CHEAT1");
                }
            });

            // Spawn Police Bike
            NativeOverride::RegisterPhoneCheat("6255556752", []
            {
                if (*_dwCurrentEpisode == 2)
                {
                    SpawnCar(ModelNameId::PoliceBike.nModelId);
                    Natives::PrintHelp((char*)"CHEAT1");
                }
            });

            // Spawn Police Buffalo
            NativeOverride::RegisterPhoneCheat("2275552833", []
            {
                if (*_dwCurrentEpisode == 2)
                {
                    SpawnCar(ModelNameId::PoliceBuffalo.nModelId);
                    Natives::PrintHelp((char*)"CHEAT1");
                }
            });

            // Spawn Police Stinger
            NativeOverride::RegisterPhoneCheat("2275557864", []
            {
                if (*_dwCurrentEpisode == 2)
                {
                    SpawnCar(ModelNameId::PoliceStinger.nModelId);
                    Natives::PrintHelp((char*)"CHEAT1");
                }
            });

            // Spawn Brickade
            NativeOverride::RegisterPhoneCheat("2725552826", []
            {
                if (*_dwCurrentEpisode == 2)
                {
                    SpawnCar(ModelNameId::Brickade.nModelId);
                    Natives::PrintHelp((char*)"CHEAT1");
                }
            });

            // Double physics' rate speed
            NativeOverride::RegisterPhoneCheat("6925550100", []
            {
                bSpeedupSimRateCheat = !bSpeedupSimRateCheat;
                if (bSpeedupSimRateCheat)
                    Natives::PrintHelp((char*)"CHEAT1");
                else
                    Natives::PrintHelp((char*)"CHEAT2");
            });
        };

        // Beta Niko: give Niko the cut gloves and haircut when a new Niko appears (new game, loading a save,
        // or the player ped changing). The game can reset parts of his outfit right after he spawns, so the look is
        // applied for a short settling window (about 10 seconds of frames, a couple of attempts per second) and then
        // left alone completely.
        //
        // Keys (each one also remembers your choice, so it is respected the next time Niko appears):
        //   F9  - if any beta part is on: remove both. Otherwise: put both on.
        //   F10 - toggle the gloves only.
        //   F11 - toggle the hair only.
        // The choice is stored in "GTAIV.EFLC.FusionFix.asi.BetaNiko.ini" next to the .asi
        // ([BETANIKO] Gloves=1/0, Hair=1/0). Edit or delete that file to change the default.
        // The phone cheats still work, but they do not change the remembered choice.
        FusionFix::onGameProcessEvent() += []()
        {
            constexpr uint32_t COMPONENT_HAND = 4;
            constexpr uint32_t COMPONENT_HAIR = 7;
            constexpr int SETTLE_FRAMES = 600;
            static Ped nLastPed = 0;
            static int nApplyFrames = 0;
            static bool bKeyWasDown[3] = { false, false, false }; // F9, F10, F11
            static bool bSettingsLoaded = false;
            static bool bWantGloves = true;
            static bool bWantHair = true;
            static wchar_t szIni[MAX_PATH] = { 0 };

            if (!bSettingsLoaded)
            {
                bSettingsLoaded = true;
                HMODULE hm = nullptr;
                GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&CheatString, &hm);
                GetModuleFileNameW(hm, szIni, MAX_PATH);
                wcscat_s(szIni, L".BetaNiko.ini");
                bWantGloves = GetPrivateProfileIntW(L"BETANIKO", L"Gloves", 1, szIni) != 0;
                bWantHair = GetPrivateProfileIntW(L"BETANIKO", L"Hair", 1, szIni) != 0;
            }

            if (*_dwCurrentEpisode != 0)
            {
                nLastPed = 0;
                nApplyFrames = 0;
                return;
            }

            auto JustPressed = [](int idx, int vk)
            {
                const bool bDown = IsKeyboardKeyPressed(vk);
                const bool bJust = bDown && !bKeyWasDown[idx];
                bKeyWasDown[idx] = bDown;
                return bJust;
            };
            const bool bF9 = JustPressed(0, VK_F9);
            const bool bF10 = JustPressed(1, VK_F10);
            const bool bF11 = JustPressed(2, VK_F11);

            Ped PlayerPed = 0;
            Natives::GetPlayerChar(Natives::ConvertIntToPlayerIndex(Natives::GetPlayerId()), &PlayerPed);
            if (!PlayerPed)
            {
                nLastPed = 0;
                nApplyFrames = 0;
                return;
            }

            if (PlayerPed != nLastPed)
            {
                nLastPed = PlayerPed;
                nApplyFrames = SETTLE_FRAMES;
            }

            // same rule as the phone cheats: vanilla is drawable 0, beta is drawable 1.
            // Only 0 and 1 are ever changed, so any other outfit piece is left alone.
            auto SetPart = [PlayerPed](uint32_t component, uint32_t value)
            {
                const uint32_t cur = Natives::GetCharDrawableVariation(PlayerPed, component);
                if ((cur == 0 || cur == 1) && cur != value)
                    Natives::SetCharComponentVariation(PlayerPed, component, value, 0);
            };

            if (bF9 || bF10 || bF11)
            {
                nApplyFrames = 0; // a manual choice ends the settling window
                const bool bGlovesBeta = Natives::GetCharDrawableVariation(PlayerPed, COMPONENT_HAND) == 1;
                const bool bHairBeta = Natives::GetCharDrawableVariation(PlayerPed, COMPONENT_HAIR) == 1;

                if (bF9)
                {
                    const bool bTurnOn = !(bGlovesBeta || bHairBeta);
                    bWantGloves = bTurnOn;
                    bWantHair = bTurnOn;
                }
                else if (bF10)
                {
                    bWantGloves = !bGlovesBeta;
                }
                else
                {
                    bWantHair = !bHairBeta;
                }

                WritePrivateProfileStringW(L"BETANIKO", L"Gloves", bWantGloves ? L"1" : L"0", szIni);
                WritePrivateProfileStringW(L"BETANIKO", L"Hair", bWantHair ? L"1" : L"0", szIni);

                if (bF9 || bF10)
                    SetPart(COMPONENT_HAND, bWantGloves ? 1 : 0);
                if (bF9 || bF11)
                    SetPart(COMPONENT_HAIR, bWantHair ? 1 : 0);
                return;
            }

            if (nApplyFrames <= 0)
                return;
            --nApplyFrames;
            if (nApplyFrames % 30 != 0)
                return; // at most about 2 attempts per second

            if (bWantGloves)
                SetPart(COMPONENT_HAND, 1);
            if (bWantHair)
                SetPart(COMPONENT_HAIR, 1);
        };
    }
} Cheats;
