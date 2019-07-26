















enum Type {
  TYPE_ZERO = 0, 

#define DECLARE_DISPLAY_ITEM_TYPE(name) TYPE_##name,
#include "nsDisplayItemTypesList.h"
#undef DECLARE_DISPLAY_ITEM_TYPE

  TYPE_MAX
};

enum {
  
  TYPE_BITS = 8
};

static const char* DisplayItemTypeName(Type aType)
{
  switch (aType) {
#define DECLARE_DISPLAY_ITEM_TYPE(name) case TYPE_##name: return #name;
#include "nsDisplayItemTypesList.h"
#undef DECLARE_DISPLAY_ITEM_TYPE

    default: return "TYPE_UNKNOWN";
  }
}
