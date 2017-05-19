#include "precomp.h"
#include <intsafe.h>
#include <propvarutil.h>
#include <shlwapi.h>

#include "ShortcutSerialization.hpp"

#pragma hdrstop

void ShortcutSerialization::s_InitPropVarFromBool(_In_ BOOL fVal, _Out_ PROPVARIANT *ppropvar)
{
    ppropvar->vt = VT_BOOL;
    ppropvar->boolVal = fVal ? VARIANT_TRUE : VARIANT_FALSE;
}

void ShortcutSerialization::s_InitPropVarFromByte(_In_ BYTE bVal, _Out_ PROPVARIANT *ppropvar)
{
    ppropvar->vt = VT_I2;
    ppropvar->iVal = bVal;
}

void ShortcutSerialization::s_SetLinkPropertyBoolValue(_In_ IPropertyStore *pps, _In_ REFPROPERTYKEY refPropKey,_In_ const BOOL fVal)
{
    PROPVARIANT propvarBool;
    s_InitPropVarFromBool(fVal, &propvarBool);
    pps->SetValue(refPropKey, propvarBool);
    PropVariantClear(&propvarBool);
}

void ShortcutSerialization::s_SetLinkPropertyByteValue(_In_ IPropertyStore *pps, _In_ REFPROPERTYKEY refPropKey,_In_ const BYTE bVal)
{
    PROPVARIANT propvarByte;
    s_InitPropVarFromByte(bVal, &propvarByte);
    pps->SetValue(refPropKey, propvarByte);
    PropVariantClear(&propvarByte);
}

HRESULT ShortcutSerialization::s_GetPropertyBoolValue(_In_ IPropertyStore * const pPropStore, _In_ REFPROPERTYKEY refPropKey, _Out_ BOOL * const pfValue)
{
    PROPVARIANT propvar;
    HRESULT hr = pPropStore->GetValue(refPropKey, &propvar);
    if (SUCCEEDED(hr))
    {
        hr = PropVariantToBoolean(propvar, pfValue);
    }

    return hr;
}

HRESULT ShortcutSerialization::s_GetPropertyByteValue(_In_ IPropertyStore * const pPropStore, _In_ REFPROPERTYKEY refPropKey, _Out_ BYTE * const pbValue)
{
    PROPVARIANT propvar;
    HRESULT hr = pPropStore->GetValue(refPropKey, &propvar);
    if (SUCCEEDED(hr))
    {
        SHORT sValue;
        hr = PropVariantToInt16(propvar, &sValue);
        if (SUCCEEDED(hr))
        {
            hr = (sValue >= 0 && sValue <= BYTE_MAX) ? S_OK : E_INVALIDARG;
            if (SUCCEEDED(hr))
            {
                *pbValue = (BYTE)sValue;
            }
        }
    }

    return hr;
}

