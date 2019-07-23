






































#ifndef NS_FT2_FONT_NODE_H
#define NS_FT2_FONT_NODE_H

#include "nsFontMetricsGTK.h"

class nsFT2FontNode {
public:
  static void       FreeGlobals();
  static nsresult   InitGlobals();
  static void       GetFontNames(const char* aPattern, nsFontNodeArray* aNodes);

#if (defined(MOZ_ENABLE_FREETYPE2))
protected:
  static PRBool ParseXLFD(char *, char**, char**, char**, char**);
  static nsFontNode* LoadNode(nsITrueTypeFontCatalogEntry*,
                              const char*,
                              nsFontNodeArray*);
  static PRBool      LoadNodeTable();
  static nsHashtable *mFreeTypeNodes;
  static PRBool      sInited;
  static nsIFontCatalogService* sFcs;
#endif 

};

#endif

