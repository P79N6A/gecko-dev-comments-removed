



"use strict";

const kButton = "test_button_for_addon";
let initialLocation = gBrowser.currentURI.spec;

add_task(function() {
  info("Check addon button functionality");

  
  let widgetSpec = {
    id: kButton,
    type: 'button',
    onClick: function() {
      gBrowser.selectedTab = gBrowser.addTab("about:addons");
    }
  };
  CustomizableUI.createWidget(widgetSpec);
  CustomizableUI.addWidgetToArea(kButton, CustomizableUI.AREA_NAVBAR);

  
  let addonButton = document.getElementById(kButton);
  let navBar = document.getElementById("nav-bar");
  ok(addonButton, "Addon button exists");
  ok(navBar.contains(addonButton), "Addon button is in the navbar");
  yield checkButtonFunctionality(addonButton);

  resetTabs();

  
  CustomizableUI.addWidgetToArea(kButton, CustomizableUI.AREA_PANEL);
  let addonButtonInNavbar = navBar.getElementsByAttribute("id", kButton);
  ok(!navBar.contains(addonButton), "Addon button was removed from the browser bar");

  
  yield PanelUI.show();
  var panelMenu = document.getElementById("PanelUI-mainView");
  let addonButtonInPanel = panelMenu.getElementsByAttribute("id", kButton);
  ok(panelMenu.contains(addonButton), "Addon button was added to the Panel Menu");
  yield checkButtonFunctionality(addonButtonInPanel[0]);
});

add_task(function asyncCleanup() {
  resetTabs();

  
  yield resetCustomization();
  ok(CustomizableUI.inDefaultState, "The UI is in default state again.");

  
  CustomizableUI.destroyWidget(kButton);
});

function resetTabs() {
  
  while(gBrowser.tabs.length > 1) {
    gBrowser.removeTab(gBrowser.selectedTab);
  }

  
  gBrowser.addTab(initialLocation);
  gBrowser.removeTab(gBrowser.selectedTab);
}

function checkButtonFunctionality(aButton) {
  aButton.click();
  yield waitForCondition(() => gBrowser.currentURI &&
                               gBrowser.currentURI.spec == "about:addons");
}
