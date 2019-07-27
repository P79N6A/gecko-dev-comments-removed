



"use strict";























































this.EXPORTED_SYMBOLS = [ "Bookmarks" ];

const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Sqlite",
                                  "resource://gre/modules/Sqlite.jsm");

const URI_LENGTH_MAX = 65536;
const TITLE_LENGTH_MAX = 4096;

let Bookmarks = Object.freeze({
  



  TYPE_BOOKMARK: 1,
  TYPE_FOLDER: 2,
  TYPE_SEPARATOR: 3,

  




































  
  
  
  
  
  
  
  
  
  
  update: Task.async(function* (info) {
    throw new Error("Not yet implemented");
  }),

  




















  
  
  
  remove: Task.async(function* (guidOrInfo) {
    throw new Error("Not yet implemented");
  }),

  

























  
  
  
  
  
  
  
  
  
  
  
  
  
  
  fetch: Task.async(function* (guidOrInfo) {
    throw new Error("Not yet implemented");
  }),

  

























































  
  
  fetchTree: Task.async(function* (guid = "", options = {}) {
    throw new Error("Not yet implemented");
  }),

  













  
  
  reorder: Task.async(function* (parentGuid, orderedChildrenGuids) {
    throw new Error("Not yet implemented");
  })
});
