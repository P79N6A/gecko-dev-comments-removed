






Cu.import("resource://webapprt/modules/WebappRT.jsm");
Cu.import("resource://gre/modules/Task.jsm");






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
  let observeInstall = Task.async(function*(subj, topic, data) {
    Services.obs.removeObserver(observeInstall, "webapps-ask-install");

    
    
    

    let scope = {};
    Cu.import("resource://gre/modules/Webapps.jsm", scope);
    Cu.import("resource://webapprt/modules/Startup.jsm", scope);
    yield scope.DOMApplicationRegistry.confirmInstall(JSON.parse(data));

    let installRecord = JSON.parse(data);
    installRecord.mm = subj;
    installRecord.registryDir = Services.dirsvc.get("ProfD", Ci.nsIFile).path;
    WebappRT.config = installRecord;

    let win = Services.wm.getMostRecentWindow("webapprt:webapp");
    if (!win) {
      win = Services.ww.openWindow(null,
                                   "chrome://webapprt/content/webapp.xul",
                                   "_blank",
                                   "chrome,dialog=no,resizable,scrollbars,centerscreen",
                                   null);
    }

    
    
    
    Services.prefs.setCharPref("webapprt.buildID", WebappRT.config.app.manifestURL);

    
    
    
    Services.obs.notifyObservers(this, "webapps-registry-start", null);
    Services.obs.notifyObservers(this, "webapps-registry-ready", null);

    yield scope.startup(win);
    onBecome();
  });
  Services.obs.addObserver(observeInstall, "webapps-ask-install", false);

  
  navigator.mozApps.install(manifestURL, parameters);
}
