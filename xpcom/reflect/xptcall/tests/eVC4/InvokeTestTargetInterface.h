



#ifndef __gen_InvokeTestTargetInterface_h__
#define __gen_InvokeTestTargetInterface_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif


#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif


#define INVOKETESTTARGETINTERFACE_IID_STR "aac1fb90-e099-11d2-984e-006008962422"

#define INVOKETESTTARGETINTERFACE_IID {0xaac1fb90, 0xe099, 0x11d2, { 0x98, 0x4e, 0x00, 0x60, 0x08, 0x96, 0x24, 0x22 }}

class NS_NO_VTABLE InvokeTestTargetInterface : public nsISupports {
 public: 



  
  NS_IMETHOD AddTwoInts(PRInt32 p1, PRInt32 p2, PRInt32 *_retval) = 0;

  
  NS_IMETHOD MultTwoInts(PRInt32 p1, PRInt32 p2, PRInt32 *_retval) = 0;

  
  NS_IMETHOD AddTwoLLs(PRInt64 p1, PRInt64 p2, PRInt64 *_retval) = 0;

  
  NS_IMETHOD MultTwoLLs(PRInt64 p1, PRInt64 p2, PRInt64 *_retval) = 0;

  
  NS_IMETHOD AddManyInts(PRInt32 p1, PRInt32 p2, PRInt32 p3, PRInt32 p4, PRInt32 p5, PRInt32 p6, PRInt32 p7, PRInt32 p8, PRInt32 p9, PRInt32 p10, PRInt32 *_retval) = 0;

  
  NS_IMETHOD AddTwoFloats(float p1, float p2, float *_retval) = 0;

  
  NS_IMETHOD AddManyDoubles(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9, double p10, double *_retval) = 0;

  
  NS_IMETHOD AddManyFloats(float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, float *_retval) = 0;

  
  NS_IMETHOD AddManyManyFloats(float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, float p11, float p12, float p13, float p14, float p15, float p16, float p17, float p18, float p19, float p20, float *_retval) = 0;

  
  NS_IMETHOD AddMixedInts(PRInt64 p1, PRInt32 p2, PRInt64 p3, PRInt32 p4, PRInt32 p5, PRInt64 p6, PRInt32 p7, PRInt32 p8, PRInt64 p9, PRInt32 p10, PRInt64 *_retval) = 0;

  
  NS_IMETHOD AddMixedInts2(PRInt32 p1, PRInt64 p2, PRInt32 p3, PRInt64 p4, PRInt64 p5, PRInt32 p6, PRInt64 p7, PRInt64 p8, PRInt32 p9, PRInt64 p10, PRInt64 *_retval) = 0;

  
  NS_IMETHOD AddMixedFloats(float p1, float p2, double p3, double p4, float p5, float p6, double p7, double p8, float p9, double p10, float p11, double *_retval) = 0;

  
  NS_IMETHOD PassTwoStrings(const char *ignore, const char *s1, const char *s2, char **retval) = 0;

};




#define NS_DECL_INVOKETESTTARGETINTERFACE \
  NS_IMETHOD AddTwoInts(PRInt32 p1, PRInt32 p2, PRInt32 *_retval); \
  NS_IMETHOD MultTwoInts(PRInt32 p1, PRInt32 p2, PRInt32 *_retval); \
  NS_IMETHOD AddTwoLLs(PRInt64 p1, PRInt64 p2, PRInt64 *_retval); \
  NS_IMETHOD MultTwoLLs(PRInt64 p1, PRInt64 p2, PRInt64 *_retval); \
  NS_IMETHOD AddManyInts(PRInt32 p1, PRInt32 p2, PRInt32 p3, PRInt32 p4, PRInt32 p5, PRInt32 p6, PRInt32 p7, PRInt32 p8, PRInt32 p9, PRInt32 p10, PRInt32 *_retval); \
  NS_IMETHOD AddTwoFloats(float p1, float p2, float *_retval); \
  NS_IMETHOD AddManyDoubles(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9, double p10, double *_retval); \
  NS_IMETHOD AddManyFloats(float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, float *_retval); \
  NS_IMETHOD AddManyManyFloats(float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, float p11, float p12, float p13, float p14, float p15, float p16, float p17, float p18, float p19, float p20, float *_retval); \
  NS_IMETHOD AddMixedInts(PRInt64 p1, PRInt32 p2, PRInt64 p3, PRInt32 p4, PRInt32 p5, PRInt64 p6, PRInt32 p7, PRInt32 p8, PRInt64 p9, PRInt32 p10, PRInt64 *_retval); \
  NS_IMETHOD AddMixedInts2(PRInt32 p1, PRInt64 p2, PRInt32 p3, PRInt64 p4, PRInt64 p5, PRInt32 p6, PRInt64 p7, PRInt64 p8, PRInt32 p9, PRInt64 p10, PRInt64 *_retval); \
  NS_IMETHOD AddMixedFloats(float p1, float p2, double p3, double p4, float p5, float p6, double p7, double p8, float p9, double p10, float p11, double *_retval); \
  NS_IMETHOD PassTwoStrings(const char *ignore, const char *s1, const char *s2, char **retval); 


