#pragma once
#include "stdafx.h"
#include <windows.h>
#include <vector>
//https://guidedhacking.com

namespace mem
{
	void PatchEx(BYTE* dst, BYTE* src, unsigned int size, HANDLE hProcess);
	void NopEx(BYTE* dst, unsigned int size, HANDLE hProcess);
	uintptr_t FindDMAAddy(HANDLE hProc, uintptr_t ptr, std::vector<unsigned int> offsets);

	void Patch(BYTE* dst, BYTE* src, unsigned int size);
	void Nop(BYTE* dst, unsigned int size);
	uintptr_t FindDMAAddy(uintptr_t ptr, std::vector<unsigned int> offsets);

	template<typename T> T RPM(uintptr_t address) { return *(T*)address; }
	template<typename T> void WPM(uintptr_t address, T value) { *(T*)address = value; }
}