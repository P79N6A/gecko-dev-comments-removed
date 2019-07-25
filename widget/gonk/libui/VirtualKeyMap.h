















#ifndef _UI_VIRTUAL_KEY_MAP_H
#define _UI_VIRTUAL_KEY_MAP_H

#include <stdint.h>

#include "Input.h"
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <utils/Tokenizer.h>
#include <utils/String8.h>
#include <utils/Unicode.h>

namespace android {


struct VirtualKeyDefinition {
    int32_t scanCode;

    
    int32_t centerX;
    int32_t centerY;
    int32_t width;
    int32_t height;
};






class VirtualKeyMap {
public:
    ~VirtualKeyMap();

    static status_t load(const String8& filename, VirtualKeyMap** outMap);

    inline const Vector<VirtualKeyDefinition>& getVirtualKeys() const {
        return mVirtualKeys;
    }

private:
    class Parser {
        VirtualKeyMap* mMap;
        Tokenizer* mTokenizer;

    public:
        Parser(VirtualKeyMap* map, Tokenizer* tokenizer);
        ~Parser();
        status_t parse();

    private:
        bool consumeFieldDelimiterAndSkipWhitespace();
        bool parseNextIntField(int32_t* outValue);
    };

    Vector<VirtualKeyDefinition> mVirtualKeys;

    VirtualKeyMap();
};

} 

#endif 
