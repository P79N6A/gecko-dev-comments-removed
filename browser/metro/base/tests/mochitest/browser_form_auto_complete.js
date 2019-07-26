




"use strict";

function clearFormHistory() {
  var formHistory = Cc["@mozilla.org/satchel/form-history;1"].getService(Ci.nsIFormHistory2);
  formHistory.removeAllEntries();
}

function test() {
  clearFormHistory();
  runTests();
  clearFormHistory();
}

function setUp() {
  PanelUI.hide();
  yield hideContextUI();
}

function tearDown() {
  PanelUI.hide();
}

function checkAutofillMenuItemContents(aItemList)
{
  let errors = 0;
  let found = 0;
  for (let idx = 0; idx < AutofillMenuUI.commands.childNodes.length; idx++) {
    let item = AutofillMenuUI.commands.childNodes[idx];
    let label = item.firstChild.getAttribute("value");
    let value = item.getAttribute("data");
    if (aItemList.indexOf(value) == -1) {
      errors++;
      info("unexpected entry:" + value);
    } else {
      found++;
    }
  }
  is(errors, 0, "autofill menu item list error check");
  is(found, aItemList.length, "autofill item list length mismatch, some items were not found.");
}

gTests.push({
  desc: "simple auto complete test to insure auto complete code doesn't break.",
  setUp: setUp,
  tearDown: tearDown,
  run: function () {
    let loadedPromise, shownPromise;

    yield addTab(chromeRoot + "browser_form_auto_complete.html");
    yield waitForCondition(function () {
      return !Browser.selectedTab.isLoading();
    });

    let tabDocument = Browser.selectedTab.browser.contentWindow.document;
    let form = tabDocument.getElementById("form1");
    let input = tabDocument.getElementById("textedit1");

    input.value = "hellothere";
    form.action = chromeRoot + "browser_form_auto_complete.html";

    loadedPromise = waitForEvent(Browser.selectedTab.browser, "DOMContentLoaded");
    form.submit();
    yield loadedPromise;

    
    
    yield waitForMs(500);

    tabDocument = Browser.selectedTab.browser.contentWindow.document;
    input = tabDocument.getElementById("textedit1");
    ok(input, "input isn't null");
    input.focus();

    
    
    shownPromise = waitForEvent(document, "popupshown");
    EventUtils.synthesizeMouseAtCenter(input, {}, Browser.selectedTab.browser.contentWindow);
    EventUtils.synthesizeMouseAtCenter(input, {}, Browser.selectedTab.browser.contentWindow);
    yield shownPromise;

    checkAutofillMenuItemContents(["hellothere", "one", "two", "three", "four", "five"]);
  }
});