function test() {
  waitForExplicitFinish();
  ignoreAllUncaughtExceptions();
  
  
  testCustomize(window, testChromeless);
}

function testChromeless() {
  
  var newWin = openDialog("chrome://browser/content/", "_blank",
                      "chrome,dialog=no,toolbar=no", "about:blank");
  ok(newWin, "got new window");

  function runWindowTest() {
    
    var searchBar = newWin.BrowserSearch.searchBar;
    ok(searchBar, "got search bar");

    var searchBarBO = searchBar.boxObject;
    is(searchBarBO.width, 0, "search bar hidden");
    is(searchBarBO.height, 0, "search bar hidden");

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
  var fileMenu = aWindow.document.getElementById("file-menu");
  ok(fileMenu, "got file menu");
  is(fileMenu.disabled, false, "file menu initially enabled");

  
  
  var ctEl = aWindow.BrowserCustomizeToolbar();

  is(fileMenu.disabled, true,
     "file menu is disabled during toolbar customization");

  aWindow.gNavToolbox.addEventListener("beforecustomization", function () {
    aWindow.gNavToolbox.removeEventListener("beforecustomization", arguments.callee, false);
    executeSoon(ctInit);
  }, false);

  function ctInit() {
    
    closeToolbarCustomization(aWindow, ctEl);

    
    
    is(fileMenu.getAttribute("disabled"), "false",
       "file menu is enabled after toolbar customization");

    if (aCallback)
      aCallback();
  }
}

function closeToolbarCustomization(aWindow, aCTWindow) {
  
  
  aCTWindow.finishToolbarCustomization();

  
  if (!gCustomizeSheet)
    aCTWindow.close();
}
