#pragma once

#include <windows.h>
#include <vector>
#include <algorithm>

void RelinkModuleToPEB(HMODULE hModule);
void UnlinkModuleFromPEB(HMODULE hModule);