



"use strict";

let openUILinkInCalled = false;
let expectOpenUILinkInCall = false;
this.originalOpenUILinkIn = openUILinkIn;
openUILinkIn = (aUrl, aWhichTab) => {
  is(aUrl, "about:home", "about:home should be requested to open.");
  is(aWhichTab, "current", "Should use the current tab for the search page.");
  openUILinkInCalled = true;
  if (!expectOpenUILinkInCall) {
    ok(false, "OpenUILinkIn was called when it shouldn't have been.");
  }
};
logActiveElement();

function* waitForSearchBarFocus()
{
  let searchbar = document.getElementById("searchbar");
  yield waitForCondition(function () {
    logActiveElement();
    return document.activeElement === searchbar.textbox.inputField;
  });
}


add_task(function() {
  let searchbar = document.getElementById("searchbar");
  gCustomizeMode.addToPanel(searchbar);
  let placement = CustomizableUI.getPlacementOfWidget("search-container");
  is(placement.area, CustomizableUI.AREA_PANEL, "Should be in panel");

  let shownPanelPromise = promisePanelShown(window);
  sendWebSearchKeyCommand();
  yield shownPanelPromise;

  yield waitForSearchBarFocus();

  let hiddenPanelPromise = promisePanelHidden(window);
  EventUtils.synthesizeKey("VK_ESCAPE", {});
  yield hiddenPanelPromise;
  CustomizableUI.reset();
});


add_task(function() {
  let searchbar = document.getElementById("searchbar");
  gCustomizeMode.addToPanel(searchbar);
  let placement = CustomizableUI.getPlacementOfWidget("search-container");
  is(placement.area, CustomizableUI.AREA_PANEL, "Should be in panel");

  let shownPanelPromise = promisePanelShown(window);
  PanelUI.toggle({type: "command"});
  yield shownPanelPromise;

  sendWebSearchKeyCommand();

  yield waitForSearchBarFocus();

  let hiddenPanelPromise = promisePanelHidden(window);
  EventUtils.synthesizeKey("VK_ESCAPE", {});
  yield hiddenPanelPromise;
  CustomizableUI.reset();
});


add_task(function() {
  this.originalWindowWidth = window.outerWidth;
  let navbar = document.getElementById(CustomizableUI.AREA_NAVBAR);
  ok(!navbar.hasAttribute("overflowing"), "Should start with a non-overflowing toolbar.");
  ok(CustomizableUI.inDefaultState, "Should start in default state.");

  window.resizeTo(360, window.outerHeight);
  yield waitForCondition(() => navbar.getAttribute("overflowing") == "true");
  ok(!navbar.querySelector("#search-container"), "Search container should be overflowing");

  let shownPanelPromise = promiseOverflowShown(window);
  sendWebSearchKeyCommand();
  yield shownPanelPromise;

  let chevron = document.getElementById("nav-bar-overflow-button");
  yield waitForCondition(function() chevron.open);

  yield waitForSearchBarFocus();

  let hiddenPanelPromise = promiseOverflowHidden(window);
  EventUtils.synthesizeKey("VK_ESCAPE", {});
  yield hiddenPanelPromise;
  navbar = document.getElementById(CustomizableUI.AREA_NAVBAR);
  window.resizeTo(this.originalWindowWidth, window.outerHeight);
  yield waitForCondition(() => !navbar.hasAttribute("overflowing"));
  ok(!navbar.hasAttribute("overflowing"), "Should not have an overflowing toolbar.");
});


add_task(function() {
  let placement = CustomizableUI.getPlacementOfWidget("search-container");
  is(placement.area, CustomizableUI.AREA_NAVBAR, "Should be in nav-bar");

  sendWebSearchKeyCommand();

  yield waitForSearchBarFocus();
});


add_task(function() {
  try {
    expectOpenUILinkInCall = true;
    CustomizableUI.removeWidgetFromArea("search-container");
    let placement = CustomizableUI.getPlacementOfWidget("search-container");
    is(placement, null, "Search container should be in palette");

    openUILinkInCalled = false;

    sendWebSearchKeyCommand();
    yield waitForCondition(function() openUILinkInCalled);
    ok(openUILinkInCalled, "The search page should have been opened.")
    expectOpenUILinkInCall = false;
  } catch (e) {
    ok(false, e);
  }
  CustomizableUI.reset();
});

registerCleanupFunction(function() {
  openUILinkIn = this.originalOpenUILinkIn;
  delete this.originalOpenUILinkIn;
});

function sendWebSearchKeyCommand() {
  if (Services.appinfo.OS === "Darwin")
    EventUtils.synthesizeKey("k", { accelKey: true });
  else
    EventUtils.synthesizeKey("k", { ctrlKey: true });
}

function logActiveElement() {
  let element = document.activeElement;
  let str = "";
  while (element && element.parentNode) {
    str = " (" + element.localName + "#" + element.id + "." + [...element.classList].join(".") + ") >" + str;
    element = element.parentNode;
  }
  info("Active element: " + element ? str : "null");
}
