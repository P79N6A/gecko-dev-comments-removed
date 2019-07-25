





































#include "nsRuleData.h"
#include "nsCSSProps.h"

namespace {

struct PropertyOffsetInfo {
  
  
  size_t struct_offset; 
  size_t member_offset; 
};

const PropertyOffsetInfo kOffsetTable[eCSSProperty_COUNT_no_shorthands] = {
  #define CSS_PROP_BACKENDONLY(name_, id_, method_, flags_, datastruct_,     \
                               member_, type_, kwtable_)                     \
      { size_t(-1), size_t(-1) },
  #define CSS_PROP(name_, id_, method_, flags_, datastruct_, member_, type_, \
                   kwtable_, stylestruct_, stylestructoffset_, animtype_)    \
      { offsetof(nsRuleData, m##datastruct_##Data),                          \
        offsetof(nsRuleData##datastruct_, member_) },
  #include "nsCSSPropList.h"
  #undef CSS_PROP
  #undef CSS_PROP_BACKENDONLY
};

} 

void*
nsRuleData::StorageFor(nsCSSProperty aProperty)
{
  NS_ABORT_IF_FALSE(aProperty < eCSSProperty_COUNT_no_shorthands,
                    "invalid or shorthand property");

  const PropertyOffsetInfo& offsets = kOffsetTable[aProperty];
  NS_ABORT_IF_FALSE(offsets.struct_offset != size_t(-1),
                    "backend-only property");

  char* cssstruct = *reinterpret_cast<char**>
    (reinterpret_cast<char*>(this) + offsets.struct_offset);
  NS_ABORT_IF_FALSE(cssstruct, "substructure pointer should never be null");

  return reinterpret_cast<void*>(cssstruct + offsets.member_offset);
}
