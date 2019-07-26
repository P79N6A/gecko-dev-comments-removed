














#include "utypeinfo.h"  

#include "unicode/chariter.h"
#include "unicode/schriter.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(StringCharacterIterator)

StringCharacterIterator::StringCharacterIterator()
  : UCharCharacterIterator(),
    text()
{
  
}

StringCharacterIterator::StringCharacterIterator(const UnicodeString& textStr)
  : UCharCharacterIterator(textStr.getBuffer(), textStr.length()),
    text(textStr)
{
    
    UCharCharacterIterator::text = this->text.getBuffer();
}

StringCharacterIterator::StringCharacterIterator(const UnicodeString& textStr,
                                                 int32_t textPos)
  : UCharCharacterIterator(textStr.getBuffer(), textStr.length(), textPos),
    text(textStr)
{
    
    UCharCharacterIterator::text = this->text.getBuffer();
}

StringCharacterIterator::StringCharacterIterator(const UnicodeString& textStr,
                                                 int32_t textBegin,
                                                 int32_t textEnd,
                                                 int32_t textPos)
  : UCharCharacterIterator(textStr.getBuffer(), textStr.length(), textBegin, textEnd, textPos),
    text(textStr)
{
    
    UCharCharacterIterator::text = this->text.getBuffer();
}

StringCharacterIterator::StringCharacterIterator(const StringCharacterIterator& that)
  : UCharCharacterIterator(that),
    text(that.text)
{
    
    UCharCharacterIterator::text = this->text.getBuffer();
}

StringCharacterIterator::~StringCharacterIterator() {
}

StringCharacterIterator&
StringCharacterIterator::operator=(const StringCharacterIterator& that) {
    UCharCharacterIterator::operator=(that);
    text = that.text;
    
    UCharCharacterIterator::text = this->text.getBuffer();
    return *this;
}

UBool
StringCharacterIterator::operator==(const ForwardCharacterIterator& that) const {
    if (this == &that) {
        return TRUE;
    }

    
    
    

    if (typeid(*this) != typeid(that)) {
        return FALSE;
    }

    StringCharacterIterator&    realThat = (StringCharacterIterator&)that;

    return text == realThat.text
        && pos == realThat.pos
        && begin == realThat.begin
        && end == realThat.end;
}

CharacterIterator*
StringCharacterIterator::clone() const {
    return new StringCharacterIterator(*this);
}

void
StringCharacterIterator::setText(const UnicodeString& newText) {
    text = newText;
    UCharCharacterIterator::setText(text.getBuffer(), text.length());
}

void
StringCharacterIterator::getText(UnicodeString& result) {
    result = text;
}
U_NAMESPACE_END
