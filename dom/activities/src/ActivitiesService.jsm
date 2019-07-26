



"use strict"

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/IndexedDBHelper.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ActivitiesServiceFilter",
  "resource://gre/modules/ActivitiesServiceFilter.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");

XPCOMUtils.defineLazyServiceGetter(this, "NetUtil",
                                   "@mozilla.org/network/util;1",
                                   "nsINetUtil");

this.EXPORTED_SYMBOLS = [];

function debug(aMsg) {
  
}

const DB_NAME    = "activities";
const DB_VERSION = 1;
const STORE_NAME = "activities";

function ActivitiesDb() {

}

ActivitiesDb.prototype = {
  __proto__: IndexedDBHelper.prototype,

  init: function actdb_init() {
    this.initDBHelper(DB_NAME, DB_VERSION, [STORE_NAME]);
  },

  












  upgradeSchema: function actdb_upgradeSchema(aTransaction, aDb, aOldVersion, aNewVersion) {
    debug("Upgrade schema " + aOldVersion + " -> " + aNewVersion);
    let objectStore = aDb.createObjectStore(STORE_NAME, { keyPath: "id" });

    
    objectStore.createIndex("name", "name", { unique: false });
    objectStore.createIndex("manifest", "manifest", { unique: false });

    debug("Created object stores and indexes");
  },

  
  createId: function actdb_createId(aObject) {
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                      .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    let hasher = Cc["@mozilla.org/security/hash;1"]
                   .createInstance(Ci.nsICryptoHash);
    hasher.init(hasher.SHA1);

    
    ["manifest", "name"].forEach(function(aProp) {
      let data = converter.convertToByteArray(aObject[aProp], {});
      hasher.update(data, data.length);
    });

    return hasher.finish(true);
  },

  
  add: function actdb_add(aObjects, aSuccess, aError) {
    this.newTxn("readwrite", STORE_NAME, function (txn, store) {
      aObjects.forEach(function (aObject) {
        let object = {
          manifest: aObject.manifest,
          name: aObject.name,
          icon: aObject.icon || "",
          description: aObject.description
        };
        object.id = this.createId(object);
        debug("Going to add " + JSON.stringify(object));
        store.put(object);
      }, this);
    }.bind(this), aSuccess, aError);
  },

  
  remove: function actdb_remove(aObjects) {
    this.newTxn("readwrite", STORE_NAME, function (txn, store) {
      aObjects.forEach(function (aObject) {
        let object = {
          manifest: aObject.manifest,
          name: aObject.name
        };
        debug("Going to remove " + JSON.stringify(object));
        store.delete(this.createId(object));
      }, this);
    }.bind(this), function() {}, function() {});
  },

  find: function actdb_find(aObject, aSuccess, aError, aMatch) {
    debug("Looking for " + aObject.options.name);

    this.newTxn("readonly", STORE_NAME, function (txn, store) {
      let index = store.index("name");
      let request = index.mozGetAll(aObject.options.name);
      request.onsuccess = function findSuccess(aEvent) {
        debug("Request successful. Record count: " + aEvent.target.result.length);
        if (!txn.result) {
          txn.result = {
            name: aObject.options.name,
            options: []
          };
        }

        aEvent.target.result.forEach(function(result) {
          if (!aMatch(result))
            return;

          txn.result.options.push({
            manifest: result.manifest,
            icon: result.icon,
            description: result.description
          });
        });
      }
    }.bind(this), aSuccess, aError);
  }
}

