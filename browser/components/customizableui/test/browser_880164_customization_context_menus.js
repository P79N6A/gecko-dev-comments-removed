



"use strict";

const isOSX = (Services.appinfo.OS === "Darwin");



add_task(function() {
  let contextMenu = document.getElementById("toolbar-context-menu");
  let shownPromise = popupShown(contextMenu);
  let homeButton = document.getElementById("home-button");
  EventUtils.synthesizeMouse(homeButton, 2, 2, {type: "contextmenu", button: 2 });
  yield shownPromise;

  let expectedEntries = [
    [".customize-context-moveToPanel", true],
    [".customize-context-removeFromToolbar", true],
    ["---"]
  ];
  if (!isOSX) {
    expectedEntries.push(["#toggle_toolbar-menubar", true]);
  }
  expectedEntries.push(
    ["#toggle_PersonalToolbar", true],
    ["---"],
    [".viewCustomizeToolbar", true]
  );
  checkContextMenu(contextMenu, expectedEntries);

  let hiddenPromise = popupHidden(contextMenu);
  contextMenu.hidePopup();
  yield hiddenPromise;
});




add_task(function() {
  
  let extraTab = gBrowser.selectedTab = gBrowser.addTab();
  yield promiseTabLoadEvent(extraTab, "http://example.com/");
  let contextMenu = document.getElementById("toolbar-context-menu");
  let shownPromise = popupShown(contextMenu);
  let tabstrip = document.getElementById("tabbrowser-tabs");
  let rect = tabstrip.getBoundingClientRect();
  EventUtils.synthesizeMouse(tabstrip, rect.width - 2, 2, {type: "contextmenu", button: 2 });
  yield shownPromise;

  let closedTabsAvailable = SessionStore.getClosedTabCount(window) == 0;
  info("Closed tabs: " + closedTabsAvailable);
  let expectedEntries = [
    ["#toolbar-context-reloadAllTabs", true],
    ["#toolbar-context-bookmarkAllTabs", true],
    ["#toolbar-context-undoCloseTab", !closedTabsAvailable],
    ["---"]
  ];
  if (!isOSX) {
    expectedEntries.push(["#toggle_toolbar-menubar", true]);
  }
  expectedEntries.push(
    ["#toggle_PersonalToolbar", true],
    ["---"],
    [".viewCustomizeToolbar", true]
  );
  checkContextMenu(contextMenu, expectedEntries);

  let hiddenPromise = popupHidden(contextMenu);
  contextMenu.hidePopup();
  yield hiddenPromise;
  gBrowser.removeTab(extraTab);
});




add_task(function() {
  let contextMenu = document.getElementById("toolbar-context-menu");
  let shownPromise = popupShown(contextMenu);
  let toolbar = createToolbarWithPlacements("880164_empty_toolbar", []);
  toolbar.setAttribute("context", "toolbar-context-menu");
  toolbar.setAttribute("toolbarname", "Fancy Toolbar for Context Menu");
  EventUtils.synthesizeMouseAtCenter(toolbar, {type: "contextmenu", button: 2 });
  yield shownPromise;

  let expectedEntries = [
    [".customize-context-moveToPanel", false],
    [".customize-context-removeFromToolbar", false],
    ["---"]
  ];
  if (!isOSX) {
    expectedEntries.push(["#toggle_toolbar-menubar", true]);
  }
  expectedEntries.push(
    ["#toggle_PersonalToolbar", true],
    ["#toggle_880164_empty_toolbar", true],
    ["---"],
    [".viewCustomizeToolbar", true]
  );
  checkContextMenu(contextMenu, expectedEntries);

  let hiddenPromise = popupHidden(contextMenu);
  contextMenu.hidePopup();
  yield hiddenPromise;
  removeCustomToolbars();
});




add_task(function() {
  let contextMenu = document.getElementById("toolbar-context-menu");
  let shownPromise = popupShown(contextMenu);
  let urlBarContainer = document.getElementById("urlbar-container");
  
  let urlbarRect = urlBarContainer.getBoundingClientRect();
  EventUtils.synthesizeMouse(urlBarContainer, 100, 1, {type: "contextmenu", button: 2 });
  yield shownPromise;

  let expectedEntries = [
    [".customize-context-moveToPanel", false],
    [".customize-context-removeFromToolbar", false],
    ["---"]
  ];
  if (!isOSX) {
    expectedEntries.push(["#toggle_toolbar-menubar", true]);
  }
  expectedEntries.push(
    ["#toggle_PersonalToolbar", true],
    ["---"],
    [".viewCustomizeToolbar", true]
  );
  checkContextMenu(contextMenu, expectedEntries);

  let hiddenPromise = popupHidden(contextMenu);
  contextMenu.hidePopup();
  yield hiddenPromise;
});



