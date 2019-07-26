






let gTab, gDebuggee, gPanel, gDebugger;
let gVariablesView;

function test() {
  initDebugger("about:blank").then(([aTab, aDebuggee, aPanel]) => {
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;
    gVariablesView = gDebugger.DebuggerView.Variables;

    performTest().then(null, aError => {
      ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
    });
  });
}

function performTest() {
  let arr = [
    42,
    true,
    "nasu",
    undefined,
    null,
    [0, 1, 2],
    { prop1: 9, prop2: 8 }
  ];

  let obj = {
    p0: 42,
    p1: true,
    p2: "nasu",
    p3: undefined,
    p4: null,
    p5: [3, 4, 5],
    p6: { prop1: 7, prop2: 6 },
    get p7() { return arr; },
    set p8(value) { arr[0] = value }
  };

  let test = {
    someProp0: 42,
    someProp1: true,
    someProp2: "nasu",
    someProp3: undefined,
    someProp4: null,
    someProp5: arr,
    someProp6: obj,
    get someProp7() { return arr; },
    set someProp7(value) { arr[0] = value }
  };

  gVariablesView.eval = function() {};
  gVariablesView.switch = function() {};
  gVariablesView.delete = function() {};
  gVariablesView.rawObject = test;
  gVariablesView.pageSize = 5;

  return Task.spawn(function() {
    yield waitForTick();

    

    gVariablesView.focusFirstVisibleItem();
    is(gVariablesView.getFocusedItem().name, "someProp0",
      "The 'someProp0' item should be focused.");

    gVariablesView.focusNextItem();
    is(gVariablesView.getFocusedItem().name, "someProp1",
      "The 'someProp1' item should be focused.");

    gVariablesView.focusPrevItem();
    is(gVariablesView.getFocusedItem().name, "someProp0",
      "The 'someProp0' item should be focused.");

    

    yield synthesizeKeyAndWaitForTick("VK_DOWN", {});
    is(gVariablesView._parent.scrollTop, 0,
      "The 'variables' view shouldn't scroll when pressing the DOWN key.");

    yield synthesizeKeyAndWaitForTick("VK_UP", {});
    is(gVariablesView._parent.scrollTop, 0,
      "The 'variables' view shouldn't scroll when pressing the UP key.");

    

    yield synthesizeKeyAndWaitForElement("VK_ENTER", {}, ".element-value-input", true);
    yield synthesizeKeyAndWaitForElement("VK_ESCAPE", {}, ".element-value-input", false);
    yield synthesizeKeyAndWaitForElement("VK_ENTER", { shiftKey: true }, ".element-name-input", true);
    yield synthesizeKeyAndWaitForElement("VK_ESCAPE", {}, ".element-name-input", false);

    

    EventUtils.sendKey("DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp1",
      "The 'someProp1' item should be focused.");

    EventUtils.sendKey("UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp0",
      "The 'someProp0' item should be focused.");

    EventUtils.sendKey("PAGE_DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp5",
      "The 'someProp5' item should be focused.");

    EventUtils.sendKey("PAGE_UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp0",
      "The 'someProp0' item should be focused.");

    EventUtils.sendKey("END", gDebugger);
    is(gVariablesView.getFocusedItem().name, "__proto__",
      "The '__proto__' item should be focused.");

    EventUtils.sendKey("HOME", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp0",
      "The 'someProp0' item should be focused.");

    

    EventUtils.sendKey("DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp1",
      "The 'someProp1' item should be focused.");

    EventUtils.sendKey("DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp2",
      "The 'someProp2' item should be focused.");

    EventUtils.sendKey("UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp1",
      "The 'someProp1' item should be focused.");

    EventUtils.sendKey("UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp0",
      "The 'someProp0' item should be focused.");

    EventUtils.sendKey("PAGE_DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp5",
      "The 'someProp5' item should be focused.");

    EventUtils.sendKey("PAGE_DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "__proto__",
      "The '__proto__' item should be focused.");

    EventUtils.sendKey("PAGE_UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp5",
      "The 'someProp5' item should be focused.");

    EventUtils.sendKey("PAGE_UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp0",
      "The 'someProp0' item should be focused.");

    

    EventUtils.sendKey("HOME", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp0",
      "The 'someProp0' item should be focused.");

    EventUtils.sendKey("HOME", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp0",
      "The 'someProp0' item should be focused.");

    EventUtils.sendKey("PAGE_UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp0",
      "The 'someProp0' item should be focused.");

    EventUtils.sendKey("HOME", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp0",
      "The 'someProp0' item should be focused.");

    EventUtils.sendKey("END", gDebugger);
    is(gVariablesView.getFocusedItem().name, "__proto__",
      "The '__proto__' item should be focused.");

    EventUtils.sendKey("END", gDebugger);
    is(gVariablesView.getFocusedItem().name, "__proto__",
      "The '__proto__' item should be focused.");

    EventUtils.sendKey("PAGE_DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "__proto__",
      "The '__proto__' item should be focused.");

    EventUtils.sendKey("END", gDebugger);
    is(gVariablesView.getFocusedItem().name, "__proto__",
      "The '__proto__' item should be focused.");

    

    EventUtils.sendKey("PAGE_UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp5",
      "The 'someProp5' item should be focused.");

    EventUtils.sendKey("PAGE_UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp0",
      "The 'someProp0' item should be focused.");

    EventUtils.sendKey("UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp0",
      "The 'someProp0' item should be focused.");

    EventUtils.sendKey("UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp0",
      "The 'someProp0' item should be focused.");

    EventUtils.sendKey("PAGE_DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp5",
      "The 'someProp5' item should be focused.");

    EventUtils.sendKey("PAGE_DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "__proto__",
      "The '__proto__' item should be focused.");

    EventUtils.sendKey("PAGE_DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "__proto__",
      "The '__proto__' item should be focused.");

    EventUtils.sendKey("PAGE_DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "__proto__",
      "The '__proto__' item should be focused.");

    

    EventUtils.sendKey("HOME", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp0",
      "The 'someProp0' item should be focused.");

    EventUtils.sendKey("DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp1",
      "The 'someProp1' item should be focused.");

    EventUtils.sendKey("END", gDebugger);
    is(gVariablesView.getFocusedItem().name, "__proto__",
      "The '__proto__' item should be focused.");

    EventUtils.sendKey("DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "__proto__",
      "The '__proto__' item should be focused.");

    

    EventUtils.sendKey("PAGE_UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp5",
      "The 'someProp5' item should be focused.");
    is(gVariablesView.getFocusedItem().expanded, false,
      "The 'someProp5' item should not be expanded yet.");

    yield synthesizeKeyAndWaitForTick("VK_RIGHT", {});
    is(gVariablesView.getFocusedItem().name, "someProp5",
      "The 'someProp5' item should be focused.");
    is(gVariablesView.getFocusedItem().expanded, true,
      "The 'someProp5' item should now be expanded.");

    EventUtils.sendKey("RIGHT", gDebugger);
    is(gVariablesView.getFocusedItem().name, "0",
      "The '0' item should be focused.");

    EventUtils.sendKey("RIGHT", gDebugger);
    is(gVariablesView.getFocusedItem().name, "0",
      "The '0' item should still be focused.");

    EventUtils.sendKey("PAGE_DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "5",
      "The '5' item should be focused.");
    is(gVariablesView.getFocusedItem().expanded, false,
      "The '5' item should not be expanded yet.");

    yield synthesizeKeyAndWaitForTick("VK_RIGHT", {});
    is(gVariablesView.getFocusedItem().name, "5",
      "The '5' item should be focused.");
    is(gVariablesView.getFocusedItem().expanded, true,
      "The '5' item should now be expanded.");

    EventUtils.sendKey("RIGHT", gDebugger);
    is(gVariablesView.getFocusedItem().name, "0",
      "The '0' item should be focused.");

    EventUtils.sendKey("RIGHT", gDebugger);
    is(gVariablesView.getFocusedItem().name, "0",
      "The '0' item should still be focused.");

    EventUtils.sendKey("PAGE_DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "6",
      "The '6' item should be focused.");
    is(gVariablesView.getFocusedItem().expanded, false,
      "The '6' item should not be expanded yet.");

    

    EventUtils.sendKey("DOWN", gDebugger);
    EventUtils.sendKey("DOWN", gDebugger);
    EventUtils.sendKey("DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp6",
      "The 'someProp6' item should be focused.");

    EventUtils.sendKey("LEFT", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp6",
      "The 'someProp6' item should be focused.");

    EventUtils.sendKey("UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "__proto__",
      "The '__proto__' item should be focused.");

    EventUtils.sendKey("LEFT", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp5",
      "The 'someProp5' item should be focused.");
    is(gVariablesView.getFocusedItem().expanded, true,
      "The '6' item should still be expanded.");

    EventUtils.sendKey("LEFT", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp5",
      "The 'someProp5' item should still be focused.");
    is(gVariablesView.getFocusedItem().expanded, false,
      "The '6' item should still not be expanded anymore.");

    EventUtils.sendKey("LEFT", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp5",
      "The 'someProp5' item should still be focused.");

    

    EventUtils.sendKey("UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp4",
      "The 'someProp4' item should be focused.");

    EventUtils.sendKey("UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp3",
      "The 'someProp3' item should be focused.");

    EventUtils.sendKey("UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp2",
      "The 'someProp2' item should be focused.");

    EventUtils.sendKey("UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp1",
      "The 'someProp1' item should be focused.");

    EventUtils.sendKey("UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp0",
      "The 'someProp0' item should be focused.");

    EventUtils.sendKey("PAGE_UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp0",
      "The 'someProp0' item should be focused.");

    EventUtils.sendKey("PAGE_DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp5",
      "The 'someProp5' item should be focused.");

    EventUtils.sendKey("DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp6",
      "The 'someProp6' item should be focused.");

    EventUtils.sendKey("DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp7",
      "The 'someProp7' item should be focused.");

    EventUtils.sendKey("DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "get",
      "The 'get' item should be focused.");

    EventUtils.sendKey("DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "set",
      "The 'set' item should be focused.");

    EventUtils.sendKey("DOWN", gDebugger);
    is(gVariablesView.getFocusedItem().name, "__proto__",
      "The '__proto__' item should be focused.");

    

    EventUtils.sendKey("BACK_SPACE", gDebugger);
    is(gVariablesView.getFocusedItem().name, "__proto__",
      "The '__proto__' variable should still be focused.");
    is(gVariablesView.getFocusedItem().value, "[object Object]",
      "The '__proto__' variable should not have an empty value.");
    is(gVariablesView.getFocusedItem().visible, false,
      "The '__proto__' variable should be hidden.");

    EventUtils.sendKey("UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "set",
      "The 'set' item should be focused.");
    is(gVariablesView.getFocusedItem().value, "[object Object]",
      "The 'set' item should not have an empty value.");
    is(gVariablesView.getFocusedItem().visible, true,
      "The 'set' item should be visible.");

    EventUtils.sendKey("BACK_SPACE", gDebugger);
    is(gVariablesView.getFocusedItem().name, "set",
      "The 'set' item should still be focused.");
    is(gVariablesView.getFocusedItem().value, "[object Object]",
      "The 'set' item should not have an empty value.");
    is(gVariablesView.getFocusedItem().visible, true,
      "The 'set' item should be visible.");
    is(gVariablesView.getFocusedItem().twisty, false,
      "The 'set' item should be disabled and have a hidden twisty.");

    EventUtils.sendKey("UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "get",
      "The 'get' item should be focused.");
    is(gVariablesView.getFocusedItem().value, "[object Object]",
      "The 'get' item should not have an empty value.");
    is(gVariablesView.getFocusedItem().visible, true,
      "The 'get' item should be visible.");

    EventUtils.sendKey("BACK_SPACE", gDebugger);
    is(gVariablesView.getFocusedItem().name, "get",
      "The 'get' item should still be focused.");
    is(gVariablesView.getFocusedItem().value, "[object Object]",
      "The 'get' item should not have an empty value.");
    is(gVariablesView.getFocusedItem().visible, true,
      "The 'get' item should be visible.");
    is(gVariablesView.getFocusedItem().twisty, false,
      "The 'get' item should be disabled and have a hidden twisty.");

    EventUtils.sendKey("UP", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp7",
      "The 'someProp7' item should be focused.");
    is(gVariablesView.getFocusedItem().value, undefined,
      "The 'someProp7' variable should have an empty value.");
    is(gVariablesView.getFocusedItem().visible, true,
      "The 'someProp7' variable should be visible.");

    EventUtils.sendKey("BACK_SPACE", gDebugger);
    is(gVariablesView.getFocusedItem().name, "someProp7",
      "The 'someProp7' variable should still be focused.");
    is(gVariablesView.getFocusedItem().value, undefined,
      "The 'someProp7' variable should have an empty value.");
    is(gVariablesView.getFocusedItem().visible, false,
      "The 'someProp7' variable should be hidden.");

    yield closeDebuggerAndFinish(gPanel);
  });
}

registerCleanupFunction(function() {
  gTab = null;
  gDebuggee = null;
  gPanel = null;
  gDebugger = null;
  gVariablesView = null;
});

function synthesizeKeyAndWaitForElement(aKey, aModifiers, aSelector, aExistence) {
  EventUtils.synthesizeKey(aKey, aModifiers, gDebugger);
  return waitForElement(aSelector, aExistence);
}

function synthesizeKeyAndWaitForTick(aKey, aModifiers) {
  EventUtils.synthesizeKey(aKey, aModifiers, gDebugger);
  return waitForTick();
}

function waitForElement(aSelector, aExistence, aInterval = 10) {
  let deferred = promise.defer();

  
  let count = 0;
  let intervalID = window.setInterval(() => {
    
    if (++count > 1000) {
      deferred.reject("Timed out while polling for the element.");
      window.clearInterval(intervalID);
      return;
    }
    
    if (!!gVariablesView._list.querySelector(aSelector) != aExistence) {
      return;
    }
    
    window.clearInterval(intervalID);
    deferred.resolve();
  }, aInterval);

  return deferred.promise;
}