#define NS_FORWARD_INVOKETESTTARGETINTERFACE(_to) \
  NS_IMETHOD AddTwoInts(PRInt32 p1, PRInt32 p2, PRInt32 *_retval) { return _to AddTwoInts(p1, p2, _retval); } \
  NS_IMETHOD MultTwoInts(PRInt32 p1, PRInt32 p2, PRInt32 *_retval) { return _to MultTwoInts(p1, p2, _retval); } \
  NS_IMETHOD AddTwoLLs(PRInt64 p1, PRInt64 p2, PRInt64 *_retval) { return _to AddTwoLLs(p1, p2, _retval); } \
  NS_IMETHOD MultTwoLLs(PRInt64 p1, PRInt64 p2, PRInt64 *_retval) { return _to MultTwoLLs(p1, p2, _retval); } \
  NS_IMETHOD AddManyInts(PRInt32 p1, PRInt32 p2, PRInt32 p3, PRInt32 p4, PRInt32 p5, PRInt32 p6, PRInt32 p7, PRInt32 p8, PRInt32 p9, PRInt32 p10, PRInt32 *_retval) { return _to AddManyInts(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, _retval); } \
  NS_IMETHOD AddTwoFloats(float p1, float p2, float *_retval) { return _to AddTwoFloats(p1, p2, _retval); } \
  NS_IMETHOD AddManyDoubles(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9, double p10, double *_retval) { return _to AddManyDoubles(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, _retval); } \
  NS_IMETHOD AddManyFloats(float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, float *_retval) { return _to AddManyFloats(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, _retval); } \
  NS_IMETHOD AddManyManyFloats(float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, float p11, float p12, float p13, float p14, float p15, float p16, float p17, float p18, float p19, float p20, float *_retval) { return _to AddManyManyFloats(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19, p20, _retval); } \
  NS_IMETHOD AddMixedInts(PRInt64 p1, PRInt32 p2, PRInt64 p3, PRInt32 p4, PRInt32 p5, PRInt64 p6, PRInt32 p7, PRInt32 p8, PRInt64 p9, PRInt32 p10, PRInt64 *_retval) { return _to AddMixedInts(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, _retval); } \
  NS_IMETHOD AddMixedInts2(PRInt32 p1, PRInt64 p2, PRInt32 p3, PRInt64 p4, PRInt64 p5, PRInt32 p6, PRInt64 p7, PRInt64 p8, PRInt32 p9, PRInt64 p10, PRInt64 *_retval) { return _to AddMixedInts2(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, _retval); } \
  NS_IMETHOD AddMixedFloats(float p1, float p2, double p3, double p4, float p5, float p6, double p7, double p8, float p9, double p10, float p11, double *_retval) { return _to AddMixedFloats(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, _retval); } \
  NS_IMETHOD PassTwoStrings(const char *ignore, const char *s1, const char *s2, char **retval) { return _to PassTwoStrings(ignore, s1, s2, retval); } 


