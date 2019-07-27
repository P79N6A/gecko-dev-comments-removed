


function test() {
  waitForExplicitFinish();

  let newTab = gBrowser.selectedTab = gBrowser.addTab("about:blank", {skipAnimation: true});
  registerCleanupFunction(function () {
    gBrowser.removeTab(newTab);
  });

  let scriptLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
                     getService(Ci.mozIJSSubScriptLoader);
  let ChromeUtils = {};
  scriptLoader.loadSubScript("chrome://mochikit/content/tests/SimpleTest/ChromeUtils.js", ChromeUtils);

  let tabContainer = gBrowser.tabContainer;
  var receivedDropCount = 0;
  function dropListener() {
    receivedDropCount++;
    if (receivedDropCount == triggeredDropCount) {
      is(openedTabs, validDropCount, "correct number of tabs were opened");
      executeSoon(finish);
    }
  }
  tabContainer.addEventListener("drop", dropListener, false);
  registerCleanupFunction(function () {
    tabContainer.removeEventListener("drop", dropListener, false);
  });

  var openedTabs = 0;
  function tabOpenListener(e) {
    openedTabs++;
    let tab = e.target;
    executeSoon(function () {
      gBrowser.removeTab(tab);
    });
  }

  tabContainer.addEventListener("TabOpen", tabOpenListener, false);
  registerCleanupFunction(function () {
    tabContainer.removeEventListener("TabOpen", tabOpenListener, false);
  });

  var triggeredDropCount = 0;
  var validDropCount = 0;
  function drop(text, valid) {
    triggeredDropCount++;
    if (valid)
      validDropCount++;
    executeSoon(function () {
      
      
      
      
      
      
      var event = {
        clientX: 0,
        clientY: 0,
        screenX: 0,
        screenY: 0,
      };
      ChromeUtils.synthesizeDrop(newTab, newTab, [[{type: "text/plain", data: text}]], "link", window, undefined, event);
    });
  }

  
  
  drop("mochi.test/first", true);
  drop("javascript:'bad'");
  drop("jAvascript:'bad'");
  drop("search this", true);
  drop("mochi.test/second", true);
  drop("data:text/html,bad");
  drop("mochi.test/third", true);
}
