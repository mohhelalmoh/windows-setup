#include "register.h"
#include <strsafe.h>

ComposerShellReg::ComposerShellReg(void) :
    m_RootKey(HKEY_LOCAL_MACHINE),  
    m_ClassPath(L"SOFTWARE\\Classes")
{
    StringFromGUID2(COMPOSER_CLSID, m_CLSIDStr, ARRAYSIZE(m_CLSIDStr));
    
    HKEY key;
    if (ERROR_SUCCESS != RegOpenKeyEx(m_RootKey, m_ClassPath, 0, KEY_WRITE, &key))
    {
        m_RootKey = HKEY_CURRENT_USER;
    }

}


HRESULT ComposerShellReg::Register(PCWSTR module)
{
    if (module == NULL) return E_INVALIDARG;

    HRESULT hr = ServerRegister(module);

    if (SUCCEEDED(hr))
    {
        hr = HandlersDoRegister(true);
    }

    return hr;
}


HRESULT ComposerShellReg::Unregister()
{
    HRESULT hr = ServerUnregister();

    if (SUCCEEDED(hr))
    {
        hr = HandlersDoRegister(false);
    }

    return hr;
}


HRESULT ComposerShellReg::HandlersDoRegister(BOOL reg)
{
    HRESULT hr = E_INVALIDARG;
    wchar_t subkey[MAX_PATH];

    for (int i = 0; i < ARRAYSIZE(COMPOSER_HANDLERS); ++i)
    {
        hr = PathGetForHandler(COMPOSER_HANDLERS[i], subkey, ARRAYSIZE(subkey));
        if (FAILED(hr)) break;
        
        if (reg)
        {
            hr = SetKeyAndValue(subkey, NULL, m_CLSIDStr);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(RegDeleteTree(m_RootKey, subkey));
        }

        if (FAILED(hr)) break;
    }

    return hr;
}


HRESULT ComposerShellReg::PathGetForHandler(PCWSTR handler, LPWSTR path, size_t cch)
{
    // Check that the handler is not for a specific file type (ie .ext) as we do not process these
    if (NULL == handler || L'.' == *handler) return E_INVALIDARG;

    // Set path to HK??\SOFTWARE\CLASSES\{handler name}\shellex\ContextMenuHandlers\{<COMPOSER_NAME>}
    return StringCchPrintf(path, cch, L"%s\\%s\\shellex\\ContextMenuHandlers\\%s", m_ClassPath, handler, COMPOSER_NAME);
}


HRESULT ComposerShellReg::PathGetForServer(LPWSTR path, size_t cch)
{
    // Set path to HK??\SOFTWARE\CLASSES\CLSID\{<CLSID>}
    return StringCchPrintf(path, cch, L"%s\\CLSID\\%s", m_ClassPath, m_CLSIDStr);
}


HRESULT ComposerShellReg::ServerRegister(PCWSTR module)
{
    HRESULT hr;
        
    wchar_t serverKey[MAX_PATH];
    hr = PathGetForServer(serverKey, ARRAYSIZE(serverKey));
    if (FAILED(hr)) return hr;
    
    // Set subkey to {serverKey}\InprocServer32
    wchar_t subkey[MAX_PATH];
    hr = StringCchPrintf(subkey, ARRAYSIZE(subkey),  L"%s\\InprocServer32", serverKey);
    if (FAILED(hr)) return hr;

    // Create the serverKey
    hr = SetKeyAndValue(serverKey, NULL, COMPOSER_NAME);
    if (SUCCEEDED(hr))
    {

        // Create subkey (InprocServer32) and set its default value to the path of the COM module
        hr = SetKeyAndValue(subkey, NULL, module);
        if (SUCCEEDED(hr))
        {
            // Set the threading model of the component
            hr = SetKeyAndValue(subkey, L"ThreadingModel", L"Apartment");
        }
    }

    return hr;
}


HRESULT ComposerShellReg::ServerUnregister()
{
    HRESULT hr;

    wchar_t subkey[MAX_PATH];
    hr = PathGetForServer(subkey, ARRAYSIZE(subkey));

    if (SUCCEEDED(hr))
    {
        hr = HRESULT_FROM_WIN32(RegDeleteTree(m_RootKey, subkey));
    }

    return hr;
}


HRESULT ComposerShellReg::SetKeyAndValue(PCWSTR subKey, PCWSTR valueName, PCWSTR data)
{
    HRESULT hr;
    HKEY hKey = NULL;
    
    // Create or open the key 
    hr = HRESULT_FROM_WIN32(RegCreateKeyEx(m_RootKey, subKey, 0, 
        NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL));

    if (SUCCEEDED(hr))
    {
        if (data != NULL)
        {
            // Set the specified value of the key
            DWORD cbData = lstrlen(data) * sizeof(*data);
            hr = HRESULT_FROM_WIN32(RegSetValueEx(hKey, valueName, 0, 
                REG_SZ, reinterpret_cast<const BYTE *>(data), cbData));
        }

        RegCloseKey(hKey);
    }

    return hr;
}
