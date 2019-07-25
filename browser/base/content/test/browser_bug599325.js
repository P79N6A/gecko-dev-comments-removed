function test() {
  waitForExplicitFinish();
  ignoreAllUncaughtExceptions();

  testCustomize(window, finish);
}

function testCustomize(aWindow, aCallback) {
  var addonBar = aWindow.document.getElementById("addon-bar");
  ok(addonBar, "got addon bar");
  ok(!isElementVisible(addonBar), "addon bar initially hidden");

  
  
  var ctEl = aWindow.BrowserCustomizeToolbar();

  aWindow.gNavToolbox.addEventListener("beforecustomization", function () {
    aWindow.gNavToolbox.removeEventListener("beforecustomization", arguments.callee, false);
    executeSoon(ctInit);
  }, false);

  function ctInit() {
    ok(isElementVisible(addonBar),
       "add-on bar is visible during toolbar customization");

    
    closeToolbarCustomization(aWindow, ctEl);

    ok(!isElementVisible(addonBar),
       "addon bar is hidden after toolbar customization");

    if (aCallback)
      aCallback();
  }
}

function closeToolbarCustomization(aWindow, aCTWindow) {
  
  
  aCTWindow.finishToolbarCustomization();

  
  if (!gCustomizeSheet)
    aCTWindow.close();
}
