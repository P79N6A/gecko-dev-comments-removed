




































#ifndef NSFILEPICKER_H
#define NSFILEPICKER_H

#include "nsIFilePicker.h"
#include "nsString.h"

class nsFilePicker : public nsIFilePicker
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFILEPICKER

private:
  nsString mFilePath;
};
#endif
