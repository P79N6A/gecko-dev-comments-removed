





let tmp = {};
Cu.import("resource://gre/modules/Promise.jsm", tmp);
Cu.import("resource://gre/modules/Task.jsm", tmp);
Cu.import("resource:///modules/CustomizableUI.jsm", tmp);
let {Promise, Task, CustomizableUI} = tmp;

let ChromeUtils = {};
let scriptLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"].getService(Ci.mozIJSSubScriptLoader);
scriptLoader.loadSubScript("chrome://mochikit/content/tests/SimpleTest/ChromeUtils.js", ChromeUtils);

let {synthesizeDragStart, synthesizeDrop} = ChromeUtils;

const kNSXUL = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

function createDummyXULButton(id, label) {
  let btn = document.createElementNS(kNSXUL, "toolbarbutton");
  btn.id = id;
  btn.setAttribute("label", label || id);
  btn.className = "toolbarbutton-1 chromeclass-toolbar-additional";
  window.gNavToolbox.palette.appendChild(btn);
  return btn;
}

function createToolbarWithPlacements(id, placements) {
  let tb = document.createElementNS(kNSXUL, "toolbar");
  tb.id = id;
  tb.setAttribute("customizable", "true");
  document.getElementById("customToolbars").appendChild(tb);
  CustomizableUI.registerArea(id, {
    type: CustomizableUI.TYPE_TOOLBAR,
    defaultPlacements: placements
  });
}

function removeCustomToolbars() {
  let customToolbarSet = document.getElementById("customToolbars");
  while (customToolbarSet.lastChild) {
    CustomizableUI.unregisterArea(customToolbarSet.lastChild.id);
    customToolbarSet.lastChild.remove();
  }
}

function resetCustomization() {
  return CustomizableUI.reset();
}

function assertAreaPlacements(areaId, expectedPlacements) {
  let actualPlacements = getAreaWidgetIds(areaId);
  is(actualPlacements.length, expectedPlacements.length,
     "Area " + areaId + " should have " + expectedPlacements.length + " items.");
  let minItems = Math.min(expectedPlacements.length, actualPlacements.length);
  for (let i = 0; i < minItems; i++) {
    if (typeof expectedPlacements[i] == "string") {
      is(actualPlacements[i], expectedPlacements[i],
         "Item " + i + " in " + areaId + " should match expectations.");
    } else if (expectedPlacements[i] instanceof RegExp) {
      ok(expectedPlacements[i].test(actualPlacements[i]),
         "Item " + i + " (" + actualPlacements[i] + ") in " +
         areaId + " should match " + expectedPlacements[i]); 
    } else {
      ok(false, "Unknown type of expected placement passed to " +
                " assertAreaPlacements. Is your test broken?");
    }
  }
}

function todoAssertAreaPlacements(areaId, expectedPlacements) {
  let actualPlacements = getAreaWidgetIds(areaId);
  let isPassing = actualPlacements.length == expectedPlacements.length;
  let minItems = Math.min(expectedPlacements.length, actualPlacements.length);
  for (let i = 0; i < minItems; i++) {
    if (typeof expectedPlacements[i] == "string") {
      isPassing = isPassing && actualPlacements[i] == expectedPlacements[i];
    } else if (expectedPlacements[i] instanceof RegExp) {
      isPassing = isPassing && expectedPlacements[i].test(actualPlacements[i]);
    } else {
      ok(false, "Unknown type of expected placement passed to " +
                " assertAreaPlacements. Is your test broken?");
    }
  }
  todo(isPassing, "The area placements for " + areaId +
                  " should equal the expected placements.")
}

function getAreaWidgetIds(areaId) {
  return CustomizableUI.getWidgetIdsInArea(areaId);
}

function simulateItemDrag(toDrag, target) {
  let docId = toDrag.ownerDocument.documentElement.id;
  let dragData = [[{type: 'text/toolbarwrapper-id/' + docId,
                    data: toDrag.id}]];
  synthesizeDragStart(toDrag.parentNode, dragData);
  synthesizeDrop(target, target, dragData);
}

