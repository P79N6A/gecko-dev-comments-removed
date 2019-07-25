






const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://webapprt/modules/WebappRT.jsm");

const MANIFEST_URL_BASE = Services.io.newURI(
  "http://mochi.test:8888/webapprtChrome/webapprt/test/chrome/", null, null);






Services.ww.registerNotification({
  observe: function(win, topic) {
    if (topic == "domwindowopened") {
      
      win.addEventListener("load", function onLoadWindow() {
        win.removeEventListener("load", onLoadWindow, false);
        if (win.location == "chrome://global/content/commonDialog.xul" &&
            win.opener == window) {
          win.close();
        }
      }, false);
    }
  }
});















function becomeWebapp(manifestURL, parameters, onBecome) {
  function observeInstall(subj, topic, data) {
    Services.obs.removeObserver(observeInstall, "webapps-ask-install");

    
    
    

    let scope = {};
    Cu.import("resource://gre/modules/Webapps.jsm", scope);
    scope.DOMApplicationRegistry.confirmInstall(JSON.parse(data));

    let installRecord = JSON.parse(data);
    installRecord.registryDir = Services.dirsvc.get("ProfD", Ci.nsIFile).path;
    WebappRT.config = installRecord;

    onBecome();
  }
  Services.obs.addObserver(observeInstall, "webapps-ask-install", false);

  
  let url = Services.io.newURI(manifestURL, null, MANIFEST_URL_BASE);
  navigator.mozApps.install(url.spec, parameters);
}
