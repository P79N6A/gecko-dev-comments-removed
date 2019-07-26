







#include "SkImageEncoder.h"

template SkImageEncoder_EncodeReg* SkImageEncoder_EncodeReg::gHead;

SkImageEncoder* SkImageEncoder::Create(Type t) {
    SkImageEncoder* codec = NULL;
    const SkImageEncoder_EncodeReg* curr = SkImageEncoder_EncodeReg::Head();
    while (curr) {
        if ((codec = curr->factory()(t)) != NULL) {
            return codec;
        }
        curr = curr->next();
    }
    return NULL;
}
