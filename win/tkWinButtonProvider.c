#include "tkWinButtonProvider.h"
#include "tkButton.h"

#include <stdio.h>
#include <assert.h>

#include "UIAutomationTypedefs.h"
#include <UIAutomationCore.h>
#include <UIAutomationClient.h>
#include <UIAutomationCoreApi.h>

#pragma comment(lib, "uiautomationcore")

#define BUTTONPROVIDER_MAGIC 0xFA1520BA

static STDMETHODIMP IRawElementProviderSimple_QueryInterface(IRawElementProviderSimple *This, REFIID riid, void *ppvObject);
static STDMETHODIMP_(ULONG) IRawElementProviderSimple_AddRef(IRawElementProviderSimple *This);
static STDMETHODIMP_(ULONG) IRawElementProviderSimple_Release(IRawElementProviderSimple *This);
static STDMETHODIMP IRawElementProviderSimple_get_ProviderOptions(IRawElementProviderSimple *This, enum ProviderOptions *pRetVal);
static STDMETHODIMP IRawElementProviderSimple_GetPatternProvider(IRawElementProviderSimple *This, PATTERNID patternId, IUnknown **pRetVal);
static STDMETHODIMP IRawElementProviderSimple_GetPropertyValue(IRawElementProviderSimple *This, PROPERTYID propertyId, VARIANT *pVal);
static STDMETHODIMP IRawElementProviderSimple_get_HostRawElementProvider(IRawElementProviderSimple *This, IRawElementProviderSimple **pRetVal);

static STDMETHODIMP IInvokeProvider_QueryInterface(IInvokeProvider *This, REFIID riid, void *ppvObject);
static STDMETHODIMP_(ULONG) IInvokeProvider_AddRef(IInvokeProvider *This);
static STDMETHODIMP_(ULONG) IInvokeProvider_Release(IInvokeProvider *This);
static STDMETHODIMP IInvokeProvider_Invoke(IInvokeProvider *This);

static STDMETHODIMP IToggleProvider_QueryInterface(IToggleProvider *This, REFIID riid, void **ppvObject);
static STDMETHODIMP_(ULONG) IToggleProvider_AddRef(IToggleProvider *This);
static STDMETHODIMP_(ULONG) IToggleProvider_Release(IToggleProvider *This);
static STDMETHODIMP IToggleProvider_Toggle(IToggleProvider *This);
static STDMETHODIMP IToggleProvider_get_ToggleState(IToggleProvider *This, ToggleState *pVal);

static STDMETHODIMP ISelectionItemProvider_QueryInterface(ISelectionItemProvider *This, REFIID riid, void **ppvObject);
static STDMETHODIMP_(ULONG) ISelectionItemProvider_AddRef(ISelectionItemProvider *This);
static STDMETHODIMP_(ULONG) ISelectionItemProvider_Release(ISelectionItemProvider *This);
static STDMETHODIMP ISelectionItemProvider_Select(ISelectionItemProvider *This);
static STDMETHODIMP ISelectionItemProvider_AddToSelection(ISelectionItemProvider *This);
static STDMETHODIMP ISelectionItemProvider_RemoveFromSelection(ISelectionItemProvider *This);
static STDMETHODIMP ISelectionItemProvider_get_IsSelected(ISelectionItemProvider *This, BOOL *pVal);
static STDMETHODIMP ISelectionItemProvider_get_SelectionContainer(ISelectionItemProvider *This, IRawElementProviderSimple **pVal);

static IRawElementProviderSimpleVtbl vtbl = {
    IRawElementProviderSimple_QueryInterface,
    IRawElementProviderSimple_AddRef,
    IRawElementProviderSimple_Release,
    IRawElementProviderSimple_get_ProviderOptions,
    IRawElementProviderSimple_GetPatternProvider,
    IRawElementProviderSimple_GetPropertyValue,
    IRawElementProviderSimple_get_HostRawElementProvider
};

