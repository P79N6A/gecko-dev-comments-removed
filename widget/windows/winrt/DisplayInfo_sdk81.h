




 


 

#ifndef ____FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable_FWD_DEFINED__
#define ____FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable_FWD_DEFINED__
typedef interface __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable;

#endif 	


#ifndef ____x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformationStatics_FWD_DEFINED__
#define ____x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformationStatics_FWD_DEFINED__
typedef interface __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformationStatics __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformationStatics;

#ifdef __cplusplus
namespace ABI {
    namespace Windows {
        namespace Graphics {
            namespace Display {
                interface IDisplayInformationStatics;
            } 
        } 
    } 
} 

#endif 

#endif 	


#ifndef ____x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation_FWD_DEFINED__
#define ____x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation_FWD_DEFINED__
typedef interface __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation;

#ifdef __cplusplus
namespace ABI {
    namespace Windows {
        namespace Graphics {
            namespace Display {
                interface IDisplayInformation;
            } 
        } 
    } 
} 

#endif 

#endif 	


#ifdef __cplusplus
namespace ABI {
namespace Windows {
namespace Graphics {
namespace Display {
class DisplayInformation;
} 
} 
} 
}
#endif

#ifdef __cplusplus
namespace ABI {
namespace Windows {
namespace Graphics {
namespace Display {
interface IDisplayInformation;
} 
} 
} 
}
#endif

interface IInspectable;



 





extern RPC_IF_HANDLE __MIDL_itf_windows2Egraphics2Edisplay_0000_0000_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_windows2Egraphics2Edisplay_0000_0000_v0_0_s_ifspec;











extern RPC_IF_HANDLE __MIDL_itf_windows2Egraphics2Edisplay_0000_0580_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_windows2Egraphics2Edisplay_0000_0580_v0_0_s_ifspec;


 

#ifndef DEF___FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable_USE
#define DEF___FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable_USE
#if defined(__cplusplus) && !defined(RO_NO_TEMPLATE_NAME)
namespace ABI { namespace Windows { namespace Foundation {
template <>
struct __declspec(uuid("86c4f619-67b6-51c7-b30d-d8cf13625327"))
ITypedEventHandler<ABI::Windows::Graphics::Display::DisplayInformation*,IInspectable*> : ITypedEventHandler_impl<ABI::Windows::Foundation::Internal::AggregateType<ABI::Windows::Graphics::Display::DisplayInformation*, ABI::Windows::Graphics::Display::IDisplayInformation*>,IInspectable*> {
static const wchar_t* z_get_rc_name_impl() {
return L"Windows.Foundation.TypedEventHandler`2<Windows.Graphics.Display.DisplayInformation, Object>"; }
};
typedef ITypedEventHandler<ABI::Windows::Graphics::Display::DisplayInformation*,IInspectable*> __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable_t;
#define ____FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable_FWD_DEFINED__
#define __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable ABI::Windows::Foundation::__FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable_t

 }  }  }
#endif 
#endif 

#ifndef DEF___FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable
#define DEF___FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable
#if !defined(__cplusplus) || defined(RO_NO_TEMPLATE_NAME)



 



extern RPC_IF_HANDLE __MIDL_itf_windows2Egraphics2Edisplay_0000_0004_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_windows2Egraphics2Edisplay_0000_0004_v0_0_s_ifspec;

#ifndef ____FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable_INTERFACE_DEFINED__
#define ____FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable_INTERFACE_DEFINED__


 




 


EXTERN_C const IID IID___FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("86c4f619-67b6-51c7-b30d-d8cf13625327")
    __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke( 
             __RPC__in_opt ABI::Windows::Graphics::Display::IDisplayInformation *sender,
             __RPC__in_opt IInspectable *e) = 0;
        
    };
    
    
