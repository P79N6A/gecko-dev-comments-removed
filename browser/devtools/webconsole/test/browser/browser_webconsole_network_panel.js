










































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test//browser/test-console.html";
const TEST_IMG = "http://example.com/browser/browser/devtools/webconsole/test//browser/test-image.png";
const TEST_ENCODING_ISO_8859_1 = "http://example.com/browser/browser/devtools/webconsole/test//browser/test-encoding-ISO-8859-1.html";

let testDriver;

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testNetworkPanel, false);
}

function testNetworkPanel() {
  browser.removeEventListener("DOMContentLoaded", testNetworkPanel,
                              false);
  openConsole();

  testDriver = testGen();
  testDriver.next();
}

function checkIsVisible(aPanel, aList) {
  for (let id in aList) {
    let node = aPanel.document.getElementById(id);
    let isVisible = aList[id];
    is(node.style.display, (isVisible ? "block" : "none"), id + " isVisible=" + isVisible);
  }
}

function checkNodeContent(aPanel, aId, aContent) {
  let node = aPanel.document.getElementById(aId);
  if (node == null) {
    ok(false, "Tried to access node " + aId + " that doesn't exist!");
  }
  else if (node.textContent.indexOf(aContent) != -1) {
    ok(true, "checking content of " + aId);
  }
  else {
    ok(false, "Got false value for " + aId + ": " + node.textContent + " doesn't have " + aContent);
  }
}

function checkNodeKeyValue(aPanel, aId, aKey, aValue) {
  let node = aPanel.document.getElementById(aId);

  let headers = node.querySelectorAll("th");
  for (let i = 0; i < headers.length; i++) {
    if (headers[i].textContent == (aKey + ":")) {
      is(headers[i].nextElementSibling.textContent, aValue,
         "checking content of " + aId + " for key " + aKey);
      return;
    }
  }

  ok(false, "content check failed for " + aId + ", key " + aKey);
}

