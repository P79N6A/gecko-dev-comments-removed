#ifndef TYPEDESC_H
#define TYPEDESC_H

class TypeDesc
{
public:
    enum Type
    {
        T_POINTER, 
        T_ARRAY,   
        T_VOID,
        T_RESULT, 
        T_CHAR,
        T_WCHAR,
        T_INT8,
        T_INT16,
        T_INT32,
        T_INT64,
        T_UINT8,
        T_UINT16,
        T_UINT32,
        T_UINT64,
        T_STRING,
        T_WSTRING,
        T_FLOAT,
        T_DOUBLE,
        T_BOOL,
        T_INTERFACE,
        T_OTHER,
        T_UNSUPPORTED
    };

    Type      mType;
    union {
        
        TypeDesc *mPtr;
        
        struct {
            long mNumElements;
            TypeDesc **mElements;
        } mArray;
        
        char *mName;
    } mData;

    TypeDesc(ITypeInfo* pti, TYPEDESC* ptdesc);
    ~TypeDesc();

	std::string ToXPIDLString();
	std::string ToCString();
};

#endif