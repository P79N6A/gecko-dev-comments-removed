






































#include "mozStorageRow.h"
#include "mozStorageResultSet.h"








NS_IMPL_THREADSAFE_ISUPPORTS1(mozStorageResultSet, mozIStorageResultSet)

mozStorageResultSet::mozStorageResultSet() :
    mCurrentIndex(0)
{
}

mozStorageResultSet::~mozStorageResultSet()
{
  mData.Clear();
}

nsresult
mozStorageResultSet::add(mozIStorageRow *aRow)
{
  return mData.AppendObject(aRow) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}




NS_IMETHODIMP
mozStorageResultSet::GetNextRow(mozIStorageRow **_row)
{
  if (mCurrentIndex >= mData.Count()) {
    
    return NS_OK;
  }

  NS_ADDREF(*_row = mData.ObjectAt(mCurrentIndex++));
  return NS_OK;
}
