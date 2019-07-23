






































#include "mozStorageRow.h"
#include "mozStorageResultSet.h"

namespace mozilla {
namespace storage {




ResultSet::ResultSet()
: mCurrentIndex(0)
{
}

ResultSet::~ResultSet()
{
  mData.Clear();
}

nsresult
ResultSet::add(mozIStorageRow *aRow)
{
  return mData.AppendObject(aRow) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}





NS_IMPL_THREADSAFE_ISUPPORTS1(
  ResultSet,
  mozIStorageResultSet
)




NS_IMETHODIMP
ResultSet::GetNextRow(mozIStorageRow **_row)
{
  NS_ENSURE_ARG_POINTER(_row);

  if (mCurrentIndex >= mData.Count()) {
    
    return NS_OK;
  }

  NS_ADDREF(*_row = mData.ObjectAt(mCurrentIndex++));
  return NS_OK;
}

} 
} 
