// ConsoleApplication1.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <iostream>
#include <process.h>

class Main {

	void ThreadProc(              )
	{
	}


	void NewThread()
	{
		_Success_(return != 0) _ACRTIMP uintptr_t __cdecl _beginthreadex
		( /*_In_opt_  void                     * _Security    */nullptr
		, /*_In_      unsigned                   _StackSize   */0
		, /*_In_      _beginthreadex_proc_type   _StartAddress*/[](void*p)->unsigned
			{
				//裏スレッドの処理を書く
				static_cast<Main*>(p)->ThreadProc();
				return 0;
			}
		, /*_In_opt_  void                     * _ArgList     */this
		, /*_In_      unsigned                   _InitFlag    */
		, /*_Out_opt_ unsigned                 * _ThrdAddr    */
		);
	}
};

int main()
{
	



}
