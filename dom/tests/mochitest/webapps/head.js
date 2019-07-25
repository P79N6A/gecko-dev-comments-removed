












function makeAllAppsLaunchable() {
  var Webapps = {};
  Components.utils.import("resource://gre/modules/Webapps.jsm", Webapps);
  var originalValue = Webapps.DOMApplicationRegistry.allAppsLaunchable;
  Webapps.DOMApplicationRegistry.allAppsLaunchable = true;

  
  window.addEventListener("unload", function restoreAllAppsLaunchable(event) {
    if (event.target == window.document) {
      window.removeEventListener("unload", restoreAllAppsLaunchable, false);
      Webapps.DOMApplicationRegistry.allAppsLaunchable = originalValue;
    }
  }, false);
}

function runAll(steps) {
  SimpleTest.waitForExplicitFinish();

  makeAllAppsLaunchable();

  
  steps = steps.concat();
  function next() {
    if (steps.length) {
      steps.shift()(next);
    }
    else {
      SimpleTest.finish();
    }
  }
  next();
}

function confirmNextInstall() {
  var Ci = Components.interfaces;

  var popupPanel = SpecialPowers.wrap(window).top.
                   QueryInterface(Ci.nsIInterfaceRequestor).
                   getInterface(Ci.nsIWebNavigation).
                   QueryInterface(Ci.nsIDocShell).
                   chromeEventHandler.ownerDocument.defaultView.
                   PopupNotifications.panel;

  function onPopupShown() {
    popupPanel.removeEventListener("popupshown", onPopupShown, false);
    SpecialPowers.wrap(this).childNodes[0].button.doCommand();
  }
  popupPanel.addEventListener("popupshown", onPopupShown, false);
}
