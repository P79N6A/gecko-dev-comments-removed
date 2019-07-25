




#include "TextLeafAccessible.h"

#include "nsAccUtils.h"
#include "DocAccessible.h"
#include "Role.h"

using namespace mozilla::a11y;





TextLeafAccessible::
  TextLeafAccessible(nsIContent* aContent, DocAccessible* aDoc) :
  nsLinkableAccessible(aContent, aDoc)
{
  mFlags |= eTextLeafAccessible;
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
TextLeafAccessible::AppendTextTo(nsAString& aText, PRUint32 aStartOffset,
                                 PRUint32 aLength)
{
  aText.Append(Substring(mText, aStartOffset, aLength));
}

ENameValueFlag
TextLeafAccessible::Name(nsString& aName)
{
  
  aName = mText;
  return eNameOK;
}

nsresult
TextLeafAccessible::GetAttributesInternal(nsIPersistentProperties *aAttributes)
{
  if (NativeRole() == roles::STATICTEXT) {
    nsAutoString oldValueUnused;
    aAttributes->SetStringProperty(NS_LITERAL_CSTRING("auto-generated"),
                                  NS_LITERAL_STRING("true"), oldValueUnused);
  }

  return NS_OK;
}

void
TextLeafAccessible::CacheChildren()
{
  
}
