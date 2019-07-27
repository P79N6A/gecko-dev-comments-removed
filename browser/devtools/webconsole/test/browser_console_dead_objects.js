













"use strict";

const TEST_URI = "data:text/html;charset=utf8,<p>dead objects!";

function test() {
  let hud = null;

  registerCleanupFunction(() => {
    Services.prefs.clearUserPref("devtools.chrome.enabled");
  });

  Task.spawn(runner).then(finishTest);

  function* runner() {
    Services.prefs.setBoolPref("devtools.chrome.enabled", true);
    yield loadTab(TEST_URI);

    info("open the browser console");

    hud = yield HUDService.toggleBrowserConsole();
    ok(hud, "browser console opened");

    let jsterm = hud.jsterm;

    jsterm.clearOutput();

    
    yield jsterm.execute("Cu = Components.utils;" +
                  "Cu.import('resource://gre/modules/Services.jsm');" +
                  "chromeWindow = Services.wm.getMostRecentWindow('" +
                  "navigator:browser');" +
                  "foobarzTezt = chromeWindow.content.document;" +
                  "delete chromeWindow");

    gBrowser.removeCurrentTab();

    let msg = yield jsterm.execute("foobarzTezt");

    isnot(hud.outputNode.textContent.indexOf("[object DeadObject]"), -1,
          "dead object found");

    jsterm.setInputValue("foobarzTezt");

    for (let c of ".hello") {
      EventUtils.synthesizeKey(c, {}, hud.iframeWindow);
    }

    yield jsterm.execute();

    isnot(hud.outputNode.textContent.indexOf("can't access dead object"), -1,
          "'cannot access dead object' message found");

    
    let clickable = msg.querySelector("a");
    ok(clickable, "clickable object found");
    isnot(clickable.textContent.indexOf("[object DeadObject]"), -1,
          "message text check");

    msg.scrollIntoView();

    executeSoon(() => {
      EventUtils.synthesizeMouseAtCenter(clickable, {}, hud.iframeWindow);
    });

    yield jsterm.once("variablesview-fetched");
    ok(true, "variables view fetched");

    msg = yield jsterm.execute("delete window.foobarzTezt; 2013-26");

    isnot(msg.textContent.indexOf("1987"), -1, "result message found");
  }
}
