



































const EXPORTED_SYMBOLS = ['Weave'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/service.js");

let Weave = {};

Weave.Service = new WeaveSyncService();
