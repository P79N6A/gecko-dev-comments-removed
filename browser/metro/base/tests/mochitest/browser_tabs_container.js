




"use strict";

function test() {
  runTests();
}

function setup() {
  yield waitForCondition(function () {
    return Elements.tabList && Elements.tabList.strip;
  });

  if (!ContextUI.tabbarVisible) {
    ContextUI.displayTabs();
  }
}
function tearDown() {
  cleanUpOpenedTabs();
}

function isElementVisible(elm) {
  return elm.ownerDocument.defaultView.getComputedStyle(elm).getPropertyValue("visibility") == "visible";
}


gTests.push({
  desc: "No scrollbuttons when tab strip doesnt require overflow",
  setUp: setup,
  run: function () {
    let tabStrip = Elements.tabList.strip;
    let tabs = Elements.tabList.strip.querySelectorAll("documenttab");

    
    is(tabs.length, 1, "1 tab present");

    
    let imprecisePromise = waitForObserver("metro_imprecise_input");
    notifyImprecise();
    yield imprecisePromise;

    ok(!isElementVisible(tabStrip._scrollButtonUp), "left scrollbutton is hidden in imprecise mode");
    ok(!isElementVisible(tabStrip._scrollButtonDown), "right scrollbutton is hidden in imprecise mode");

    
    let precisePromise = waitForObserver("metro_precise_input");
    notifyPrecise();
    yield precisePromise;

    todo(!isElementVisible(tabStrip._scrollButtonUp), "Bug 952297 - left scrollbutton is hidden in precise mode");
    ok(!isElementVisible(tabStrip._scrollButtonDown), "right scrollbutton is hidden in precise mode");
  },
  tearDown: tearDown
});

gTests.push({
  desc: "Scrollbuttons not visible when tabstrip has overflow in imprecise input mode",
  setUp: function(){
    yield setup();
    
    let imprecisePromise = waitForObserver("metro_imprecise_input");
    notifyImprecise();
    yield imprecisePromise;
  },
  run: function () {
    
    let strip = Elements.tabList.strip;
    ok(strip && strip.scrollClientSize && strip.scrollSize, "Sanity check tabstrip strip is present and expected properties available");

    while (strip.scrollSize <= strip.scrollClientSize) {
      yield addTab("about:mozilla");
    }

    ok(!isElementVisible(Elements.tabList.strip._scrollButtonUp), "left scrollbutton is hidden in imprecise mode");
    ok(!isElementVisible(Elements.tabList.strip._scrollButtonDown), "right scrollbutton is hidden in imprecise mode");

  }
});

gTests.push({
  desc: "Scrollbuttons become visible when tabstrip has overflow in precise input mode",
  setUp: function(){
    yield setup();
    
    let precisePromise = waitForObserver("metro_precise_input");
    notifyPrecise();
    yield precisePromise;
  },
  run: function () {
    let strip = Elements.tabList.strip;
    ok(strip && strip.scrollClientSize && strip.scrollSize, "Sanity check tabstrip is present and expected properties available");

    
    while (strip.scrollSize <= strip.scrollClientSize) {
      yield addTab("about:mozilla");
    }

    let tabs = Elements.tabList.strip.querySelectorAll("documenttab");
    
    Elements.tabs.selectedTab = tabs[0];
    ok(isElementVisible(Elements.tabList.strip._scrollButtonDown), "right scrollbutton should be visible when tabList has overflow");
    todo(!isElementVisible(Elements.tabList.strip._scrollButtonUp), "Bug 952297 - left scrollbutton should not visible when 1st tab is selected and tablist has overflow");


    
    Elements.tabs.selectedTab = tabs[tabs.length-1];
    ok(isElementVisible(Elements.tabList.strip._scrollButtonUp), "left scrollbutton should be visible when tablist has overflow and last tab is selected");
    todo(!isElementVisible(Elements.tabList.strip._scrollButtonDown), "Bug 952297 - right scrollbutton should not visible when last tab is selected and tablist has overflow");

  }
});
