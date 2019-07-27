







#include "common/winrt/CoreWindowNativeWindow.h"

namespace rx
{
NativeWindow::NativeWindow(EGLNativeWindowType window)
{
    mWindow = window;
}

bool NativeWindow::initialize()
{
    
    
    
    ComPtr<ABI::Windows::Foundation::Collections::IPropertySet> propertySet;
    ComPtr<IInspectable> eglNativeWindow;
    if (isEGLConfiguredPropertySet(mWindow, &propertySet, &eglNativeWindow))
    {
        
        
        
        
        
        mWindow = eglNativeWindow.Get();
    }

    
    ComPtr<ABI::Windows::UI::Core::ICoreWindow> coreWindow;
    if (isCoreWindow(mWindow, &coreWindow))
    {
        mImpl = std::make_shared<CoreWindowNativeWindow>();
        if (mImpl)
        {
            return mImpl->initialize(mWindow, propertySet.Get());
        }
    }
    else
    {
        ERR("Invalid IInspectable EGLNativeWindowType detected. Valid IInspectables include ICoreWindow and IPropertySet");
    }

    return false;
}

bool NativeWindow::getClientRect(RECT *rect)
{
    if (mImpl)
    {
        return mImpl->getClientRect(rect);
    }

    return false;
}

bool NativeWindow::isIconic()
{
    return false;
}

HRESULT NativeWindow::createSwapChain(ID3D11Device *device, DXGIFactory *factory, DXGI_FORMAT format, unsigned int width, unsigned int height, DXGISwapChain **swapChain)
{
    if (mImpl)
    {
        return mImpl->createSwapChain(device, factory, format, width, height, swapChain);
    }

    return E_UNEXPECTED;
}

}

bool isCoreWindow(EGLNativeWindowType window, ComPtr<ABI::Windows::UI::Core::ICoreWindow> *coreWindow)
{
    if (!window)
    {
        return false;
    }

    ComPtr<IInspectable> win = window;
    ComPtr<ABI::Windows::UI::Core::ICoreWindow> coreWin;
    if (SUCCEEDED(win.As(&coreWin)))
    {
        if (coreWindow != nullptr)
        {
            *coreWindow = coreWin.Detach();
        }
        return true;
    }

    return false;
}

bool isEGLConfiguredPropertySet(EGLNativeWindowType window, ABI::Windows::Foundation::Collections::IPropertySet **propertySet, IInspectable **eglNativeWindow)
{
    if (!window)
    {
        return false;
    }

    ComPtr<IInspectable> props = window;
    ComPtr<IPropertySet> propSet;
    ComPtr<IInspectable> nativeWindow;
    ComPtr<ABI::Windows::Foundation::Collections::IMap<HSTRING, IInspectable*>> propMap;
    boolean hasEglNativeWindowPropertyKey = false;

    HRESULT result = props.As(&propSet);
    if (SUCCEEDED(result))
    {
        result = propSet.As(&propMap);
    }

    
    if (SUCCEEDED(result))
    {
        result = propMap->HasKey(HStringReference(EGLNativeWindowTypeProperty).Get(), &hasEglNativeWindowPropertyKey);
    }

    
    
    if (SUCCEEDED(result) && !hasEglNativeWindowPropertyKey)
    {
        ERR("Could not find EGLNativeWindowTypeProperty in IPropertySet. Valid EGLNativeWindowTypeProperty values include ICoreWindow");
        return false;
    }

    
    if (SUCCEEDED(result) && hasEglNativeWindowPropertyKey)
    {
        result = propMap->Lookup(HStringReference(EGLNativeWindowTypeProperty).Get(), &nativeWindow);
    }

    if (SUCCEEDED(result))
    {
        if (propertySet != nullptr)
        {
            result = propSet.CopyTo(propertySet);
        }
    }

    if (SUCCEEDED(result))
    {
        if (eglNativeWindow != nullptr)
        {
            result = nativeWindow.CopyTo(eglNativeWindow);
        }
    }

    if (SUCCEEDED(result))
    {
        return true;
    }

    return false;
}







bool isValidEGLNativeWindowType(EGLNativeWindowType window)
{
    return isCoreWindow(window) || isEGLConfiguredPropertySet(window);
}













HRESULT getOptionalSizePropertyValue(const ComPtr<ABI::Windows::Foundation::Collections::IMap<HSTRING, IInspectable*>>& propertyMap, const wchar_t *propertyName, SIZE *value, bool *valueExists)
{
    if (!propertyMap || !propertyName || !value || !valueExists)
    {
        return false;
    }

    
    *valueExists = false;
    *value = { 0, 0 };

    ComPtr<ABI::Windows::Foundation::IPropertyValue> propertyValue;
    ABI::Windows::Foundation::PropertyType propertyType = ABI::Windows::Foundation::PropertyType::PropertyType_Empty;
    Size sizeValue = { 0, 0 };
    boolean hasKey = false;

    HRESULT result = propertyMap->HasKey(HStringReference(propertyName).Get(), &hasKey);
    if (SUCCEEDED(result) && !hasKey)
    {
        
        
        *valueExists = false;
        return S_OK;
    }

    if (SUCCEEDED(result))
    {
        result = propertyMap->Lookup(HStringReference(propertyName).Get(), &propertyValue);
    }

    if (SUCCEEDED(result))
    {
        result = propertyValue->get_Type(&propertyType);
    }

    
    if (SUCCEEDED(result) && propertyType == ABI::Windows::Foundation::PropertyType::PropertyType_Size)
    {
        if (SUCCEEDED(propertyValue->GetSize(&sizeValue)) && (sizeValue.Width > 0 && sizeValue.Height > 0))
        {
            
            *value = { static_cast<long>(sizeValue.Width), static_cast<long>(sizeValue.Height) };
            *valueExists = true;
            result = S_OK;
        }
        else
        {
            
            result = E_INVALIDARG;
        }
    }
    else
    {
        
        result = E_INVALIDARG;
    }

    return result;
}