



"use strict";

let initialLocation = gBrowser.currentURI.spec;
let newTab = null;

add_task(function() {
  info("Check Sync button functionality");
  Services.prefs.setCharPref("identity.fxaccounts.remote.signup.uri", "http://example.com/");

  
  CustomizableUI.addWidgetToArea("sync-button", CustomizableUI.AREA_PANEL);

  
  yield PanelUI.show();

  let syncButton = document.getElementById("sync-button");
  ok(syncButton, "The Sync button was added to the Panel Menu");
  syncButton.click();

  newTab = gBrowser.selectedTab;
  yield promiseTabLoadEvent(newTab, "about:accounts");

  is(gBrowser.currentURI.spec, "about:accounts", "Firefox Sync page opened");
  ok(!isPanelUIOpen(), "The panel closed");

  if(isPanelUIOpen()) {
    let panelHidePromise = promisePanelHidden(window);
    PanelUI.hide();
    yield panelHidePromise;
  }
});

add_task(function asyncCleanup() {
  Services.prefs.clearUserPref("identity.fxaccounts.remote.signup.uri");
  
  yield resetCustomization();
  ok(CustomizableUI.inDefaultState, "The panel UI is in default state again.");

  
  gBrowser.addTab(initialLocation);
  gBrowser.removeTab(newTab);
});
