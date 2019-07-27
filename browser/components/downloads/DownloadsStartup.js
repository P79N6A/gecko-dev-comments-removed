











"use strict";




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");




const kTransferCid = Components.ID("{1b4c85df-cbdd-4bb6-b04e-613caece083c}");
const kTransferContractId = "@mozilla.org/transfer;1";




function DownloadsStartup() { }

DownloadsStartup.prototype = {
  classID: Components.ID("{49507fe5-2cee-4824-b6a3-e999150ce9b8}"),

  _xpcom_factory: XPCOMUtils.generateSingletonFactory(DownloadsStartup),

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  
  

  observe: function DS_observe(aSubject, aTopic, aData)
  {
    
    
    Components.manager.QueryInterface(Ci.nsIComponentRegistrar)
                      .registerFactory(kTransferCid, "",
                                       kTransferContractId, null);
  },
};




this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DownloadsStartup]);