static IInvokeProviderVtbl vtbl2 = {
    IInvokeProvider_QueryInterface,
    IInvokeProvider_AddRef,
    IInvokeProvider_Release,
    IInvokeProvider_Invoke
};

static IToggleProviderVtbl vtbl3 = {
    IToggleProvider_QueryInterface,
    IToggleProvider_AddRef,
    IToggleProvider_Release,
    IToggleProvider_Toggle,
    IToggleProvider_get_ToggleState
};

static ISelectionItemProviderVtbl vtbl4 = {
    ISelectionItemProvider_QueryInterface,
    ISelectionItemProvider_AddRef,
    ISelectionItemProvider_Release,
    ISelectionItemProvider_Select,
    ISelectionItemProvider_AddToSelection,
    ISelectionItemProvider_RemoveFromSelection,
    ISelectionItemProvider_get_IsSelected,
    ISelectionItemProvider_get_SelectionContainer
};

typedef struct {
    IRawElementProviderSimpleVtbl *lpVtbl;
    IInvokeProviderVtbl *lpVtbl2;
    IToggleProviderVtbl *lpVtbl3;
    ISelectionItemProviderVtbl *lpVtbl4;
    DWORD magic;
    ULONG refcount[4];
    HWND m_hWnd;
    Tk_Window tkwin;
} InstanceData;

static void _cdecl TraceW(LPCWSTR format, ...)
{
    WCHAR buffer[512] = {0};
    va_list args;
    va_start(args, format);
    vswprintf_s(buffer, sizeof(buffer)/sizeof(WCHAR), format, args);
    OutputDebugStringW(buffer);
    va_end(args);
}

#ifdef _DEBUG
#define TRACE TraceW
#else
#define TRACE 1?((void)0):TraceW
#endif

// Check the reference count for _all_ interfaces which are managing their own
// individual counts and only delete the instance if the sum is 0.
static ULONG CheckInstance(InstanceData* instance)
{
    ULONG refcount = instance->refcount[0] + instance->refcount[1] + instance->refcount[2] + instance->refcount[3];
    if (refcount == 0)
    {
        TRACE(L"Destroy %p\n", instance);
        free(instance);
    }
    return refcount;
}

static STDMETHODIMP IRawElementProviderSimple_QueryInterface(IRawElementProviderSimple *This, REFIID riid, void **ppv)
{
    InstanceData *instance = (InstanceData *)This;
	assert(instance->magic == BUTTONPROVIDER_MAGIC);
    HRESULT hr = E_POINTER;
    if (ppv)
    {
        *ppv = NULL;
        hr = S_OK;
        if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IRawElementProviderSimple, riid)) {
            *ppv = &instance->lpVtbl;
            instance->lpVtbl->AddRef((IRawElementProviderSimple*)(*ppv));
        }
        else if (IsEqualIID(&IID_IInvokeProvider, riid)) {
            *ppv = &instance->lpVtbl2;
            instance->lpVtbl2->AddRef((IInvokeProvider*)(*ppv));
        }
        else if (IsEqualIID(&IID_IToggleProvider, riid)) {
            *ppv = &instance->lpVtbl3;
            instance->lpVtbl3->AddRef((IToggleProvider*)(*ppv));
        }
	else if (IsEqualIID(&IID_ISelectionItemProvider, riid)) {
	    *ppv = &instance->lpVtbl4;
	    instance->lpVtbl4->AddRef((ISelectionItemProvider *)(*ppv));
	}
        else
            hr = E_NOINTERFACE;
    }
    return hr;
}

STDMETHODIMP_(ULONG) IRawElementProviderSimple_AddRef(IRawElementProviderSimple *This)
{
    InstanceData * instance = (InstanceData *)This;
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    ULONG result = InterlockedIncrement(&instance->refcount[0]);
    TRACE(L"IRawElementProviderSimple AddRef %lu\n", result);
    return result;
}

