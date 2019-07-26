



"use strict";

#ifndef MERGED_COMPARTMENT

this.EXPORTED_SYMBOLS = ["Metrics"];

const {utils: Cu} = Components;

const MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000;

#endif




#define MERGED_COMPARTMENT

#include collector.jsm
;
#include dataprovider.jsm
;
#include storage.jsm
;

this.Metrics = {
  Collector: Collector,
  DailyValues: DailyValues,
  Measurement: Measurement,
  Provider: Provider,
  Storage: MetricsStorageBackend,
  dateToDays: dateToDays,
  daysToDate: daysToDate,
};

