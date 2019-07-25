



#include "metrics.h"

#include "head.h"
#include "maxp.h"





namespace ots {

bool ParseMetricsHeader(OpenTypeFile *file, Buffer *table,
                        OpenTypeMetricsHeader *header) {
  if (!table->ReadS16(&header->ascent) ||
      !table->ReadS16(&header->descent) ||
      !table->ReadS16(&header->linegap) ||
      !table->ReadU16(&header->adv_width_max) ||
      !table->ReadS16(&header->min_sb1) ||
      !table->ReadS16(&header->min_sb2) ||
      !table->ReadS16(&header->max_extent) ||
      !table->ReadS16(&header->caret_slope_rise) ||
      !table->ReadS16(&header->caret_slope_run) ||
      !table->ReadS16(&header->caret_offset)) {
    return OTS_FAILURE();
  }

  if (header->ascent < 0) {
    OTS_WARNING("bad ascent: %d", header->ascent);
    header->ascent = 0;
  }
  if (header->linegap < 0) {
    OTS_WARNING("bad linegap: %d", header->linegap);
    header->linegap = 0;
  }

  if (!file->head) {
    return OTS_FAILURE();
  }

  
  if (!(file->head->mac_style & 2) &&
      (header->caret_offset != 0)) {
    OTS_WARNING("bad caret offset: %d", header->caret_offset);
    header->caret_offset = 0;
  }

  
  if (!table->Skip(8)) {
    return OTS_FAILURE();
  }

  int16_t data_format;
  if (!table->ReadS16(&data_format)) {
    return OTS_FAILURE();
  }
  if (data_format) {
    return OTS_FAILURE();
  }

  if (!table->ReadU16(&header->num_metrics)) {
    return OTS_FAILURE();
  }

  if (!file->maxp) {
    return OTS_FAILURE();
  }

  if (header->num_metrics > file->maxp->num_glyphs) {
    return OTS_FAILURE();
  }

  return true;
}

bool SerialiseMetricsHeader(OTSStream *out,
                            const OpenTypeMetricsHeader *header) {
  if (!out->WriteU32(header->version) ||
      !out->WriteS16(header->ascent) ||
      !out->WriteS16(header->descent) ||
      !out->WriteS16(header->linegap) ||
      !out->WriteU16(header->adv_width_max) ||
      !out->WriteS16(header->min_sb1) ||
      !out->WriteS16(header->min_sb2) ||
      !out->WriteS16(header->max_extent) ||
      !out->WriteS16(header->caret_slope_rise) ||
      !out->WriteS16(header->caret_slope_run) ||
      !out->WriteS16(header->caret_offset) ||
      !out->WriteR64(0) ||  
      !out->WriteS16(0) ||  
      !out->WriteU16(header->num_metrics)) {
    return OTS_FAILURE();
  }

  return true;
}

bool ParseMetricsTable(Buffer *table,
                       const uint16_t num_glyphs,
                       const OpenTypeMetricsHeader *header,
                       OpenTypeMetricsTable *metrics) {
  
  
  const unsigned num_metrics = header->num_metrics;

  if (num_metrics > num_glyphs) {
    return OTS_FAILURE();
  }
  if (!num_metrics) {
    return OTS_FAILURE();
  }
  const unsigned num_sbs = num_glyphs - num_metrics;

  metrics->entries.reserve(num_metrics);
  for (unsigned i = 0; i < num_metrics; ++i) {
    uint16_t adv = 0;
    int16_t sb = 0;
    if (!table->ReadU16(&adv) || !table->ReadS16(&sb)) {
      return OTS_FAILURE();
    }

    
    
    
    if (adv > header->adv_width_max) {
      OTS_WARNING("bad adv: %u > %u", adv, header->adv_width_max);
      adv = header->adv_width_max;
    }

    if (sb < header->min_sb1) {
      OTS_WARNING("bad sb: %d < %d", sb, header->min_sb1);
      sb = header->min_sb1;
    }

    metrics->entries.push_back(std::make_pair(adv, sb));
  }

  metrics->sbs.reserve(num_sbs);
  for (unsigned i = 0; i < num_sbs; ++i) {
    int16_t sb;
    if (!table->ReadS16(&sb)) {
      
      return OTS_FAILURE();
    }

    if (sb < header->min_sb1) {
      
      
      OTS_WARNING("bad lsb: %d < %d", sb, header->min_sb1);
      sb = header->min_sb1;
    }

    metrics->sbs.push_back(sb);
  }

  return true;
}

bool SerialiseMetricsTable(OTSStream *out,
                           const OpenTypeMetricsTable *metrics) {
  for (unsigned i = 0; i < metrics->entries.size(); ++i) {
    if (!out->WriteU16(metrics->entries[i].first) ||
        !out->WriteS16(metrics->entries[i].second)) {
      return OTS_FAILURE();
    }
  }

  for (unsigned i = 0; i < metrics->sbs.size(); ++i) {
    if (!out->WriteS16(metrics->sbs[i])) {
      return OTS_FAILURE();
    }
  }

  return true;
}

}  

