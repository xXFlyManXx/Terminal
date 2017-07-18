/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/

#include "precomp.h"

#include "screenInfoUiaProvider.hpp"
#include "../../host/screenInfo.hpp"
#include "../inc/ServiceLocator.hpp"

#include "windowUiaProvider.hpp"
#include "window.hpp"
#include "windowdpiapi.hpp"

#include "UiaTextRange.hpp"

using namespace Microsoft::Console::Interactivity::Win32;

// A helper function to create a SafeArray Version of an int array of a specified length
SAFEARRAY* BuildIntSafeArray(_In_reads_(length) const int* const data, _In_ int const length)
{
    SAFEARRAY *psa = SafeArrayCreateVector(VT_I4, 0, length);
    if (psa != nullptr)
    {
        for (long i = 0; i < length; i++)
        {
            if (FAILED(SafeArrayPutElement(psa, &i, (void *)&(data[i]))))
            {
                SafeArrayDestroy(psa);
                psa = nullptr;
                break;
            }
        }
    }

    return psa;
}

ScreenInfoUiaProvider::ScreenInfoUiaProvider(_In_ WindowUiaProvider* const pUiaParent) :
    _pUiaParent(THROW_HR_IF_NULL(E_INVALIDARG, pUiaParent)),
    _signalFiringMapping{},
    _cRefs(1)
{
}

ScreenInfoUiaProvider::~ScreenInfoUiaProvider()
{
}

HRESULT ScreenInfoUiaProvider::Signal(_In_ EVENTID id)
{
    HRESULT hr = S_OK;
    // check to see if we're already firing this particular event
    if (_signalFiringMapping.find(id) != _signalFiringMapping.end() &&
        _signalFiringMapping[id] == true)
    {
        return hr;
    }

    _signalFiringMapping[id] = true;
    IRawElementProviderSimple* pProvider = static_cast<IRawElementProviderSimple*>(this);
    hr = UiaRaiseAutomationEvent(pProvider, id);
    _signalFiringMapping[id] = false;
    return hr;
}

#pragma region IUnknown

IFACEMETHODIMP_(ULONG) ScreenInfoUiaProvider::AddRef()
{
    return InterlockedIncrement(&_cRefs);
}

IFACEMETHODIMP_(ULONG) ScreenInfoUiaProvider::Release()
{
    long val = InterlockedDecrement(&_cRefs);
    if (val == 0)
    {
        delete this;
    }
    return val;
}

IFACEMETHODIMP ScreenInfoUiaProvider::QueryInterface(_In_ REFIID riid,
                                                     _COM_Outptr_result_maybenull_ void** ppInterface)
{
    if (riid == __uuidof(IUnknown))
    {
        *ppInterface = static_cast<IRawElementProviderSimple*>(this);
    }
    else if (riid == __uuidof(IRawElementProviderSimple))
    {
        *ppInterface = static_cast<IRawElementProviderSimple*>(this);
    }
    else if (riid == __uuidof(IRawElementProviderFragment))
    {
        *ppInterface = static_cast<IRawElementProviderFragment*>(this);
    }
    else if (riid == __uuidof(ITextProvider))
    {
        *ppInterface = static_cast<ITextProvider*>(this);
    }
    else
    {
        *ppInterface = nullptr;
        return E_NOINTERFACE;
    }

    (static_cast<IUnknown*>(*ppInterface))->AddRef();

    return S_OK;
}

#pragma endregion

#pragma region IRawElementProviderSimple

// Implementation of IRawElementProviderSimple::get_ProviderOptions.
// Gets UI Automation provider options.
IFACEMETHODIMP ScreenInfoUiaProvider::get_ProviderOptions(_Out_ ProviderOptions* pOptions)
{
    *pOptions = ProviderOptions_ServerSideProvider;
    return S_OK;
}

// Implementation of IRawElementProviderSimple::get_PatternProvider.
// Gets the object that supports ISelectionPattern.
IFACEMETHODIMP ScreenInfoUiaProvider::GetPatternProvider(_In_ PATTERNID patternId,
                                                         _COM_Outptr_result_maybenull_ IUnknown** ppInterface)
{
    *ppInterface = nullptr;
    HRESULT hr = S_OK;

    if (patternId == UIA_TextPatternId)
    {
        hr = this->QueryInterface(__uuidof(ITextProvider), reinterpret_cast<void**>(ppInterface));
        if (FAILED(hr))
        {
            *ppInterface = nullptr;
        }
    }
    return hr;
}

