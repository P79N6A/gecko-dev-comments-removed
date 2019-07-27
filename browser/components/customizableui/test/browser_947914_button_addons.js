



"use strict";

let initialLocation = gBrowser.currentURI.spec;
let newTab = null;

add_task(function() {
  info("Check addons button existence and functionality");

  yield PanelUI.show();
  info("Menu panel was opened");

  let addonsButton = document.getElementById("add-ons-button");
  ok(addonsButton, "Add-ons button exists in Panel Menu");
  addonsButton.click();

  newTab = gBrowser.selectedTab;
  yield waitForCondition(function() gBrowser.currentURI &&
                                    gBrowser.currentURI.spec == "about:addons");

  let addonsPage = gBrowser.selectedBrowser.contentWindow.document.
                            getElementById("addons-page");
  ok(addonsPage, "Add-ons page was opened");
});

add_task(function asyncCleanup() {
  gBrowser.addTab(initialLocation);
  gBrowser.removeTab(gBrowser.selectedTab);
  info("Tabs were restored");
});
