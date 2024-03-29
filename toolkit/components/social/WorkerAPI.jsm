





"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "getFrameWorkerHandle", "resource://gre/modules/FrameWorker.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "openChatWindow", "resource://gre/modules/MozSocialAPI.jsm");

this.EXPORTED_SYMBOLS = ["WorkerAPI"];

this.WorkerAPI = function WorkerAPI(provider, port) {
  if (!port)
    throw new Error("Can't initialize WorkerAPI with a null port");

  this._provider = provider;
  this._port = port;
  this._port.onmessage = this._handleMessage.bind(this);

  
  
  
  this._port.postMessage({topic: "social.initialize"});
}

WorkerAPI.prototype = {
  terminate: function terminate() {
    this._port.close();
  },

  _handleMessage: function _handleMessage(event) {
    let {topic, data} = event.data;
    let handler = this.handlers[topic];
    if (!handler) {
      Cu.reportError("WorkerAPI: topic doesn't have a handler: '" + topic + "'");
      return;
    }
    try {
      handler.call(this, data);
    } catch (ex) {
      Cu.reportError("WorkerAPI: failed to handle message '" + topic + "': " + ex + "\n" + ex.stack);
    }
  },

  handlers: {
    "social.manifest-get": function(data) {
      
      this._port.postMessage({topic: "social.manifest", data: this._provider.manifest});
    },
    "social.manifest-set": function(data) {
      
      let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;
      let origin = this._provider.origin;
      SocialService.updateProvider(origin, data);
    },
    "social.reload-worker": function(data) {
      this._provider.reload();
    },
    "social.user-profile": function (data) {
      this._provider.updateUserProfile(data);
    },
    "social.ambient-notification": function (data) {
      this._provider.setAmbientNotification(data);
    },
    "social.cookies-get": function(data) {
      
      
      
      
      let port = this._port;
      let whandle = getFrameWorkerHandle(this._provider.workerURL, null);
      whandle.port.close();
      whandle._worker.browserPromise.then(browser => {
        let mm = browser.messageManager;
        mm.addMessageListener("frameworker:cookie-get-response", function _onCookieResponse(msg) {
          mm.removeMessageListener("frameworker:cookie-get-response", _onCookieResponse);
          let cookies = msg.json.split(";");
          let results = [];
          cookies.forEach(function(aCookie) {
            let [name, value] = aCookie.split("=");
            if (name || value) {
              results.push({name: unescape(name.trim()),
                            value: value ? unescape(value.trim()) : ""});
            }
          });
          port.postMessage({topic: "social.cookies-get-response",
                            data: results});
        });
        mm.sendAsyncMessage("frameworker:cookie-get");
      });
    },
    'social.request-chat': function(data) {
      openChatWindow(null, this._provider, data);
    },
    'social.notification-create': function(data) {
      if (!Services.prefs.getBoolPref("social.toast-notifications.enabled"))
        return;

      let port = this._port;
      let provider = this._provider;
      let {id, type, icon, body, action, actionArgs} = data;
      let alertsService = Cc["@mozilla.org/alerts-service;1"]
                              .getService(Ci.nsIAlertsService);
      function listener(subject, topic, data) {
        if (topic === "alertclickcallback") {
          
          port.postMessage({topic: "social.notification-action",
                            data: {id: id,
                                   action: action,
                                   actionArgs: actionArgs}});
          switch (action) {
            case "link":
              
              if (actionArgs.toURL) {
                let uriToOpen = provider.resolveUri(actionArgs.toURL);
                
                
                let pUri = Services.io.newURI(provider.origin, null, null);
                if (uriToOpen.scheme != pUri.scheme)
                  uriToOpen.scheme = pUri.scheme;
                if (provider.isSameOrigin(uriToOpen)) {
                  let xulWindow = Services.wm.getMostRecentWindow("navigator:browser");
                  xulWindow.openUILinkIn(uriToOpen.spec, "tab");
                } else {
                  Cu.reportError("Not opening notification link " + actionArgs.toURL
                                 + " as not in provider origin");
                }
              }
              break;
            default:
              break;
          }
        }
      }
      alertsService.showAlertNotification(icon,
                                          this._provider.name, 
                                          body,
                                          !!action, 
                                                    
                                          null,
                                          listener,
                                          type); 
    },
  }
}
