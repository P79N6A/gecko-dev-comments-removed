






































#ifndef nsITreeImageListener_h__
#define nsITreeImageListener_h__



#define NS_ITREEIMAGELISTENER_IID \
{ 0x90586540, 0x2d50, 0x403e, { 0x8d, 0xce, 0x98, 0x1c, 0xaa, 0x77, 0x84, 0x44 } }

class nsITreeImageListener : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITREEIMAGELISTENER_IID)

  NS_IMETHOD AddCell(PRInt32 aIndex, nsITreeColumn* aCol) = 0;

  



  NS_IMETHOD ClearFrame() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsITreeImageListener, NS_ITREEIMAGELISTENER_IID)

#endif
