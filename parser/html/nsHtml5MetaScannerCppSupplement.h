


 
#include "nsEncoderDecoderUtils.h"
#include "nsISupportsImpl.h"

#include "mozilla/dom/EncodingUtils.h"

using mozilla::dom::EncodingUtils;

void
nsHtml5MetaScanner::sniff(nsHtml5ByteReadable* bytes, nsACString& charset)
{
  readable = bytes;
  stateLoop(stateSave);
  readable = nullptr;
  charset.Assign(mCharset);
}

bool
nsHtml5MetaScanner::tryCharset(nsString* charset)
{
  
  
  
  nsAutoCString label;
  CopyUTF16toUTF8(*charset, label);
  nsAutoCString encoding;
  if (!EncodingUtils::FindEncodingForLabel(label, encoding)) {
    return false;
  }
  if (encoding.EqualsLiteral("UTF-16BE") ||
      encoding.EqualsLiteral("UTF-16LE")) {
    mCharset.AssignLiteral("UTF-8");
    return true;
  }
  if (encoding.EqualsLiteral("x-user-defined")) {
    
    mCharset.AssignLiteral("windows-1252");
    return true;
  }
  mCharset.Assign(encoding);
  return true;
}