add_task(function() {
  let searchbar = document.getElementById("searchbar");
  gCustomizeMode.addToPanel(searchbar);
  let placement = CustomizableUI.getPlacementOfWidget("search-container");
  is(placement.area, CustomizableUI.AREA_PANEL, "Should be in panel");

  let shownPanelPromise = promisePanelShown(window);
  PanelUI.toggle({type: "command"});
  yield shownPanelPromise;
  let hiddenPanelPromise = promisePanelHidden(window);
  PanelUI.toggle({type: "command"});
  yield hiddenPanelPromise;

  gCustomizeMode.addToToolbar(searchbar);
  placement = CustomizableUI.getPlacementOfWidget("search-container");
  is(placement.area, CustomizableUI.AREA_NAVBAR, "Should be in navbar");
  gCustomizeMode.removeFromArea(searchbar);
  placement = CustomizableUI.getPlacementOfWidget("search-container");
  is(placement, null, "Should be in palette");
  CustomizableUI.reset();
  placement = CustomizableUI.getPlacementOfWidget("search-container");
  is(placement.area, CustomizableUI.AREA_NAVBAR, "Should be in navbar");
});



add_task(function() {
  let shownPanelPromise = promisePanelShown(window);
  PanelUI.toggle({type: "command"});
  yield shownPanelPromise;

  let contextMenu = document.getElementById("customizationPanelItemContextMenu");
  let shownContextPromise = popupShown(contextMenu);
  let newWindowButton = document.getElementById("new-window-button");
  ok(newWindowButton, "new-window-button was found");
  EventUtils.synthesizeMouse(newWindowButton, 2, 2, {type: "contextmenu", button: 2});
  yield shownContextPromise;

  is(PanelUI.panel.state, "open", "The PanelUI should still be open.");

  let expectedEntries = [
    [".customize-context-moveToToolbar", true],
    [".customize-context-removeFromPanel", true],
    ["---"],
    [".viewCustomizeToolbar", true]
  ];
  checkContextMenu(contextMenu, expectedEntries);

  let hiddenContextPromise = popupHidden(contextMenu);
  contextMenu.hidePopup();
  yield hiddenContextPromise;

  let hiddenPromise = promisePanelHidden(window);
  PanelUI.toggle({type: "command"});
  yield hiddenPromise;
});



add_task(function() {
  yield startCustomizing();
  let contextMenu = document.getElementById("toolbar-context-menu");
  let shownPromise = popupShown(contextMenu);
  let homeButton = document.getElementById("wrapper-home-button");
  EventUtils.synthesizeMouse(homeButton, 2, 2, {type: "contextmenu", button: 2});
  yield shownPromise;

  let expectedEntries = [
    [".customize-context-moveToPanel", true],
    [".customize-context-removeFromToolbar", true],
    ["---"]
  ];
  if (!isOSX) {
    expectedEntries.push(["#toggle_toolbar-menubar", true]);
  }
  expectedEntries.push(
    ["#toggle_PersonalToolbar", true],
    ["---"],
    [".viewCustomizeToolbar", false]
  );
  checkContextMenu(contextMenu, expectedEntries);

  let hiddenContextPromise = popupHidden(contextMenu);
  contextMenu.hidePopup();
  yield hiddenContextPromise;
});



add_task(function() {
  let contextMenu = document.getElementById("customizationPaletteItemContextMenu");
  let shownPromise = popupShown(contextMenu);
  let openFileButton = document.getElementById("wrapper-open-file-button");
  EventUtils.synthesizeMouse(openFileButton, 2, 2, {type: "contextmenu", button: 2});
  yield shownPromise;

  let expectedEntries = [
    [".customize-context-addToToolbar", true],
    [".customize-context-addToPanel", true]
  ];
  checkContextMenu(contextMenu, expectedEntries);

  let hiddenContextPromise = popupHidden(contextMenu);
  contextMenu.hidePopup();
  yield hiddenContextPromise;
});



add_task(function() {
  let contextMenu = document.getElementById("customizationPanelItemContextMenu");
  let shownPromise = popupShown(contextMenu);
  let newWindowButton = document.getElementById("wrapper-new-window-button");
  EventUtils.synthesizeMouse(newWindowButton, 2, 2, {type: "contextmenu", button: 2});
  yield shownPromise;

  let expectedEntries = [
    [".customize-context-moveToToolbar", true],
    [".customize-context-removeFromPanel", true],
    ["---"],
    [".viewCustomizeToolbar", false]
  ];
  checkContextMenu(contextMenu, expectedEntries);

  let hiddenContextPromise = popupHidden(contextMenu);
  contextMenu.hidePopup();
  yield hiddenContextPromise;
  yield endCustomizing();
});



