









const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/HUDService.jsm");

const TEST_NETWORK_REQUEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-network-request.html";

const TEST_IMG = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-image.png";

const TEST_DATA_JSON_CONTENT =
  '{ id: "test JSON data", myArray: [ "foo", "bar", "baz", "biff" ] }';

var hud;
var hudId;

function testOpenWebConsole()
{
  HUDService.activateHUDForContext(gBrowser.selectedTab);
  is(HUDService.displaysIndex().length, 1, "WebConsole was opened");

  hudId = HUDService.displaysIndex()[0];
  hud = HUDService.getHeadsUpDisplay(hudId);

  testNetworkLogging();
}

function finishTest() {
  hud = null;
  hudId = null;

  let tab = gBrowser.selectedTab;
  HUDService.deactivateHUDForContext(tab);
  executeSoon(function() {
    gBrowser.removeCurrentTab();
    finish();
  });
}

function testNetworkLogging()
{
  var lastFinishedRequest = null;
  HUDService.lastFinishedRequestCallback =
    function requestDoneCallback(aHttpRequest)
    {
      lastFinishedRequest = aHttpRequest;
    }

  let browser = gBrowser.selectedBrowser;
  let loggingGen;
  
  function loggingGeneratorFunc() {
    browser.addEventListener("load", function onLoad () {
      browser.removeEventListener("load", onLoad, true);
      loggingGen.next();
    }, true);
    content.location = TEST_NETWORK_REQUEST_URI;
    yield;

    
    let httpActivity = lastFinishedRequest;
    isnot(httpActivity, null, "Page load was logged");
    is(httpActivity.url, TEST_NETWORK_REQUEST_URI,
      "Logged network entry is page load");
    is(httpActivity.method, "GET", "Method is correct");
    is(httpActivity.request.body, undefined, "No request body sent");

    
    
    
    if (navigator.platform.indexOf("Linux") != 0) {
      ok(httpActivity.response.body.indexOf("<!DOCTYPE HTML>") == 0,
        "Response body's beginning is okay");
    }

    
    browser.contentWindow.wrappedJSObject.testXhrGet(loggingGen);
    yield;

    
    
    
    executeSoon(function() {
      
      httpActivity = lastFinishedRequest;
      isnot(httpActivity, null, "testXhrGet() was logged");
      is(httpActivity.method, "GET", "Method is correct");
      is(httpActivity.request.body, undefined, "No request body was sent");
      is(httpActivity.response.body, TEST_DATA_JSON_CONTENT,
        "Response is correct");
      lastFinishedRequest = null;
      loggingGen.next();
    });
    yield;

    
    browser.contentWindow.wrappedJSObject.testXhrPost(loggingGen);
    yield;

    executeSoon(function() {
      
      httpActivity = lastFinishedRequest;
      isnot(httpActivity, null, "testXhrPost() was logged");
      is(httpActivity.method, "POST", "Method is correct");
      is(httpActivity.request.body, "Hello world!",
        "Request body was logged");
      is(httpActivity.response.body, TEST_DATA_JSON_CONTENT,
        "Response is correct");
      lastFinishedRequest = null
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
    isnot(httpActivity.request.body.indexOf(
      "Content-Type: application/x-www-form-urlencoded"), -1,
      "Content-Type is correct");
    isnot(httpActivity.request.body.indexOf(
      "Content-Length: 20"), -1, "Content-length is correct");
    isnot(httpActivity.request.body.indexOf(
      "name=foo+bar&age=144"), -1, "Form data is correct");
    ok(httpActivity.response.body.indexOf("<!DOCTYPE HTML>") == 0,
      "Response body's beginning is okay");

    lastFinishedRequest = null

    
    
    let filterBox = hud.querySelectorAll(".hud-filter-box")[0];
    let networkPanel = HUDService.openNetworkPanel(filterBox, httpActivity);
    is (networkPanel, httpActivity.panels[0].get(), "Network panel stored on httpActivity object");
    networkPanel.panel.addEventListener("load", function onLoad() {
      networkPanel.panel.removeEventListener("load", onLoad, true);

      ok(true, "NetworkPanel was opened");
      networkPanel.panel.hidePopup();

      
      browser = null;
      lastFinishedRequest = null;
      HUDService.lastFinishedRequestCallback = null;

      finishTest();
    }, true);
  }

  loggingGen = loggingGeneratorFunc();
  loggingGen.next();
}

function test()
{
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab();

  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);
    waitForFocus(testOpenWebConsole, content);
  }, true);

  content.location = "data:text/html,WebConsole network logging tests";
}
