


const RELATIVE_DIR = "browser/extensions/pdfjs/test/";
const TESTROOT = "http://example.com/browser/" + RELATIVE_DIR;

function test() {
  var tab;

  let handlerService = Cc["@mozilla.org/uriloader/handler-service;1"].getService(Ci.nsIHandlerService);
  let mimeService = Cc["@mozilla.org/mime;1"].getService(Ci.nsIMIMEService);
  let handlerInfo = mimeService.getFromTypeAndExtension('application/pdf', 'pdf');

  
  is(handlerInfo.alwaysAskBeforeHandling, false, 'pdf handler defaults to always-ask is false');
  is(handlerInfo.preferredAction, Ci.nsIHandlerInfo.handleInternally, 'pdf handler defaults to internal');

  info('Pref action: ' + handlerInfo.preferredAction);

  waitForExplicitFinish();
  registerCleanupFunction(function() {
    gBrowser.removeTab(tab);
  });

  tab = gBrowser.addTab(TESTROOT + "file_pdfjs_test.pdf");
  var newTabBrowser = gBrowser.getBrowserForTab(tab);
  newTabBrowser.addEventListener("load", function eventHandler() {
    newTabBrowser.removeEventListener("load", eventHandler, true);

    var document = newTabBrowser.contentDocument,
        window = newTabBrowser.contentWindow;

    
    window.addEventListener("documentload", function() {
      runTests(document, window, tab, finish);
    }, false, true);
  }, true);
}


function runTests(document, window, tab, callback) {

  
  
  
  ok(document.querySelector('div#viewer'), "document content has viewer UI");
  ok('PDFJS' in window.wrappedJSObject, "window content has PDFJS object");

  
  
  
  ok(gBrowser.isFindBarInitialized(tab), "Browser FindBar initialized!");

  
  
  
  var sidebar = document.querySelector('button#sidebarToggle'),
      outerContainer = document.querySelector('div#outerContainer');

  sidebar.click();
  ok(outerContainer.classList.contains('sidebarOpen'), 'sidebar opens on click');

  
  
  
  sidebar.click();
  ok(!outerContainer.classList.contains('sidebarOpen'), 'sidebar closes on click');

  
  
  
  var prevPage = document.querySelector('button#previous'),
      nextPage = document.querySelector('button#next');

  var pageNumber = document.querySelector('input#pageNumber');
  is(parseInt(pageNumber.value), 1, 'initial page is 1');

  
  
  
  var viewBookmark = document.querySelector('a#viewBookmark');
  viewBookmark.click();
  ok(viewBookmark.href.length > 0, 'viewBookmark button has href');

  callback();
}
