


 
#ifndef nsHtml5ByteReadable_h__
#define nsHtml5ByteReadable_h__

#include "prtypes.h"




class nsHtml5ByteReadable
{
  public:

    nsHtml5ByteReadable(const uint8_t* current, const uint8_t* end)
     : current(current),
       end(end)
    {
    }

    inline int32_t read() {
      if (current < end) {
        return *(current++);
      } else {
        return -1;
      }
    }

  private:
    const uint8_t* current;
    const uint8_t* end;
};
#endif
