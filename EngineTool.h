#pragma once
#include <iostream>
#include "Windows.h"

class EngineTool
{
public:
    bool IsInGame()
    {
        using original_fn = bool(__thiscall*)(void*);
        return (*(original_fn**)this)[28](this);
    }
    bool IsGamePaused()
    {
        using original_fn = bool(__thiscall*)(void*);
        return (*(original_fn**)this)[31](this);
    }
    bool IsConnected()
    {
        using original_fn = bool(__thiscall*)(void*);
        return (*(original_fn**)this)[29](this);
    }
    bool SetIsInGame(bool isInGame)
    {
        using original_fn = bool(__thiscall*)(void*, bool);
        return (*(original_fn**)this)[90](this, isInGame);
    }
    const char* Command(char const* cmd)
    {
        using original_fn = char const*(__thiscall*)(void*, char const*);
        return (*(original_fn**)this)[22](this, cmd);
    }
};