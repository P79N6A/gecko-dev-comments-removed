function test() {
  waitForExplicitFinish();
  
  
  testCustomize(window, testChromeless);
}

function testChromeless() {
  
  var newWin = openDialog("chrome://browser/content/", "_blank",
                      "chrome,dialog=no,toolbar=no", "about:blank");
  ok(newWin, "got new window");

  function runWindowTest() {
    function finalize() {
      newWin.removeEventListener("load", runWindowTest, false);
      newWin.close();
      finish();
    }
    testCustomize(newWin, finalize);
  }

  newWin.addEventListener("load", runWindowTest, false);
}

function testCustomize(aWindow, aCallback) {
  var addonBar = aWindow.document.getElementById("addon-bar");
  ok(addonBar, "got addon bar");
  is(addonBar.collapsed, true, "addon bar initially disabled");

  
  
  var ctEl = aWindow.BrowserCustomizeToolbar();

  is(addonBar.collapsed, false,
     "file menu is not collapsed during toolbar customization");

  aWindow.gNavToolbox.addEventListener("beforecustomization", function () {
    aWindow.gNavToolbox.removeEventListener("beforecustomization", arguments.callee, false);
    executeSoon(ctInit);
  }, false);

  function ctInit() {
    
    closeToolbarCustomization(aWindow, ctEl);

    is(addonBar.getAttribute("collapsed"), "true",
       "addon bar is collapsed after toolbar customization");

    if (aCallback)
      aCallback();
  }
}

function closeToolbarCustomization(aWindow, aCTWindow) {
  
  
  aCTWindow.finishToolbarCustomization();

  
  if (!gCustomizeSheet)
    aCTWindow.close();
}
