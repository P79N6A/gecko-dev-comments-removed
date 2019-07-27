






"use strict";

let gTestTab;
let gContentAPI;
let gContentWindow;
let gContentDoc;

Components.utils.import("resource:///modules/UITour.jsm");

function test() {
  registerCleanupFunction(function() {
    gContentDoc = null;
  });
  UITourTest();
}






let tests = [
  taskify(function* test_move_tab_to_new_window(done) {
    let onVisibilityChange = (aEvent) => {
      if (!document.hidden && window != UITour.getChromeWindow(aEvent.target)) {
        gContentAPI.showHighlight("appMenu");
      }
    };

    let highlight = document.getElementById("UITourHighlight");
    let windowDestroyedDeferred = Promise.defer();
    let onDOMWindowDestroyed = (aWindow) => {
      if (gContentWindow && aWindow == gContentWindow) {
        Services.obs.removeObserver(onDOMWindowDestroyed, "dom-window-destroyed", false);
        windowDestroyedDeferred.resolve();
      }
    };

    let browserStartupDeferred = Promise.defer();
    Services.obs.addObserver(function onBrowserDelayedStartup(aWindow) {
      Services.obs.removeObserver(onBrowserDelayedStartup, "browser-delayed-startup-finished");
      browserStartupDeferred.resolve(aWindow);
    }, "browser-delayed-startup-finished", false);

    
    
    
    gContentDoc = gBrowser.selectedTab.linkedBrowser.contentDocument;
    gContentDoc.addEventListener("visibilitychange", onVisibilityChange, false);
    gContentAPI.showHighlight("appMenu");

    yield elementVisiblePromise(highlight);

    gBrowser.replaceTabWithWindow(gBrowser.selectedTab);

    gContentWindow = yield browserStartupDeferred.promise;

    
    let newWindowHighlight = gContentWindow.document.getElementById("UITourHighlight");
    yield elementVisiblePromise(newWindowHighlight);

    let selectedTab = gContentWindow.gBrowser.selectedTab;
    is(selectedTab.linkedBrowser && selectedTab.linkedBrowser.contentDocument, gContentDoc, "Document should be selected in new window");
    ok(UITour.originTabs && UITour.originTabs.has(gContentWindow), "Window should be known");
    ok(UITour.originTabs.get(gContentWindow).has(selectedTab), "Tab should be known");

    let shownPromise = promisePanelShown(gContentWindow);
    gContentAPI.showMenu("appMenu");
    yield shownPromise;

    isnot(gContentWindow.PanelUI.panel.state, "closed", "Panel should be open");
    ok(gContentWindow.PanelUI.contents.children.length > 0, "Panel contents should have children");
    gContentAPI.hideHighlight();
    gContentAPI.hideMenu("appMenu");
    gTestTab = null;

    Services.obs.addObserver(onDOMWindowDestroyed, "dom-window-destroyed", false);
    gContentWindow.close();

    yield windowDestroyedDeferred.promise;
  }),
];
