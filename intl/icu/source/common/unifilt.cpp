









#include "unicode/unifilt.h"
#include "unicode/rep.h"
#include "unicode/utf16.h"

U_NAMESPACE_BEGIN
UOBJECT_DEFINE_ABSTRACT_RTTI_IMPLEMENTATION(UnicodeFilter)




UnicodeMatcher::~UnicodeMatcher() {}

UnicodeFilter::~UnicodeFilter() {}





UnicodeMatcher* UnicodeFilter::toMatcher() const {
  return const_cast<UnicodeFilter *>(this);
}

void UnicodeFilter::setData(const TransliterationRuleData*) {}






UMatchDegree UnicodeFilter::matches(const Replaceable& text,
                                    int32_t& offset,
                                    int32_t limit,
                                    UBool incremental) {
    UChar32 c;
    if (offset < limit &&
        contains(c = text.char32At(offset))) {
        offset += U16_LENGTH(c);
        return U_MATCH;
    }
    if (offset > limit &&
        contains(c = text.char32At(offset))) {
        
        
        
        --offset;
        if (offset >= 0) {
            offset -= U16_LENGTH(text.char32At(offset)) - 1;
        }
        return U_MATCH;
    }
    if (incremental && offset == limit) {
        return U_PARTIAL_MATCH;
    }
    return U_MISMATCH;
}

U_NAMESPACE_END


