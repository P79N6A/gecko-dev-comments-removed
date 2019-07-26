








"use strict";

let { classes: Cc, interfaces: Ci, utils: Cu }  = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");

XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

XPCOMUtils.defineLazyModuleGetter(this, "Logger",
                                  "resource://gre/modules/identity/LogUtils.jsm");

function log(...aMessageArgs) {
  Logger.log.apply(Logger, ["injected identity.js"].concat(aMessageArgs));
}

log("\n\n======================= identity.js =======================\n\n");



if (typeof kIdentityJSLoaded === 'undefined') {
  const kIdentityDelegateWatch = "identity-delegate-watch";
  const kIdentityDelegateRequest = "identity-delegate-request";
  const kIdentityDelegateLogout = "identity-delegate-logout";
  const kIdentityDelegateReady = "identity-delegate-ready";
  const kIdentityDelegateFinished = "identity-delegate-finished";
  const kIdentityControllerDoMethod = "identity-controller-doMethod";
  const kIdentktyJSLoaded = true;
}

var showUI = false;
var options = {};
var isLoaded = false;
var func = null;








function identityCall(message) {
  sendAsyncMessage(kIdentityControllerDoMethod, message);
}







function closeIdentityDialog() {
  
  func = null; options = null;
  sendAsyncMessage(kIdentityDelegateFinished);
}





function doInternalWatch() {
  log("doInternalWatch:", options, isLoaded);
  if (options && isLoaded) {
    let BrowserID = content.wrappedJSObject.BrowserID;
    BrowserID.internal.watch(function(aParams) {
        identityCall(aParams);
        if (aParams.method === "ready") {
          closeIdentityDialog();
        }
      },
      JSON.stringify(options),
      function(...things) {
        log("internal: ", things);
      }
    );
  }
}

function doInternalRequest() {
  log("doInternalRequest:", options && isLoaded);
  if (options && isLoaded) {
    content.wrappedJSObject.BrowserID.internal.get(
      options.origin,
      function(assertion) {
        if (assertion) {
          identityCall({method: 'login', assertion: assertion});
        }
        closeIdentityDialog();
      },
      options);
  }
}

function doInternalLogout(aOptions) {
  log("doInternalLogout:", (options && isLoaded));
  if (options && isLoaded) {
    let BrowserID = content.wrappedJSObject.BrowserID;
    BrowserID.internal.logout(options.origin, function() {
      identityCall({method:'logout'});
      closeIdentityDialog();
    });
  }
}

addEventListener("DOMContentLoaded", function(e) {
  content.addEventListener("load", function(e) {
    isLoaded = true;
     
     if (func) func();
  });
});


addMessageListener(kIdentityDelegateRequest, function(aMessage) {
  log("injected identity.js received", kIdentityDelegateRequest, "\n\n\n");
  options = aMessage.json;
  showUI = true;
  func = doInternalRequest;
  func();
});


addMessageListener(kIdentityDelegateWatch, function(aMessage) {
  log("injected identity.js received", kIdentityDelegateWatch, "\n\n\n");
  options = aMessage.json;
  showUI = false;
  func = doInternalWatch;
  func();
});


addMessageListener(kIdentityDelegateLogout, function(aMessage) {
  log("injected identity.js received", kIdentityDelegateLogout, "\n\n\n");
  options = aMessage.json;
  showUI = false;
  func = doInternalLogout;
  func();
});