HRESULT ShortcutSerialization::s_PopulateV1Properties(_In_ IShellLink * const pslConsole, _In_ PCONSOLE_STATE_INFO pStateInfo)
{
    IShellLinkDataList *pConsoleLnkDataList;
    HRESULT hr = pslConsole->QueryInterface(IID_PPV_ARGS(&pConsoleLnkDataList));
    if (SUCCEEDED(hr))
    {
        // get/apply standard console properties
        NT_CONSOLE_PROPS *pNtConsoleProps = nullptr;
        hr = pConsoleLnkDataList->CopyDataBlock(NT_CONSOLE_PROPS_SIG, reinterpret_cast<void**>(&pNtConsoleProps));
        if (SUCCEEDED(hr))
        {
            pStateInfo->ScreenAttributes = pNtConsoleProps->wFillAttribute;
            pStateInfo->PopupAttributes = pNtConsoleProps->wPopupFillAttribute;
            pStateInfo->ScreenBufferSize = pNtConsoleProps->dwScreenBufferSize;
            pStateInfo->WindowSize = pNtConsoleProps->dwWindowSize;
            pStateInfo->WindowPosX = pNtConsoleProps->dwWindowOrigin.X;
            pStateInfo->WindowPosY = pNtConsoleProps->dwWindowOrigin.Y;
            pStateInfo->FontSize = pNtConsoleProps->dwFontSize;
            pStateInfo->FontFamily = pNtConsoleProps->uFontFamily;
            pStateInfo->FontWeight = pNtConsoleProps->uFontWeight;
            StringCchCopyW(pStateInfo->FaceName, ARRAYSIZE(pStateInfo->FaceName), pNtConsoleProps->FaceName);
            pStateInfo->CursorSize = pNtConsoleProps->uCursorSize;
            pStateInfo->FullScreen = pNtConsoleProps->bFullScreen;
            pStateInfo->QuickEdit = pNtConsoleProps->bQuickEdit;
            pStateInfo->InsertMode = pNtConsoleProps->bInsertMode;
            pStateInfo->AutoPosition = pNtConsoleProps->bAutoPosition;
            pStateInfo->HistoryBufferSize = pNtConsoleProps->uHistoryBufferSize;
            pStateInfo->NumberOfHistoryBuffers = pNtConsoleProps->uNumberOfHistoryBuffers;
            pStateInfo->HistoryNoDup = pNtConsoleProps->bHistoryNoDup;
            CopyMemory(pStateInfo->ColorTable, pNtConsoleProps->ColorTable, sizeof(pStateInfo->ColorTable));

            LocalFree(pNtConsoleProps);
        }

        // get/apply international console properties
        if (SUCCEEDED(hr))
        {
            NT_FE_CONSOLE_PROPS *pNtFEConsoleProps;
            if (SUCCEEDED(pConsoleLnkDataList->CopyDataBlock(NT_FE_CONSOLE_PROPS_SIG, reinterpret_cast<void**>(&pNtFEConsoleProps))))
            {
                pNtFEConsoleProps->uCodePage = pStateInfo->CodePage;
                LocalFree(pNtFEConsoleProps);
            }
        }

        pConsoleLnkDataList->Release();
    }

    return hr;
}

HRESULT ShortcutSerialization::s_PopulateV2Properties(_In_ IShellLink * const pslConsole, _In_ PCONSOLE_STATE_INFO pStateInfo)
{
    IPropertyStore *pPropStoreLnk;
    HRESULT hr = pslConsole->QueryInterface(IID_PPV_ARGS(&pPropStoreLnk));
    if (SUCCEEDED(hr))
    {
        hr = s_GetPropertyBoolValue(pPropStoreLnk, PKEY_Console_WrapText, &pStateInfo->fWrapText);
        if (SUCCEEDED(hr))
        {
            hr = s_GetPropertyBoolValue(pPropStoreLnk, PKEY_Console_FilterOnPaste, &pStateInfo->fFilterOnPaste);
            if (SUCCEEDED(hr))
            {
                hr = s_GetPropertyBoolValue(pPropStoreLnk, PKEY_Console_CtrlKeyShortcutsDisabled, &pStateInfo->fCtrlKeyShortcutsDisabled);
                if (SUCCEEDED(hr))
                {
                    hr = s_GetPropertyBoolValue(pPropStoreLnk, PKEY_Console_LineSelection, &pStateInfo->fLineSelection);
                    if (SUCCEEDED(hr))
                    {
                        hr = s_GetPropertyByteValue(pPropStoreLnk, PKEY_Console_WindowTransparency, &pStateInfo->bWindowTransparency);
                    }
                }
            }
        }

        pPropStoreLnk->Release();
    }

    return hr;
}

