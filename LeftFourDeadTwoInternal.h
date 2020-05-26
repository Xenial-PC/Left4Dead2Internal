// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the LEFTFOURDEADTWOINTERNAL_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// LEFTFOURDEADTWOINTERNAL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef LEFTFOURDEADTWOINTERNAL_EXPORTS
#define LEFTFOURDEADTWOINTERNAL_API __declspec(dllexport)
#else
#define LEFTFOURDEADTWOINTERNAL_API __declspec(dllimport)
#endif

// This class is exported from the dll
class LEFTFOURDEADTWOINTERNAL_API CLeftFourDeadTwoInternal {
public:
	CLeftFourDeadTwoInternal(void);
	// TODO: add your methods here.
};

extern LEFTFOURDEADTWOINTERNAL_API int nLeftFourDeadTwoInternal;

LEFTFOURDEADTWOINTERNAL_API int fnLeftFourDeadTwoInternal(void);
