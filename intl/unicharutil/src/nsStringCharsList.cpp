




































#include "nsStringCharsList.h"

nsStringCharsList::nsStringCharsList(nsString&  aList)
  : mList(aList)
{
}

nsStringCharsList::~nsStringCharsList()
{
}

PRUnichar nsStringCharsList::Get( PRUint32 aIdx) 
{
  return (PRUnichar)mList(aIdx);
}

PRUint32 nsStringCharsList::Length() 
{
  return mList.Length();
}
