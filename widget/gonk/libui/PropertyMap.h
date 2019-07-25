















#ifndef _UTILS_PROPERTY_MAP_H
#define _UTILS_PROPERTY_MAP_H

#include <utils/KeyedVector.h>
#include "String8.h"
#include <utils/Errors.h>
#include "Tokenizer.h"

namespace android {






















class PropertyMap {
public:
    
    PropertyMap();
    ~PropertyMap();

    
    void clear();

    


    void addProperty(const String8& key, const String8& value);

    
    bool hasProperty(const String8& key) const;

    



    bool tryGetProperty(const String8& key, String8& outValue) const;
    bool tryGetProperty(const String8& key, bool& outValue) const;
    bool tryGetProperty(const String8& key, int32_t& outValue) const;
    bool tryGetProperty(const String8& key, float& outValue) const;

    
    void addAll(const PropertyMap* map);

    
    inline const KeyedVector<String8, String8>& getProperties() const { return mProperties; }

    
    static status_t load(const String8& filename, PropertyMap** outMap);

private:
    class Parser {
        PropertyMap* mMap;
        Tokenizer* mTokenizer;

    public:
        Parser(PropertyMap* map, Tokenizer* tokenizer);
        ~Parser();
        status_t parse();

    private:
        status_t parseType();
        status_t parseKey();
        status_t parseKeyProperty();
        status_t parseModifier(const String8& token, int32_t* outMetaState);
        status_t parseCharacterLiteral(char16_t* outCharacter);
    };

    KeyedVector<String8, String8> mProperties;
};

} 

#endif 
