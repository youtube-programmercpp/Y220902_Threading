#pragma once
#include <Windows.h>
#include <memory>

#define	WM_APP_EXIT_THREAD	(WM_APP + 1)

class MainDialog
{
	CRITICAL_SECTION cs;
	std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(CloseHandle)*> hStopEvent;
	template<WORD command>static INT_PTR HandleCommand(HWND hDlg, WORD wNotify, HWND hCtrl) noexcept;
	template<UINT message>static INT_PTR HandleMessage(HWND hDlg, WPARAM wParam, LPARAM lParam) noexcept;
	unsigned ThreadProc(HWND hListView) noexcept;
	auto EnterCriticalSection()->std::unique_ptr<CRITICAL_SECTION, decltype(LeaveCriticalSection)*>;
public:
	MainDialog(std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(CloseHandle)*>&& hStopEvent) noexcept;
	~MainDialog();
	static INT_PTR DialogBoxParamW
	( _In_opt_ HINSTANCE hInstance
	, _In_opt_ HWND      hWndParent
	) noexcept;
};
