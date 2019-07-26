




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

        HUDService.toggleWebConsole();
      });
    });
  }, true);

  content.location = "data:text/html;charset=utf8,<p>test window focus for Scratchpad.";
}

function testFocus(sw, hud) {
  let sp = sw.Scratchpad;

  function onMessage(event, messages) {
    let msg = [...messages][0];

    var loc = msg.querySelector(".location");
    ok(loc, "location element exists");
    is(loc.textContent.trim(), sw.Scratchpad.uniqueName + ":1",
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

  
  
  hud.ui.once("messages-added", onMessage);

  sp.setText("console.log('foo');");
  sp.run().then(function ([selection, error, result]) {
    is(selection, "console.log('foo');", "selection is correct");
    is(error, undefined, "error is correct");
    is(result.type, "undefined", "result is correct");
  });
}
