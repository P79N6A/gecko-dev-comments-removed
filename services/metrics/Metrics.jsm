



"use strict";

this.EXPORTED_SYMBOLS = ["Metrics"];

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/services/metrics/collector.jsm");
Cu.import("resource://gre/modules/services/metrics/dataprovider.jsm");
Cu.import("resource://gre/modules/services/metrics/storage.jsm");


this.Metrics = {
  Collector: Collector,
  Measurement: Measurement,
  Provider: Provider,
  Storage: MetricsStorageBackend,
  dateToDays: dateToDays,
  daysToDate: daysToDate,
};

