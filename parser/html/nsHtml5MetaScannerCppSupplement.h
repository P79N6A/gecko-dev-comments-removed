


 
#include "nsEncoderDecoderUtils.h"
#include "nsTraceRefcnt.h"

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
    mCharset.Assign("UTF-8");
    return true;
  }
  mCharset.Assign(encoding);
  return true;
}
