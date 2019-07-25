



let console = (function() {
  let tempScope = {};
  Components.utils.import("resource://gre/modules/devtools/Console.jsm", tempScope);
  return tempScope.console;
})();




function addTab(aURL, aCallback)
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  content.location = aURL;

  let tab = gBrowser.selectedTab;
  let browser = gBrowser.getBrowserForTab(tab);

  function onTabLoad() {
    browser.removeEventListener("load", onTabLoad, true);
    aCallback(browser, tab, browser.contentDocument);
  }

  browser.addEventListener("load", onTabLoad, true);
}

registerCleanupFunction(function tearDown() {
  while (gBrowser.tabs.length > 1) {
    gBrowser.removeCurrentTab();
  }

  console = undefined;
});







let DeveloperToolbarTest = {
  


  show: function DTT_show(aCallback) {
    if (DeveloperToolbar.visible) {
      ok(false, "DeveloperToolbar.visible at start of openDeveloperToolbar");
    }
    else {
      DeveloperToolbar.show(true, aCallback);
    }
  },

  


  hide: function DTT_hide() {
    if (!DeveloperToolbar.visible) {
      ok(false, "!DeveloperToolbar.visible at start of closeDeveloperToolbar");
    }
    else {
      DeveloperToolbar.display.inputter.setInput("");
      DeveloperToolbar.hide();
    }
  },

  












  test: function DTT_test(uri, testFunc) {
    let menuItem = document.getElementById("menu_devToolbar");
    let command = document.getElementById("Tools:DevToolbar");
    let appMenuItem = document.getElementById("appmenu_devToolbar");

    registerCleanupFunction(function() {
      DeveloperToolbarTest.hide();

      
      if (menuItem) menuItem.hidden = true;
      if (command) command.setAttribute("disabled", "true");
      if (appMenuItem) appMenuItem.hidden = true;
    });

    
    if (menuItem) menuItem.hidden = false;
    if (command) command.removeAttribute("disabled");
    if (appMenuItem) appMenuItem.hidden = false;

    addTab(uri, function(browser, tab) {
      DeveloperToolbarTest.show(function() {

        try {
          testFunc(browser, tab);
        }
        catch (ex) {
          ok(false, "" + ex);
          console.error(ex);
          finish();
          throw ex;
        }
      });
    });
  },
};

function catchFail(func) {
  return function() {
    try {
      return func.apply(null, arguments);
    }
    catch (ex) {
      ok(false, ex);
      console.error(ex);
      finish();
      throw ex;
    }
  };
}






























function waitForValue(aOptions)
{
  let start = Date.now();
  let timeout = aOptions.timeout || 5000;
  let lastValue;

  function wait(validatorFn, successFn, failureFn)
  {
    if ((Date.now() - start) > timeout) {
      
      ok(false, "Timed out while waiting for: " + aOptions.name);
      let expected = "value" in aOptions ?
                     "'" + aOptions.value + "'" :
                     "a trueish value";
      info("timeout info :: got '" + lastValue + "', expected " + expected);
      failureFn(aOptions, lastValue);
      return;
    }

    lastValue = validatorFn(aOptions, lastValue);
    let successful = "value" in aOptions ?
                      lastValue == aOptions.value :
                      lastValue;
    if (successful) {
      ok(true, aOptions.name);
      successFn(aOptions, lastValue);
    }
    else {
      setTimeout(function() wait(validatorFn, successFn, failureFn), 100);
    }
  }

  wait(aOptions.validator, aOptions.success, aOptions.failure);
}
