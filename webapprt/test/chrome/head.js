



Cu.import("resource://gre/modules/Services.jsm");



Services.scriptloader
        .loadSubScript("chrome://webapprt/content/mochitest-shared.js", this);

const MANIFEST_URL_BASE = Services.io.newURI(
  "http://test/webapprtChrome/webapprt/test/chrome/", null, null);











function loadWebapp(manifest, parameters, onLoad) {
  let url = Services.io.newURI(manifest, null, MANIFEST_URL_BASE);

  becomeWebapp(url.spec, parameters, function onBecome() {
    function onLoadApp() {
      gAppBrowser.removeEventListener("load", onLoadApp, true);
      onLoad();
    }
    gAppBrowser.addEventListener("load", onLoadApp, true);
    gAppBrowser.setAttribute("src", WebappRT.launchURI);
  });

  registerCleanupFunction(function() {
    
    
    let { DOMApplicationRegistry } = Cu.import("resource://gre/modules/Webapps.jsm", {});

    return new Promise(function(resolve, reject) {
      DOMApplicationRegistry.uninstall(url.spec, resolve, reject);
    });
  });
}
