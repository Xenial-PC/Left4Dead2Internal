#pragma once
#include <iostream>
#include "Windows.h"

class IDebugOverlay
{
public:
    int GetScreenPosition(const Vector& point, Vector& screen)
    {
        using WorldToScreen = int(__thiscall*)(void*, const Vector&, Vector&);
        return method<WorldToScreen>(11, this)(this, point, screen);
    }
};