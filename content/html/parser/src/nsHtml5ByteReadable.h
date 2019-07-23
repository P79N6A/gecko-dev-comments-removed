




































#ifndef nsHtml5ByteReadable_h__
#define nsHtml5ByteReadable_h__

#include "prtypes.h"




class nsHtml5ByteReadable
{
  public:
    nsHtml5ByteReadable(const PRUint8* current, const PRUint8* end)
     : current(current),
       end(end)
    {
    }
    inline PRInt32 read() {
      if (current < end) {
        return *(current++);
      } else {
        return -1;
      }
    }
  private:
    const PRUint8* current;
    const PRUint8* end;
};

#endif