STDMETHODIMP_(ULONG) IRawElementProviderSimple_Release(IRawElementProviderSimple *This)
{
    InstanceData *instance = (InstanceData *)This;
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    ULONG result = InterlockedDecrement(&instance->refcount[0]);
    TRACE(L"IRawElementProviderSimple Release %lu\n", result);
    return CheckInstance(instance);
}

STDMETHODIMP IRawElementProviderSimple_get_ProviderOptions(IRawElementProviderSimple *This, enum ProviderOptions *pRetVal)
{
    InstanceData *instance = (InstanceData *)This;
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    HRESULT hr = E_POINTER;
    if (pRetVal)
    {
        if (!IsWindow(instance->m_hWnd))
            hr = UIA_E_ELEMENTNOTAVAILABLE;
        else
        {
            *pRetVal = ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading;
            hr = S_OK;
        }
    }
    return hr;
}

STDMETHODIMP IRawElementProviderSimple_GetPatternProvider(IRawElementProviderSimple *This, PATTERNID patternId, IUnknown **ppVal)
{
    InstanceData *instance = (InstanceData *)This;
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    HRESULT hr = E_POINTER;
    if (ppVal)
    {
	TkButton *butPtr = (TkButton *)((TkWindow *)instance->tkwin)->instanceData;
	hr = S_OK;
	*ppVal = NULL;
	// NOTE: nothing returned for TYPE_LABEL
	if (butPtr->type == TYPE_CHECK_BUTTON) {
	    if (patternId == UIA_InvokePatternId || patternId == UIA_TogglePatternId)
	    {
		hr = instance->lpVtbl->QueryInterface(This, &IID_IUnknown, ppVal);
	    }
	} else if (butPtr->type == TYPE_RADIO_BUTTON) {
	    if (patternId == UIA_InvokePatternId || patternId == UIA_SelectionItemPatternId)
	    {
		hr = instance->lpVtbl->QueryInterface(This, &IID_IUnknown, ppVal);
	    }
	} else if (butPtr->type == TYPE_BUTTON) {
	    if (patternId == UIA_InvokePatternId)
	    {
		hr = instance->lpVtbl->QueryInterface(This, &IID_IUnknown, ppVal);
	    }	    
	}
    }
    return hr;
}

STDMETHODIMP IRawElementProviderSimple_GetPropertyValue(IRawElementProviderSimple *This, PROPERTYID propertyId, VARIANT *pVal)
{
    HRESULT hr = E_POINTER;
    if (pVal)
    {
	hr = S_OK;
	VariantInit(pVal);
	pVal->vt = VT_EMPTY;

	InstanceData *instance = (InstanceData *)This;
	TkButton *butPtr = (TkButton *)((TkWindow *)instance->tkwin)->instanceData;
	assert(instance->magic == BUTTONPROVIDER_MAGIC);
	Tcl_DString ds;
	Tcl_DStringInit(&ds);

	if (propertyId == UIA_ControlTypePropertyId)
	{
	    pVal->vt = VT_I4;
	    switch (butPtr->type)
	    {
		case TYPE_CHECK_BUTTON:
		    pVal->lVal = UIA_CheckBoxControlTypeId;
		    break;
		case TYPE_RADIO_BUTTON:
		    pVal->lVal = UIA_RadioButtonControlTypeId;
		    break;
		case TYPE_LABEL:
		    pVal->lVal = UIA_TextControlTypeId;
		    break;
		case TYPE_BUTTON:
		default:
            	    pVal->lVal = UIA_ButtonControlTypeId;
		    break;
	    }
	}
	else if (propertyId == UIA_AutomationIdPropertyId)
	{
	    pVal->bstrVal = SysAllocString(Tcl_UtfToWCharDString(Tk_PathName(instance->tkwin), -1, &ds));
	    pVal->vt = VT_BSTR;
	}
	else if (propertyId == UIA_IsControlElementPropertyId)
	{
	    pVal->vt = VT_BOOL;
	    pVal->boolVal = VARIANT_TRUE;
	}
	else if (propertyId == UIA_IsEnabledPropertyId)
	{
	    pVal->vt = VT_BOOL;
	    pVal->boolVal = (butPtr->state == STATE_DISABLED) ? VARIANT_FALSE : VARIANT_TRUE;
	}
	else if (propertyId == UIA_IsKeyboardFocusablePropertyId)
	{
	    int takeFocus = 0;
	    if (butPtr->takeFocusPtr != NULL)
		Tcl_GetBooleanFromObj(NULL, butPtr->takeFocusPtr, &takeFocus);
	    pVal->vt = VT_BOOL;
	    pVal->boolVal = takeFocus ? VARIANT_TRUE : VARIANT_FALSE;
	}
	else if (propertyId == UIA_HasKeyboardFocusPropertyId)
	{
	    pVal->vt = VT_BOOL;
	    pVal->boolVal = (butPtr->flags & GOT_FOCUS) ? VARIANT_TRUE : VARIANT_FALSE;
	}
        else if (propertyId == UIA_ClassNamePropertyId)
        {
	    pVal->bstrVal = SysAllocString(Tcl_UtfToWCharDString(Tk_Class(instance->tkwin), -1, &ds));
	    pVal->vt = VT_BSTR;
        }
	else if (propertyId == UIA_NamePropertyId)
	{
	    if (butPtr && butPtr->textPtr) {
		pVal->bstrVal = SysAllocString(Tcl_GetUnicode(butPtr->textPtr));
	    } else {
		pVal->bstrVal = SysAllocString(Tcl_UtfToWCharDString(Tk_Name(instance->tkwin), -1, &ds));
	    }
	    pVal->vt = VT_BSTR;
	}
	else if (propertyId == UIA_FrameworkIdPropertyId)
	{
	    pVal->bstrVal = SysAllocString(L"Tk");
	    pVal->vt = VT_BSTR;
	}
	Tcl_DStringFree(&ds);
    }
    return hr;
}

