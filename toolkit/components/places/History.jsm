



"use strict";

























































this.EXPORTED_SYMBOLS = [ "History" ];

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


this.History = Object.freeze({
  

















  fetch: function (guidOrURI) {
    throw new Error("Method not implemented");
  },

  





















































  update: function (infos, onResult) {
    throw new Error("Method not implemented");
  },

  






















  remove: function (pages, onResult) {
    throw new Error("Method not implemented");
  },

  















  hasVisits: function(page, onResult) {
    throw new Error("Method not implemented");
  },

  




  


  TRANSITION_LINK: Ci.nsINavHistoryService.TRANSITION_LINK,

  





  TRANSITION_TYPED: Ci.nsINavHistoryService.TRANSITION_TYPED,

  


  TRANSITION_BOOKMARK: Ci.nsINavHistoryService.TRANSITION_BOOKMARK,

  





  TRANSITION_EMBED: Ci.nsINavHistoryService.TRANSITION_EMBED,

  


  TRANSITION_REDIRECT_PERMANENT: Ci.nsINavHistoryService.TRANSITION_REDIRECT_PERMANENT,

  


  TRANSITION_REDIRECT_TEMPORARY: Ci.nsINavHistoryService.TRANSITION_REDIRECT_TEMPORARY,

  


  TRANSITION_DOWNLOAD: Ci.nsINavHistoryService.TRANSITION_REDIRECT_DOWNLOAD,

  


  TRANSITION_FRAMED_LINK: Ci.nsINavHistoryService.TRANSITION_FRAMED_LINK,
});

