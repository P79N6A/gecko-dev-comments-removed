






































#ifndef EMBEDGLOBALHISTORY_H
#define EMBEDGLOBALHISTORY_H

#include "nsIGlobalHistory.h"

#define NS_EMBEDGLOBALHISTORY_CID \
  { 0x2f977d51, 0x5485, 0x11d4, \
  { 0x87, 0xe2, 0x00, 0x10, 0xa4, 0xe7, 0x5e, 0xf2 } }


class EmbedGlobalHistory : public nsIGlobalHistory
{
public:
    EmbedGlobalHistory();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIGLOBALHISTORY

    NS_IMETHOD        Init();



private:
    ~EmbedGlobalHistory();
};

#endif
