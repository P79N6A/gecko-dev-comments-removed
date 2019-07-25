









const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-bug-600183-charset.html";

function performTest(lastFinishedRequest)
{
  ok(lastFinishedRequest, "charset test page was loaded and logged");

  let body = lastFinishedRequest.log.entries[0].response.content.text;
  ok(body, "we have the response body");

  let chars = "\u7684\u95ee\u5019!"; 
  isnot(body.indexOf("<p>" + chars + "</p>"), -1,
    "found the chinese simplified string");

  HUDService.lastFinishedRequestCallback = null;
  executeSoon(finishTest);
}

function test()
{
  addTab("data:text/html;charset=utf-8,Web Console - bug 600183 test");

  let initialLoad = true;

  browser.addEventListener("load", function onLoad() {
    if (initialLoad) {
      openConsole(null, function(hud) {

        hud.ui.saveRequestAndResponseBodies = true;
        HUDService.lastFinishedRequestCallback = performTest;

        content.location = TEST_URI;
      });
      initialLoad = false;
    } else {
      browser.removeEventListener("load", onLoad, true);
    }
  }, true);
}
