









function makeTimelineTest(frameScriptName, url) {
  info("in timelineTest");
  return Task.async(function*() {
    info("in in timelineTest");
    waitForExplicitFinish();

    yield timelineTestOpenUrl(url);

    const here = "chrome://mochitests/content/browser/docshell/test/browser/";

    let mm = gBrowser.selectedBrowser.messageManager;
    mm.loadFrameScript(here + "frame-head.js", false);
    mm.loadFrameScript(here + frameScriptName, false);

    
    
    mm.addMessageListener("browser:test:ok", function(message) {
      ok(message.data.value, message.data.message);
    });
    mm.addMessageListener("browser:test:info", function(message) {
      info(message.data.message);
    });
    mm.addMessageListener("browser:test:finish", function(ignore) {
      finish();
      gBrowser.removeCurrentTab();
    });
  });
}


function timelineTestOpenUrl(url) {
  return new Promise(function(resolve, reject) {
    window.focus();

    let tab = window.gBrowser.selectedTab = window.gBrowser.addTab(url);
    let linkedBrowser = tab.linkedBrowser;

    linkedBrowser.addEventListener("load", function onload() {
      linkedBrowser.removeEventListener("load", onload, true);
      resolve(tab);
    }, true);
  });
}
