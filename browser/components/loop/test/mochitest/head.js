


const MozLoopServiceInternal = Cu.import("resource:///modules/loop/MozLoopService.jsm", {}).
                               MozLoopServiceInternal;

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
    loopPanel.removeChild(document.getElementById(btn.getAttribute("notificationFrameId")));
  });

  return deferred.promise;
}







function loadLoopPanel() {
  
  Services.prefs.setCharPref("services.push.serverURL", "ws://localhost/");
  Services.prefs.setCharPref("loop.server", "http://localhost/");

  
  
  
  
  let wasOffline = Services.io.offline;
  Services.io.offline = true;

  registerCleanupFunction(function() {
    Services.prefs.clearUserPref("services.push.serverURL");
    Services.prefs.clearUserPref("loop.server");
    Services.io.offline = wasOffline;
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
  global.gFxAOAuthClientPromise = null;
  global.gFxAOAuthClient = null;
  global.gFxAOAuthTokenData = null;
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
