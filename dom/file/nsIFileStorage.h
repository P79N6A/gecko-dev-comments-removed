





#ifndef nsIFileStorage_h__
#define nsIFileStorage_h__

#include "nsISupports.h"

#define NS_FILESTORAGE_IID \
  {0xbba9c2ff, 0x85c9, 0x47c1, \
  { 0xaf, 0xce, 0x0a, 0x7e, 0x6f, 0x21, 0x50, 0x95 } }

class nsIFileStorage : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_FILESTORAGE_IID)

  virtual nsISupports*
  StorageId() = 0;

  virtual bool
  IsStorageInvalidated() = 0;

  virtual bool
  IsStorageShuttingDown() = 0;

  virtual void
  SetThreadLocals() = 0;

  virtual void
  UnsetThreadLocals() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFileStorage, NS_FILESTORAGE_IID)

#define NS_DECL_NSIFILESTORAGE \
  virtual nsISupports*         \
  StorageId();                 \
                               \
  virtual bool                 \
  IsStorageInvalidated();      \
                               \
  virtual bool                 \
  IsStorageShuttingDown();     \
                               \
  virtual void                 \
  SetThreadLocals();           \
                               \
  virtual void                 \
  UnsetThreadLocals();

#endif 
