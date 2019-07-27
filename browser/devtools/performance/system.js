


"use strict";






const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;
const { devtools: loader } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const require = loader.require;

const SYSTEM = {};


#ifdef E10S_TESTING_ONLY
SYSTEM.MULTIPROCESS_SUPPORTED = true;
#endif
