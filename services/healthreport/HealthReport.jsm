



"use strict";

this.EXPORTED_SYMBOLS = [
  "HealthReporter",
  "AddonsProvider",
  "AppInfoProvider",
  "CrashesProvider",
  "Metrics",
  "PlacesProvider",
  "ProfileMetadataProvider",
  "SearchesProvider",
  "SessionsProvider",
  "SysInfoProvider",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

const MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000;




#define MERGED_COMPARTMENT

#include ../common/async.js
;
#include ../common/bagheeraclient.js
;
#include ../metrics/Metrics.jsm
;
#include healthreporter.jsm
;
#include profile.jsm
;
#include providers.jsm
;