function endCustomizing() {
  let deferredEndCustomizing = Promise.defer();
  function onCustomizationEnds() {
    window.gNavToolbox.removeEventListener("aftercustomization", onCustomizationEnds);
    deferredEndCustomizing.resolve();
  }
  window.gNavToolbox.addEventListener("aftercustomization", onCustomizationEnds);
  window.gCustomizeMode.exit();

  return deferredEndCustomizing.promise.then(function() {
    let deferredLoadNewTab = Promise.defer();

    
    let newTabBrowser = window.gBrowser.selectedBrowser;
    newTabBrowser.stop();

    
    if (newTabBrowser.contentDocument.location.href == "about:blank") {
      return;
    }

    
    function onNewTabLoaded(e) {
      newTabBrowser.removeEventListener("load", onNewTabLoaded, true);
      deferredLoadNewTab.resolve();
    }
    newTabBrowser.addEventListener("load", onNewTabLoaded, true);
    newTabBrowser.contentDocument.location.replace("about:blank");
    return deferredLoadNewTab.promise;
  });
}

function startCustomizing() {
  if (document.documentElement.getAttribute("customizing") == "true") {
    return;
  }
  let deferred = Promise.defer();
  function onCustomizing() {
    window.gNavToolbox.removeEventListener("customizationready", onCustomizing);
    deferred.resolve();
  }
  window.gNavToolbox.addEventListener("customizationready", onCustomizing);
  window.gCustomizeMode.enter();
  return deferred.promise;
}

function openAndLoadWindow(aOptions, aWaitForDelayedStartup=false) {
  let deferred = Promise.defer();
  let win = OpenBrowserWindow(aOptions);
  if (aWaitForDelayedStartup) {
    Services.obs.addObserver(function onDS(aSubject, aTopic, aData) {
      if (aSubject != win) {
        return;
      }
      Services.obs.removeObserver(onDS, "browser-delayed-startup-finished");
      deferred.resolve(win);
    }, "browser-delayed-startup-finished", false);

  } else {
    win.addEventListener("load", function onLoad() {
      win.removeEventListener("load", onLoad);
      deferred.resolve(win);
    });
  }
  return deferred.promise;
}

function waitForCondition(aConditionFn, aMaxTries=50, aCheckInterval=100) {
  function tryNow() {
    tries++;
    if (aConditionFn()) {
      deferred.resolve();
    } else if (tries < aMaxTries) {
      tryAgain();
    } else {
      deferred.reject("Condition timed out: " + aConditionFn.toSource());
    }
  }
  function tryAgain() {
    setTimeout(tryNow, aCheckInterval);
  }
  let deferred = Promise.defer();
  let tries = 0;
  tryAgain();
  return deferred.promise;
}

function testRunner(testAry, asyncCleanup) {
  Services.prefs.setBoolPref("browser.uiCustomization.disableAnimation", true);
  for (let test of testAry) {
    info(test.desc);

    if (test.setup)
      yield test.setup();

    info("Running test");
    yield test.run();
    info("Cleanup");
    ok(!document.getElementById(CustomizableUI.AREA_NAVBAR).hasAttribute("overflowing"), "Shouldn't overflow");
    if (test.teardown)
      yield test.teardown();
  }
  if (asyncCleanup) {
    yield asyncCleanup();
  }
  ok(CustomizableUI.inDefaultState, "Should remain in default state");
  Services.prefs.clearUserPref("browser.uiCustomization.disableAnimation");
}

function runTests(testAry, asyncCleanup) {
  Task.spawn(testRunner(gTests, asyncCleanup)).then(finish, ex => {
    
    
    ok(false, "Unexpected exception: " + ex + " With stack: " + ex.stack);
    finish();
  }).then(null, Cu.reportError);
}
