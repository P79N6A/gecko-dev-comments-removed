


const HAWK_TOKEN_LENGTH = 64;
const {
  LOOP_SESSION_TYPE,
  MozLoopServiceInternal,
} = Cu.import("resource:///modules/loop/MozLoopService.jsm", {});




const WAS_OFFLINE = Services.io.offline;

var gMozLoopAPI;

function promiseGetMozLoopAPI() {
  let deferred = Promise.defer();
  let loopPanel = document.getElementById("loop-notification-panel");
  let btn = document.getElementById("loop-call-button");

  
  
  loopPanel.addEventListener("popupshown", function onpopupshown() {
    loopPanel.removeEventListener("popupshown", onpopupshown, true);
    let iframe = document.getElementById(btn.getAttribute("notificationFrameId"));

    if (iframe.contentDocument &&
        iframe.contentDocument.readyState == "complete") {
      gMozLoopAPI = iframe.contentWindow.navigator.wrappedJSObject.mozLoop;

      deferred.resolve();
    } else {
      iframe.addEventListener("load", function panelOnLoad(e) {
        iframe.removeEventListener("load", panelOnLoad, true);

        gMozLoopAPI = iframe.contentWindow.navigator.wrappedJSObject.mozLoop;

        
        
        deferred.resolve();
      }, true);
    }
  }, true);

  
  btn.click();

  
  
  
  registerCleanupFunction(function() {
    loopPanel.hidePopup();
    let frameId = btn.getAttribute("notificationFrameId");
    let frame = document.getElementById(frameId);
    if (frame) {
      loopPanel.removeChild(frame);
    }
  });

  return deferred.promise;
}







function loadLoopPanel(aOverrideOptions = {}) {
  
  Services.prefs.setCharPref("services.push.serverURL", aOverrideOptions.pushURL || "ws://localhost/");
  Services.prefs.setCharPref("loop.server", aOverrideOptions.loopURL || "http://localhost/");

  
  
  
  
  if (!aOverrideOptions.stayOnline) {
    Services.io.offline = true;
  }

  registerCleanupFunction(function() {
    Services.prefs.clearUserPref("services.push.serverURL");
    Services.prefs.clearUserPref("loop.server");
    Services.io.offline = WAS_OFFLINE;
  });

  
  let loopPanel = document.getElementById("loop-notification-panel");
  loopPanel.setAttribute("animate", "false");

  
  yield promiseGetMozLoopAPI();
}

function promiseOAuthParamsSetup(baseURL, params) {
  let deferred = Promise.defer();
  let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
              createInstance(Ci.nsIXMLHttpRequest);
  xhr.open("POST", baseURL + "/setup_params", true);
  xhr.setRequestHeader("X-Params", JSON.stringify(params));
  xhr.addEventListener("load", () => deferred.resolve(xhr));
  xhr.addEventListener("error", error => deferred.reject(error));
  xhr.send();

  return deferred.promise;
}

function resetFxA() {
  let global = Cu.import("resource:///modules/loop/MozLoopService.jsm", {});
  global.gHawkClient = null;
  global.gFxAOAuthClientPromise = null;
  global.gFxAOAuthClient = null;
  global.gFxAOAuthTokenData = null;
  global.gFxAOAuthProfile = null;
  const fxASessionPref = MozLoopServiceInternal.getSessionTokenPrefName(LOOP_SESSION_TYPE.FXA);
  Services.prefs.clearUserPref(fxASessionPref);
}

function setInternalLoopGlobal(aName, aValue) {
  let global = Cu.import("resource:///modules/loop/MozLoopService.jsm", {});
  global[aName] = aValue;
}

function promiseDeletedOAuthParams(baseURL) {
  let deferred = Promise.defer();
  let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
              createInstance(Ci.nsIXMLHttpRequest);
  xhr.open("DELETE", baseURL + "/setup_params", true);
  xhr.addEventListener("load", () => deferred.resolve(xhr));
  xhr.addEventListener("error", deferred.reject);
  xhr.send();

  return deferred.promise;
}

function promiseObserverNotified(aTopic, aExpectedData = null) {
  let deferred = Promise.defer();
  Services.obs.addObserver(function onNotification(aSubject, aTopic, aData) {
    Services.obs.removeObserver(onNotification, aTopic);
    is(aData, aExpectedData, "observer data should match expected data")
    deferred.resolve({subject: aSubject, data: aData});
  }, aTopic, false);
  return deferred.promise;
}




function promiseOAuthGetRegistration(baseURL) {
  let deferred = Promise.defer();
  let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
              createInstance(Ci.nsIXMLHttpRequest);
  xhr.open("GET", baseURL + "/get_registration", true);
  xhr.responseType = "json";
  xhr.addEventListener("load", () => deferred.resolve(xhr));
  xhr.addEventListener("error", deferred.reject);
  xhr.send();

  return deferred.promise;
}






let mockPushHandler = {
  
  
  registrationResult: null,
  pushUrl: undefined,

  


  initialize: function(registerCallback, notificationCallback) {
    registerCallback(this.registrationResult, this.pushUrl);
    this._notificationCallback = notificationCallback;
  },

  


  notify: function(version) {
    this._notificationCallback(version);
  }
};
