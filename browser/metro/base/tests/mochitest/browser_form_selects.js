




"use strict";

function test() {
  runTests();
}

gTests.push({
  desc: "form multi-select test 1",
  setUp: function () {
  },
  tearDown: function () {
  },
  run: function () {
    yield addTab(chromeRoot + "browser_form_selects.html");
    yield waitForCondition(function () {
      return !Browser.selectedTab.isLoading();
    });

    let win = Browser.selectedTab.browser.contentWindow;
    let tabdoc = Browser.selectedTab.browser.contentWindow.document;
    let select = tabdoc.getElementById("selectelement");

    
    let promise = waitForEvent(tabdoc, "popupshown");
    sendNativeTap(select);
    yield promise;

    
    for (let node of SelectHelperUI._listbox.childNodes) {
      sendNativeTap(node);
    }

    yield waitForMs(100);

    
    for (let node of SelectHelperUI._listbox.childNodes) {
      ok(node.selected, "option is selected");
    }

    
    for (let index = 1; index < 10; index++) {
      let option = tabdoc.getElementById("opt" + index);
      ok(option.selected, "opt" + index + " form option selected");
    }

    
    for (let node of SelectHelperUI._listbox.childNodes) {
      sendNativeTap(node);
    }

    yield waitForMs(100);

    
    for (let node of SelectHelperUI._listbox.childNodes) {
      ok(!node.selected, "option is not selected");
    }

    
    for (let index = 1; index < 10; index++) {
      let option = tabdoc.getElementById("opt" + index);
      ok(!option.selected, "opt" + index + " form option not selected");
    }

  }
});

