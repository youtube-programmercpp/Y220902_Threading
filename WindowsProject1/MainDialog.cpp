#include "MainDialog.h"
#include "resource.h"
#include <stdexcept>
#include <sstream>
#include <tuple>
#include <CommCtrl.h>
#include <process.h>

MainDialog::MainDialog(std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(CloseHandle)*>&& hStopEvent) noexcept
	: hStopEvent{std::move(hStopEvent)}
{
	InitializeCriticalSection(&cs);
}
MainDialog::~MainDialog()
{
	DeleteCriticalSection(&cs);
}
auto MainDialog::EnterCriticalSection()->std::unique_ptr<CRITICAL_SECTION, decltype(LeaveCriticalSection)*>
{
	::EnterCriticalSection(&cs);
	return { &cs, LeaveCriticalSection };
}


template<>INT_PTR MainDialog::HandleMessage<WM_INITDIALOG>(HWND hDlg, WPARAM wParam, LPARAM lParam) noexcept
{
	try {
		auto pThis = std::make_unique<MainDialog>
			(std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(CloseHandle)*>{CreateEventW(nullptr, true, false, nullptr), CloseHandle}
		);
		SetWindowLongPtrW(hDlg, DWLP_USER, LONG_PTR(pThis.get()));


		const auto hListView = GetDlgItem(hDlg, IDC_LIST1);
		static const LVCOLUMN cols[]
		{
			{ /*UINT   mask       */LVCF_TEXT | LVCF_WIDTH
			, /*int    fmt        */0
			, /*int    cx         */100
			, /*LPTSTR pszText    */const_cast<LPTSTR>(TEXT("Thread"))
			, /*int    cchTextMax */0
			, /*int    iSubItem   */0
			, /*int    iImage     */0
			, /*int    iOrder     */0
			},
			{ /*UINT   mask       */LVCF_TEXT | LVCF_WIDTH
			, /*int    fmt        */0
			, /*int    cx         */100
			, /*LPTSTR pszText    */const_cast<LPTSTR>(TEXT("Value"))
			, /*int    cchTextMax */0
			, /*int    iSubItem   */0
			, /*int    iImage     */0
			, /*int    iOrder     */0
			}
		};
		for (const auto& r : cols)
			ListView_InsertColumn(hListView, static_cast<int>(&r - cols), &r);

		(void)pThis.release();
		return true;
	}
	catch (const std::exception& e) {
		OutputDebugStringA(__FILE__ "(" _CRT_STRINGIZE(__LINE__) "): " __FUNCTION__ "で例外が投入されました。...");
		OutputDebugStringA(e.what());
		OutputDebugStringA("\n");
		EndDialog(hDlg, IDCANCEL);
		return true;
	}
}
template<>INT_PTR MainDialog::HandleMessage<WM_NCDESTROY>(HWND hDlg, WPARAM wParam, LPARAM lParam) noexcept
{
	delete reinterpret_cast<MainDialog*>(GetWindowLongPtrW(hDlg, DWLP_USER));
	return 0;
}
template<>INT_PTR MainDialog::HandleCommand<IDCANCEL>(HWND hDlg, WORD wNotify, HWND hCtrl) noexcept
{
	EndDialog(hDlg, IDCANCEL);
	return true;
}
unsigned MainDialog::ThreadProc(HWND hListView) noexcept
{
	try {
		const auto dwThreadId = GetCurrentThreadId();
		auto s = (std::basic_ostringstream<TCHAR>() << dwThreadId).str();
		LVITEM m
		{ /*UINT   mask       */LVIF_TEXT | LVIF_PARAM
		, /*int    iItem      */INT_MAX
		, /*int    iSubItem   */0
		, /*UINT   state      */0
		, /*UINT   stateMask  */0
		, /*LPTSTR pszText    */&s.front()
		, /*int    cchTextMax */0
		, /*int    iImage     */0
		, /*LPARAM lParam     */dwThreadId
		};
		ListView_InsertItem(hListView, &m);
		m.mask = LVIF_TEXT;
		m.iSubItem = 1;
		for (int i = 0; i < 10; ++i) {

			[hListView, &m, dwThreadId, &s, i](auto&& lock)
			{
				LV_FINDINFO lvfi
				{ /*UINT    flags      ;*/LVFI_PARAM
				, /*LPCWSTR psz        ;*/nullptr
				, /*LPARAM  lParam     ;*/dwThreadId
				, /*POINT   pt         ;*/{}
				, /*UINT    vkDirection;*/0
				};
				m.iItem = ListView_FindItem(hListView, -1, &lvfi);
				s = (std::basic_ostringstream<TCHAR>() << i).str();
				m.pszText = &s.front();
				ListView_SetItem(hListView, &m);
			}(EnterCriticalSection());

			(void)WaitForSingleObject(hStopEvent.get(), 100);
		}
		return EXIT_SUCCESS;
	}
	catch (const std::exception& e) {
		OutputDebugStringA(e.what());
		OutputDebugStringA("\n");
		return EXIT_FAILURE;
	}
}
template<>INT_PTR MainDialog::HandleCommand<IDOK>(HWND hDlg, WORD wNotify, HWND hCtrl) noexcept
{
	const auto pThis = reinterpret_cast<MainDialog*>(GetWindowLongPtrW(hDlg, DWLP_USER));
	typedef std::tuple<MainDialog*, HANDLE, HWND, HWND> ThreadParameterListType;
	const auto thread_parameter = new ThreadParameterListType{ pThis, nullptr, hDlg, GetDlgItem(hDlg, IDC_LIST1) };
	if (const auto hThread = HANDLE(/*_Success_(return != 0) _ACRTIMP uintptr_t __cdecl*/_beginthreadex
	( /*_In_opt_  void                   * _Security    */nullptr
	, /*_In_      unsigned                 _StackSize   */0
	, /*_In_      _beginthreadex_proc_type _StartAddress*/[](void* _ArgList)->unsigned
		{
			//裏スレッドの処理内容をここに書く
#define	pThis     std::get<0>(*static_cast<ThreadParameterListType*>(_ArgList))
#define	hThread   std::get<1>(*static_cast<ThreadParameterListType*>(_ArgList))
#define	hDlg      std::get<2>(*static_cast<ThreadParameterListType*>(_ArgList))
#define	hListView std::get<3>(*static_cast<ThreadParameterListType*>(_ArgList))
			const auto nExitCode = pThis->ThreadProc(hListView);
			PostMessage(hDlg, WM_APP_EXIT_THREAD, 0, LPARAM(hThread));
			return nExitCode;
#undef	hListView 
#undef	hDlg     
#undef	hThread   
#undef	pThis     
		}
	, /*_In_opt_  void                   * _ArgList     */thread_parameter
	, /*_In_      unsigned                 _InitFlag    */CREATE_SUSPENDED
	, /*_Out_opt_ unsigned               * _ThrdAddr    */nullptr
	))) {
		std::get<1>(*thread_parameter) = hThread;
		(void)ResumeThread(hThread);
	}
	else
		delete thread_parameter;

	;



	return true;
}
template<>INT_PTR MainDialog::HandleMessage<WM_COMMAND>(HWND hDlg, WPARAM wParam, LPARAM lParam) noexcept
{
	switch (LOWORD(wParam)) {
	case IDCANCEL: return HandleCommand<IDCANCEL>(hDlg, HIWORD(wParam), HWND(lParam));
	case IDOK    : return HandleCommand<IDOK    >(hDlg, HIWORD(wParam), HWND(lParam));
	default:
		return false;
	}
}
template<>INT_PTR MainDialog::HandleMessage<WM_APP_EXIT_THREAD>(HWND hDlg, WPARAM wParam, LPARAM lParam) noexcept
{
	[](auto && lock, auto&& hThread, HWND hListView)
	{
		const auto dwThreadId = GetThreadId(hThread.get());

		LV_FINDINFO lvfi
		{ /*UINT    flags      ;*/LVFI_PARAM
		, /*LPCWSTR psz        ;*/nullptr
		, /*LPARAM  lParam     ;*/dwThreadId
		, /*POINT   pt         ;*/{}
		, /*UINT    vkDirection;*/0
		};
		const auto iItem = ListView_FindItem(hListView, -1, &lvfi);
		(void)WaitForSingleObject(hThread.get(), INFINITE);
		ListView_DeleteItem(hListView, iItem);
	}
	( reinterpret_cast<MainDialog*>(GetWindowLongPtr(hDlg, DWLP_USER))->EnterCriticalSection()
	, std::unique_ptr<std::remove_pointer_t<HANDLE>, decltype(CloseHandle)*>{HANDLE(lParam), CloseHandle}
	, GetDlgItem(hDlg, IDC_LIST1)
	);
	return 0;
}

INT_PTR MainDialog::DialogBoxParamW
( _In_opt_ HINSTANCE hInstance
, _In_opt_ HWND      hWndParent
) noexcept
{
	return /*WINUSERAPI INT_PTR WINAPI*/::DialogBoxParamW
	( /*_In_opt_ HINSTANCE hInstance     */hInstance
	, /*_In_     LPCWSTR   lpTemplateName*/MAKEINTRESOURCEW(IDD_DIALOG1)
	, /*_In_opt_ HWND      hWndParent    */nullptr
	, /*_In_opt_ DLGPROC   lpDialogFunc  */[](HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)->INT_PTR
		{
			switch (message) {
			case WM_INITDIALOG     : return HandleMessage<WM_INITDIALOG     >(hDlg, wParam, lParam);
			case WM_NCDESTROY      : return HandleMessage<WM_NCDESTROY      >(hDlg, wParam, lParam);
			case WM_COMMAND        : return HandleMessage<WM_COMMAND        >(hDlg, wParam, lParam);
			case WM_APP_EXIT_THREAD: return HandleMessage<WM_APP_EXIT_THREAD>(hDlg, wParam, lParam);
			default:
				return false;
			}
		}
	, /*_In_     LPARAM    dwInitParam   */{}
	);
}
