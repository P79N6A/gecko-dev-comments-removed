


"use strict";

const HTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";








function createHiddenBrowser(aURL) {
  let deferred = Promise.defer();
  let hiddenDoc = Services.appShell.hiddenDOMWindow.document;

  
  let iframe = hiddenDoc.createElementNS(HTML_NS, "iframe");
  iframe.setAttribute("src", "chrome://global/content/mozilla.xhtml");
  iframe.addEventListener("load", function onLoad() {
    iframe.removeEventListener("load", onLoad, true);

    let browser = iframe.contentDocument.createElementNS(XUL_NS, "browser");
    browser.setAttribute("type", "content");
    browser.setAttribute("disableglobalhistory", "true");
    browser.setAttribute("src", aURL);

    iframe.contentDocument.documentElement.appendChild(browser);
    deferred.resolve({frame: iframe, browser: browser});
  }, true);

  hiddenDoc.documentElement.appendChild(iframe);
  return deferred.promise;
};









function destroyHiddenBrowser(aFrame, aBrowser) {
  
  aBrowser.remove();

  
  if (!Cu.isDeadWrapper(aFrame)) {
    aFrame.remove();
  }
};





add_task(function* test_windowless_UITour(){
  
  let pageURL = getRootDirectory(gTestPath) + "uitour.html";

  
  info("Adding UITour permission to the test page.");
  let pageURI = Services.io.newURI(pageURL, null, null);
  Services.perms.add(pageURI, "uitour", Services.perms.ALLOW_ACTION);

  
  let deferredPing = Promise.defer();

  
  let browserPromise = createHiddenBrowser(pageURL);
  browserPromise.then(frameInfo => {
    isnot(frameInfo.browser, null, "The browser must exist and not be null.");

    
    frameInfo.browser.messageManager.loadFrameScript(
      "chrome://browser/content/content-UITour.js", false);

    
    frameInfo.browser.addEventListener("load", function loadListener() {
      info("The test page was correctly loaded.");

      frameInfo.browser.removeEventListener("load", loadListener, true);

      
      info("Testing access to the UITour API.");
      let contentWindow = Cu.waiveXrays(frameInfo.browser.contentDocument.defaultView);
      isnot(contentWindow, null, "The content window must exist and not be null.");

      let uitourAPI = contentWindow.Mozilla.UITour;

      
      uitourAPI.ping(function() {
        info("Ping response received from the UITour API.");

        
        destroyHiddenBrowser(frameInfo.frame, frameInfo.browser);

        
        deferredPing.resolve();
      });
    }, true);
  });

  
  yield deferredPing.promise;
});
