




[

#define DO_PROP(method, pref) \
  [ #method, pref ],
#define CSS_PROP(name, id, method, flags, pref, parsevariant, kwtable, \
		 stylestruct, stylestructofset, animtype) \
  DO_PROP(method, pref)
#define CSS_PROP_SHORTHAND(name, id, method, flags, pref) \
  DO_PROP(method, pref)
#define CSS_PROP_PUBLIC_OR_PRIVATE(publicname_, privatename_) publicname_
#define CSS_PROP_LIST_EXCLUDE_INTERNAL

#include "nsCSSPropList.h"

#undef CSS_PROP_LIST_EXCLUDE_INTERNAL
#undef CSS_PROP_PUBLIC_OR_PRIVATE
#undef CSS_PROP_SHORTHAND
#undef CSS_PROP

#define CSS_PROP_ALIAS(name, id, method, pref) \
  DO_PROP(method, pref)

#include "nsCSSPropAliasList.h"

#undef CSS_PROP_ALIAS

#undef DO_PROP

]
