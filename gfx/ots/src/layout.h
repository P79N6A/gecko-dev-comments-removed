



#ifndef OTS_LAYOUT_H_
#define OTS_LAYOUT_H_

#include "ots.h"




namespace ots {


struct LookupSubtableParser {
  struct TypeParser {
    uint16_t type;
    bool (*parse)(const OpenTypeFile *file, const uint8_t *data,
                  const size_t length);
  };
  size_t num_types;
  uint16_t extension_type;
  const TypeParser *parsers;

  bool Parse(const OpenTypeFile *file, const uint8_t *data,
             const size_t length, const uint16_t lookup_type) const;
};

bool ParseScriptListTable(const uint8_t *data, const size_t length,
                          const uint16_t num_features);

bool ParseFeatureListTable(const uint8_t *data, const size_t length,
                           const uint16_t num_lookups,
                           uint16_t *num_features);

bool ParseLookupListTable(OpenTypeFile *file, const uint8_t *data,
                          const size_t length,
                          const LookupSubtableParser* parser,
                          uint16_t* num_lookups);

bool ParseClassDefTable(const uint8_t *data, size_t length,
                        const uint16_t num_glyphs,
                        const uint16_t num_classes);

bool ParseCoverageTable(const uint8_t *data, size_t length,
                        const uint16_t num_glyphs);

bool ParseDeviceTable(const uint8_t *data, size_t length);


bool ParseContextSubtable(const uint8_t *data, const size_t length,
                          const uint16_t num_glyphs,
                          const uint16_t num_lookups);


bool ParseChainingContextSubtable(const uint8_t *data, const size_t length,
                                  const uint16_t num_glyphs,
                                  const uint16_t num_lookups);

bool ParseExtensionSubtable(const OpenTypeFile *file,
                            const uint8_t *data, const size_t length,
                            const LookupSubtableParser* parser);

}  

#endif  