#define NS_FORWARD_SAFE_INVOKETESTTARGETINTERFACE(_to) \
  NS_IMETHOD AddTwoInts(PRInt32 p1, PRInt32 p2, PRInt32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddTwoInts(p1, p2, _retval); } \
  NS_IMETHOD MultTwoInts(PRInt32 p1, PRInt32 p2, PRInt32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->MultTwoInts(p1, p2, _retval); } \
  NS_IMETHOD AddTwoLLs(PRInt64 p1, PRInt64 p2, PRInt64 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddTwoLLs(p1, p2, _retval); } \
  NS_IMETHOD MultTwoLLs(PRInt64 p1, PRInt64 p2, PRInt64 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->MultTwoLLs(p1, p2, _retval); } \
  NS_IMETHOD AddManyInts(PRInt32 p1, PRInt32 p2, PRInt32 p3, PRInt32 p4, PRInt32 p5, PRInt32 p6, PRInt32 p7, PRInt32 p8, PRInt32 p9, PRInt32 p10, PRInt32 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddManyInts(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, _retval); } \
  NS_IMETHOD AddTwoFloats(float p1, float p2, float *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddTwoFloats(p1, p2, _retval); } \
  NS_IMETHOD AddManyDoubles(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9, double p10, double *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddManyDoubles(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, _retval); } \
  NS_IMETHOD AddManyFloats(float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, float *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddManyFloats(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, _retval); } \
  NS_IMETHOD AddManyManyFloats(float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, float p11, float p12, float p13, float p14, float p15, float p16, float p17, float p18, float p19, float p20, float *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddManyManyFloats(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15, p16, p17, p18, p19, p20, _retval); } \
  NS_IMETHOD AddMixedInts(PRInt64 p1, PRInt32 p2, PRInt64 p3, PRInt32 p4, PRInt32 p5, PRInt64 p6, PRInt32 p7, PRInt32 p8, PRInt64 p9, PRInt32 p10, PRInt64 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddMixedInts(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, _retval); } \
  NS_IMETHOD AddMixedInts2(PRInt32 p1, PRInt64 p2, PRInt32 p3, PRInt64 p4, PRInt64 p5, PRInt32 p6, PRInt64 p7, PRInt64 p8, PRInt32 p9, PRInt64 p10, PRInt64 *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddMixedInts2(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, _retval); } \
  NS_IMETHOD AddMixedFloats(float p1, float p2, double p3, double p4, float p5, float p6, double p7, double p8, float p9, double p10, float p11, double *_retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddMixedFloats(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, _retval); } \
  NS_IMETHOD PassTwoStrings(const char *ignore, const char *s1, const char *s2, char **retval) { return !_to ? NS_ERROR_NULL_POINTER : _to->PassTwoStrings(ignore, s1, s2, retval); } 

#if 0



class _MYCLASS_ : public InvokeTestTargetInterface
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_INVOKETESTTARGETINTERFACE

  _MYCLASS_();

private:
  ~_MYCLASS_();

protected:
  
};


NS_IMPL_ISUPPORTS1(_MYCLASS_, InvokeTestTargetInterface)

_MYCLASS_::_MYCLASS_()
{
  
}

_MYCLASS_::~_MYCLASS_()
{
  
}


NS_IMETHODIMP _MYCLASS_::AddTwoInts(PRInt32 p1, PRInt32 p2, PRInt32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::MultTwoInts(PRInt32 p1, PRInt32 p2, PRInt32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::AddTwoLLs(PRInt64 p1, PRInt64 p2, PRInt64 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::MultTwoLLs(PRInt64 p1, PRInt64 p2, PRInt64 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::AddManyInts(PRInt32 p1, PRInt32 p2, PRInt32 p3, PRInt32 p4, PRInt32 p5, PRInt32 p6, PRInt32 p7, PRInt32 p8, PRInt32 p9, PRInt32 p10, PRInt32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::AddTwoFloats(float p1, float p2, float *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::AddManyDoubles(double p1, double p2, double p3, double p4, double p5, double p6, double p7, double p8, double p9, double p10, double *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::AddManyFloats(float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, float *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::AddManyManyFloats(float p1, float p2, float p3, float p4, float p5, float p6, float p7, float p8, float p9, float p10, float p11, float p12, float p13, float p14, float p15, float p16, float p17, float p18, float p19, float p20, float *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::AddMixedInts(PRInt64 p1, PRInt32 p2, PRInt64 p3, PRInt32 p4, PRInt32 p5, PRInt64 p6, PRInt32 p7, PRInt32 p8, PRInt64 p9, PRInt32 p10, PRInt64 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::AddMixedInts2(PRInt32 p1, PRInt64 p2, PRInt32 p3, PRInt64 p4, PRInt64 p5, PRInt32 p6, PRInt64 p7, PRInt64 p8, PRInt32 p9, PRInt64 p10, PRInt64 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::AddMixedFloats(float p1, float p2, double p3, double p4, float p5, float p6, double p7, double p8, float p9, double p10, float p11, double *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP _MYCLASS_::PassTwoStrings(const char *ignore, const char *s1, const char *s2, char **retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


#endif


#endif
