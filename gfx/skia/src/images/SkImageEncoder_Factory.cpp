








#include "SkImageEncoder.h"
#include "SkTRegistry.h"

typedef SkTRegistry<SkImageEncoder*, SkImageEncoder::Type> EncodeReg;


template EncodeReg* SkTRegistry<SkImageEncoder*, SkImageEncoder::Type>::gHead;

SkImageEncoder* SkImageEncoder::Create(Type t) {
    SkImageEncoder* codec = NULL;
    const EncodeReg* curr = EncodeReg::Head();
    while (curr) {
        if ((codec = curr->factory()(t)) != NULL) {
            return codec;
        }
        curr = curr->next();
    }
    return NULL;
}