#else 	

    typedef struct __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectableVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            __RPC__in __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable * This,
             __RPC__in REFIID riid,
             
            _COM_Outptr_  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            __RPC__in __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            __RPC__in __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable * This);
        
        HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            __RPC__in __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable * This,
             __RPC__in_opt __x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation *sender,
             __RPC__in_opt IInspectable *e);
        
        END_INTERFACE
    } __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectableVtbl;

    interface __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable
    {
        CONST_VTBL struct __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectableVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable_Invoke(This,sender,e)	\
    ( (This)->lpVtbl -> Invoke(This,sender,e) ) 

#endif 


#endif 	




#endif 	



 

#endif 
#endif 



 



 




 





 

#if !defined(____x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformationStatics_INTERFACE_DEFINED__)
extern const __declspec(selectany) _Null_terminated_ WCHAR InterfaceName_Windows_Graphics_Display_IDisplayInformationStatics[] = L"Windows.Graphics.Display.IDisplayInformationStatics";
#endif 



 



extern RPC_IF_HANDLE __MIDL_itf_windows2Egraphics2Edisplay_0000_0006_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_windows2Egraphics2Edisplay_0000_0006_v0_0_s_ifspec;

#ifndef ____x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformationStatics_INTERFACE_DEFINED__
#define ____x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformationStatics_INTERFACE_DEFINED__


 




 


EXTERN_C const IID IID___x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformationStatics;

#if defined(__cplusplus) && !defined(CINTERFACE)
    namespace ABI {
        namespace Windows {
            namespace Graphics {
                namespace Display {
                    
                    MIDL_INTERFACE("C6A02A6C-D452-44DC-BA07-96F3C6ADF9D1")
                    IDisplayInformationStatics : public IInspectable
                    {
                    public:
                        virtual HRESULT STDMETHODCALLTYPE GetForCurrentView( 
                             __RPC__deref_out_opt ABI::Windows::Graphics::Display::IDisplayInformation **current) = 0;
                        
                        virtual  HRESULT STDMETHODCALLTYPE get_AutoRotationPreferences( 
                             __RPC__out ABI::Windows::Graphics::Display::DisplayOrientations *value) = 0;
                        
                        virtual  HRESULT STDMETHODCALLTYPE put_AutoRotationPreferences( 
                             ABI::Windows::Graphics::Display::DisplayOrientations value) = 0;
                        
                        virtual HRESULT STDMETHODCALLTYPE add_DisplayContentsInvalidated( 
                             __RPC__in_opt __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable *handler,
                             __RPC__out EventRegistrationToken *token) = 0;
                        
                        virtual HRESULT STDMETHODCALLTYPE remove_DisplayContentsInvalidated( 
                             EventRegistrationToken token) = 0;
                        
                    };

                    extern const __declspec(selectany) IID & IID_IDisplayInformationStatics = __uuidof(IDisplayInformationStatics);

                    
                }  
            }  
        }  
    }  

#endif 	




#endif 	



 

#if !defined(____x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation_INTERFACE_DEFINED__)
extern const __declspec(selectany) _Null_terminated_ WCHAR InterfaceName_Windows_Graphics_Display_IDisplayInformation[] = L"Windows.Graphics.Display.IDisplayInformation";
#endif 



 



extern RPC_IF_HANDLE __MIDL_itf_windows2Egraphics2Edisplay_0000_0007_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_windows2Egraphics2Edisplay_0000_0007_v0_0_s_ifspec;

#ifndef ____x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation_INTERFACE_DEFINED__
#define ____x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation_INTERFACE_DEFINED__


 




 


EXTERN_C const IID IID___x_ABI_CWindows_CGraphics_CDisplay_CIDisplayInformation;

#if defined(__cplusplus) && !defined(CINTERFACE)
    namespace ABI {
        namespace Windows {
            namespace Graphics {
                namespace Display {
                    
                    MIDL_INTERFACE("BED112AE-ADC3-4DC9-AE65-851F4D7D4799")
                    IDisplayInformation : public IInspectable
                    {
                    public:
                        virtual  HRESULT STDMETHODCALLTYPE get_CurrentOrientation( 
                             __RPC__out ABI::Windows::Graphics::Display::DisplayOrientations *value) = 0;
                        
                        virtual  HRESULT STDMETHODCALLTYPE get_NativeOrientation( 
                             __RPC__out ABI::Windows::Graphics::Display::DisplayOrientations *value) = 0;
                        
                        virtual HRESULT STDMETHODCALLTYPE add_OrientationChanged( 
                             __RPC__in_opt __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable *handler,
                             __RPC__out EventRegistrationToken *token) = 0;
                        
                        virtual HRESULT STDMETHODCALLTYPE remove_OrientationChanged( 
                             EventRegistrationToken token) = 0;
                        
                        virtual  HRESULT STDMETHODCALLTYPE get_ResolutionScale( 
                             __RPC__out ABI::Windows::Graphics::Display::ResolutionScale *value) = 0;
                        
                        virtual  HRESULT STDMETHODCALLTYPE get_LogicalDpi( 
                             __RPC__out FLOAT *value) = 0;
                        
                        virtual  HRESULT STDMETHODCALLTYPE get_RawDpiX( 
                             __RPC__out FLOAT *value) = 0;
                        
                        virtual  HRESULT STDMETHODCALLTYPE get_RawDpiY( 
                             __RPC__out FLOAT *value) = 0;
                        
                        virtual HRESULT STDMETHODCALLTYPE add_DpiChanged( 
                             __RPC__in_opt __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable *handler,
                             __RPC__out EventRegistrationToken *token) = 0;
                        
                        virtual HRESULT STDMETHODCALLTYPE remove_DpiChanged( 
                             EventRegistrationToken token) = 0;
                        
                        virtual  HRESULT STDMETHODCALLTYPE get_StereoEnabled( 
                             __RPC__out boolean *value) = 0;
                        
                        virtual HRESULT STDMETHODCALLTYPE add_StereoEnabledChanged( 
                             __RPC__in_opt __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable *handler,
                             __RPC__out EventRegistrationToken *token) = 0;
                        
                        virtual HRESULT STDMETHODCALLTYPE remove_StereoEnabledChanged( 
                             EventRegistrationToken token) = 0;
                        
                        virtual HRESULT STDMETHODCALLTYPE GetColorProfileAsync( 
                             __RPC__deref_out_opt __FIAsyncOperation_1_Windows__CStorage__CStreams__CIRandomAccessStream **asyncInfo) = 0;
                        
                        virtual HRESULT STDMETHODCALLTYPE add_ColorProfileChanged( 
                             __RPC__in_opt __FITypedEventHandler_2_Windows__CGraphics__CDisplay__CDisplayInformation_IInspectable *handler,
                             __RPC__out EventRegistrationToken *token) = 0;
                        
                        virtual HRESULT STDMETHODCALLTYPE remove_ColorProfileChanged( 
                             EventRegistrationToken token) = 0;
                        
                    };

                    extern const __declspec(selectany) IID & IID_IDisplayInformation = __uuidof(IDisplayInformation);

                    
                }  
            }  
        }  
    }  

#endif 	




#endif 	



 



 

#ifndef RUNTIMECLASS_Windows_Graphics_Display_DisplayInformation_DEFINED
#define RUNTIMECLASS_Windows_Graphics_Display_DisplayInformation_DEFINED
extern const __declspec(selectany) _Null_terminated_ WCHAR RuntimeClass_Windows_Graphics_Display_DisplayInformation[] = L"Windows.Graphics.Display.DisplayInformation";
#endif



 



extern RPC_IF_HANDLE __MIDL_itf_windows2Egraphics2Edisplay_0000_0009_v0_0_c_ifspec;
extern RPC_IF_HANDLE __MIDL_itf_windows2Egraphics2Edisplay_0000_0009_v0_0_s_ifspec;




