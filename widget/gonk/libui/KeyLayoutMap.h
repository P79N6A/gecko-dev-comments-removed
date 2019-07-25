















#ifndef _UI_KEY_LAYOUT_MAP_H
#define _UI_KEY_LAYOUT_MAP_H

#include <stdint.h>
#include <utils/Errors.h>
#include <utils/KeyedVector.h>
#include <utils/Tokenizer.h>

namespace android {

struct AxisInfo {
    enum Mode {
        
        MODE_NORMAL = 0,
        
        MODE_INVERT = 1,
        
        MODE_SPLIT = 2,
    };

    
    Mode mode;

    
    
    int32_t axis;

    
    int32_t highAxis;

    
    int32_t splitValue;

    
    int32_t flatOverride;

    AxisInfo() : mode(MODE_NORMAL), axis(-1), highAxis(-1), splitValue(0), flatOverride(-1) {
    }
};




class KeyLayoutMap {
public:
    ~KeyLayoutMap();

    static status_t load(const String8& filename, KeyLayoutMap** outMap);

    status_t mapKey(int32_t scanCode, int32_t* keyCode, uint32_t* flags) const;
    status_t findScanCodesForKey(int32_t keyCode, Vector<int32_t>* outScanCodes) const;

    status_t mapAxis(int32_t scanCode, AxisInfo* outAxisInfo) const;

private:
    struct Key {
        int32_t keyCode;
        uint32_t flags;
    };

    KeyedVector<int32_t, Key> mKeys;
    KeyedVector<int32_t, AxisInfo> mAxes;

    KeyLayoutMap();

    class Parser {
        KeyLayoutMap* mMap;
        Tokenizer* mTokenizer;

    public:
        Parser(KeyLayoutMap* map, Tokenizer* tokenizer);
        ~Parser();
        status_t parse();

    private:
        status_t parseKey();
        status_t parseAxis();
    };
};

} 

#endif 
