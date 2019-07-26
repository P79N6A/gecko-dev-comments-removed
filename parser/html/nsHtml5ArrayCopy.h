





















#ifndef nsHtml5ArrayCopy_h
#define nsHtml5ArrayCopy_h


class nsString;
class nsHtml5StackNode;
class nsHtml5AttributeName;



class nsHtml5ArrayCopy {
  public:

    static inline void
    arraycopy(char16_t* source, int32_t sourceOffset, char16_t* target, int32_t targetOffset, int32_t length)
    {
      memcpy(&(target[targetOffset]), &(source[sourceOffset]), length * sizeof(char16_t));
    }

    static inline void
    arraycopy(char16_t* source, char16_t* target, int32_t length)
    {
      memcpy(target, source, length * sizeof(char16_t));
    }

    static inline void
    arraycopy(int32_t* source, int32_t* target, int32_t length)
    {
      memcpy(target, source, length * sizeof(int32_t));
    }

    static inline void
    arraycopy(nsString** source, nsString** target, int32_t length)
    {
      memcpy(target, source, length * sizeof(nsString*));
    }

    static inline void
    arraycopy(nsHtml5AttributeName** source, nsHtml5AttributeName** target, int32_t length)
    {
      memcpy(target, source, length * sizeof(nsHtml5AttributeName*));
    }

    static inline void
    arraycopy(nsHtml5StackNode** source, nsHtml5StackNode** target, int32_t length)
    {
      memcpy(target, source, length * sizeof(nsHtml5StackNode*));
    }

    static inline void
    arraycopy(nsHtml5StackNode** arr, int32_t sourceOffset, int32_t targetOffset, int32_t length)
    {
      memmove(&(arr[targetOffset]), &(arr[sourceOffset]), length * sizeof(nsHtml5StackNode*));
    }
};
#endif 
