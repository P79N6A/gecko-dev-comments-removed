



const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

Services.scriptloader
        .loadSubScript("chrome://webapprt/content/mochitest-shared.js", this);





WebappRT.config = {
  registryDir: Services.dirsvc.get("ProfD", Ci.nsIFile).path,
};

Cu.import("resource://gre/modules/Webapps.jsm");

becomeWebapp("http://mochi.test:8888/webapprtContent/webapprt/test/content/test.webapp",
             undefined, function onBecome() {
  if (window.arguments && window.arguments[0]) {
    let testUrl = window.arguments[0].QueryInterface(Ci.nsIPropertyBag2).get("url");

    if (testUrl) {
      let win = Services.wm.getMostRecentWindow("webapprt:webapp");
      win.document.getElementById("content").setAttribute("src", testUrl);
    }
  }

  window.close();
});
