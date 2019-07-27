








#ifndef _COMPILER_INTERFACE_VARIABLES_
#define _COMPILER_INTERFACE_VARIABLES_

#include <string>
#include <vector>
#include <algorithm>




namespace sh
{


enum InterpolationType
{
    INTERPOLATION_SMOOTH,
    INTERPOLATION_CENTROID,
    INTERPOLATION_FLAT
};


enum BlockLayoutType
{
    BLOCKLAYOUT_STANDARD,
    BLOCKLAYOUT_PACKED,
    BLOCKLAYOUT_SHARED
};





struct COMPILER_EXPORT ShaderVariable
{
    ShaderVariable();
    ShaderVariable(GLenum typeIn, unsigned int arraySizeIn);
    ~ShaderVariable();
    ShaderVariable(const ShaderVariable &other);
    ShaderVariable &operator=(const ShaderVariable &other);

    bool isArray() const { return arraySize > 0; }
    unsigned int elementCount() const { return std::max(1u, arraySize); }
    bool isStruct() const { return !fields.empty(); }

    
    
    
    
    
    
    
    
    
    
    
    bool findInfoByMappedName(const std::string &mappedFullName,
                              const ShaderVariable **leafVar,
                              std::string* originalFullName) const;

    GLenum type;
    GLenum precision;
    std::string name;
    std::string mappedName;
    unsigned int arraySize;
    bool staticUse;
    std::vector<ShaderVariable> fields;
    std::string structName;

  protected:
    bool isSameVariableAtLinkTime(const ShaderVariable &other,
                                  bool matchPrecision) const;

    bool operator==(const ShaderVariable &other) const;
    bool operator!=(const ShaderVariable &other) const
    {
        return !operator==(other);
    }
};

struct COMPILER_EXPORT Uniform : public ShaderVariable
{
    Uniform();
    ~Uniform();
    Uniform(const Uniform &other);
    Uniform &operator=(const Uniform &other);
    bool operator==(const Uniform &other) const;
    bool operator!=(const Uniform &other) const
    {
        return !operator==(other);
    }

    
    
    
    bool isSameUniformAtLinkTime(const Uniform &other) const;
};

struct COMPILER_EXPORT Attribute : public ShaderVariable
{
    Attribute();
    ~Attribute();
    Attribute(const Attribute &other);
    Attribute &operator=(const Attribute &other);
    bool operator==(const Attribute &other) const;
    bool operator!=(const Attribute &other) const
    {
        return !operator==(other);
    }

    int location;
};

struct COMPILER_EXPORT InterfaceBlockField : public ShaderVariable
{
    InterfaceBlockField();
    ~InterfaceBlockField();
    InterfaceBlockField(const InterfaceBlockField &other);
    InterfaceBlockField &operator=(const InterfaceBlockField &other);
    bool operator==(const InterfaceBlockField &other) const;
    bool operator!=(const InterfaceBlockField &other) const
    {
        return !operator==(other);
    }

    
    
    
    
    bool isSameInterfaceBlockFieldAtLinkTime(
        const InterfaceBlockField &other) const;

    bool isRowMajorLayout;
};

struct COMPILER_EXPORT Varying : public ShaderVariable
{
    Varying();
    ~Varying();
    Varying(const Varying &otherg);
    Varying &operator=(const Varying &other);
    bool operator==(const Varying &other) const;
    bool operator!=(const Varying &other) const
    {
        return !operator==(other);
    }

    
    
    
    bool isSameVaryingAtLinkTime(const Varying &other) const;

    InterpolationType interpolation;
    bool isInvariant;
};

struct COMPILER_EXPORT InterfaceBlock
{
    InterfaceBlock();
    ~InterfaceBlock();
    InterfaceBlock(const InterfaceBlock &other);
    InterfaceBlock &operator=(const InterfaceBlock &other);

    std::string name;
    std::string mappedName;
    std::string instanceName;
    unsigned int arraySize;
    BlockLayoutType layout;
    bool isRowMajorLayout;
    bool staticUse;
    std::vector<InterfaceBlockField> fields;
};

}

#endif 
