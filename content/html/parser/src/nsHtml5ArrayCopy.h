




































#ifndef nsHtml5ArrayCopy_h__
#define nsHtml5ArrayCopy_h__

#include "prtypes.h"

class nsString;
class nsHtml5StackNode;
class nsHtml5AttributeName;




class nsHtml5ArrayCopy {
  public: 
    static
    inline
    void
    arraycopy(PRUnichar* source, PRInt32 sourceOffset, PRUnichar* target, PRInt32 targetOffset, PRInt32 length)
    {
      memcpy(&(target[targetOffset]), &(source[sourceOffset]), length * sizeof(PRUnichar));
    }
    
    static
    inline
    void
    arraycopy(PRUnichar* source, PRUnichar* target, PRInt32 length)
    {
      memcpy(target, source, length * sizeof(PRUnichar));
    }
    
    static
    inline
    void
    arraycopy(nsString** source, nsString** target, PRInt32 length)
    {
      memcpy(target, source, length * sizeof(nsString*));
    }
    
    static
    inline
    void
    arraycopy(nsHtml5AttributeName** source, nsHtml5AttributeName** target, PRInt32 length)
    {
      memcpy(target, source, length * sizeof(nsHtml5AttributeName*));
    }
    
    static
    inline
    void
    arraycopy(nsHtml5StackNode** source, nsHtml5StackNode** target, PRInt32 length)
    {
      memcpy(target, source, length * sizeof(nsHtml5StackNode*));
    }
    
    static
    inline
    void
    arraycopy(nsHtml5StackNode** arr, PRInt32 sourceOffset, PRInt32 targetOffset, PRInt32 length)
    {
      memmove(&(arr[targetOffset]), &(arr[sourceOffset]), length * sizeof(nsHtml5StackNode*));
    }
  
};

#endif 
