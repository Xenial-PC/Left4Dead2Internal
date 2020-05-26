// LeftFourDeadTwoInternal.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "LeftFourDeadTwoInternal.h"


// This is an example of an exported variable
LEFTFOURDEADTWOINTERNAL_API int nLeftFourDeadTwoInternal=0;

// This is an example of an exported function.
LEFTFOURDEADTWOINTERNAL_API int fnLeftFourDeadTwoInternal(void)
{
    return 0;
}

// This is the constructor of a class that has been exported.
CLeftFourDeadTwoInternal::CLeftFourDeadTwoInternal()
{
    return;
}
