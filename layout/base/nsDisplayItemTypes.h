
















enum Type {
  TYPE_ZERO = 0, 

#define DECLARE_DISPLAY_ITEM_TYPE(name) TYPE_##name,
#define DECLARE_DISPLAY_ITEM_TYPE_FLAGS(name,flags) TYPE_##name,
#include "nsDisplayItemTypesList.h"
#undef DECLARE_DISPLAY_ITEM_TYPE
#undef DECLARE_DISPLAY_ITEM_TYPE_FLAGS

  TYPE_MAX
};

enum {
  
  TYPE_BITS = 8
};

enum DisplayItemFlags {
  TYPE_RENDERS_NO_IMAGES = 1 << 0
};

static const char* DisplayItemTypeName(Type aType)
{
  switch (aType) {
#define DECLARE_DISPLAY_ITEM_TYPE(name) case TYPE_##name: return #name;
#define DECLARE_DISPLAY_ITEM_TYPE_FLAGS(name,flags) case TYPE_##name: return #name;
#include "nsDisplayItemTypesList.h"
#undef DECLARE_DISPLAY_ITEM_TYPE
#undef DECLARE_DISPLAY_ITEM_TYPE_FLAGS

    default: return "TYPE_UNKNOWN";
  }
}

static uint8_t GetDisplayItemFlagsForType(Type aType)
{
  static const uint8_t flags[TYPE_MAX] = {
    0
#define DECLARE_DISPLAY_ITEM_TYPE(name) ,0
#define DECLARE_DISPLAY_ITEM_TYPE_FLAGS(name,flags) ,flags
#include "nsDisplayItemTypesList.h"
#undef DECLARE_DISPLAY_ITEM_TYPE
#undef DECLARE_DISPLAY_ITEM_TYPE_FLAGS
  };

  return flags[aType];
}

static Type GetDisplayItemTypeFromKey(uint32_t aDisplayItemKey)
{
  static const uint32_t typeMask = (1 << TYPE_BITS) - 1;
  Type type = static_cast<Type>(aDisplayItemKey & typeMask);
  NS_ASSERTION(type > TYPE_ZERO && type < TYPE_MAX, "Invalid display item type!");
  return type;
}