// Implementation of IRawElementProviderSimple::get_PropertyValue.
// Gets custom properties.
IFACEMETHODIMP ScreenInfoUiaProvider::GetPropertyValue(_In_ PROPERTYID propertyId,
                                                       _Out_ VARIANT* pVariant)
{
    pVariant->vt = VT_EMPTY;

    // Returning the default will leave the property as the default
    // so we only really need to touch it for the properties we want to implement
    if (propertyId == UIA_ControlTypePropertyId)
    {
        // This control is the Document control type, implying that it is
        // a complex document that supports text pattern
        pVariant->vt = VT_I4;
        pVariant->lVal = UIA_DocumentControlTypeId;
    }
    else if (propertyId == UIA_NamePropertyId)
    {
        // TODO: MSFT: 7960168 - These strings should be localized text in the final UIA work
        pVariant->bstrVal = SysAllocString(L"Text Area");
        if (pVariant->bstrVal != nullptr)
        {
            pVariant->vt = VT_BSTR;
        }
    }
    else if (propertyId == UIA_AutomationIdPropertyId)
    {
        pVariant->bstrVal = SysAllocString(L"Text Area");
        if (pVariant->bstrVal != nullptr)
        {
            pVariant->vt = VT_BSTR;
        }
    }
    else if (propertyId == UIA_IsControlElementPropertyId)
    {
        pVariant->vt = VT_BOOL;
        pVariant->boolVal = VARIANT_TRUE;
    }
    else if (propertyId == UIA_IsContentElementPropertyId)
    {
        pVariant->vt = VT_BOOL;
        pVariant->boolVal = VARIANT_TRUE;
    }
    else if (propertyId == UIA_IsKeyboardFocusablePropertyId)
    {
        pVariant->vt = VT_BOOL;
        pVariant->boolVal = VARIANT_TRUE;
    }
    else if (propertyId == UIA_HasKeyboardFocusPropertyId)
    {
        pVariant->vt = VT_BOOL;
        pVariant->boolVal = VARIANT_TRUE;
    }
    else if (propertyId == UIA_ProviderDescriptionPropertyId)
    {
        pVariant->bstrVal = SysAllocString(L"Microsoft Console Host: Screen Information Text Area");
        if (pVariant->bstrVal != nullptr)
        {
            pVariant->vt = VT_BSTR;
        }
    }
    else if (propertyId == UIA_IsEnabledPropertyId)
    {
        pVariant->vt = VT_BOOL;
        pVariant->boolVal = VARIANT_TRUE;
    }

    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::get_HostRawElementProvider(_COM_Outptr_result_maybenull_ IRawElementProviderSimple** ppProvider)
{
    *ppProvider = nullptr;

    return S_OK;
}
#pragma endregion

#pragma region IRawElementProviderFragment

IFACEMETHODIMP ScreenInfoUiaProvider::Navigate(_In_ NavigateDirection direction,
                                               _COM_Outptr_result_maybenull_ IRawElementProviderFragment** ppProvider)
{
    *ppProvider = nullptr;

    if (direction == NavigateDirection_Parent)
    {
        try
        {
            _pUiaParent->QueryInterface(IID_PPV_ARGS(ppProvider));
        }
        catch (...)
        {
            *ppProvider = nullptr;
            return wil::ResultFromCaughtException();
        }
        RETURN_IF_NULL_ALLOC(*ppProvider);
    }

    // For the other directions the default of nullptr is correct
    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::GetRuntimeId(_Outptr_result_maybenull_ SAFEARRAY** ppRuntimeId)
{
    // Root defers this to host, others must implement it...
    *ppRuntimeId = nullptr;

    // AppendRuntimeId is a magic Number that tells UIAutomation to Append its own Runtime ID(From the HWND)
    int rId[] = { UiaAppendRuntimeId, -1 };
    // BuildIntSafeArray is a custom function to hide the SafeArray creation
    *ppRuntimeId = BuildIntSafeArray(rId, 2);
    RETURN_IF_NULL_ALLOC(*ppRuntimeId);

    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::get_BoundingRectangle(_Out_ UiaRect* pRect)
{
    const IConsoleWindow* const pIConsoleWindow = _getIConsoleWindow();
    RETURN_HR_IF_NULL((HRESULT)UIA_E_ELEMENTNOTAVAILABLE, pIConsoleWindow);

    RECT rc = pIConsoleWindow->GetWindowRect();

    pRect->left = rc.left;
    pRect->top = rc.top;
    pRect->width = rc.right - rc.left;
    pRect->height = rc.bottom - rc.top;

    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::GetEmbeddedFragmentRoots(_Outptr_result_maybenull_ SAFEARRAY** ppRoots)
{
    *ppRoots = nullptr;
    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::SetFocus()
{
    return Signal(UIA_AutomationFocusChangedEventId);
}

IFACEMETHODIMP ScreenInfoUiaProvider::get_FragmentRoot(_COM_Outptr_result_maybenull_ IRawElementProviderFragmentRoot** ppProvider)
{
    try
    {
        _pUiaParent->QueryInterface(IID_PPV_ARGS(ppProvider));
    }
    catch (...)
    {
        *ppProvider = nullptr;
        return wil::ResultFromCaughtException();
    }
    RETURN_IF_NULL_ALLOC(*ppProvider);
    return S_OK;
}

#pragma endregion

#pragma region ITextProvider

IFACEMETHODIMP ScreenInfoUiaProvider::GetSelection(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal)
{
    ServiceLocator::LocateGlobals()->getConsoleInformation()->LockConsole();
    auto Unlock = wil::ScopeExit([&]
    {
        ServiceLocator::LocateGlobals()->getConsoleInformation()->UnlockConsole();
    });

    *ppRetVal = nullptr;
    HRESULT hr;

    if (!Selection::Instance().IsAreaSelected())
    {
        // return a degenerate range at the cursor position
        SCREEN_INFORMATION* const pScreenInfo = _getScreenInfo();
        RETURN_HR_IF_NULL(E_POINTER, pScreenInfo);
        TEXT_BUFFER_INFO* const pTextBuffer = pScreenInfo->TextInfo;
        RETURN_HR_IF_NULL(E_POINTER, pTextBuffer);
        const Cursor* const pCursor = pTextBuffer->GetCursor();
        RETURN_HR_IF_NULL(E_POINTER, pCursor);

        // make a safe array
        *ppRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, 1);
        if (*ppRetVal == nullptr)
        {
            return E_OUTOFMEMORY;
        }

        IRawElementProviderSimple* pProvider;
        hr = this->QueryInterface(IID_PPV_ARGS(&pProvider));
        if (FAILED(hr))
        {
            SafeArrayDestroy(*ppRetVal);
            *ppRetVal = nullptr;
            return hr;
        }

        UiaTextRange* range;
        try
        {
            range = new UiaTextRange(pProvider,
                                     pCursor);
        }
        catch (...)
        {
            (static_cast<IUnknown*>(pProvider))->Release();
            SafeArrayDestroy(*ppRetVal);
            *ppRetVal = nullptr;
            return wil::ResultFromCaughtException();
        }

        LONG currentIndex = 0;
        hr = SafeArrayPutElement(*ppRetVal, &currentIndex, reinterpret_cast<void*>(range));
        if (FAILED(hr))
        {
            SafeArrayDestroy(*ppRetVal);
            *ppRetVal = nullptr;
            return hr;
        }
        return S_OK;
    }

    // get the selection rects
    SMALL_RECT* pSelectionRects;
    UINT rectCount;
    NTSTATUS status = Selection::Instance().GetSelectionRects(&pSelectionRects, &rectCount);
    RETURN_IF_NTSTATUS_FAILED(status);
    auto selectionRectCleanup = wil::ScopeExit([&] { delete[] pSelectionRects; });

    // make a safe array
    *ppRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, static_cast<ULONG>(rectCount));
    if (*ppRetVal == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    // stuff the selected lines into the safe array
    const COORD screenBufferCoords = _getScreenBufferCoords();
    const int totalLines = screenBufferCoords.Y;

    for (size_t i = 0; i < rectCount; ++i)
    {
        IRawElementProviderSimple* pProvider;
        hr = this->QueryInterface(IID_PPV_ARGS(&pProvider));
        if (FAILED(hr))
        {
            SafeArrayDestroy(*ppRetVal);
            *ppRetVal = nullptr;
            return hr;
        }
        const int lineNumber = pSelectionRects[i].Top;
        const int start = lineNumber * screenBufferCoords.X;
        // - 1 to get the last column in the row
        const int end = start + screenBufferCoords.X - 1;

        UiaTextRange* range;
        try
        {
            range = new UiaTextRange(pProvider,
                                     start,
                                     end,
                                     false);
        }
        catch (...)
        {
            (static_cast<IUnknown*>(pProvider))->Release();
            SafeArrayDestroy(*ppRetVal);
            *ppRetVal = nullptr;
            return wil::ResultFromCaughtException();
        }
        LONG currentIndex = static_cast<LONG>(i);
        hr = SafeArrayPutElement(*ppRetVal, &currentIndex, reinterpret_cast<void*>(range));
        if (FAILED(hr))
        {
            SafeArrayDestroy(*ppRetVal);
            *ppRetVal = nullptr;
            return hr;
        }
    }
    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::GetVisibleRanges(_Outptr_result_maybenull_ SAFEARRAY** ppRetVal)
{
    ServiceLocator::LocateGlobals()->getConsoleInformation()->LockConsole();
    auto Unlock = wil::ScopeExit([&]
    {
        ServiceLocator::LocateGlobals()->getConsoleInformation()->UnlockConsole();
    });

    const SCREEN_INFORMATION* const pScreenInfo = _getScreenInfo();
    RETURN_HR_IF_NULL(E_POINTER, pScreenInfo);
    const SMALL_RECT viewport = pScreenInfo->GetBufferViewport();
    const COORD screenBufferCoords = _getScreenBufferCoords();
    const int totalLines = screenBufferCoords.Y;

    // make a safe array
    const size_t rowCount = viewport.Bottom - viewport.Top + 1;
    *ppRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, static_cast<ULONG>(rowCount));
    if (*ppRetVal == nullptr)
    {
        return E_OUTOFMEMORY;
    }

    // stuff each visible line in the safearray
    for (size_t i = 0; i < rowCount; ++i)
    {
        const int lineNumber = (viewport.Top + i) % totalLines;
        const int start = lineNumber * screenBufferCoords.X;
        // - 1 to get the last column in the row
        const int end = start + screenBufferCoords.X - 1;

        IRawElementProviderSimple* pProvider;
        HRESULT hr = this->QueryInterface(IID_PPV_ARGS(&pProvider));
        if (FAILED(hr))
        {
            SafeArrayDestroy(*ppRetVal);
            *ppRetVal = nullptr;
            return hr;
        }

        UiaTextRange* range;
        try
        {
            range = new UiaTextRange(pProvider,
                                     start,
                                     end,
                                     false);
        }
        catch (...)
        {
            (static_cast<IUnknown*>(pProvider))->Release();
            SafeArrayDestroy(*ppRetVal);
            *ppRetVal = nullptr;
            return wil::ResultFromCaughtException();
        }
        LONG currentIndex = static_cast<LONG>(i);
        hr = SafeArrayPutElement(*ppRetVal, &currentIndex, reinterpret_cast<void*>(range));
        if (FAILED(hr))
        {
            SafeArrayDestroy(*ppRetVal);
            *ppRetVal = nullptr;
            return hr;
        }
    }
    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::RangeFromChild(_In_ IRawElementProviderSimple* /*childElement*/,
                                                     _COM_Outptr_result_maybenull_ ITextRangeProvider** ppRetVal)
{
    IRawElementProviderSimple* pProvider;
    RETURN_IF_FAILED(this->QueryInterface(IID_PPV_ARGS(&pProvider)));

    try
    {
        *ppRetVal = new UiaTextRange(pProvider);
    }
    catch (...)
    {
        *ppRetVal = nullptr;
        (static_cast<IUnknown*>(pProvider))->Release();
        return wil::ResultFromCaughtException();
    }
    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::RangeFromPoint(_In_ UiaPoint point,
                                                     _COM_Outptr_result_maybenull_ ITextRangeProvider** ppRetVal)
{
    IRawElementProviderSimple* pProvider;
    RETURN_IF_FAILED(this->QueryInterface(IID_PPV_ARGS(&pProvider)));

    try
    {
        *ppRetVal = new UiaTextRange(pProvider,
                                     point);
    }
    catch(...)
    {
        *ppRetVal = nullptr;
        (static_cast<IUnknown*>(pProvider))->Release();
        return wil::ResultFromCaughtException();
    }
    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::get_DocumentRange(_COM_Outptr_result_maybenull_ ITextRangeProvider** ppRetVal)
{
    IRawElementProviderSimple* pProvider;
    RETURN_IF_FAILED(this->QueryInterface(IID_PPV_ARGS(&pProvider)));

    try
    {
        *ppRetVal = new UiaTextRange(pProvider);
        RETURN_HR_IF_NULL(E_OUTOFMEMORY, *ppRetVal);
        (*ppRetVal)->ExpandToEnclosingUnit(TextUnit::TextUnit_Document);
    }
    catch (...)
    {
        *ppRetVal = nullptr;
        (static_cast<IUnknown*>(pProvider))->Release();
        return wil::ResultFromCaughtException();
    }
    return S_OK;
}

IFACEMETHODIMP ScreenInfoUiaProvider::get_SupportedTextSelection(_Out_ SupportedTextSelection* pRetVal)
{
    *pRetVal = SupportedTextSelection::SupportedTextSelection_Single;
    return S_OK;
}

#pragma endregion

const COORD ScreenInfoUiaProvider::_getScreenBufferCoords() const
{
    return ServiceLocator::LocateGlobals()->getConsoleInformation()->GetScreenBufferSize();
}

SCREEN_INFORMATION* const ScreenInfoUiaProvider::_getScreenInfo()
{
    return ServiceLocator::LocateGlobals()->getConsoleInformation()->CurrentScreenBuffer;
}

IConsoleWindow* const ScreenInfoUiaProvider::_getIConsoleWindow()
{
    return ServiceLocator::LocateConsoleWindow();
}