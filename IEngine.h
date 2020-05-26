#pragma once
#include "Helper.h"
#include <iostream>
#include <vector>
#include "VMatrix.hpp"

class Engine
{
public:
    DWORD GetLocalPlayer()
    {
        using GetName = DWORD (__thiscall*)(void*);
        return method<GetName>(12, this)(this);
    }
    bool IsInGame()
    {
        using original_fn = bool(__thiscall*)(void*);
        return (*(original_fn**)this)[25](this);
    }
    bool IsConnected()
    {
        using original_fn = bool(__thiscall*)(void*);
        return (*(original_fn**)this)[29](this);
    }
    bool IsGamePaused()
    {
        using original_fn = bool(__thiscall*)(void*);
        return (*(original_fn**)this)[88](this);
    }
    inline void GetScreenSize(std::int32_t& w, std::int32_t& h)
    {
        using original_fn = void(__thiscall*)(void*, std::int32_t&, std::int32_t&);
        return (*(original_fn**)this)[5](this, w, h);
    }
    const VMatrix& WorldToScreenMatrix()
    {
        using original_fn = VMatrix&(__thiscall*)(void*);
        return (*(original_fn**)this)[37](this);
    }
};