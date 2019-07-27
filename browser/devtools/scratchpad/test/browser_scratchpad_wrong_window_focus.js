





function test()
{
  waitForExplicitFinish();

  
  
  
  
  
  
  
  
  
  
  

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);

    openScratchpad(function () {
      let sw = gScratchpadWindow;
      let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});

      openScratchpad(function () {
        let target = devtools.TargetFactory.forTab(gBrowser.selectedTab);
        gDevTools.showToolbox(target, "webconsole").then((toolbox) => {
          let hud = toolbox.getCurrentPanel().hud;
          hud.jsterm.clearOutput(true);
          testFocus(sw, hud);
        });
      });
    });
  }, true);

  content.location = "data:text/html;charset=utf8,<p>test window focus for Scratchpad.";
}

function testFocus(sw, hud) {
  let sp = sw.Scratchpad;

  function onMessage(event, messages) {
    let msg = [...messages][0];
    let node = msg.node;

    var loc = node.querySelector(".message-location");
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

  
  
  hud.ui.once("new-messages", onMessage);

  sp.setText("console.log('foo');");
  sp.run().then(function ([selection, error, result]) {
    is(selection, "console.log('foo');", "selection is correct");
    is(error, undefined, "error is correct");
    is(result.type, "undefined", "result is correct");
  });
}
