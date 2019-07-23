




































#ifndef nsPIBoxObject_h___
#define nsPIBoxObject_h___


#define NS_PIBOXOBJECT_IID \
{ 0x2b8bb262, 0x1b0f, 0x4572, \
  { 0xba, 0x87, 0x5d, 0x4a, 0xe4, 0x95, 0x44, 0x45 } }


class nsIPresShell;
class nsIContent;
class nsIDocument;

class nsPIBoxObject : public nsIBoxObject
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PIBOXOBJECT_IID)

  virtual nsresult Init(nsIContent* aContent) = 0;

  
  virtual void Clear() = 0;

  
  
  virtual void ClearCachedValues() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPIBoxObject, NS_PIBOXOBJECT_IID)

#endif

