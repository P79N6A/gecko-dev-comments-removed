



































#ifndef nsIContentUtils_h__
#define nsIContentUtils_h__


#define NS_ICONTENTUTILS_IID \
{ 0xC4EA618E, 0xA3D9, 0x4524, \
  { 0x8E, 0xEA, 0xE9, 0x2F, 0x26, 0xFC, 0x44, 0xDB } }

class nsIContentUtils : public nsISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICONTENTUTILS_IID)
    NS_DECL_ISUPPORTS

    virtual PRBool IsSafeToRunScript();
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIContentUtils, NS_ICONTENTUTILS_IID)

#endif 
