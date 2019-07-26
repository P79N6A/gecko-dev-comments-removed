















#ifndef STRING_H_

#define STRING_H_

#include <utils/String8.h>

namespace android {

class string {
public:
    typedef size_t size_type;
    static size_type npos;

    string();
    string(const char *s);
    string(const char *s, size_t length);
    string(const string &from, size_type start, size_type length = npos);

    const char *c_str() const;
    size_type size() const;

    void clear();
    void erase(size_type from, size_type length);

    size_type find(char c) const;

    bool operator<(const string &other) const;
    bool operator==(const string &other) const;

    string &operator+=(char c);

private:
    String8 mString;
};

}  

#endif  
