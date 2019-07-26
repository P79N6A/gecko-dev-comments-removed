















#ifndef ANDROID_STRING16_H
#define ANDROID_STRING16_H

#include <utils/Errors.h>
#include <utils/SharedBuffer.h>
#include <utils/Unicode.h>
#include <utils/TypeHelpers.h>



extern "C" {

}



namespace stagefright {



class String8;
class TextOutput;


class String16
{
public:
    





    enum StaticLinkage { kEmptyString };

                                String16();
    explicit                    String16(StaticLinkage);
                                String16(const String16& o);
                                String16(const String16& o,
                                         size_t len,
                                         size_t begin=0);
    explicit                    String16(const char16_t* o);
    explicit                    String16(const char16_t* o, size_t len);
    explicit                    String16(const String8& o);
    explicit                    String16(const char* o);
    explicit                    String16(const char* o, size_t len);

                                ~String16();
    
    inline  const char16_t*     string() const;
    inline  size_t              size() const;
    
    inline  const SharedBuffer* sharedBuffer() const;
    
            void                setTo(const String16& other);
            status_t            setTo(const char16_t* other);
            status_t            setTo(const char16_t* other, size_t len);
            status_t            setTo(const String16& other,
                                      size_t len,
                                      size_t begin=0);
    
            status_t            append(const String16& other);
            status_t            append(const char16_t* other, size_t len);
            
    inline  String16&           operator=(const String16& other);
    
    inline  String16&           operator+=(const String16& other);
    inline  String16            operator+(const String16& other) const;

            status_t            insert(size_t pos, const char16_t* chrs);
            status_t            insert(size_t pos,
                                       const char16_t* chrs, size_t len);

            ssize_t             findFirst(char16_t c) const;
            ssize_t             findLast(char16_t c) const;

            bool                startsWith(const String16& prefix) const;
            bool                startsWith(const char16_t* prefix) const;
            
            status_t            makeLower();

            status_t            replaceAll(char16_t replaceThis,
                                           char16_t withThis);

            status_t            remove(size_t len, size_t begin=0);

    inline  int                 compare(const String16& other) const;

    inline  bool                operator<(const String16& other) const;
    inline  bool                operator<=(const String16& other) const;
    inline  bool                operator==(const String16& other) const;
    inline  bool                operator!=(const String16& other) const;
    inline  bool                operator>=(const String16& other) const;
    inline  bool                operator>(const String16& other) const;
    
    inline  bool                operator<(const char16_t* other) const;
    inline  bool                operator<=(const char16_t* other) const;
    inline  bool                operator==(const char16_t* other) const;
    inline  bool                operator!=(const char16_t* other) const;
    inline  bool                operator>=(const char16_t* other) const;
    inline  bool                operator>(const char16_t* other) const;
    
    inline                      operator const char16_t*() const;
    
private:
            const char16_t*     mString;
};



ANDROID_TRIVIAL_MOVE_TRAIT(String16)




inline int compare_type(const String16& lhs, const String16& rhs)
{
    return lhs.compare(rhs);
}

inline int strictly_order_type(const String16& lhs, const String16& rhs)
{
    return compare_type(lhs, rhs) < 0;
}

inline const char16_t* String16::string() const
{
    return mString;
}

inline size_t String16::size() const
{
    return SharedBuffer::sizeFromData(mString)/sizeof(char16_t)-1;
}

inline const SharedBuffer* String16::sharedBuffer() const
{
    return SharedBuffer::bufferFromData(mString);
}

inline String16& String16::operator=(const String16& other)
{
    setTo(other);
    return *this;
}

inline String16& String16::operator+=(const String16& other)
{
    append(other);
    return *this;
}

inline String16 String16::operator+(const String16& other) const
{
    String16 tmp(*this);
    tmp += other;
    return tmp;
}

inline int String16::compare(const String16& other) const
{
    return strzcmp16(mString, size(), other.mString, other.size());
}

inline bool String16::operator<(const String16& other) const
{
    return strzcmp16(mString, size(), other.mString, other.size()) < 0;
}

inline bool String16::operator<=(const String16& other) const
{
    return strzcmp16(mString, size(), other.mString, other.size()) <= 0;
}

inline bool String16::operator==(const String16& other) const
{
    return strzcmp16(mString, size(), other.mString, other.size()) == 0;
}

inline bool String16::operator!=(const String16& other) const
{
    return strzcmp16(mString, size(), other.mString, other.size()) != 0;
}

inline bool String16::operator>=(const String16& other) const
{
    return strzcmp16(mString, size(), other.mString, other.size()) >= 0;
}

inline bool String16::operator>(const String16& other) const
{
    return strzcmp16(mString, size(), other.mString, other.size()) > 0;
}

inline bool String16::operator<(const char16_t* other) const
{
    return strcmp16(mString, other) < 0;
}

inline bool String16::operator<=(const char16_t* other) const
{
    return strcmp16(mString, other) <= 0;
}

inline bool String16::operator==(const char16_t* other) const
{
    return strcmp16(mString, other) == 0;
}

inline bool String16::operator!=(const char16_t* other) const
{
    return strcmp16(mString, other) != 0;
}

inline bool String16::operator>=(const char16_t* other) const
{
    return strcmp16(mString, other) >= 0;
}

inline bool String16::operator>(const char16_t* other) const
{
    return strcmp16(mString, other) > 0;
}

inline String16::operator const char16_t*() const
{
    return mString;
}

}; 



#endif 