add_task(function() {
  this.otherWin = yield openAndLoadWindow(null, true);

  yield startCustomizing(this.otherWin);

  let contextMenu = this.otherWin.document.getElementById("customizationPanelItemContextMenu");
  let shownPromise = popupShown(contextMenu);
  let newWindowButton = this.otherWin.document.getElementById("wrapper-new-window-button");
  EventUtils.synthesizeMouse(newWindowButton, 2, 2, {type: "contextmenu", button: 2}, this.otherWin);
  yield shownPromise;

  let expectedEntries = [
    [".customize-context-moveToToolbar", true],
    [".customize-context-removeFromPanel", true],
    ["---"],
    [".viewCustomizeToolbar", false]
  ];
  checkContextMenu(contextMenu, expectedEntries, this.otherWin);

  let hiddenContextPromise = popupHidden(contextMenu);
  contextMenu.hidePopup();
  yield hiddenContextPromise;
  yield endCustomizing(this.otherWin);
  yield promiseWindowClosed(this.otherWin);
  this.otherWin = null;
});



add_task(function() {
  yield startCustomizing();
  let contextMenu = document.getElementById("customizationPanelItemContextMenu");
  let shownPromise = popupShown(contextMenu);
  let zoomControls = document.getElementById("wrapper-zoom-controls");
  EventUtils.synthesizeMouse(zoomControls, 2, 2, {type: "contextmenu", button: 2});
  yield shownPromise;
  
  contextMenu.childNodes[0].doCommand();
  let hiddenPromise = popupHidden(contextMenu);
  contextMenu.hidePopup();
  yield hiddenPromise;
  yield endCustomizing();

  zoomControls = document.getElementById("zoom-controls");
  is(zoomControls.parentNode.id, "nav-bar-customization-target", "Zoom-controls should be on the nav-bar");

  contextMenu = document.getElementById("toolbar-context-menu");
  shownPromise = popupShown(contextMenu);
  EventUtils.synthesizeMouse(zoomControls, 2, 2, {type: "contextmenu", button: 2});
  yield shownPromise;

  let expectedEntries = [
    [".customize-context-moveToPanel", true],
    [".customize-context-removeFromToolbar", true],
    ["---"]
  ];
  if (!isOSX) {
    expectedEntries.push(["#toggle_toolbar-menubar", true]);
  }
  expectedEntries.push(
    ["#toggle_PersonalToolbar", true],
    ["---"],
    [".viewCustomizeToolbar", true]
  );
  checkContextMenu(contextMenu, expectedEntries);

  hiddenPromise = popupHidden(contextMenu);
  contextMenu.hidePopup();
  yield hiddenPromise;
  yield resetCustomization();
});


add_task(function() {
  yield startCustomizing();
  yield endCustomizing();

  yield PanelUI.show();

  let contextMenu = document.getElementById("customizationPanelItemContextMenu");
  let shownContextPromise = popupShown(contextMenu);
  let newWindowButton = document.getElementById("new-window-button");
  ok(newWindowButton, "new-window-button was found");
  EventUtils.synthesizeMouse(newWindowButton, 2, 2, {type: "contextmenu", button: 2});
  yield shownContextPromise;

  is(PanelUI.panel.state, "open", "The PanelUI should still be open.");

  let expectedEntries = [
    [".customize-context-moveToToolbar", true],
    [".customize-context-removeFromPanel", true],
    ["---"],
    [".viewCustomizeToolbar", true]
  ];
  checkContextMenu(contextMenu, expectedEntries);

  let hiddenContextPromise = popupHidden(contextMenu);
  contextMenu.hidePopup();
  yield hiddenContextPromise;

  let hiddenPromise = promisePanelHidden(window);
  PanelUI.hide();
  yield hiddenPromise;
});



add_task(function() {
  let widgetId = "custom-context-menu-toolbarbutton";
  let expectedContext = "myfancycontext";
  let widget = createDummyXULButton(widgetId, "Test ctxt menu");
  widget.setAttribute("context", expectedContext);
  CustomizableUI.addWidgetToArea(widgetId, CustomizableUI.AREA_NAVBAR);
  is(widget.getAttribute("context"), expectedContext, "Should have context menu when added to the toolbar.");

  yield startCustomizing();
  is(widget.getAttribute("context"), "", "Should not have own context menu in the toolbar now that we're customizing.");
  is(widget.getAttribute("wrapped-context"), expectedContext, "Should keep own context menu wrapped when in toolbar.");

  let panel = PanelUI.contents;
  simulateItemDrag(widget, panel);
  is(widget.getAttribute("context"), "", "Should not have own context menu when in the panel.");
  is(widget.getAttribute("wrapped-context"), expectedContext, "Should keep own context menu wrapped now that we're in the panel.");

  simulateItemDrag(widget, document.getElementById("nav-bar").customizationTarget);
  is(widget.getAttribute("context"), "", "Should not have own context menu when back in toolbar because we're still customizing.");
  is(widget.getAttribute("wrapped-context"), expectedContext, "Should keep own context menu wrapped now that we're back in the toolbar.");

  yield endCustomizing();
  is(widget.getAttribute("context"), expectedContext, "Should have context menu again now that we're out of customize mode.");
  CustomizableUI.removeWidgetFromArea(widgetId);
  widget.remove();
  ok(CustomizableUI.inDefaultState, "Should be in default state after removing button.");
});
