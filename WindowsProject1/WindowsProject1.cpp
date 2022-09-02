#include <Windows.h>
#include <CommCtrl.h>
#pragma comment(lib, "comctl32")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


#include "MainDialog.h"
int APIENTRY wWinMain
( _In_ HINSTANCE hInstance
, _In_opt_ HINSTANCE hPrevInstance
, _In_ LPWSTR    lpCmdLine
, _In_ int       nCmdShow
)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

    InitCommonControls();
	(void)MainDialog::DialogBoxParamW(hInstance, nullptr);
}
