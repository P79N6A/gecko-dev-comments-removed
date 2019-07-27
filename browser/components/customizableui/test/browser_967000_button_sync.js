





"use strict";

XPCOMUtils.defineLazyModuleGetter(this, "UITour", "resource:///modules/UITour.jsm");

let initialLocation = gBrowser.currentURI.spec;
let newTab = null;

function openAboutAccountsFromMenuPanel(entryPoint) {
  info("Check Sync button functionality");
  Services.prefs.setCharPref("identity.fxaccounts.remote.signup.uri", "http://example.com/");

  
  CustomizableUI.addWidgetToArea("sync-button", CustomizableUI.AREA_PANEL);

  
  yield PanelUI.show();

  if (entryPoint == "uitour") {
    UITour.originTabs.set(window, new Set());
    UITour.originTabs.get(window).add(gBrowser.selectedTab);
  }

  let syncButton = document.getElementById("sync-button");
  ok(syncButton, "The Sync button was added to the Panel Menu");

  let deferred = Promise.defer();
  let handler = () => {
    gBrowser.selectedTab.removeEventListener("load", handler, true);
    deferred.resolve();
  }
  gBrowser.selectedTab.addEventListener("load", handler, true);

  syncButton.click();
  yield deferred.promise;
  newTab = gBrowser.selectedTab;

  is(gBrowser.currentURI.spec, "about:accounts?entrypoint=" + entryPoint,
    "Firefox Sync page opened with `menupanel` entrypoint");
  ok(!isPanelUIOpen(), "The panel closed");

  if(isPanelUIOpen()) {
    let panelHidePromise = promisePanelHidden(window);
    PanelUI.hide();
    yield panelHidePromise;
  }
}

function asyncCleanup() {
  Services.prefs.clearUserPref("identity.fxaccounts.remote.signup.uri");
  
  yield resetCustomization();
  ok(CustomizableUI.inDefaultState, "The panel UI is in default state again.");

  
  gBrowser.addTab(initialLocation);
  gBrowser.removeTab(newTab);
  UITour.originTabs.delete(window);
}

add_task(() => openAboutAccountsFromMenuPanel("syncbutton"));
add_task(asyncCleanup);

add_task(() => openAboutAccountsFromMenuPanel("uitour"));
add_task(asyncCleanup);
