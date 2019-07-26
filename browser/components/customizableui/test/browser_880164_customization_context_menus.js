



Cu.import("resource://gre/modules/Promise.jsm");

const isOSX = (Services.appinfo.OS === "Darwin");

let gTests = [
  {
    desc: "Right-click on the home button should show a context menu with options to move it.",
    setup: null,
    run: function() {
      let contextMenu = document.getElementById("toolbar-context-menu");
      let shownPromise = contextMenuShown(contextMenu);
      let homeButton = document.getElementById("home-button");
      EventUtils.synthesizeMouse(homeButton, 2, 2, {type: "contextmenu", button: 2 });
      yield shownPromise;

      let expectedEntries = [
        [".customize-context-addToPanel", true],
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

      let hiddenPromise = contextMenuHidden(contextMenu);
      contextMenu.hidePopup();
      yield hiddenPromise;
    },
    teardown: null
  },
  {
    desc: "Right-click on the urlbar-container should show a context menu with disabled options to move it.",
    setup: null,
    run: function() {
      let contextMenu = document.getElementById("toolbar-context-menu");
      let shownPromise = contextMenuShown(contextMenu);
      let urlBarContainer = document.getElementById("urlbar-container");
      
      let urlbarRect = urlBarContainer.getBoundingClientRect();
      EventUtils.synthesizeMouse(urlBarContainer, 100, urlbarRect.height - 1, {type: "contextmenu", button: 2 });
      yield shownPromise;

      let expectedEntries = [
        [".customize-context-addToPanel", false],
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

      let hiddenPromise = contextMenuHidden(contextMenu);
      contextMenu.hidePopup();
      yield hiddenPromise;
    },
    teardown: null
  },
  {
    desc: "Right-click on the searchbar and moving it to the menu and back should move the search-container instead.",
    run: function() {
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
    }
  },
  {
    desc: "Right-click on an item within the menu panel should show a context menu with options to move it.",
    setup: null,
    run: function() {
      let shownPanelPromise = promisePanelShown(window);
      PanelUI.toggle({type: "command"});
      yield shownPanelPromise;

      let contextMenu = document.getElementById("customizationPanelItemContextMenu");
      let shownContextPromise = contextMenuShown(contextMenu);
      let newWindowButton = document.getElementById("new-window-button");
      ok(newWindowButton, "new-window-button was found");
      EventUtils.synthesizeMouse(newWindowButton, 2, 2, {type: "contextmenu", button: 2});
      yield shownContextPromise;

      is(PanelUI.panel.state, "open", "The PanelUI should still be open.");

      let expectedEntries = [
        [".customize-context-addToToolbar", true],
        [".customize-context-removeFromPanel", true],
        ["---"],
        [".viewCustomizeToolbar", true]
      ];
      checkContextMenu(contextMenu, expectedEntries);

      let hiddenContextPromise = contextMenuHidden(contextMenu);
      contextMenu.hidePopup();
      yield hiddenContextPromise;

      let hiddenPromise = promisePanelHidden(window);
      PanelUI.toggle({type: "command"});
      yield hiddenPromise;
    },
    teardown: null
  },
  {
    desc: "Right-click on the home button while in customization mode should show a context menu with options to move it.",
    setup: startCustomizing,
    run: function () {
      let contextMenu = document.getElementById("toolbar-context-menu");
      let shownPromise = contextMenuShown(contextMenu);
      let homeButton = document.getElementById("wrapper-home-button");
      EventUtils.synthesizeMouse(homeButton, 2, 2, {type: "contextmenu", button: 2});
      yield shownPromise;

      let expectedEntries = [
        [".customize-context-addToPanel", true],
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

      let hiddenContextPromise = contextMenuHidden(contextMenu);
      contextMenu.hidePopup();
      yield hiddenContextPromise;
    },
    teardown: null
  },
  {
    desc: "Right-click on an item in the palette should show a context menu with options to move it.",
    setup: null,
    run: function () {
      let contextMenu = document.getElementById("customizationPaletteItemContextMenu");
      let shownPromise = contextMenuShown(contextMenu);
      let openFileButton = document.getElementById("wrapper-open-file-button");
      EventUtils.synthesizeMouse(openFileButton, 2, 2, {type: "contextmenu", button: 2});
      yield shownPromise;

      let expectedEntries = [
        [".customize-context-addToToolbar", true],
        [".customize-context-addToPanel", true],
      ];
      checkContextMenu(contextMenu, expectedEntries);

      let hiddenContextPromise = contextMenuHidden(contextMenu);
      contextMenu.hidePopup();
      yield hiddenContextPromise;
    },
    teardown: null
  },
  {
    desc: "Right-click on an item in the panel while in customization mode should show a context menu with options to move it.",
    setup: null,
    run: function () {
      let contextMenu = document.getElementById("customizationPanelItemContextMenu");
      let shownPromise = contextMenuShown(contextMenu);
      let newWindowButton = document.getElementById("wrapper-new-window-button");
      EventUtils.synthesizeMouse(newWindowButton, 2, 2, {type: "contextmenu", button: 2});
      yield shownPromise;

      let expectedEntries = [
        [".customize-context-addToToolbar", true],
        [".customize-context-removeFromPanel", true],
        ["---"],
        [".viewCustomizeToolbar", false]
      ];
      checkContextMenu(contextMenu, expectedEntries);

      let hiddenContextPromise = contextMenuHidden(contextMenu);
      contextMenu.hidePopup();
      yield hiddenContextPromise;
    },
    teardown: endCustomizing
  },
  {
    desc: "Test the toolbarbutton panel context menu in customization mode without opening the panel before customization mode",
    setup: null,
    run: function() {
      this.otherWin = yield openAndLoadWindow(null, true);

      yield startCustomizing(this.otherWin);

      let contextMenu = this.otherWin.document.getElementById("customizationPanelItemContextMenu");
      let shownPromise = contextMenuShown(contextMenu);
      let newWindowButton = this.otherWin.document.getElementById("wrapper-new-window-button");
      EventUtils.synthesizeMouse(newWindowButton, 2, 2, {type: "contextmenu", button: 2}, this.otherWin);
      yield shownPromise;

      let expectedEntries = [
        [".customize-context-addToToolbar", true],
        [".customize-context-removeFromPanel", true],
        ["---"],
        [".viewCustomizeToolbar", false]
      ];
      checkContextMenu(contextMenu, expectedEntries, this.otherWin);

      let hiddenContextPromise = contextMenuHidden(contextMenu);
      contextMenu.hidePopup();
      yield hiddenContextPromise;
    },
    teardown: function() {
      yield endCustomizing(this.otherWin);
      this.otherWin.close();
      this.otherWin = null;
    }
  },
];

function test() {
  waitForExplicitFinish();
  runTests(gTests);
}

function contextMenuShown(aContextMenu) {
  let deferred = Promise.defer();
  let win = aContextMenu.ownerDocument.defaultView;
  let timeoutId = win.setTimeout(() => {
    deferred.reject("Context menu (" + aContextMenu.id + ") did not show within 20 seconds.");
  }, 20000);
  function onPopupShown(e) {
    aContextMenu.removeEventListener("popupshown", onPopupShown);
    win.clearTimeout(timeoutId);
    deferred.resolve();
  };
  aContextMenu.addEventListener("popupshown", onPopupShown);
  return deferred.promise;
}

function contextMenuHidden(aContextMenu) {
  let deferred = Promise.defer();
  let win = aContextMenu.ownerDocument.defaultView;
  let timeoutId = win.setTimeout(() => {
    deferred.reject("Context menu (" + aContextMenu.id + ") did not hide within 20 seconds.");
  }, 20000);
  function onPopupHidden(e) {
    win.clearTimeout(timeoutId);
    aContextMenu.removeEventListener("popuphidden", onPopupHidden);
    deferred.resolve();
  };
  aContextMenu.addEventListener("popuphidden", onPopupHidden);
  return deferred.promise;
}



function checkContextMenu(aContextMenu, aExpectedEntries, aWindow=window) {
  let childNodes = aContextMenu.childNodes;
  for (let i = 0; i < childNodes.length; i++) {
    let menuitem = childNodes[i];
    try {
      if (aExpectedEntries[i][0] == "---") {
        is(menuitem.localName, "menuseparator", "menuseparator expected");
        continue;
      }

      let selector = aExpectedEntries[i][0];
      ok(menuitem.mozMatchesSelector(selector), "menuitem should match " + selector + " selector");
      let commandValue = menuitem.getAttribute("command");
      let relatedCommand = commandValue ? aWindow.document.getElementById(commandValue) : null;
      let menuItemDisabled = relatedCommand ?
                               relatedCommand.getAttribute("disabled") == "true" :
                               menuitem.getAttribute("disabled") == "true";
      is(menuItemDisabled, !aExpectedEntries[i][1], "disabled state for " + selector);
    } catch (e) {
      ok(false, "Exception when checking context menu: " + e);
    }
  }
}
