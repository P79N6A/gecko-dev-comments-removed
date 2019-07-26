


function runAll(steps) {
  SimpleTest.waitForExplicitFinish();

  










  SpecialPowers.setAllAppsLaunchable(true);

  
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
  var Ci = SpecialPowers.Ci;

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
