




#ifndef HTMLSplitOnSpacesTokenizer_h
#define HTMLSplitOnSpacesTokenizer_h

#include "nsCharSeparatedTokenizer.h"

typedef nsCharSeparatedTokenizerTemplate<nsContentUtils::IsHTMLWhitespace>
                                                    HTMLSplitOnSpacesTokenizer;

#endif
