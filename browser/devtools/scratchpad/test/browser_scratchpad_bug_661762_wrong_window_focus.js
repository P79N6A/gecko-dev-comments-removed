



let tempScope = {};
Cu.import("resource:///modules/HUDService.jsm", tempScope);
let HUDService = tempScope.HUDService;

function test()
{
  waitForExplicitFinish();

  
  
  
  
  
  
  
  
  
  
  

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);

    openScratchpad(function () {
      let sw = gScratchpadWindow;

      openScratchpad(function () {
        function onWebConsoleOpen(subj) {
          Services.obs.removeObserver(onWebConsoleOpen,
            "web-console-created");
          subj.QueryInterface(Ci.nsISupportsString);

          let hud = HUDService.getHudReferenceById(subj.data);
          hud.jsterm.clearOutput(true);
          executeSoon(testFocus.bind(null, sw, hud));
        }

        Services.obs.
          addObserver(onWebConsoleOpen, "web-console-created", false);

        HUDService.consoleUI.toggleHUD();
      });
    });
  }, true);

  content.location = "data:text/html;charset=utf8,<p>test window focus for Scratchpad.";
}

function testFocus(sw, hud) {
  let sp = sw.Scratchpad;

  function onMessage(subj) {
    Services.obs.removeObserver(onMessage, "web-console-message-created");

    var loc = hud.jsterm.outputNode.querySelector(".webconsole-location");
    ok(loc, "location element exists");
    is(loc.value, sw.Scratchpad.uniqueName + ":1",
        "location value is correct");

    sw.addEventListener("focus", function onFocus() {
      sw.removeEventListener("focus", onFocus, true);

      let win = Services.wm.getMostRecentWindow("devtools:scratchpad");

      ok(win, "there are active Scratchpad windows");
      is(win.Scratchpad.uniqueName, sw.Scratchpad.uniqueName,
          "correct window is in focus");

      
      
      sw.close();
      finish();
    }, true);

    
    EventUtils.synthesizeMouse(loc, 2, 2, {}, hud.iframeWindow);
  }

  
  
  Services.obs.addObserver(onMessage, "web-console-message-created", false);

  sp.setText("console.log('foo');");
  let [selection, error, result] = sp.run();
  is(selection, "console.log('foo');", "selection is correct");
  is(error, undefined, "error is correct");
  is(result, undefined, "result is correct");
}