function testGen() {
  let filterBox = HUDService.getHudByWindow(content).filterBox;

  var httpActivity = {
    url: "http://www.testpage.com",
    method: "GET",

    panels: [],
    request: {
      header: {
        foo: "bar"
      }
    },
    response: { },
    timing: {
      "REQUEST_HEADER": 0
    }
  };

  let networkPanel = HUDService.openNetworkPanel(filterBox, httpActivity);

  is (networkPanel, httpActivity.panels[0].get(), "Network panel stored on httpActivity object");
  networkPanel.panel.addEventListener("load", function onLoad() {
    networkPanel.panel.removeEventListener("load", onLoad, true);
    testDriver.next();
  }, true);

  yield;

  checkIsVisible(networkPanel, {
    requestCookie: false,
    requestFormData: false,
    requestBody: false,
    responseContainer: false,
    responseBody: false,
    responseNoBody: false,
    responseImage: false,
    responseImageCached: false
  });

  checkNodeContent(networkPanel, "header", "http://www.testpage.com");
  checkNodeContent(networkPanel, "header", "GET");
  checkNodeKeyValue(networkPanel, "requestHeadersContent", "foo", "bar");

  
  httpActivity.request.body = "hello world";
  networkPanel.update();
  checkIsVisible(networkPanel, {
    requestBody: true,
    requestFormData: false,
    requestCookie: false,
    responseContainer: false,
    responseBody: false,
    responseNoBody: false,
    responseImage: false,
    responseImageCached: false
  });
  checkNodeContent(networkPanel, "requestBodyContent", "hello world");

  
  httpActivity.timing.RESPONSE_HEADER = 1000;
  httpActivity.response.status = "999 earthquake win";
  httpActivity.response.header = {
    "Content-Type": "text/html",
    leaveHouses: "true"
  }
  networkPanel.update();
  checkIsVisible(networkPanel, {
    requestBody: true,
    requestFormData: false,
    requestCookie: false,
    responseContainer: true,
    responseBody: false,
    responseNoBody: false,
    responseImage: false,
    responseImageCached: false
  });

  checkNodeContent(networkPanel, "header", "999 earthquake win");
  checkNodeKeyValue(networkPanel, "responseHeadersContent", "leaveHouses", "true");
  checkNodeContent(networkPanel, "responseHeadersInfo", "1ms");

  httpActivity.timing.RESPONSE_COMPLETE = 2500;
  
  httpActivity.timing.TRANSACTION_CLOSE = 2500;
  networkPanel.update();

  checkIsVisible(networkPanel, {
    requestBody: true,
    requestCookie: false,
    requestFormData: false,
    responseContainer: true,
    responseBody: false,
    responseNoBody: false,
    responseImage: false,
    responseImageCached: false
  });

  httpActivity.response.isDone = true;
  networkPanel.update();

  checkNodeContent(networkPanel, "responseNoBodyInfo", "2ms");
  checkIsVisible(networkPanel, {
    requestBody: true,
    requestCookie: false,
    responseContainer: true,
    responseBody: false,
    responseNoBody: true,
    responseImage: false,
    responseImageCached: false
  });

  networkPanel.panel.hidePopup();

  
  httpActivity.request.header.Cookie = "foo=bar;  hello=world";
  httpActivity.response.body = "get out here";

  networkPanel = HUDService.openNetworkPanel(filterBox, httpActivity);
  is (networkPanel, httpActivity.panels[1].get(), "Network panel stored on httpActivity object");
  networkPanel.panel.addEventListener("load", function onLoad() {
    networkPanel.panel.removeEventListener("load", onLoad, true);
    testDriver.next();
  }, true);

  yield;


  checkIsVisible(networkPanel, {
    requestBody: true,
    requestFormData: false,
    requestCookie: true,
    responseContainer: true,
    responseBody: true,
    responseNoBody: false,
    responseImage: false,
    responseImageCached: false
  });

  checkNodeKeyValue(networkPanel, "requestCookieContent", "foo", "bar");
  checkNodeKeyValue(networkPanel, "requestCookieContent", "hello", "world");
  checkNodeContent(networkPanel, "responseBodyContent", "get out here");
  checkNodeContent(networkPanel, "responseBodyInfo", "2ms");

  networkPanel.panel.hidePopup();

  
  httpActivity.response.header["Content-Type"] = "image/png";
  httpActivity.url = TEST_IMG;

  networkPanel = HUDService.openNetworkPanel(filterBox, httpActivity);
  networkPanel.panel.addEventListener("load", function onLoad() {
    networkPanel.panel.removeEventListener("load", onLoad, true);
    testDriver.next();
  }, true);

  yield;

  checkIsVisible(networkPanel, {
    requestBody: true,
    requestFormData: false,
    requestCookie: true,
    responseContainer: true,
    responseBody: false,
    responseNoBody: false,
    responseImage: true,
    responseImageCached: false
  });

  let imgNode = networkPanel.document.getElementById("responseImageNode");
  is(imgNode.getAttribute("src"), TEST_IMG, "Displayed image is correct");

  function checkImageResponseInfo() {
    checkNodeContent(networkPanel, "responseImageInfo", "2ms");
    checkNodeContent(networkPanel, "responseImageInfo", "16x16px");
  }

  
  if (imgNode.width == 0) {
    imgNode.addEventListener("load", function onLoad() {
      imgNode.removeEventListener("load", onLoad, false);
      checkImageResponseInfo();
      networkPanel.panel.hidePopup();
      testDriver.next();
    }, false);
    
    yield;
  }
  else {
    checkImageResponseInfo();
    networkPanel.panel.hidePopup();
  }

  
  httpActivity.response.status = "HTTP/1.1 304 Not Modified";

  networkPanel = HUDService.openNetworkPanel(filterBox, httpActivity);
  networkPanel.panel.addEventListener("load", function onLoad() {
    networkPanel.panel.removeEventListener("load", onLoad, true);
    testDriver.next();
  }, true);

  yield;

  checkIsVisible(networkPanel, {
    requestBody: true,
    requestFormData: false,
    requestCookie: true,
    responseContainer: true,
    responseBody: false,
    responseNoBody: false,
    responseImage: false,
    responseImageCached: true
  });

  let imgNode = networkPanel.document.getElementById("responseImageCachedNode");
  is(imgNode.getAttribute("src"), TEST_IMG, "Displayed image is correct");

  networkPanel.panel.hidePopup();

  
  httpActivity.request.body = [
    "Content-Type:      application/x-www-form-urlencoded\n" +
    "Content-Length: 59\n" +
    "name=rob&age=20"
  ].join("");

  networkPanel = HUDService.openNetworkPanel(filterBox, httpActivity);
  networkPanel.panel.addEventListener("load", function onLoad() {
    networkPanel.panel.removeEventListener("load", onLoad, true);
    testDriver.next();
  }, true);

  yield;

  checkIsVisible(networkPanel, {
    requestBody: false,
    requestFormData: true,
    requestCookie: true,
    responseContainer: true,
    responseBody: false,
    responseNoBody: false,
    responseImage: false,
    responseImageCached: true
  });

  checkNodeKeyValue(networkPanel, "requestFormDataContent", "name", "rob");
  checkNodeKeyValue(networkPanel, "requestFormDataContent", "age", "20");
  networkPanel.panel.hidePopup();

  
  httpActivity.request.body = "Content-Type:application/x-www-form-urlencoded\n";

  networkPanel = HUDService.openNetworkPanel(filterBox, httpActivity);
  networkPanel.panel.addEventListener("load", function onLoad() {
    networkPanel.panel.removeEventListener("load", onLoad, true);
    testDriver.next();
  }, true);

  yield;

  checkIsVisible(networkPanel, {
    requestBody: false,
    requestFormData: true,
    requestCookie: true,
    responseContainer: true,
    responseBody: false,
    responseNoBody: false,
    responseImage: false,
    responseImageCached: true
  });

  networkPanel.panel.hidePopup();

  

  
  browser.addEventListener("load", function onLoad () {
    browser.removeEventListener("load", onLoad, true);
    httpActivity.charset = content.document.characterSet;
    testDriver.next();
  }, true);
  browser.contentWindow.wrappedJSObject.document.location = TEST_ENCODING_ISO_8859_1;

  yield;

  httpActivity.url = TEST_ENCODING_ISO_8859_1;
  httpActivity.response.header["Content-Type"] = "application/json";
  httpActivity.response.body = "";

  networkPanel = HUDService.openNetworkPanel(filterBox, httpActivity);
  networkPanel.isDoneCallback = function NP_doneCallback() {
    networkPanel.isDoneCallback = null;
    testDriver.next();
  }

  yield;

  checkIsVisible(networkPanel, {
    requestBody: false,
    requestFormData: true,
    requestCookie: true,
    responseContainer: true,
    responseBody: false,
    responseBodyCached: true,
    responseNoBody: false,
    responseImage: false,
    responseImageCached: false
  });

  checkNodeContent(networkPanel, "responseBodyCachedContent", "<body>\u00fc\u00f6\u00E4</body>");
  networkPanel.panel.hidePopup();

  
  
  httpActivity.response.header["Content-Type"] = "application/x-shockwave-flash";

  networkPanel = HUDService.openNetworkPanel(filterBox, httpActivity);
  networkPanel.isDoneCallback = function NP_doneCallback() {
    networkPanel.isDoneCallback = null;
    try {
      testDriver.next();
    } catch (e if e instanceof StopIteration) {
    }
  }

  yield;

  checkIsVisible(networkPanel, {
    requestBody: false,
    requestFormData: true,
    requestCookie: true,
    responseContainer: true,
    responseBody: false,
    responseBodyCached: false,
    responseBodyUnknownType: true,
    responseNoBody: false,
    responseImage: false,
    responseImageCached: false
  });

  let responseString = HUDService.getFormatStr("NetworkPanel.responseBodyUnableToDisplay.content", ["application/x-shockwave-flash"]);
  checkNodeContent(networkPanel, "responseBodyUnknownTypeContent", responseString);
  networkPanel.panel.hidePopup();

  





































  
  finish();
}
