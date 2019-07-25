









const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-bug-599725-response-headers.sjs";

let lastFinishedRequest = null;

function requestDoneCallback(aHttpRequest)
{
  lastFinishedRequest = aHttpRequest;
}

function performTest()
{
  ok(lastFinishedRequest, "page load was logged");

  let headers = lastFinishedRequest.response.header;
  ok(headers, "we have the response headers");
  ok(!headers["Content-Type"], "we do not have the Content-Type header");
  ok(headers["Content-Length"] != 60, "Content-Length != 60");

  lastFinishedRequest = null;
  HUDService.lastFinishedRequestCallback = null;
  finishTest();
}

function test()
{
  addTab(TEST_URI);

  browser.addEventListener("load", function(aEvent) {
    browser.removeEventListener(aEvent.type, arguments.callee, true);

    openConsole();

    HUDService.lastFinishedRequestCallback = requestDoneCallback;

    browser.addEventListener("load", performTest, true);
    content.location.reload();
  }, true);
}
