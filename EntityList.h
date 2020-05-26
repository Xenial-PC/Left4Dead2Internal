#pragma once
#include "Windows.h"
#include "Helper.h"

class EntityList
{
public:
    inline void* GetClientEntity(int id)
    {
        using GetClientEntity = void* (__thiscall*)(void*, int);
        return method<GetClientEntity>(3, this)(this, id);
    }
    inline void* GetClientEntityFromHandle(int handle)
    {
        using GetClientEntityFromHandle = void* (__thiscall*)(void*, int);
        return method<GetClientEntityFromHandle>(4, this)(this, handle);
    }
    inline int GetHighestEntityIndex()
    {
        using GetHighestEntityIndex = int(__thiscall*)(void*);
        return method<GetHighestEntityIndex>(6, this)(this);
    }
};