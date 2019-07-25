















const TEST_NETWORK_REQUEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-network-request.html";

const TEST_IMG = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-image.png";

const TEST_DATA_JSON_CONTENT =
  '{ id: "test JSON data", myArray: [ "foo", "bar", "baz", "biff" ] }';

function test()
{
  addTab("data:text/html,WebConsole network logging tests");

  browser.addEventListener("load", function() {
    browser.removeEventListener("load", arguments.callee, true);
    testOpenWebConsole();
  }, true);
}

function testOpenWebConsole()
{
  openConsole();
  is(HUDService.displaysIndex().length, 1, "WebConsole was opened");

  hudId = HUDService.displaysIndex()[0];
  hud = HUDService.getHeadsUpDisplay(hudId);

  testNetworkLogging();
}

function testNetworkLogging()
{
  var lastFinishedRequest = null;
  HUDService.lastFinishedRequestCallback =
    function requestDoneCallback(aHttpRequest)
    {
      lastFinishedRequest = aHttpRequest;
    };

  let loggingGen;
  
  function loggingGeneratorFunc() {
    browser.addEventListener("load", function onLoad () {
      browser.removeEventListener("load", onLoad, true);
      loggingGen.next();
    }, true);
    browser.contentWindow.wrappedJSObject.document.location =
      TEST_NETWORK_REQUEST_URI;
    yield;

    
    let httpActivity = lastFinishedRequest;
    isnot(httpActivity, null, "Page load was logged");
    is(httpActivity.url, TEST_NETWORK_REQUEST_URI,
      "Logged network entry is page load");
    is(httpActivity.method, "GET", "Method is correct");
    ok(!("body" in httpActivity.request), "No request body was stored");
    ok(!("body" in httpActivity.response), "No response body was stored");
    ok(!httpActivity.response.listener, "No response listener is stored");

    
    
    
    
    
    
    
    

    
    
    
    

    
    browser.contentWindow.wrappedJSObject.testXhrGet(loggingGen);
    yield;

    
    
    
    executeSoon(function() {
      
      httpActivity = lastFinishedRequest;
      isnot(httpActivity, null, "testXhrGet() was logged");
      is(httpActivity.method, "GET", "Method is correct");
      is(httpActivity.request.body, null, "No request body was sent");
      
      
      lastFinishedRequest = null;
      loggingGen.next();
    });
    yield;

    
    log("WHA WHA?\n\n\n");
    log(browser.contentWindow.wrappedJSObject.testXhrPost);
    browser.contentWindow.wrappedJSObject.testXhrPost(loggingGen);
    yield;

    executeSoon(function() {
      
      httpActivity = lastFinishedRequest;
      isnot(httpActivity, null, "testXhrPost() was logged");
      is(httpActivity.method, "POST", "Method is correct");
      
      
      
      
      lastFinishedRequest = null;
      loggingGen.next();
    });
    yield;

    
    
    browser.addEventListener("load", function onLoad () {
      browser.removeEventListener("load", onLoad, true);
      loggingGen.next();
    }, true);
    browser.contentWindow.wrappedJSObject.testSubmitForm();
    yield;

    
    httpActivity = lastFinishedRequest;
    isnot(httpActivity, null, "testSubmitForm() was logged");
    is(httpActivity.method, "POST", "Method is correct");
    
    
    
    
    
    
    
    
    

    lastFinishedRequest = null;

    
    
    let filterBox = hud.querySelectorAll(".hud-filter-box")[0];
    let networkPanel = HUDService.openNetworkPanel(filterBox, httpActivity);
    is (networkPanel, httpActivity.panels[0].get(), "Network panel stored on httpActivity object");
    networkPanel.panel.addEventListener("load", function onLoad() {
      networkPanel.panel.removeEventListener("load", onLoad, true);

      ok(true, "NetworkPanel was opened");
      networkPanel.panel.hidePopup();
      
      lastFinishedRequest = null;
      HUDService.lastFinishedRequestCallback = null;
      finishTest();
    }, true);
  }

  loggingGen = loggingGeneratorFunc();
  loggingGen.next();
}
