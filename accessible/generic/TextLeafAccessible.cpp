




#include "TextLeafAccessible.h"

#include "nsAccUtils.h"
#include "DocAccessible.h"
#include "Role.h"

using namespace mozilla::a11y;





TextLeafAccessible::
  TextLeafAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  LinkableAccessible(aContent, aDoc)
{
  mType = eTextLeafType;
}

TextLeafAccessible::~TextLeafAccessible()
{
}

role
TextLeafAccessible::NativeRole()
{
  nsIFrame* frame = GetFrame();
  if (frame && frame->IsGeneratedContentFrame())
    return roles::STATICTEXT;

  return roles::TEXT_LEAF;
}

void
TextLeafAccessible::AppendTextTo(nsAString& aText, uint32_t aStartOffset,
                                 uint32_t aLength)
{
  aText.Append(Substring(mText, aStartOffset, aLength));
}

ENameValueFlag
TextLeafAccessible::Name(nsString& aName)
{
  
  aName = mText;
  return eNameOK;
}

void
TextLeafAccessible::CacheChildren()
{
  
}
