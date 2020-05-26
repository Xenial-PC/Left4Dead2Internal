#pragma once
#include <iostream>
#include "Windows.h"

class c_base_entity
{
public:
	int GetHealth()
	{
		return *reinterpret_cast<int*>(reinterpret_cast<DWORD>(this) + 0xEC);
	}
	const char* GetEntityName()
	{
		return  reinterpret_cast<const char*>(reinterpret_cast<DWORD>(this) + 0x1148);
	}
	int GetTeamId()
	{
		return *reinterpret_cast<int*>(reinterpret_cast<DWORD>(this) + 0x8f4);
	}
	Vector GetOrigin()
	{
		return *(Vector*)(reinterpret_cast<std::uintptr_t>(this) + 0x94);
	}
};