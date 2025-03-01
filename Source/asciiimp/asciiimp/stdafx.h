// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#if MAX_API_NUM != MAX_API_NUM_R160
#pragma comment(lib,"acap.lib")
#endif
#pragma comment(lib,"geom.lib")
#pragma comment(lib,"RenderUtil.lib")
#pragma comment(lib,"mesh.lib")
#pragma comment(lib,"Maxscrpt.lib")
#pragma comment(lib,"maxutil.lib")
#pragma comment(lib,"maxnet.lib")
#pragma comment(lib,"ManipSys.lib")
#pragma comment(lib,"imageViewers.lib")
#pragma comment(lib,"IGame.lib")
#pragma comment(lib,"gfx.lib")
#pragma comment(lib, "bmm.lib") 
#if MAX_API_NUM != MAX_API_NUM_R160
#pragma comment(lib,"geomimp.lib")
#endif
#pragma comment(lib,"core.lib")
#pragma comment(lib,"expr.lib")

#pragma comment(lib,"comctl32.lib")

#include "targetver.h"

#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX 1

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

#include <iostream>
#include <string>
#include <vector>

//Convert Char to WChar
const wchar_t *GetWC( const char *c );

// TODO: reference additional headers your program requires here
#include "error.h"