STDMETHODIMP IRawElementProviderSimple_get_HostRawElementProvider(IRawElementProviderSimple *This, IRawElementProviderSimple **ppVal)
{
    InstanceData *instance = (InstanceData *)This;
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    HRESULT hr = E_POINTER;
    if (ppVal)
    {
        hr = UiaHostProviderFromHwnd(instance->m_hWnd, ppVal);
    }
    return hr;
}

STDMETHODIMP IInvokeProvider_QueryInterface(IInvokeProvider *This, REFIID riid, void **ppvObject)
{
    InstanceData *instance = (InstanceData *)(This - 1);
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    return instance->lpVtbl->QueryInterface((IRawElementProviderSimple *)instance, riid, ppvObject);
}

STDMETHODIMP_(ULONG) IInvokeProvider_AddRef(IInvokeProvider *This)
{
    InstanceData *instance = (InstanceData *)(This - 1);
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    ULONG result = InterlockedIncrement(&instance->refcount[1]);
    TRACE(L"IInvokeProvider AddRef %lu\n", result);
    return result;
}

STDMETHODIMP_(ULONG) IInvokeProvider_Release(IInvokeProvider *This)
{
    InstanceData *instance = (InstanceData *)(This - 1);
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    ULONG result = InterlockedDecrement(&instance->refcount[1]);
    TRACE(L"IInvokeProvider Release %lu\n", result);
    return CheckInstance(instance);
}

STDMETHODIMP IInvokeProvider_Invoke(IInvokeProvider *This)
{
    InstanceData *instance = (InstanceData *)(This - 1);
    TkButton *butPtr = (TkButton *)((TkWindow *)instance->tkwin)->instanceData;
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    TkInvokeButton(butPtr);
    return S_OK;
}

STDMETHODIMP IToggleProvider_QueryInterface(IToggleProvider *This, REFIID riid, void **ppvObject)
{
    InstanceData *instance = (InstanceData *)(This - 2);
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    return instance->lpVtbl->QueryInterface((IRawElementProviderSimple *)instance, riid, ppvObject);
}

