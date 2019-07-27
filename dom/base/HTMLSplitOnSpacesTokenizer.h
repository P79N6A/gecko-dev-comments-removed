




#ifndef HTMLSplitOnSpacesTokenizer_h
#define HTMLSplitOnSpacesTokenizer_h

#include "nsCharSeparatedTokenizer.h"
#include "nsContentUtils.h"

typedef nsCharSeparatedTokenizerTemplate<nsContentUtils::IsHTMLWhitespace>
                                                    HTMLSplitOnSpacesTokenizer;

#endif
