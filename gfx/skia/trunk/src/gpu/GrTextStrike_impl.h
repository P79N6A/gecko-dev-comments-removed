









#ifndef GrTextStrike_impl_DEFINED
#define GrTextStrike_impl_DEFINED

void GrFontCache::detachStrikeFromList(GrTextStrike* strike) {
    if (strike->fPrev) {
        SkASSERT(fHead != strike);
        strike->fPrev->fNext = strike->fNext;
    } else {
        SkASSERT(fHead == strike);
        fHead = strike->fNext;
    }

    if (strike->fNext) {
        SkASSERT(fTail != strike);
        strike->fNext->fPrev = strike->fPrev;
    } else {
        SkASSERT(fTail == strike);
        fTail = strike->fPrev;
    }
}

GrTextStrike* GrFontCache::getStrike(GrFontScaler* scaler, bool useDistanceField) {
    this->validate();

    GrTextStrike* strike = fCache.find(*(scaler->getKey()));
    if (NULL == strike) {
        strike = this->generateStrike(scaler);
    } else if (strike->fPrev) {
        
        
        this->detachStrikeFromList(strike);
        
        fHead->fPrev = strike;
        strike->fNext = fHead;
        strike->fPrev = NULL;
        fHead = strike;
    }
    strike->fUseDistanceField = useDistanceField;
    this->validate();
    return strike;
}



GrGlyph* GrTextStrike::getGlyph(GrGlyph::PackedID packed,
                                GrFontScaler* scaler) {
    GrGlyph* glyph = fCache.find(packed);
    if (NULL == glyph) {
        glyph = this->generateGlyph(packed, scaler);
    }
    return glyph;
}

#endif