// Given a shortcut filename, determine what title we should use. Under normal circumstances, we rely on the shell to
// provide the correct title. However, if that fails, we'll just use the shortcut filename minus the extension.
void ShortcutSerialization::s_GetLinkTitle(_In_ PCWSTR pwszShortcutFilename, _Out_writes_(cchShortcutTitle) PWSTR pwszShortcutTitle, _In_ const size_t cchShortcutTitle)
{
    NTSTATUS Status = (cchShortcutTitle > 0) ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER_2;
    if (NT_SUCCESS(Status))
    {
        pwszShortcutTitle[0] = L'\0';

        WCHAR szTemp[MAX_PATH];
        Status = StringCchCopyW(szTemp, ARRAYSIZE(szTemp), pwszShortcutFilename);
        if (NT_SUCCESS(Status))
        {
            // Now load the localized title for the shortcut
            IShellItem *psi;
            HRESULT hrShellItem = SHCreateItemFromParsingName(pwszShortcutFilename, nullptr, IID_PPV_ARGS(&psi));
            if (SUCCEEDED(hrShellItem))
            {
                PWSTR pwszShortcutDisplayName;
                hrShellItem = psi->GetDisplayName(SIGDN_NORMALDISPLAY, &pwszShortcutDisplayName);
                if (SUCCEEDED(hrShellItem))
                {
                    Status = StringCchCopyW(pwszShortcutTitle, cchShortcutTitle, pwszShortcutDisplayName);
                    CoTaskMemFree(pwszShortcutDisplayName);
                }

                psi->Release();
            }
        }

        if (!NT_SUCCESS(Status))
        {
            // default to an extension-free version of the filename passed in
            Status = StringCchCopyW(pwszShortcutTitle, cchShortcutTitle, pwszShortcutFilename);
            if (NT_SUCCESS(Status))
            {
                // don't care if we can't remove the extension
                (void)PathCchRemoveExtension(pwszShortcutTitle, cchShortcutTitle);
            }
        }
    }
}

// Given a shortcut filename, retrieve IShellLink and IPersistFile itf ptrs, and ensure that the link is loaded.
HRESULT ShortcutSerialization::s_GetLoadedShellLinkForShortcut(_In_ PCWSTR pwszShortcutFileName, _In_ const DWORD dwMode, _COM_Outptr_ IShellLink **ppsl, _COM_Outptr_ IPersistFile **ppPf)
{
    *ppsl = nullptr;
    *ppPf = nullptr;
    IShellLink * psl;
    HRESULT hr = SHCoCreateInstance(NULL, &CLSID_ShellLink, NULL, IID_PPV_ARGS(&psl));
    if (SUCCEEDED(hr))
    {
        IPersistFile * pPf;
        hr = psl->QueryInterface(IID_PPV_ARGS(&pPf));
        if (SUCCEEDED(hr))
        {
            hr = pPf->Load(pwszShortcutFileName, dwMode);
            if (SUCCEEDED(hr))
            {
                hr = psl->QueryInterface(IID_PPV_ARGS(ppsl));
                if (SUCCEEDED(hr))
                {
                    hr = pPf->QueryInterface(IID_PPV_ARGS(ppPf));
                    if (FAILED(hr))
                    {
                        (*ppsl)->Release();
                        *ppsl = nullptr;
                    }
                }
            }
            pPf->Release();
        }
        psl->Release();
    }

    return hr;
}

