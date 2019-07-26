





#ifndef COMPILER_PREPROCESSOR_SOURCE_LOCATION_H_
#define COMPILER_PREPROCESSOR_SOURCE_LOCATION_H_

namespace pp
{

struct SourceLocation
{
    SourceLocation() : file(0), line(0) { }
    SourceLocation(int f, int l) : file(f), line(l) { }

    bool equals(const SourceLocation& other) const
    {
        return (file == other.file) && (line == other.line);
    }

    int file;
    int line;
};

inline bool operator==(const SourceLocation& lhs, const SourceLocation& rhs)
{
    return lhs.equals(rhs);
}

inline bool operator!=(const SourceLocation& lhs, const SourceLocation& rhs)
{
    return !lhs.equals(rhs);
}

}  
#endif  
