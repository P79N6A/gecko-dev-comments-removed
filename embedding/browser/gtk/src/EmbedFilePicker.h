






































#ifndef __EmbedFilePicker_h
#define __EmbedFilePicker_h

#include "nsIFilePicker.h"
#include "nsISupports.h"

#define EMBED_FILEPICKER_CID           \
{ /* f097d33b-1c97-48a6-af4c-07022857eb7c */         \
    0xf097d33b,                                      \
    0x1c97,                                          \
    0x48a6,                                          \
    {0xaf, 0x4c, 0x07, 0x02, 0x28, 0x57, 0xeb, 0x7c} \
}

#define EMBED_FILEPICKER_CONTRACTID  "@mozilla.org/filepicker;1"
#define EMBED_FILEPICKER_CLASSNAME  "File Picker Implementation"

class EmbedFilePicker : public nsIFilePicker
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFILEPICKER
  EmbedFilePicker ();
  virtual ~EmbedFilePicker();
private:
  nsIDOMWindow *mParent;
  PRInt16 mMode;
  char *mFilename;
};
#endif
