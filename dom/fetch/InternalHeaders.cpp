





#include "mozilla/dom/InternalHeaders.h"

#include "mozilla/ErrorResult.h"
#include "mozilla/dom/PHeaders.h"

#include "nsCharSeparatedTokenizer.h"
#include "nsContentUtils.h"
#include "nsNetUtil.h"
#include "nsReadableUtils.h"

namespace mozilla {
namespace dom {

InternalHeaders::InternalHeaders(const nsTArray<PHeadersEntry>& aHeaders,
                                 HeadersGuardEnum aGuard)
  : mGuard(aGuard)
{
  for (uint32_t i = 0; i < aHeaders.Length(); ++i) {
    mList.AppendElement(Entry(aHeaders[i].name(), aHeaders[i].value()));
  }
}

void
InternalHeaders::GetPHeaders(nsTArray<PHeadersEntry>& aPHeadersOut) const
{
  for (uint32_t i = 0; i < mList.Length(); ++i) {
    aPHeadersOut.AppendElement(PHeadersEntry(mList[i].mName, mList[i].mValue));
  }
}

void
InternalHeaders::Append(const nsACString& aName, const nsACString& aValue,
                        ErrorResult& aRv)
{
  nsAutoCString lowerName;
  ToLowerCase(aName, lowerName);

  if (IsInvalidMutableHeader(lowerName, aValue, aRv)) {
    return;
  }

  mList.AppendElement(Entry(lowerName, aValue));
}

void
InternalHeaders::Delete(const nsACString& aName, ErrorResult& aRv)
{
  nsAutoCString lowerName;
  ToLowerCase(aName, lowerName);

  if (IsInvalidMutableHeader(lowerName, aRv)) {
    return;
  }

  
  for (int32_t i = mList.Length() - 1; i >= 0; --i) {
    if (lowerName == mList[i].mName) {
      mList.RemoveElementAt(i);
    }
  }
}

void
InternalHeaders::Get(const nsACString& aName, nsCString& aValue, ErrorResult& aRv) const
{
  nsAutoCString lowerName;
  ToLowerCase(aName, lowerName);

  if (IsInvalidName(lowerName, aRv)) {
    return;
  }

  for (uint32_t i = 0; i < mList.Length(); ++i) {
    if (lowerName == mList[i].mName) {
      aValue = mList[i].mValue;
      return;
    }
  }

  
  aValue.SetIsVoid(true);
}

void
InternalHeaders::GetAll(const nsACString& aName, nsTArray<nsCString>& aResults,
                        ErrorResult& aRv) const
{
  nsAutoCString lowerName;
  ToLowerCase(aName, lowerName);

  if (IsInvalidName(lowerName, aRv)) {
    return;
  }

  aResults.SetLength(0);
  for (uint32_t i = 0; i < mList.Length(); ++i) {
    if (lowerName == mList[i].mName) {
      aResults.AppendElement(mList[i].mValue);
    }
  }
}

bool
InternalHeaders::Has(const nsACString& aName, ErrorResult& aRv) const
{
  nsAutoCString lowerName;
  ToLowerCase(aName, lowerName);

  if (IsInvalidName(lowerName, aRv)) {
    return false;
  }

  for (uint32_t i = 0; i < mList.Length(); ++i) {
    if (lowerName == mList[i].mName) {
      return true;
    }
  }
  return false;
}

void
InternalHeaders::Set(const nsACString& aName, const nsACString& aValue, ErrorResult& aRv)
{
  nsAutoCString lowerName;
  ToLowerCase(aName, lowerName);

  if (IsInvalidMutableHeader(lowerName, aValue, aRv)) {
    return;
  }

  int32_t firstIndex = INT32_MAX;

  
  for (int32_t i = mList.Length() - 1; i >= 0; --i) {
    if (lowerName == mList[i].mName) {
      firstIndex = std::min(firstIndex, i);
      mList.RemoveElementAt(i);
    }
  }

  if (firstIndex < INT32_MAX) {
    Entry* entry = mList.InsertElementAt(firstIndex);
    entry->mName = lowerName;
    entry->mValue = aValue;
  } else {
    mList.AppendElement(Entry(lowerName, aValue));
  }
}

void
InternalHeaders::Clear()
{
  mList.Clear();
}

void
InternalHeaders::SetGuard(HeadersGuardEnum aGuard, ErrorResult& aRv)
{
  
  
  
  
  if (aGuard != HeadersGuardEnum::Immutable && mList.Length() > 0) {
    aRv.Throw(NS_ERROR_FAILURE);
  }
  mGuard = aGuard;
}

InternalHeaders::~InternalHeaders()
{
}


bool
InternalHeaders::IsSimpleHeader(const nsACString& aName, const nsACString& aValue)
{
  
  
  
  return aName.EqualsLiteral("accept") ||
         aName.EqualsLiteral("accept-language") ||
         aName.EqualsLiteral("content-language") ||
         (aName.EqualsLiteral("content-type") &&
          nsContentUtils::IsAllowedNonCorsContentType(aValue));
}


bool
InternalHeaders::IsInvalidName(const nsACString& aName, ErrorResult& aRv)
{
  if (!NS_IsValidHTTPToken(aName)) {
    NS_ConvertUTF8toUTF16 label(aName);
    aRv.ThrowTypeError(MSG_INVALID_HEADER_NAME, &label);
    return true;
  }

  return false;
}


bool
InternalHeaders::IsInvalidValue(const nsACString& aValue, ErrorResult& aRv)
{
  if (!NS_IsReasonableHTTPHeaderValue(aValue)) {
    NS_ConvertUTF8toUTF16 label(aValue);
    aRv.ThrowTypeError(MSG_INVALID_HEADER_VALUE, &label);
    return true;
  }
  return false;
}

bool
InternalHeaders::IsImmutable(ErrorResult& aRv) const
{
  if (mGuard == HeadersGuardEnum::Immutable) {
    aRv.ThrowTypeError(MSG_HEADERS_IMMUTABLE);
    return true;
  }
  return false;
}

bool
InternalHeaders::IsForbiddenRequestHeader(const nsACString& aName) const
{
  return mGuard == HeadersGuardEnum::Request &&
         nsContentUtils::IsForbiddenRequestHeader(aName);
}

bool
InternalHeaders::IsForbiddenRequestNoCorsHeader(const nsACString& aName) const
{
  return mGuard == HeadersGuardEnum::Request_no_cors &&
         !IsSimpleHeader(aName, EmptyCString());
}

bool
InternalHeaders::IsForbiddenRequestNoCorsHeader(const nsACString& aName,
                                                const nsACString& aValue) const
{
  return mGuard == HeadersGuardEnum::Request_no_cors &&
         !IsSimpleHeader(aName, aValue);
}

bool
InternalHeaders::IsForbiddenResponseHeader(const nsACString& aName) const
{
  return mGuard == HeadersGuardEnum::Response &&
         nsContentUtils::IsForbiddenResponseHeader(aName);
}

void
InternalHeaders::Fill(const InternalHeaders& aInit, ErrorResult& aRv)
{
  const nsTArray<Entry>& list = aInit.mList;
  for (uint32_t i = 0; i < list.Length() && !aRv.Failed(); ++i) {
    const Entry& entry = list[i];
    Append(entry.mName, entry.mValue, aRv);
  }
}

void
InternalHeaders::Fill(const Sequence<Sequence<nsCString>>& aInit, ErrorResult& aRv)
{
  for (uint32_t i = 0; i < aInit.Length() && !aRv.Failed(); ++i) {
    const Sequence<nsCString>& tuple = aInit[i];
    if (tuple.Length() != 2) {
      aRv.ThrowTypeError(MSG_INVALID_HEADER_SEQUENCE);
      return;
    }
    Append(tuple[0], tuple[1], aRv);
  }
}

void
InternalHeaders::Fill(const MozMap<nsCString>& aInit, ErrorResult& aRv)
{
  nsTArray<nsString> keys;
  aInit.GetKeys(keys);
  for (uint32_t i = 0; i < keys.Length() && !aRv.Failed(); ++i) {
    Append(NS_ConvertUTF16toUTF8(keys[i]), aInit.Get(keys[i]), aRv);
  }
}

bool
InternalHeaders::HasOnlySimpleHeaders() const
{
  for (uint32_t i = 0; i < mList.Length(); ++i) {
    if (!IsSimpleHeader(mList[i].mName, mList[i].mValue)) {
      return false;
    }
  }

  return true;
}


already_AddRefed<InternalHeaders>
InternalHeaders::BasicHeaders(InternalHeaders* aHeaders)
{
  nsRefPtr<InternalHeaders> basic = new InternalHeaders(*aHeaders);
  ErrorResult result;
  
  
  basic->Delete(NS_LITERAL_CSTRING("Set-Cookie"), result);
  MOZ_ASSERT(!result.Failed());
  basic->Delete(NS_LITERAL_CSTRING("Set-Cookie2"), result);
  MOZ_ASSERT(!result.Failed());
  return basic.forget();
}


already_AddRefed<InternalHeaders>
InternalHeaders::CORSHeaders(InternalHeaders* aHeaders)
{
  nsRefPtr<InternalHeaders> cors = new InternalHeaders(aHeaders->mGuard);
  ErrorResult result;

  nsAutoCString acExposedNames;
  aHeaders->Get(NS_LITERAL_CSTRING("Access-Control-Expose-Headers"), acExposedNames, result);
  MOZ_ASSERT(!result.Failed());

  nsAutoTArray<nsCString, 5> exposeNamesArray;
  nsCCharSeparatedTokenizer exposeTokens(acExposedNames, ',');
  while (exposeTokens.hasMoreTokens()) {
    const nsDependentCSubstring& token = exposeTokens.nextToken();
    if (token.IsEmpty()) {
      continue;
    }

    if (!NS_IsValidHTTPToken(token)) {
      NS_WARNING("Got invalid HTTP token in Access-Control-Expose-Headers. Header value is:");
      NS_WARNING(acExposedNames.get());
      exposeNamesArray.Clear();
      break;
    }

    exposeNamesArray.AppendElement(token);
  }

  nsCaseInsensitiveCStringArrayComparator comp;
  for (uint32_t i = 0; i < aHeaders->mList.Length(); ++i) {
    const Entry& entry = aHeaders->mList[i];
    if (entry.mName.EqualsASCII("cache-control") ||
        entry.mName.EqualsASCII("content-language") ||
        entry.mName.EqualsASCII("content-type") ||
        entry.mName.EqualsASCII("expires") ||
        entry.mName.EqualsASCII("last-modified") ||
        entry.mName.EqualsASCII("pragma") ||
        exposeNamesArray.Contains(entry.mName, comp)) {
      cors->Append(entry.mName, entry.mValue, result);
      MOZ_ASSERT(!result.Failed());
    }
  }

  return cors.forget();
}

void
InternalHeaders::GetEntries(nsTArray<InternalHeaders::Entry>& aEntries) const
{
  MOZ_ASSERT(aEntries.IsEmpty());
  aEntries.AppendElements(mList);
}

void
InternalHeaders::GetUnsafeHeaders(nsTArray<nsCString>& aNames) const
{
  MOZ_ASSERT(aNames.IsEmpty());
  for (uint32_t i = 0; i < mList.Length(); ++i) {
    const Entry& header = mList[i];
    if (!InternalHeaders::IsSimpleHeader(header.mName, header.mValue)) {
      aNames.AppendElement(header.mName);
    }
  }
}
} 
} 
