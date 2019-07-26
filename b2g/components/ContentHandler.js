


"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const PDF_CONTENT_TYPE = "application/pdf";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "cpmm", function() {
  return Cc["@mozilla.org/childprocessmessagemanager;1"]
           .getService(Ci.nsIMessageSender);
});

function debug(aMsg) {
  
}

const NS_ERROR_WONT_HANDLE_CONTENT = 0x805d0001;

let ActivityContentFactory = {
  createInstance: function createInstance(outer, iid) {
    if (outer != null) {
      throw Cr.NS_ERROR_NO_AGGREGATION;
    }
    return new ActivityContentHandler().QueryInterface(iid);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIFactory])
}

function ActivityContentHandler() {
}

ActivityContentHandler.prototype = {
  handleContent: function handleContent(aMimetype, aContext, aRequest) {
    if (!(aRequest instanceof Ci.nsIChannel))
      throw NS_ERROR_WONT_HANDLE_CONTENT;

    let detail = {
      "type": aMimetype,
      "url": aRequest.URI.spec
    };
    cpmm.sendAsyncMessage("content-handler", detail);

    aRequest.cancel(Cr.NS_BINDING_ABORTED);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIContentHandler])
}

function ContentHandler() {
  this.classIdMap = {};
}

ContentHandler.prototype = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "app-startup") {
      
      let appInfo = Cc["@mozilla.org/xre/app-info;1"];
      if (appInfo.getService(Ci.nsIXULRuntime)
          .processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT) {
        return;
      }
    }

    cpmm.addMessageListener("Activities:RegisterContentTypes", this);
    cpmm.addMessageListener("Activities:UnregisterContentTypes", this);
    cpmm.sendAsyncMessage("Activities:GetContentTypes", { });
  },

  





  registerContentHandler: function registerContentHandler(aContentType) {
    debug("Registering " + aContentType);

    
    
    if (this.classIdMap[aContentType]) {
      this.classIdMap[aContentType].count++;
      return;
    }

    let contractID = "@mozilla.org/uriloader/content-handler;1?type=" +
                     aContentType;
    let uuidGen = Cc["@mozilla.org/uuid-generator;1"]
                    .getService(Ci.nsIUUIDGenerator);
    let id = Components.ID(uuidGen.generateUUID().toString());
    this.classIdMap[aContentType] = { count: 1, id: id };
    let cr = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
    cr.registerFactory(Components.ID(id), "Activity Content Handler", contractID,
                       ActivityContentFactory);
  },

  


  unregisterContentHandler: function registerContentHandler(aContentType) {
    debug("Unregistering " + aContentType);

    let record = this.classIdMap[aContentType];
    if (!record) {
      return;
    }

    
    if (--record.count > 0) {
      return;
    }

    let contractID = "@mozilla.org/uriloader/content-handler;1?type=" +
                     aContentType;
    let cr = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
    cr.unregisterFactory(record.id, ActivityContentFactory);
    delete this.classIdMap[aContentType]
  },

  receiveMessage: function(aMessage) {
    let data = aMessage.data;

    switch (aMessage.name) {
      case "Activities:RegisterContentTypes":
        data.contentTypes.forEach(this.registerContentHandler, this);
        break;
      case "Activities:UnregisterContentTypes":
        data.contentTypes.forEach(this.unregisterContentHandler, this);
        break;
    }
  },

  classID: Components.ID("{d18d0216-d50c-11e1-ba54-efb18d0ef0ac}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIContentHandler,
                                         Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ContentHandler]);
