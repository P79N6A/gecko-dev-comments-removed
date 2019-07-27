




"use strict";

const TEST_URI = "http://example.com/";

let hud, hudId, hudBox;

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  hud = yield openConsole();
  hudId = hud.hudId;
  hudBox = hud.ui.rootElement;

  testFilterButtons();

  hud = hudId = hudBox = null;
});

function testFilterButtons() {
  testMenuFilterButton("net");
  testMenuFilterButton("css");
  testMenuFilterButton("js");
  testMenuFilterButton("logging");
  testMenuFilterButton("security");

  testIsolateFilterButton("net");
  testIsolateFilterButton("css");
  testIsolateFilterButton("js");
  testIsolateFilterButton("logging");
  testIsolateFilterButton("security");
}

function testMenuFilterButton(category) {
  let selector = ".webconsole-filter-button[category=\"" + category + "\"]";
  let button = hudBox.querySelector(selector);
  ok(button, "we have the \"" + category + "\" button");

  let firstMenuItem = button.querySelector("menuitem");
  ok(firstMenuItem, "we have the first menu item for the \"" + category +
     "\" button");

  
  let menuItem = firstMenuItem;
  while (menuItem != null) {
    if (menuItem.hasAttribute("prefKey") && isChecked(menuItem)) {
      chooseMenuItem(menuItem);
    }
    menuItem = menuItem.nextSibling;
  }

  
  menuItem = firstMenuItem;
  let prefKey;
  while (menuItem) {
    if (menuItem.hasAttribute("prefKey")) {
      prefKey = menuItem.getAttribute("prefKey");
      chooseMenuItem(menuItem);
      ok(isChecked(menuItem), "menu item " + prefKey + " for category " +
         category + " is checked after clicking it");
      ok(hud.ui.filterPrefs[prefKey], prefKey + " messages are " +
         "on after clicking the appropriate menu item");
    }
    menuItem = menuItem.nextSibling;
  }
  ok(isChecked(button), "the button for category " + category + " is " +
     "checked after turning on all its menu items");

  
  prefKey = firstMenuItem.getAttribute("prefKey");
  chooseMenuItem(firstMenuItem);
  ok(!isChecked(firstMenuItem), "the first menu item for category " +
     category + " is no longer checked after clicking it");
  ok(!hud.ui.filterPrefs[prefKey], prefKey + " messages are " +
     "turned off after clicking the appropriate menu item");
  ok(isChecked(button), "the button for category " + category + " is still " +
     "checked after turning off its first menu item");

  
  let subbutton = getMainButton(button);
  ok(subbutton, "we have the subbutton for category " + category);

  clickButton(subbutton);
  ok(!isChecked(button), "the button for category " + category + " is " +
     "no longer checked after clicking its main part");

  menuItem = firstMenuItem;
  while (menuItem) {
    prefKey = menuItem.getAttribute("prefKey");
    if (prefKey) {
      ok(!isChecked(menuItem), "menu item " + prefKey + " for category " +
         category + " is no longer checked after clicking the button");
      ok(!hud.ui.filterPrefs[prefKey], prefKey + " messages are " +
         "off after clicking the button");
    }
    menuItem = menuItem.nextSibling;
  }

  
  clickButton(subbutton);

  ok(isChecked(button), "the button for category " + category + " is " +
     "checked after clicking its main part");

  menuItem = firstMenuItem;
  while (menuItem) {
    if (menuItem.hasAttribute("prefKey")) {
      prefKey = menuItem.getAttribute("prefKey");
      
      if (category == "css" && prefKey == "csslog") {
        ok(!isChecked(menuItem), "menu item " + prefKey + " for category " +
           category + " should not be checked after clicking the button");
        ok(!hud.ui.filterPrefs[prefKey], prefKey + " messages are " +
           "off after clicking the button");
      } else {
        ok(isChecked(menuItem), "menu item " + prefKey + " for category " +
           category + " is checked after clicking the button");
        ok(hud.ui.filterPrefs[prefKey], prefKey + " messages are " +
           "on after clicking the button");
      }
    }
    menuItem = menuItem.nextSibling;
  }

  
  menuItem = firstMenuItem;
  while (menuItem) {
    
    
    prefKey = menuItem.getAttribute("prefKey");
    if (prefKey && prefKey != "csslog") {
      chooseMenuItem(menuItem);
    }
    menuItem = menuItem.nextSibling;
  }

  ok(!isChecked(button), "the button for category " + category + " is " +
     "unchecked after unchecking all its filters");

  
  clickButton(subbutton);
}

function testIsolateFilterButton(category) {
  let selector = ".webconsole-filter-button[category=\"" + category + "\"]";
  let targetButton = hudBox.querySelector(selector);
  ok(targetButton, "we have the \"" + category + "\" button");

  
  let subbutton = getMainButton(targetButton);
  ok(subbutton, "we have the subbutton for category " + category);

  
  altClickButton(subbutton);
  ok(isChecked(targetButton), "the button for category " + category +
     " is checked after isolating for filter");

  
  let menuItems = targetButton.querySelectorAll("menuitem");
  Array.forEach(menuItems, (item) => {
    let prefKey = item.getAttribute("prefKey");
    
    if (category == "css" && prefKey == "csslog") {
      ok(!isChecked(item), "menu item " + prefKey + " for category " +
        category + " should not be checked after isolating for " + category);
      ok(!hud.ui.filterPrefs[prefKey], prefKey + " messages should be " +
        "turned off after isolating for " + category);
    } else if (prefKey) {
      ok(isChecked(item), "menu item " + prefKey + " for category " +
        category + " is checked after isolating for " + category);
      ok(hud.ui.filterPrefs[prefKey], prefKey + " messages are " +
        "turned on after isolating for " + category);
    }
  });

  
  
  let buttons = hudBox.querySelectorAll(".webconsole-filter-button[category]");
  Array.forEach(buttons, (filterButton) => {
    if (filterButton !== targetButton) {
      let categoryBtn = filterButton.getAttribute("category");
      ok(!isChecked(filterButton), "the button for category " +
         categoryBtn + " is unchecked after isolating for " + category);

      menuItems = filterButton.querySelectorAll("menuitem");
      Array.forEach(menuItems, (item) => {
        let prefKey = item.getAttribute("prefKey");
        if (prefKey) {
          ok(!isChecked(item), "menu item " + prefKey + " for category " +
            category + " is unchecked after isolating for " + category);
          ok(!hud.ui.filterPrefs[prefKey], prefKey + " messages are " +
            "turned off after isolating for " + category);
        }
      });

      
      let mainButton = getMainButton(filterButton);
      clickButton(mainButton);
    }
  });
}




function getMainButton(targetButton) {
  let anonymousNodes = hud.ui.document.getAnonymousNodes(targetButton);
  let subbutton;

  for (let i = 0; i < anonymousNodes.length; i++) {
    let node = anonymousNodes[i];
    if (node.classList.contains("toolbarbutton-menubutton-button")) {
      subbutton = node;
      break;
    }
  }

  return subbutton;
}

function clickButton(node) {
  EventUtils.sendMouseEvent({ type: "click" }, node);
}

function altClickButton(node) {
  EventUtils.sendMouseEvent({ type: "click", altKey: true }, node);
}

function chooseMenuItem(node) {
  let event = document.createEvent("XULCommandEvent");
  event.initCommandEvent("command", true, true, window, 0, false, false, false,
                         false, null);
  node.dispatchEvent(event);
}

function isChecked(node) {
  return node.getAttribute("checked") === "true";
}
