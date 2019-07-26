















#ifndef ANDROID_STRING8_H
#define ANDROID_STRING8_H

#include <utils/Errors.h>



#include <utils/String16.h>

#include <stdint.h>
#include <string.h>
#include <sys/types.h>



extern "C" {

#if !defined(__cplusplus) || __cplusplus == 199711L 
typedef uint32_t char32_t;
#endif

size_t strlen32(const char32_t *);
size_t strnlen32(const char32_t *, size_t);















size_t utf8_length(const char *src);




size_t utf32_length(const char *src, size_t src_len);




size_t utf8_length_from_utf16(const char16_t *src, size_t src_len);




size_t utf8_length_from_utf32(const char32_t *src, size_t src_len);








int32_t utf32_at(const char *src, size_t src_len,
                 size_t index, size_t *next_index);








size_t utf8_to_utf32(const char* src, size_t src_len,
                     char32_t* dst, size_t dst_len);




































size_t utf32_to_utf8(const char32_t* src, size_t src_len,
                     char* dst, size_t dst_len);

size_t utf16_to_utf8(const char16_t* src, size_t src_len,
                     char* dst, size_t dst_len);

}



namespace android {

class TextOutput;



class String8
{
public:
                                String8();
                                String8(const String8& o);
    explicit                    String8(const char* o);
    explicit                    String8(const char* o, size_t numChars);
    
    explicit                    String8(const String16& o);
    explicit                    String8(const char16_t* o);
    explicit                    String8(const char16_t* o, size_t numChars);
    explicit                    String8(const char32_t* o);
    explicit                    String8(const char32_t* o, size_t numChars);
                                ~String8();
    
    inline  const char*         string() const;
    inline  size_t              size() const;
    inline  size_t              length() const;
    inline  size_t              bytes() const;
    
    inline  const SharedBuffer* sharedBuffer() const;
    
            void                setTo(const String8& other);
            status_t            setTo(const char* other);
            status_t            setTo(const char* other, size_t numChars);
            status_t            setTo(const char16_t* other, size_t numChars);
            status_t            setTo(const char32_t* other,
                                      size_t length);

            status_t            append(const String8& other);
            status_t            append(const char* other);
            status_t            append(const char* other, size_t numChars);

            
            
            size_t              getUtf32Length() const;
            int32_t             getUtf32At(size_t index,
                                           size_t *next_index) const;
            size_t              getUtf32(char32_t* dst, size_t dst_len) const;

    inline  String8&            operator=(const String8& other);
    inline  String8&            operator=(const char* other);
    
    inline  String8&            operator+=(const String8& other);
    inline  String8             operator+(const String8& other) const;
    
    inline  String8&            operator+=(const char* other);
    inline  String8             operator+(const char* other) const;

    inline  int                 compare(const String8& other) const;

    inline  bool                operator<(const String8& other) const;
    inline  bool                operator<=(const String8& other) const;
    inline  bool                operator==(const String8& other) const;
    inline  bool                operator!=(const String8& other) const;
    inline  bool                operator>=(const String8& other) const;
    inline  bool                operator>(const String8& other) const;
    
    inline  bool                operator<(const char* other) const;
    inline  bool                operator<=(const char* other) const;
    inline  bool                operator==(const char* other) const;
    inline  bool                operator!=(const char* other) const;
    inline  bool                operator>=(const char* other) const;
    inline  bool                operator>(const char* other) const;
    
    inline                      operator const char*() const;
    
            char*               lockBuffer(size_t size);
            void                unlockBuffer();
            status_t            unlockBuffer(size_t size);
            
            
            
            ssize_t             find(const char* other, size_t start = 0) const;

            void                toLower();
            void                toLower(size_t start, size_t numChars);
            void                toUpper();
            void                toUpper(size_t start, size_t numChars);

    



    




    void setPathName(const char* name);
    void setPathName(const char* name, size_t numChars);

    




    String8 getPathLeaf(void) const;

    







    String8 getPathDir(void) const;

    







    String8 walkPath(String8* outRemains = NULL) const;

    










    String8 getPathExtension(void) const;

    





    String8 getBasePath(void) const;

    







    String8& appendPath(const char* leaf);
    String8& appendPath(const String8& leaf)  { return appendPath(leaf.string()); }

    


    String8 appendPathCopy(const char* leaf) const
                                             { String8 p(*this); p.appendPath(leaf); return p; }
    String8 appendPathCopy(const String8& leaf) const { return appendPathCopy(leaf.string()); }

    






    String8& convertToResPath();

private:
            status_t            real_append(const char* other, size_t numChars);
            char*               find_extension(void) const;

            const char* mString;
};

TextOutput& operator<<(TextOutput& to, const String16& val);




inline int compare_type(const String8& lhs, const String8& rhs)
{
    return lhs.compare(rhs);
}

inline int strictly_order_type(const String8& lhs, const String8& rhs)
{
    return compare_type(lhs, rhs) < 0;
}

inline const char* String8::string() const
{
    return mString;
}

inline size_t String8::length() const
{
    return SharedBuffer::sizeFromData(mString)-1;
}

inline size_t String8::size() const
{
    return length();
}

inline size_t String8::bytes() const
{
    return SharedBuffer::sizeFromData(mString)-1;
}

inline const SharedBuffer* String8::sharedBuffer() const
{
    return SharedBuffer::bufferFromData(mString);
}

inline String8& String8::operator=(const String8& other)
{
    setTo(other);
    return *this;
}

inline String8& String8::operator=(const char* other)
{
    setTo(other);
    return *this;
}

inline String8& String8::operator+=(const String8& other)
{
    append(other);
    return *this;
}

inline String8 String8::operator+(const String8& other) const
{
    String8 tmp;
    tmp += other;
    return tmp;
}

inline String8& String8::operator+=(const char* other)
{
    append(other);
    return *this;
}

inline String8 String8::operator+(const char* other) const
{
    String8 tmp;
    tmp += other;
    return tmp;
}

inline int String8::compare(const String8& other) const
{
    return strcmp(mString, other.mString);
}

inline bool String8::operator<(const String8& other) const
{
    return strcmp(mString, other.mString) < 0;
}

inline bool String8::operator<=(const String8& other) const
{
    return strcmp(mString, other.mString) <= 0;
}

inline bool String8::operator==(const String8& other) const
{
    return strcmp(mString, other.mString) == 0;
}

inline bool String8::operator!=(const String8& other) const
{
    return strcmp(mString, other.mString) != 0;
}

inline bool String8::operator>=(const String8& other) const
{
    return strcmp(mString, other.mString) >= 0;
}

inline bool String8::operator>(const String8& other) const
{
    return strcmp(mString, other.mString) > 0;
}

inline bool String8::operator<(const char* other) const
{
    return strcmp(mString, other) < 0;
}

inline bool String8::operator<=(const char* other) const
{
    return strcmp(mString, other) <= 0;
}

inline bool String8::operator==(const char* other) const
{
    return strcmp(mString, other) == 0;
}

inline bool String8::operator!=(const char* other) const
{
    return strcmp(mString, other) != 0;
}

inline bool String8::operator>=(const char* other) const
{
    return strcmp(mString, other) >= 0;
}

inline bool String8::operator>(const char* other) const
{
    return strcmp(mString, other) > 0;
}

inline String8::operator const char*() const
{
    return mString;
}

}  



#endif 
