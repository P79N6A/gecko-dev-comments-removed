




function test()
{
  addTab("data:text/html;charset=utf-8,Web Console test for reflow activity");

  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(gBrowser.selectedTab, function(hud) {

      function onReflowListenersReady(aType, aPacket) {
        browser.contentDocument.body.style.display = "none";
        browser.contentDocument.body.clientTop;
      }

      Services.prefs.setBoolPref("devtools.webconsole.filter.csslog", true);
      hud.ui._updateReflowActivityListener(onReflowListenersReady);
      Services.prefs.clearUserPref("devtools.webconsole.filter.csslog");

      waitForMessages({
        webconsole: hud,
        messages: [{
          text: /reflow: /,
          category: CATEGORY_CSS,
          severity: SEVERITY_LOG,
        }],
      }).then(() => {
        finishTest();
      });
    });
  }, true);
}
