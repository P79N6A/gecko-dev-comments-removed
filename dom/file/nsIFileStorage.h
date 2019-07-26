





#ifndef nsIFileStorage_h__
#define nsIFileStorage_h__

#include "nsISupports.h"

#define NS_FILESTORAGE_IID \
  {0x6278f453, 0xd557, 0x4a55, \
  { 0x99, 0x3e, 0xf4, 0x69, 0xe2, 0xa5, 0xe1, 0xd0 } }

class nsIAtom;

class nsIFileStorage : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_FILESTORAGE_IID)

  NS_IMETHOD_(nsIAtom*)
  Id() = 0;

  
  
  NS_IMETHOD_(bool)
  IsInvalidated() = 0;

  NS_IMETHOD_(bool)
  IsShuttingDown() = 0;

  NS_IMETHOD_(void)
  SetThreadLocals() = 0;

  NS_IMETHOD_(void)
  UnsetThreadLocals() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFileStorage, NS_FILESTORAGE_IID)

#define NS_DECL_NSIFILESTORAGE                                                 \
  NS_IMETHOD_(nsIAtom*)                                                        \
  Id() MOZ_OVERRIDE;                                                           \
                                                                               \
  NS_IMETHOD_(bool)                                                            \
  IsInvalidated() MOZ_OVERRIDE;                                                \
                                                                               \
  NS_IMETHOD_(bool)                                                            \
  IsShuttingDown() MOZ_OVERRIDE;                                               \
                                                                               \
  NS_IMETHOD_(void)                                                            \
  SetThreadLocals() MOZ_OVERRIDE;                                              \
                                                                               \
  NS_IMETHOD_(void)                                                            \
  UnsetThreadLocals() MOZ_OVERRIDE;

#endif 
