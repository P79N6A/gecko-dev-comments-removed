


"use strict";

let gTestTab;
let gContentAPI;
let gContentWindow;

Components.utils.import("resource:///modules/UITour.jsm");

function test() {
  requestLongerTimeout(2);
  UITourTest();
}

let tests = [
  function test_availableTargets(done) {
    gContentAPI.getConfiguration("availableTargets", (data) => {
      ok_targets(data, [
        "accountStatus",
        "addons",
        "appMenu",
        "backForward",
        "bookmarks",
        "customize",
        "help",
        "home",
        "loop",
        "pinnedTab",
        "privateWindow",
        "quit",
        "search",
        "searchProvider",
        "urlbar",
      ]);
      ok(UITour.availableTargetsCache.has(window),
         "Targets should now be cached");
      done();
    });
  },

  function test_availableTargets_changeWidgets(done) {
    CustomizableUI.removeWidgetFromArea("bookmarks-menu-button");
    ok(!UITour.availableTargetsCache.has(window),
       "Targets should be evicted from cache after widget change");
    gContentAPI.getConfiguration("availableTargets", (data) => {
      ok_targets(data, [
        "accountStatus",
        "addons",
        "appMenu",
        "backForward",
        "customize",
        "help",
        "loop",
        "home",
        "pinnedTab",
        "privateWindow",
        "quit",
        "search",
        "searchProvider",
        "urlbar",
      ]);
      ok(UITour.availableTargetsCache.has(window),
         "Targets should now be cached again");
      CustomizableUI.reset();
      ok(!UITour.availableTargetsCache.has(window),
         "Targets should not be cached after reset");
      done();
    });
  },

  function test_availableTargets_exceptionFromGetTarget(done) {
    
    
    CustomizableUI.removeWidgetFromArea("search-container");
    gContentAPI.getConfiguration("availableTargets", (data) => {
      
      ok_targets(data, [
        "accountStatus",
        "addons",
        "appMenu",
        "backForward",
        "bookmarks",
        "customize",
        "help",
        "home",
        "loop",
        "pinnedTab",
        "privateWindow",
        "quit",
        "urlbar",
      ]);
      CustomizableUI.reset();
      done();
    });
  },
];

function ok_targets(actualData, expectedTargets) {
  
  
  
  let selectedTabIcon =
    document.getAnonymousElementByAttribute(gBrowser.selectedTab,
                                            "anonid",
                                            "tab-icon-image");
  if (selectedTabIcon && UITour.isElementVisible(selectedTabIcon))
    expectedTargets.push("selectedTabIcon");

  ok(Array.isArray(actualData.targets), "data.targets should be an array");
  is(actualData.targets.sort().toString(), expectedTargets.sort().toString(),
     "Targets should be as expected");
}
