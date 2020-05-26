#pragma once
#include <Windows.h>
#include "IEngine.h"
#include "EntityList.h"
#include "EngineTool.h"
#include "DebugOverlay.h"

inline void* capture_interface(const char* module_name, const char* interface_name)
{
	const auto create_interface_fn = reinterpret_cast<void* (*)(const char* p_name, int* p_return_code)>(GetProcAddress(GetModuleHandleA(module_name), "CreateInterface"));
    return create_interface_fn(interface_name, nullptr);
}

namespace i
{
    Engine* engine = nullptr;
    EntityList* entityList = nullptr;
    EngineTool* engineTool = nullptr;
    IDebugOverlay* debugOverlay = nullptr;
}

inline void init_interfaces()
{
    i::engine = static_cast<Engine*>(capture_interface("engine.dll", "VEngineClient013"));
    i::engineTool = static_cast<EngineTool*>(capture_interface("engine.dll", "VENGINETOOL003"));
    i::entityList = static_cast<EntityList*>(capture_interface("client.dll", "VClientEntityList003"));
    i::debugOverlay = static_cast<IDebugOverlay*>(capture_interface("engine.dll", "VDebugOverlay003"));
}