let Activities = {
  messages: [
    
    "Activity:Start",

    
    "Activity:Ready",

    
    "Activity:PostResult",
    "Activity:PostError",

    "Activities:Register",
    "Activities:Unregister",
    "Activities:GetContentTypes",

    "child-process-shutdown"
  ],

  init: function activities_init() {
    this.messages.forEach(function(msgName) {
      ppmm.addMessageListener(msgName, this);
    }, this);

    Services.obs.addObserver(this, "xpcom-shutdown", false);

    this.db = new ActivitiesDb();
    this.db.init();
    this.callers = {};
  },

  observe: function activities_observe(aSubject, aTopic, aData) {
    this.messages.forEach(function(msgName) {
      ppmm.removeMessageListener(msgName, this);
    }, this);
    ppmm = null;

    if (this.db) {
      this.db.close();
      this.db = null;
    }

    Services.obs.removeObserver(this, "xpcom-shutdown");
  },

  






  startActivity: function activities_startActivity(aMsg) {
    debug("StartActivity: " + JSON.stringify(aMsg));

    let successCb = function successCb(aResults) {
      debug(JSON.stringify(aResults));

      function getActivityChoice(aResultType, aResult) {
        switch(aResultType) {
          case Ci.nsIActivityUIGlueCallback.NATIVE_ACTIVITY: {
            Activities.callers[aMsg.id].mm.sendAsyncMessage("Activity:FireSuccess", {
              "id": aMsg.id,
              "result": aResult
            });
            break;
          }
          case Ci.nsIActivityUIGlueCallback.WEBAPPS_ACTIVITY: {
            debug("Activity choice: " + aResult);

            
            
            
            if (aResults.options.length === 0) {
              Activities.callers[aMsg.id].mm.sendAsyncMessage("Activity:FireError", {
                "id": aMsg.id,
                "error": "NO_PROVIDER"
              });
              delete Activities.callers[aMsg.id];
              return;
            }

            
            if (aResult === -1) {
              Activities.callers[aMsg.id].mm.sendAsyncMessage("Activity:FireError", {
                "id": aMsg.id,
                "error": "ActivityCanceled"
              });
              delete Activities.callers[aMsg.id];
              return;
            }

            let sysmm = Cc["@mozilla.org/system-message-internal;1"]
                          .getService(Ci.nsISystemMessagesInternal);
            if (!sysmm) {
              
              delete Activities.callers[aMsg.id];
              return;
            }

            debug("Sending system message...");
            let result = aResults.options[aResult];
            sysmm.sendMessage("activity", {
                "id": aMsg.id,
                "payload": aMsg.options,
                "target": result.description
              },
              Services.io.newURI(result.description.href, null, null),
              Services.io.newURI(result.manifest, null, null),
              {
                "manifestURL": Activities.callers[aMsg.id].manifestURL,
                "pageURL": Activities.callers[aMsg.id].pageURL
              });

            if (!result.description.returnValue) {
              Activities.callers[aMsg.id].mm.sendAsyncMessage("Activity:FireSuccess", {
                "id": aMsg.id,
                "result": null
              });
              
              
              delete Activities.callers[aMsg.id];
            }
            break;
          }
        }
      };

      let glue = Cc["@mozilla.org/dom/activities/ui-glue;1"]
                   .createInstance(Ci.nsIActivityUIGlue);
      glue.chooseActivity(aMsg.options, aResults.options, getActivityChoice);
    };

    let errorCb = function errorCb(aError) {
      
      debug("Error in startActivity: " + aError + "\n");
    };

    let matchFunc = function matchFunc(aResult) {
      return ActivitiesServiceFilter.match(aMsg.options.data,
                                           aResult.description.filters);
    };

    this.db.find(aMsg, successCb, errorCb, matchFunc);
  },

  receiveMessage: function activities_receiveMessage(aMessage) {
    let mm = aMessage.target;
    let msg = aMessage.json;

    let caller;
    let obsData;

    if (aMessage.name == "Activity:PostResult" ||
        aMessage.name == "Activity:PostError" ||
        aMessage.name == "Activity:Ready") {
      caller = this.callers[msg.id];
      if (!caller) {
        debug("!! caller is null for msg.id=" + msg.id);
        return;
      }
      obsData = JSON.stringify({ manifestURL: caller.manifestURL,
                                 pageURL: caller.pageURL,
                                 success: aMessage.name == "Activity:PostResult" });
    }

    switch(aMessage.name) {
      case "Activity:Start":
        this.callers[msg.id] = { mm: mm,
                                 manifestURL: msg.manifestURL,
                                 pageURL: msg.pageURL };
        this.startActivity(msg);
        break;

      case "Activity:Ready":
        caller.childMM = mm;
        break;

      case "Activity:PostResult":
        caller.mm.sendAsyncMessage("Activity:FireSuccess", msg);
        delete this.callers[msg.id];
        break;
      case "Activity:PostError":
        caller.mm.sendAsyncMessage("Activity:FireError", msg);
        delete this.callers[msg.id];
        break;

      case "Activities:Register":
        let self = this;
        this.db.add(msg,
          function onSuccess(aEvent) {
            mm.sendAsyncMessage("Activities:Register:OK", null);
            let res = [];
            msg.forEach(function(aActivity) {
              self.updateContentTypeList(aActivity, res);
            });
          },
          function onError(aEvent) {
            msg.error = "REGISTER_ERROR";
            mm.sendAsyncMessage("Activities:Register:KO", msg);
          });
        break;
      case "Activities:Unregister":
        this.db.remove(msg);
        let res = [];
        msg.forEach(function(aActivity) {
          this.updateContentTypeList(aActivity, res);
        }, this);
        break;
      case "child-process-shutdown":
        for (let id in this.callers) {
          if (this.callers[id].childMM == mm) {
            this.callers[id].mm.sendAsyncMessage("Activity:FireError", {
              "id": id,
              "error": "ActivityCanceled"
            });
            delete this.callers[id];
            break;
          }
        }
        break;
    }
  },

  updateContentTypeList: function updateContentTypeList(aActivity, aResult) {
    
    if (aActivity.name != "view") {
      return;
    }

    let types = aActivity.description.filters.type;
    if (typeof types == "string") {
      types = [types];
    }

    
    types.forEach(function(aContentType) {
      let hadCharset = { };
      let charset = { };
      let contentType =
        NetUtil.parseContentType(aContentType, charset, hadCharset);
      if (contentType) {
        aResult.push(contentType);
      }
    });
  }
}

Activities.init();
