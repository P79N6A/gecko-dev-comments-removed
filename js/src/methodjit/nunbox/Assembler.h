






































#if !defined jsjaeger_assembler_h__ && defined JS_METHODJIT
#define jsjaeger_assembler_h__

#include "methodjit/BaseAssembler.h"

namespace js {
namespace mjit {

class Assembler : public BaseAssembler
{
    static const uint32 PAYLOAD_OFFSET = 0;
    static const uint32 TAG_OFFSET     = 4;

  public:
    Address payloadOf(Address address) {
        return address;
    }

    void loadTypeTag(Address address, RegisterID reg) {
        load32(Address(address.base, address.offset + TAG_OFFSET), reg);
    }

    void storeTypeTag(Imm32 imm, Address address) {
        store32(imm, Address(address.base, address.offset + TAG_OFFSET));
    }

    void storeTypeTag(RegisterID reg, Address address) {
        store32(reg, Address(address.base, address.offset + TAG_OFFSET));
    }

    void loadData32(Address address, RegisterID reg) {
        load32(Address(address.base, address.offset + PAYLOAD_OFFSET), reg);
    }

    void storeData32(Imm32 imm, Address address) {
        store32(imm, Address(address.base, address.offset + PAYLOAD_OFFSET));
    }

    void storeData32(RegisterID reg, Address address) {
        store32(reg, Address(address.base, address.offset + PAYLOAD_OFFSET));
    }
};

} 
} 

#endif

