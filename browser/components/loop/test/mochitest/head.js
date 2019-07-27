


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

  registerCleanupFunction(function() {
    Services.prefs.clearUserPref("services.push.serverURL");
    Services.prefs.clearUserPref("loop.server");
  });

  
  let loopPanel = document.getElementById("loop-notification-panel");
  loopPanel.setAttribute("animate", "false");

  
  yield promiseGetMozLoopAPI();
}
