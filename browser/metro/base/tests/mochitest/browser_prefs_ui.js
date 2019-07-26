




"use strict";

var SanitizeHelper = {
  _originalSanitizer: null,

  MockSanitizer: {
    clearCalled: [],
    clearItem: function clearItem(aItemName) {
      info("Clear item called for: " + aItemName);
      this.clearCalled.push(aItemName);
    }
  },

  setUp: function setUp() {
    SanitizeHelper._originalSanitizer = SanitizeUI._sanitizer;
    SanitizeUI._sanitizer = SanitizeHelper.MockSanitizer;
  },

  tearDown: function tearDown() {
    SanitizeUI._sanitizer = SanitizeHelper._originalSanitizer;
  },
};

function getAllSelected() {
  return document.getElementById("prefs-privdata").querySelectorAll(
    "#prefs-privdata-history[checked], " +
      "#prefs-privdata-other[checked] + #prefs-privdata-subitems .privdata-subitem-item[checked]");
}

gTests.push({
  tearDown: SanitizeHelper.tearDown,
  desc: "Test sanitizer UI",
  run: function testSanitizeUI() {
    
    let promise = waitForEvent(Elements.prefsFlyout, "PopupChanged", 2000);
    Elements.prefsFlyout.show();
    yield promise;

    
    yield waitForEvent(Elements.prefsFlyout, "transitionend", 1000);

    SanitizeHelper.setUp();

    
    let allSelected = getAllSelected();
    
    ok(allSelected.length === 1 && allSelected[0].getAttribute("itemName") === "history", "History is initially selected.");

    let othersCheck = document.getElementById("prefs-privdata-other");
    othersCheck.setAttribute("checked", "true");

    let othersSubitems = document.getElementById("prefs-privdata-subitems");
    yield waitForCondition(function (){
      return othersSubitems.style.display !== "none";
    }, 500);

    allSelected = getAllSelected();
    
    ok(allSelected.length === 1 + othersSubitems.querySelectorAll("checkbox").length,
      "All checkboxes are selected.");

    
    let callItems = ["downloads", "passwords"];
    for (let checkbox of allSelected) {
      if (callItems.indexOf(checkbox.getAttribute("itemName")) === -1) {
        checkbox.removeAttribute("checked");
      }
    }

    let clearButton = document.getElementById("prefs-clear-data");
    clearButton.doCommand();

    let clearNotificationDeck = document.getElementById("clear-notification");
    let clearNotificationDone = document.getElementById("clear-notification-done");

    
    yield waitForCondition(function (){
      return clearNotificationDeck.selectedPanel == clearNotificationDone;
    }, 1000);

    ok(SanitizeHelper.MockSanitizer.clearCalled.length === callItems.length, "All expected items were called");

    SanitizeHelper.MockSanitizer.clearCalled.forEach(function(item) {
      ok(callItems.indexOf(item) >= 0, "Sanitized " + item);
    });

    
    let promise = waitForEvent(Elements.prefsFlyout, "PopupChanged", 2000);
    Elements.prefsFlyout.hide();
    yield promise;
  }
});

function checkDNTPrefs(aExpectedEnabled, aExpectedValue) {
  let currentEnabled = Services.prefs.getBoolPref("privacy.donottrackheader.enabled");
  let currentValue = Services.prefs.getIntPref("privacy.donottrackheader.value");

  let enabledTestMsg = "testing privacy.donottrackheader.enabled, expected "
    + aExpectedEnabled + " got " + currentEnabled;

  ok(aExpectedEnabled === currentEnabled, enabledTestMsg);

  let valueTestMsg = "testing privacy.donottrackheader.value, expected "
    + aExpectedValue + " got " + currentValue;

  ok(aExpectedValue === currentValue, valueTestMsg);
}

gTests.push({
  desc: "Test do not track settings",
  run: function testDNT() {
    let noTrack = document.getElementById("prefs-dnt-notrack");
    let noPref = document.getElementById("prefs-dnt-nopref");
    let okTrack = document.getElementById("prefs-dnt-oktrack");

    
    let promise = waitForEvent(Elements.prefsFlyout, "PopupChanged", 2000);
    Elements.prefsFlyout.show();
    yield promise;

    noPref.click();
    
    
    yield waitForCondition(() => Services.prefs.getIntPref("privacy.donottrackheader.value") === -1);
    checkDNTPrefs(false, -1);

    noTrack.click();
    
    yield waitForCondition(() => Services.prefs.getIntPref("privacy.donottrackheader.value") === 1);
    checkDNTPrefs(true, 1);

    okTrack.click();
    
    yield waitForCondition(() => Services.prefs.getIntPref("privacy.donottrackheader.value") === 0);
    checkDNTPrefs(true, 0);

    
    let promise = waitForEvent(Elements.prefsFlyout, "PopupChanged", 2000);
    Elements.prefsFlyout.hide();
    yield promise;
  }
});

function test() {
  runTests();
}
