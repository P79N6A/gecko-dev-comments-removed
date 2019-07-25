













const TEST_URI = "data:text/html,Web Console test for bug 595350";

let win1 = window, win2;
let openTabs = [];
let loadedTabCount = 0;

function test() {
  
  addTabs(win1);

  
  win2 = OpenBrowserWindow();
  win2.addEventListener("load", onWindowLoad, true);
}

function onWindowLoad(aEvent) {
  win2.removeEventListener(aEvent.type, arguments.callee, true);

  
  addTabs(win2);
}

function addTabs(aWindow) {
  for (let i = 0; i < 2; i++) {
    let tab = aWindow.gBrowser.addTab(TEST_URI);
    openTabs.push(tab);

    tab.linkedBrowser.addEventListener("load", function(aEvent) {
      tab.linkedBrowser.removeEventListener(aEvent.type, arguments.callee,
        true);

      loadedTabCount++;
      if (loadedTabCount >= 4) {
        executeSoon(performTest);
      }
    }, true);
  }
}

function performTest() {
  
  for (let i = 0; i < openTabs.length; i++) {
    let tab = openTabs[i];
    HUDService.activateHUDForContext(tab);
    let hudId = HUDService.getHudIdByWindow(tab.linkedBrowser.contentWindow);
    ok(hudId, "HUD is open for tab " + i);
    let HUD = HUDService.hudWeakReferences[hudId].get();
    HUD.console.log("message for tab " + i);
  }

  let displays = HUDService.displaysIndex();
  is(displays.length, 4, "four displays found");

  win2.close();

  executeSoon(function() {
    win1.gBrowser.removeTab(openTabs[0]);
    win1.gBrowser.removeTab(openTabs[1]);

    executeSoon(function() {
      displays = HUDService.displaysIndex();
      is(displays.length, 0, "no displays found");

      displays = openTabs = win1 = win2 = null;

      finishTest();
    });
  });
}