STDMETHODIMP_(ULONG) IToggleProvider_AddRef(IToggleProvider *This)
{
    InstanceData *instance = (InstanceData *)(This - 2);
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    ULONG result = InterlockedIncrement(&instance->refcount[2]);
    TRACE(L"IToggleProvider AddRef %lu\n", result);
    return result;
}

STDMETHODIMP_(ULONG) IToggleProvider_Release(IToggleProvider *This)
{
    InstanceData *instance = (InstanceData *)(This - 2);
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    ULONG result = InterlockedDecrement(&instance->refcount[2]);
    TRACE(L"IToggleProvider Release %lu\n", result);
    return CheckInstance(instance);
}

STDMETHODIMP IToggleProvider_Toggle(IToggleProvider *This)
{
    InstanceData *instance = (InstanceData *)(This - 2);
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    HRESULT hr = UIA_E_ELEMENTNOTAVAILABLE;
    if (IsWindow(instance->m_hWnd))
    {
	TkButton *butPtr = (TkButton *)((TkWindow *)instance->tkwin)->instanceData;
	// FIXME: eval $widget toggle
	TkInvokeButton(butPtr);
	hr = S_OK;
    }
    return hr;
}

STDMETHODIMP IToggleProvider_get_ToggleState(IToggleProvider *This, ToggleState *pVal)
{
    InstanceData *instance = (InstanceData *)(This - 2);
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    HRESULT hr = E_POINTER;
    if (pVal != NULL)
    {
        if (!IsWindow(instance->m_hWnd))
            hr = UIA_E_ELEMENTNOTAVAILABLE;
        else
        {
	    // call $widget toggle
            int state = (int)SendMessage(instance->m_hWnd, BM_GETCHECK, 0L, 0L);
            *pVal = (state == BST_CHECKED) ? ToggleState_On : ToggleState_Off;
            hr = S_OK;
        }
    }
    return hr;
}

static STDMETHODIMP ISelectionItemProvider_QueryInterface(ISelectionItemProvider *This, REFIID riid, void **ppvObject)
{
    InstanceData *instance = (InstanceData *)(This - 3);
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    return instance->lpVtbl->QueryInterface((IRawElementProviderSimple *)instance, riid, ppvObject);
}

static STDMETHODIMP_(ULONG) ISelectionItemProvider_AddRef(ISelectionItemProvider *This)
{
    InstanceData *instance = (InstanceData *)(This - 3);
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    ULONG result = InterlockedIncrement(&instance->refcount[3]);
    TRACE(L"ISelectionItemProvider AddRef %lu\n", result);
    return result;
}
static STDMETHODIMP_(ULONG) ISelectionItemProvider_Release(ISelectionItemProvider *This)
{
    InstanceData *instance = (InstanceData *)(This - 3);
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    ULONG result = InterlockedDecrement(&instance->refcount[3]);
    TRACE(L"ISelectionItemProvider Release %lu\n", result);
    return CheckInstance(instance);
}

static STDMETHODIMP ISelectionItemProvider_Select(ISelectionItemProvider *This)
{
    InstanceData *instance = (InstanceData *)(This - 3);
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    HRESULT hr = UIA_E_ELEMENTNOTAVAILABLE;
    if (IsWindow(instance->m_hWnd))
    {
	TkButton *butPtr = (TkButton *)((TkWindow *)instance->tkwin)->instanceData;
	TkInvokeButton(butPtr);
	// should raise some UIA events
	hr = S_OK;
    }
    return hr;
}

static STDMETHODIMP ISelectionItemProvider_AddToSelection(ISelectionItemProvider *This)
{
    return ISelectionItemProvider_Select(This);
}

static STDMETHODIMP ISelectionItemProvider_RemoveFromSelection(ISelectionItemProvider *This)
{
    // eval $widget deselect
    return S_OK;
}

