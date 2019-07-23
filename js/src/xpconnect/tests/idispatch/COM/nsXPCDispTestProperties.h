



#if !defined(AFX_NSXPCDISPTESTPROPERTIES_H__9E10C7AC_36AF_4A3A_91C7_2CFB9EB166A5__INCLUDED_)
#define AFX_NSXPCDISPTESTPROPERTIES_H__9E10C7AC_36AF_4A3A_91C7_2CFB9EB166A5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 

#include "resource.h"       




class nsXPCDispTestProperties : 
    public IDispatchImpl<nsIXPCDispTestProperties, &IID_nsIXPCDispTestProperties, &LIBID_IDispatchTestLib>, 
    public ISupportErrorInfo,
    public CComObjectRoot,
    public CComCoClass<nsXPCDispTestProperties,&CLSID_nsXPCDispTestProperties>
{
public:
    nsXPCDispTestProperties();
    virtual ~nsXPCDispTestProperties();
BEGIN_CATEGORY_MAP(nsXPCDispTestProperties)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
END_CATEGORY_MAP()
BEGIN_COM_MAP(nsXPCDispTestProperties)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(nsIXPCDispTestProperties)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(nsXPCDispTestProperties) 

DECLARE_REGISTRY_RESOURCEID(IDR_nsXPCDispTestProperties)

    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);


public:
	STDMETHOD(get_ParameterizedPropertyCount)( long *pVal);
	STDMETHOD(get_ParameterizedProperty)(long aIndex,  long *pVal);
	STDMETHOD(put_ParameterizedProperty)(long aIndex,  long newVal);
    STDMETHOD(get_Char)( unsigned char *pVal);
    STDMETHOD(put_Char)( unsigned char newVal);
    STDMETHOD(get_COMPtr)( IUnknown* *pVal);
    STDMETHOD(put_COMPtr)( IUnknown* newVal);
    STDMETHOD(get_Variant)( VARIANT *pVal);
    STDMETHOD(put_Variant)( VARIANT newVal);
    STDMETHOD(get_Boolean)( BOOL *pVal);
    STDMETHOD(put_Boolean)( BOOL newVal);
    STDMETHOD(get_SCode)( SCODE *pVal);
    STDMETHOD(put_SCode)( SCODE newVal);
    STDMETHOD(get_DispatchPtr)( IDispatch* *pVal);
    STDMETHOD(put_DispatchPtr)( IDispatch* newVal);
    STDMETHOD(get_String)( BSTR *pVal);
    STDMETHOD(put_String)( BSTR newVal);
    STDMETHOD(get_Date)( DATE *pVal);
    STDMETHOD(put_Date)( DATE newVal);
    STDMETHOD(get_Currency)( CURRENCY *pVal);
    STDMETHOD(put_Currency)( CURRENCY newVal);
    STDMETHOD(get_Double)( double *pVal);
    STDMETHOD(put_Double)( double newVal);
    STDMETHOD(get_Float)( float *pVal);
    STDMETHOD(put_Float)( float newVal);
    STDMETHOD(get_Long)( long *pVal);
    STDMETHOD(put_Long)( long newVal);
    STDMETHOD(get_Short)( short *pVal);
    STDMETHOD(put_Short)( short newVal);
private:
    unsigned char       mChar;
    CComPtr<IUnknown>   mIUnknown;
    CComVariant         mVariant;
    BOOL                mBOOL;
    SCODE               mSCode;
    CComPtr<IDispatch>  mIDispatch;
    CComBSTR            mBSTR;
    DATE                mDATE;
    CURRENCY            mCURRENCY;
    double              mDouble;
    float               mFloat;
    long                mLong;
    short               mShort;
    long *              mParameterizedProperty;
};

#endif 
