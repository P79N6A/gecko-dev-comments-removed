



Cu.import("resource://gre/modules/Services.jsm");



Services.scriptloader.loadSubScript("chrome://webapprt/content/mochitest.js",
                                    this);











function loadWebapp(manifest, parameters, onLoad) {
  becomeWebapp(manifest, parameters, function onBecome() {
    function onLoadApp() {
      gAppBrowser.removeEventListener("load", onLoadApp, true);
      onLoad();
    }
    gAppBrowser.addEventListener("load", onLoadApp, true);
    gAppBrowser.setAttribute("src", WebappRT.launchURI.spec);
  });
}
