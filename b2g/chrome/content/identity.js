








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
  const kReceivedIdentityAssertion = "received-id-assertion";
  const kIdentityDelegateWatch = "identity-delegate-watch";
  const kIdentityDelegateRequest = "identity-delegate-request";
  const kIdentityDelegateLogout = "identity-delegate-logout";
  const kIdentityDelegateReady = "identity-delegate-ready";
  const kIdentityDelegateFinished = "identity-delegate-finished";
  const kIdentityControllerDoMethod = "identity-controller-doMethod";
  const kIdentktyJSLoaded = true;
}

var showUI = false;
var options = null;
var isLoaded = false;
var func = null;








function identityCall(message) {
  sendAsyncMessage(kIdentityControllerDoMethod, message);
}

function identityFinished() {
  log("identity finished.  closing dialog");
  closeIdentityDialog(function notifySuccess() {
    
    func = null; options = null;

    sendAsyncMessage(kIdentityDelegateFinished);
  });
}




function closeIdentityDialog(aCallback) {
  let randomId = uuidgen.generateUUID().toString();
  let id = kReceivedIdentityAssertion + "-" + randomId;
  let browser = Services.wm.getMostRecentWindow("navigator:browser");

  let detail = {
    type: kReceivedIdentityAssertion,
    id: id,
    showUI: showUI
  };

  
  
  
  content.addEventListener("mozContentEvent", function closeIdentityDialogFinished(evt) {
    content.removeEventListener("mozContentEvent", closeIdentityDialogFinished);

    if (evt.detail.id == id && aCallback) {
      aCallback();
    }
  });

  browser.shell.sendChromeEvent(detail);
}





function doInternalWatch() {
  log("doInternalWatch:", options, isLoaded);
  if (options && isLoaded) {
    log("internal watch options:", options);
    let BrowserID = content.wrappedJSObject.BrowserID;
    BrowserID.internal.watch(function(aParams) {
        log("sending watch method message:", aParams.method);
        identityCall(aParams);
        if (aParams.method === "ready") {
          log("watch finished.");
          identityFinished();
        }
      },
      JSON.stringify({loggedInUser: options.loggedInUser, origin: options.origin}),
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
          log("request -> assertion, so do login");
          identityCall({method:'login',assertion:assertion});
        }
        identityFinished();
      },
      options);
  }
}

function doInternalLogout(aOptions) {
  log("doInternalLogout:", (options && isLoaded));
  if (options && isLoaded) {
    let BrowserID = content.wrappedJSObject.BrowserID;
    log("logging you out of ", options.origin);
    BrowserID.internal.logout(options.origin, function() {
      identityCall({method:'logout'});
      identityFinished();
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
    log("\n\n* * * * injected identity.js received", kIdentityDelegateRequest, "\n\n\n");
  options = aMessage.json;
  showUI = true;
  func = doInternalRequest;
  func();
});


addMessageListener(kIdentityDelegateWatch, function(aMessage) {
    log("\n\n* * * * injected identity.js received", kIdentityDelegateWatch, "\n\n\n");
  options = aMessage.json;
  showUI = false;
  func = doInternalWatch;
  func();
});


addMessageListener(kIdentityDelegateLogout, function(aMessage) {
    log("\n\n* * * * injected identity.js received", kIdentityDelegateLogout, "\n\n\n");
  options = aMessage.json;
  showUI = false;
  func = doInternalLogout;
  func();
});