static STDMETHODIMP ISelectionItemProvider_get_IsSelected(ISelectionItemProvider *This, BOOL *pVal)
{
    InstanceData *instance = (InstanceData *)(This - 3);
    assert(instance->magic == BUTTONPROVIDER_MAGIC);
    HRESULT hr = UIA_E_ELEMENTNOTAVAILABLE;
    if (IsWindow(instance->m_hWnd))
    {
	ToggleState state;
	hr = IToggleProvider_get_ToggleState((IToggleProvider *)&instance->lpVtbl3, &state);
	*pVal = (state == ToggleState_On);
    }
    return hr;
}

static STDMETHODIMP ISelectionItemProvider_get_SelectionContainer(ISelectionItemProvider *This, IRawElementProviderSimple **pVal)
{
    // ISelectionProvider instance -- should be 1 for a group of radio buttons.
    // hmm.
    return E_FAIL;
}

static HRESULT ButtonProvider_CreateInstance(Tk_Window tkwin, HWND hwnd, IRawElementProviderSimple **ppProvider)
{
    HRESULT hr = E_POINTER;
    if (ppProvider)
    {
	hr = E_OUTOFMEMORY;
	InstanceData *instance = (InstanceData *)malloc(sizeof(InstanceData));
	if (instance)
	{
	    instance->lpVtbl = &vtbl;
	    instance->lpVtbl2 = &vtbl2;
	    instance->lpVtbl3 = &vtbl3;
	    instance->lpVtbl4 = &vtbl4;
	    instance->magic = BUTTONPROVIDER_MAGIC;
	    instance->m_hWnd = hwnd;
	    instance->tkwin = tkwin;
	    ZeroMemory(instance->refcount, sizeof(instance->refcount));
	    TRACE(L"CreateInstance %p\n", instance);
	    hr = instance->lpVtbl->QueryInterface((IRawElementProviderSimple *)instance, &IID_IRawElementProviderSimple, ppProvider);
	}
    }
    return hr;
}

LRESULT ButtonProvider_OnGetObject(_In_ Tk_Window tkwin, _In_ WPARAM wParam, _In_ LPARAM lParam, _Inout_ IUnknown **ppunk)
{
    LRESULT result = 0L;
    if (UiaRootObjectId == lParam)
    {
	HWND hwnd = Tk_GetHWND(((TkWindow *)tkwin)->window);
        IRawElementProviderSimple *provider = NULL;
        if (*ppunk == NULL)
        {
            ButtonProvider_CreateInstance(tkwin, hwnd, &provider);
            provider->lpVtbl->QueryInterface(provider, &IID_IUnknown, ppunk);
        }
        else
        {
            (*ppunk)->lpVtbl->QueryInterface(*ppunk, &IID_IRawElementProviderSimple, &provider);
        }
        
        result = UiaReturnRawElementProvider(hwnd, wParam, lParam, provider);
        provider->lpVtbl->Release(provider);
    }
    return result;
}

LRESULT ButtonProvider_OnDestroy(_In_ Tk_Window tkwin, _In_ IUnknown *punkProvider)
{
    HWND hwnd = Tk_GetHWND(((TkWindow *)tkwin)->window);

    UiaReturnRawElementProvider(hwnd, 0, 0, NULL);
    UiaDisconnectProvider((IRawElementProviderSimple *)punkProvider);
    return 0L;
}

void ButtonProvider_OnInvoke(_In_ Tk_Window tkwin, _In_opt_ IUnknown *punk)
{
    if (punk != NULL && UiaClientsAreListening())
    {
        IRawElementProviderSimple *provider = NULL;
        if (SUCCEEDED(punk->lpVtbl->QueryInterface(punk, &IID_IRawElementProviderSimple, &provider)))
        {
	    TkButton *butPtr = (TkButton *)((TkWindow *)tkwin)->instanceData;

	    UiaRaiseAutomationEvent(provider, UIA_Invoke_InvokedEventId);
            if (butPtr->type == TYPE_RADIO_BUTTON)
	    {
        	UiaRaiseAutomationEvent(provider, UIA_SelectionItem_ElementSelectedEventId);
	    }
            provider->lpVtbl->Release(provider);
        }
    }
}