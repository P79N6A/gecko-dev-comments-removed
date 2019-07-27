


const RELATIVE_DIR = "browser/extensions/pdfjs/test/";
const TESTROOT = "http://example.com/browser/" + RELATIVE_DIR;

const TESTS = [
  {
    action: {
      selector: "button#zoomIn",
      event: "click"
    },
    expectedZoom: 1, 
    message: "Zoomed in using the '+' (zoom in) button"
  },

  {
    action: {
      selector: "button#zoomOut",
      event: "click"
    },
    expectedZoom: -1, 
    message: "Zoomed out using the '-' (zoom out) button"
  },

  {
    action: {
      keyboard: true,
      event: "+"
    },
    expectedZoom: 1, 
    message: "Zoomed in using the CTRL++ keys"
  },

  {
    action: {
      keyboard: true,
      event: "-"
    },
    expectedZoom: -1, 
    message: "Zoomed out using the CTRL+- keys"
  },

  {
    action: {
      selector: "select#scaleSelect",
      index: 5,
      event: "change"
    },
    expectedZoom: -1, 
    message: "Zoomed using the zoom picker"
  }
];

let initialWidth; 
var previousWidth; 

function test() {
  var tab;
  let handlerService = Cc["@mozilla.org/uriloader/handler-service;1"]
                       .getService(Ci.nsIHandlerService);
  let mimeService = Cc["@mozilla.org/mime;1"].getService(Ci.nsIMIMEService);
  let handlerInfo = mimeService.getFromTypeAndExtension('application/pdf', 'pdf');

  
  is(handlerInfo.alwaysAskBeforeHandling, false,
     'pdf handler defaults to always-ask is false');
  is(handlerInfo.preferredAction, Ci.nsIHandlerInfo.handleInternally,
    'pdf handler defaults to internal');

  info('Pref action: ' + handlerInfo.preferredAction);

  waitForExplicitFinish();
  registerCleanupFunction(function() {
    gBrowser.removeTab(tab);
  });

  tab = gBrowser.selectedTab = gBrowser.addTab(TESTROOT + "file_pdfjs_test.pdf");
  var newTabBrowser = gBrowser.getBrowserForTab(tab);

  newTabBrowser.addEventListener("load", function eventHandler() {
    newTabBrowser.removeEventListener("load", eventHandler, true);

    var document = newTabBrowser.contentDocument,
        window = newTabBrowser.contentWindow;

    
    window.addEventListener("documentload", function() {
      initialWidth = parseInt(document.querySelector("div#pageContainer1").style.width);
      previousWidth = initialWidth;
      runTests(document, window, finish);
    }, false, true);
  }, true);
}

function runTests(document, window, callback) {
  
  ok(document.querySelector('div#viewer'), "document content has viewer UI");
  ok('PDFJS' in window.wrappedJSObject, "window content has PDFJS object");

  
  waitForDocumentLoad(document).then(function () {
    zoomPDF(document, window, TESTS.shift(), finish);
  });
}

function waitForDocumentLoad(document) {
  var deferred = Promise.defer();
  var interval = setInterval(function () {
    if (document.querySelector("div#pageContainer1") != null){
      clearInterval(interval);
      deferred.resolve();
    }
  }, 500);

  return deferred.promise;
}

function zoomPDF(document, window, test, endCallback) {
  var renderedPage;

  document.addEventListener("pagerendered", function onPageRendered(e) {
    if(e.detail.pageNumber !== 1) {
      return;
    }

    document.removeEventListener("pagerendered", onPageRendered, true);

    var pageZoomScale = document.querySelector('select#scaleSelect');

    
    var zoomValue = pageZoomScale.options[pageZoomScale.selectedIndex].innerHTML;

    let pageContainer = document.querySelector('div#pageContainer1');
    let actualWidth  = parseInt(pageContainer.style.width);

    
    let computedZoomValue = parseInt(((actualWidth/initialWidth).toFixed(2))*100) + "%";
    is(computedZoomValue, zoomValue, "Content has correct zoom");

    
    let zoom = (actualWidth - previousWidth) * test.expectedZoom;
    ok(zoom > 0, test.message);

    
    var nextTest = TESTS.shift();
    if (nextTest) {
      previousWidth = actualWidth;
      zoomPDF(document, window, nextTest, endCallback);
    }
    else
      endCallback();
  }, true);

  
  if (test.action.selector) {
    
    var el = document.querySelector(test.action.selector);
    ok(el, "Element '" + test.action.selector + "' has been found");

    if (test.action.index){
      el.selectedIndex = test.action.index;
    }

    
    el.dispatchEvent(new Event(test.action.event));
  }
  
  else {
    
    EventUtils.synthesizeKey(test.action.event, { ctrlKey: true });
  }
}
