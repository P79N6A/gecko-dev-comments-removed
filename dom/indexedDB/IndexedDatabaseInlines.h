





#ifndef mozilla_dom_indexeddb_indexeddatabase_h__
#error Must include IndexedDatabase.h first
#endif

BEGIN_INDEXEDDB_NAMESPACE

inline
StructuredCloneWriteInfo::StructuredCloneWriteInfo()
: mTransaction(nullptr),
  mOffsetToKeyProp(0)
{
}

inline
bool
StructuredCloneWriteInfo::SetFromSerialized(
                               const SerializedStructuredCloneWriteInfo& aOther)
{
  if (!aOther.dataLength) {
    mCloneBuffer.clear();
  }
  else if (!mCloneBuffer.copy(aOther.data, aOther.dataLength)) {
    return false;
  }

  mFiles.Clear();
  mOffsetToKeyProp = aOther.offsetToKeyProp;
  return true;
}

inline
StructuredCloneReadInfo::StructuredCloneReadInfo()
: mDatabase(nullptr)
{
}

inline
bool
StructuredCloneReadInfo::SetFromSerialized(
                                const SerializedStructuredCloneReadInfo& aOther)
{
  if (aOther.dataLength &&
      !mCloneBuffer.copy(aOther.data, aOther.dataLength)) {
    return false;
  }

  mFiles.Clear();
  return true;
}

inline
void
AppendConditionClause(const nsACString& aColumnName,
                      const nsACString& aArgName,
                      bool aLessThan,
                      bool aEquals,
                      nsACString& aResult)
{
  aResult += NS_LITERAL_CSTRING(" AND ") + aColumnName +
             NS_LITERAL_CSTRING(" ");

  if (aLessThan) {
    aResult.AppendLiteral("<");
  }
  else {
    aResult.AppendLiteral(">");
  }

  if (aEquals) {
    aResult.AppendLiteral("=");
  }

  aResult += NS_LITERAL_CSTRING(" :") + aArgName;
}

END_INDEXEDDB_NAMESPACE
