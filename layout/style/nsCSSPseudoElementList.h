




























































#ifndef CSS2_PSEUDO_ELEMENT
#define DEFINED_CSS2_PSEUDO_ELEMENT
#define CSS2_PSEUDO_ELEMENT(name_, value_) CSS_PSEUDO_ELEMENT(name_, value_)
#endif 

CSS2_PSEUDO_ELEMENT(after, ":after")
CSS2_PSEUDO_ELEMENT(before, ":before")

CSS2_PSEUDO_ELEMENT(firstLetter, ":first-letter")
CSS2_PSEUDO_ELEMENT(firstLine, ":first-line")

CSS_PSEUDO_ELEMENT(mozSelection, ":-moz-selection")

CSS_PSEUDO_ELEMENT(mozFocusInner, ":-moz-focus-inner")
CSS_PSEUDO_ELEMENT(mozFocusOuter, ":-moz-focus-outer")

CSS_PSEUDO_ELEMENT(mozListBullet, ":-moz-list-bullet")
CSS_PSEUDO_ELEMENT(mozListNumber, ":-moz-list-number")

CSS_PSEUDO_ELEMENT(horizontalFramesetBorder, ":-moz-hframeset-border")
CSS_PSEUDO_ELEMENT(verticalFramesetBorder, ":-moz-vframeset-border")

#ifdef DEFINED_CSS2_PSEUDO_ELEMENT
#undef CSS2_PSEUDO_ELEMENT
#endif
