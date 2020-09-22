#include <Windows.h>
#include <Ole2.h>
#include "tkWinInt.h"

LRESULT ButtonProvider_OnGetObject(_In_ Tk_Window tkwin, _In_ WPARAM wParam, _In_ LPARAM lParam, _Inout_ IUnknown **ppunk);
LRESULT ButtonProvider_OnDestroy(_In_ Tk_Window tkwin, _In_ IUnknown *punkProvider);
void ButtonProvider_OnInvoke(_In_ Tk_Window tkwin, _In_opt_ IUnknown *provider);
