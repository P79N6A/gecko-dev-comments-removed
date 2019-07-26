


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
      runTests(document, window, finish);
    }, false, true);
  }, true);
}

function runTests(document, window, callback) {
  
  ok(document.querySelector('div#viewer'), "document content has viewer UI");
  ok('PDFJS' in window.wrappedJSObject, "window content has PDFJS object");

  
  var sidebar = document.querySelector('button#sidebarToggle');
  var outerContainer = document.querySelector('div#outerContainer');

  sidebar.click();
  ok(outerContainer.classList.contains('sidebarOpen'), 'sidebar opens on click');

  
  var thumbnailView = document.querySelector('div#thumbnailView');
  var outlineView = document.querySelector('div#outlineView');

  is(thumbnailView.getAttribute('class'), null, 'Initial view is thumbnail view');
  is(outlineView.getAttribute('class'), 'hidden', 'Outline view is hidden initially');

  
  var viewOutlineButton = document.querySelector('button#viewOutline');
  viewOutlineButton.click();

  is(outlineView.getAttribute('class'), '', 'Outline view is visible when selected');
  is(thumbnailView.getAttribute('class'), 'hidden', 'Thumbnail view is hidden when outline is selected');

  
  var viewThumbnailButton = document.querySelector('button#viewThumbnail');
  viewThumbnailButton.click();

  is(thumbnailView.getAttribute('class'), '', 'Thumbnail view is visible when selected');
  is(outlineView.getAttribute('class'), 'hidden', 'Outline view is hidden when thumbnail is selected');

  sidebar.click();

  callback();
}
