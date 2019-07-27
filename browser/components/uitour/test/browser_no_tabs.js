


"use strict";

let HiddenFrame = Cu.import("resource:///modules/HiddenFrame.jsm", {}).HiddenFrame;

const HTML_NS = "http://www.w3.org/1999/xhtml";
const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";








function createHiddenBrowser(aURL) {
  let frame = new HiddenFrame();
  return new Promise(resolve =>
    frame.get().then(aFrame => {
      let doc = aFrame.document;
      let browser = doc.createElementNS(XUL_NS, "browser");
      browser.setAttribute("type", "content");
      browser.setAttribute("disableglobalhistory", "true");
      browser.setAttribute("src", aURL);

      doc.documentElement.appendChild(browser);
      resolve({frame: frame, browser: browser});
    }));
}









function destroyHiddenBrowser(aFrame, aBrowser) {
  
  aBrowser.remove();

  
  aFrame.destroy();
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