// Retrieves console-only properties from the shortcut file specified in pStateInfo. Used by the console properties sheet.
NTSTATUS ShortcutSerialization::s_GetLinkConsoleProperties(_Inout_ PCONSOLE_STATE_INFO pStateInfo)
{
    IShellLink * psl;
    IPersistFile * ppf;
    HRESULT hr = s_GetLoadedShellLinkForShortcut(pStateInfo->LinkTitle, STGM_READ, &psl, &ppf);
    if (SUCCEEDED(hr))
    {
        hr = s_PopulateV1Properties(psl, pStateInfo);
        if (SUCCEEDED(hr))
        {
            hr = s_PopulateV2Properties(psl, pStateInfo);
        }

        ppf->Release();
        psl->Release();
    }
    return (SUCCEEDED(hr)) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

// Retrieves all shortcut properties from the file specified in pStateInfo. Used by conhostv2.dll
NTSTATUS ShortcutSerialization::s_GetLinkValues(_Inout_ PCONSOLE_STATE_INFO pStateInfo,
                                              _Out_ BOOL * const pfReadConsoleProperties,
                                              _Out_writes_opt_(cchShortcutTitle) PWSTR pwszShortcutTitle,
                                              _In_ const size_t cchShortcutTitle,
                                              _Out_writes_opt_(cchIconLocation) PWSTR pwszIconLocation,
                                              _In_ const size_t cchIconLocation,
                                              _Out_opt_ int * const piIcon,
                                              _Out_opt_ int * const piShowCmd,
                                              _Out_opt_ WORD * const pwHotKey)
{
    *pfReadConsoleProperties = false;

    if (pwszShortcutTitle && cchShortcutTitle > 0)
    {
        pwszShortcutTitle[0] = L'\0';
    }

    if (pwszIconLocation && cchIconLocation > 0)
    {
        pwszIconLocation[0] = L'\0';
    }

    IShellLink * psl;
    IPersistFile * ppf;
    HRESULT hr = s_GetLoadedShellLinkForShortcut(pStateInfo->LinkTitle, STGM_READ, &psl, &ppf);
    if (SUCCEEDED(hr))
    {
        // first, load non-console-specific shortcut properties, if requested
        if (pwszShortcutTitle)
        {
            // note: the "LinkTitle" member actually holds the filename of the shortcut, it's just poorly named.
            s_GetLinkTitle(pStateInfo->LinkTitle, pwszShortcutTitle, cchShortcutTitle);
        }

        if (pwszIconLocation && piIcon)
        {
            hr = psl->GetIconLocation(pwszIconLocation, static_cast<int>(cchIconLocation), piIcon);
        }

        if (SUCCEEDED(hr) && piShowCmd)
        {
            hr = psl->GetShowCmd(piShowCmd);
        }

        if (SUCCEEDED(hr) && pwHotKey)
        {
            hr = psl->GetHotkey(pwHotKey);
        }

        if (SUCCEEDED(hr))
        {
            // now load console-specific shortcut properties. note that we don't want to propagate errors out
            // here, since we've historically had two outcomes from this function -- we read generic shortcut
            // properties (above), and then more specific ones. if the specific ones fail, we still treat it
            // like a success so that we can continue to load.
            HRESULT hrProps = s_PopulateV1Properties(psl, pStateInfo);
            if (SUCCEEDED(hrProps))
            {
                *pfReadConsoleProperties = true;
                s_PopulateV2Properties(psl, pStateInfo);
            }
        }
        ppf->Release();
        psl->Release();
    }

    return (SUCCEEDED(hr)) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}


/**
Writes the console properties out to the link it was opened from.
Arguments:
    pStateInfo - pointer to structure containing information
Return Value:
    A status code if something failed or S_OK
*/
NTSTATUS ShortcutSerialization::s_SetLinkValues(_In_ PCONSOLE_STATE_INFO pStateInfo, _In_ const BOOL fEastAsianSystem, _In_ const BOOL fForceV2)
{
    IShellLinkW * psl;
    IPersistFile * ppf;
    HRESULT hr = s_GetLoadedShellLinkForShortcut(pStateInfo->LinkTitle, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, &psl, &ppf);
    if (SUCCEEDED(hr))
    {
        IShellLinkDataList * psldl;
        hr = psl->QueryInterface(IID_PPV_ARGS(&psldl));
        if (SUCCEEDED(hr))
        {
            // Now the link is loaded, generate new console settings section to replace the one in the link.
            NT_CONSOLE_PROPS props;
            ((LPDBLIST)&props)->cbSize      = sizeof(props);
            ((LPDBLIST)&props)->dwSignature = NT_CONSOLE_PROPS_SIG;
            props.wFillAttribute            = pStateInfo->ScreenAttributes;
            props.wPopupFillAttribute       = pStateInfo->PopupAttributes;
            props.dwScreenBufferSize        = pStateInfo->ScreenBufferSize;
            props.dwWindowSize              = pStateInfo->WindowSize;
            props.dwWindowOrigin.X          = (SHORT)pStateInfo->WindowPosX;
            props.dwWindowOrigin.Y          = (SHORT)pStateInfo->WindowPosY;
            props.nFont                     = 0;
            props.nInputBufferSize          = 0;
            props.dwFontSize                = pStateInfo->FontSize;
            props.uFontFamily               = pStateInfo->FontFamily;
            props.uFontWeight               = pStateInfo->FontWeight;
            CopyMemory(props.FaceName, pStateInfo->FaceName, sizeof(props.FaceName));
            props.uCursorSize               = pStateInfo->CursorSize;
            props.bFullScreen               = pStateInfo->FullScreen;
            props.bQuickEdit                = pStateInfo->QuickEdit;
            props.bInsertMode               = pStateInfo->InsertMode;
            props.bAutoPosition             = pStateInfo->AutoPosition;
            props.uHistoryBufferSize        = pStateInfo->HistoryBufferSize;
            props.uNumberOfHistoryBuffers   = pStateInfo->NumberOfHistoryBuffers;
            props.bHistoryNoDup             = pStateInfo->HistoryNoDup;
            CopyMemory(props.ColorTable, pStateInfo->ColorTable, sizeof(props.ColorTable));

            // Store the changes back into the link...
            hr = psldl->RemoveDataBlock(NT_CONSOLE_PROPS_SIG);
            if (SUCCEEDED(hr))
            {
                hr = psldl->AddDataBlock((LPVOID)&props);
            }

            if (SUCCEEDED(hr) && fEastAsianSystem)
            {
                NT_FE_CONSOLE_PROPS fe_props;
                ((LPDBLIST)&fe_props)->cbSize      = sizeof(fe_props);
                ((LPDBLIST)&fe_props)->dwSignature = NT_FE_CONSOLE_PROPS_SIG;
                fe_props.uCodePage                 = pStateInfo->CodePage;

                hr = psldl->RemoveDataBlock(NT_FE_CONSOLE_PROPS_SIG);
                if (SUCCEEDED(hr))
                {
                    hr = psldl->AddDataBlock((LPVOID)&fe_props);
                }
            }

            if (SUCCEEDED(hr))
            {
                IPropertyStore * pps;
                hr = psl->QueryInterface(IID_IPropertyStore, reinterpret_cast<void**>(&pps));
                if (SUCCEEDED(hr))
                {
                    s_SetLinkPropertyBoolValue(pps, PKEY_Console_ForceV2, fForceV2);
                    s_SetLinkPropertyBoolValue(pps, PKEY_Console_WrapText, pStateInfo->fWrapText);
                    s_SetLinkPropertyBoolValue(pps, PKEY_Console_FilterOnPaste, pStateInfo->fFilterOnPaste);
                    s_SetLinkPropertyBoolValue(pps, PKEY_Console_CtrlKeyShortcutsDisabled, pStateInfo->fCtrlKeyShortcutsDisabled);
                    s_SetLinkPropertyBoolValue(pps, PKEY_Console_LineSelection, pStateInfo->fLineSelection);
                    s_SetLinkPropertyByteValue(pps, PKEY_Console_WindowTransparency, pStateInfo->bWindowTransparency);

                    hr = pps->Commit();
                    pps->Release();
                }
            }

            psldl->Release();
        }

        if (SUCCEEDED(hr))
        {
            // Only persist changes if we've successfully made them.
            hr = ppf->Save(NULL, TRUE);
        }

        ppf->Release();
        psl->Release();
    }

    return (SUCCEEDED(hr)) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

extern "C"
{
NTSTATUS ShortcutSerializationSetLinkValues(_In_ PCONSOLE_STATE_INFO pStateInfo , _In_ BOOL fEastAsianSystem, _In_ BOOL fForceV2)
{
    return ShortcutSerialization::s_SetLinkValues(pStateInfo, fEastAsianSystem, fForceV2);
}

NTSTATUS ShortcutSerializationGetLinkConsoleProperties(_In_ PCONSOLE_STATE_INFO pStateInfo)
{
    return ShortcutSerialization::s_GetLinkConsoleProperties(pStateInfo);
}

NTSTATUS ShortcutSerializationGetLinkValues(_In_ PCONSOLE_STATE_INFO pStateInfo,
                                            _Out_ BOOL * const pfReadConsoleProperties,
                                            _Out_writes_opt_(cchShortcutTitle) PWSTR pwszShortcutTitle,
                                            _In_ const size_t cchShortcutTitle,
                                            _Out_writes_opt_(cchIconLocation) PWSTR pwszIconLocation,
                                            _In_ const size_t cchIconLocation,
                                            _Out_opt_ int * const piIcon,
                                            _Out_opt_ int * const piShowCmd,
                                            _Out_opt_ WORD * const pwHotKey)
{
    return ShortcutSerialization::s_GetLinkValues(pStateInfo, pfReadConsoleProperties, pwszShortcutTitle, cchShortcutTitle, pwszIconLocation, cchIconLocation, piIcon, piShowCmd, pwHotKey);
}
}
