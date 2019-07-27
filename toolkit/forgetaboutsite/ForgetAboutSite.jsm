



"use strict";

Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/NetUtil.jsm");
Components.utils.import("resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Downloads",
                                  "resource://gre/modules/Downloads.jsm");

this.EXPORTED_SYMBOLS = ["ForgetAboutSite"];







function hasRootDomain(str, aDomain)
{
  let index = str.indexOf(aDomain);
  
  if (index == -1)
    return false;

  
  if (str == aDomain)
    return true;

  
  
  
  let prevChar = str[index - 1];
  return (index == (str.length - aDomain.length)) &&
         (prevChar == "." || prevChar == "/");
}

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

this.ForgetAboutSite = {
  removeDataFromDomain: function CRH_removeDataFromDomain(aDomain)
  {
    PlacesUtils.history.removePagesFromHost(aDomain, true);

    
    let cs = Cc["@mozilla.org/netwerk/cache-storage-service;1"].
             getService(Ci.nsICacheStorageService);
    
    
    try {
      cs.clear();
    } catch (ex) {
      Cu.reportError("Exception thrown while clearing the cache: " +
        ex.toString());
    }

    
    let imageCache = Cc["@mozilla.org/image/tools;1"].
                     getService(Ci.imgITools).getImgCacheForDocument(null);
    try {
      imageCache.clearCache(false); 
    } catch (ex) {
      Cu.reportError("Exception thrown while clearing the image cache: " +
        ex.toString());
    }

    
    let cm = Cc["@mozilla.org/cookiemanager;1"].
             getService(Ci.nsICookieManager2);
    let enumerator = cm.getCookiesFromHost(aDomain);
    while (enumerator.hasMoreElements()) {
      let cookie = enumerator.getNext().QueryInterface(Ci.nsICookie);
      cm.remove(cookie.host, cookie.name, cookie.path, false);
    }

    
    let mps = Cc["@mozilla.org/gecko-media-plugin-service;1"].
               getService(Ci.mozIGeckoMediaPluginChromeService);
    mps.forgetThisSite(aDomain);

    
    const phInterface = Ci.nsIPluginHost;
    const FLAG_CLEAR_ALL = phInterface.FLAG_CLEAR_ALL;
    let ph = Cc["@mozilla.org/plugin/host;1"].getService(phInterface);
    let tags = ph.getPluginTags();
    for (let i = 0; i < tags.length; i++) {
      try {
        ph.clearSiteData(tags[i], aDomain, FLAG_CLEAR_ALL, -1);
      } catch (e) {
        
      }
    }

    
    Task.spawn(function*() {
      let list = yield Downloads.getList(Downloads.ALL);
      list.removeFinished(download => hasRootDomain(
           NetUtil.newURI(download.source.url).host, aDomain));
    }).then(null, Cu.reportError);

    
    let lm = Cc["@mozilla.org/login-manager;1"].
             getService(Ci.nsILoginManager);
    
    try {
      let logins = lm.getAllLogins();
      for (let i = 0; i < logins.length; i++)
        if (hasRootDomain(logins[i].hostname, aDomain))
          lm.removeLogin(logins[i]);
    }
    
    
    catch (ex if ex.message.indexOf("User canceled Master Password entry") != -1) { }

    
    let disabledHosts = lm.getAllDisabledHosts();
    for (let i = 0; i < disabledHosts.length; i++)
      if (hasRootDomain(disabledHosts[i], aDomain))
        lm.setLoginSavingEnabled(disabledHosts, true);

    
    let pm = Cc["@mozilla.org/permissionmanager;1"].
             getService(Ci.nsIPermissionManager);
    
    enumerator = pm.enumerator;
    while (enumerator.hasMoreElements()) {
      let perm = enumerator.getNext().QueryInterface(Ci.nsIPermission);
      if (hasRootDomain(perm.host, aDomain))
        pm.remove(perm.host, perm.type);
    }

    
    let qm = Cc["@mozilla.org/dom/quota/manager;1"].
             getService(Ci.nsIQuotaManager);
    
    let caUtils = {};
    let scriptLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
                       getService(Ci.mozIJSSubScriptLoader);
    scriptLoader.loadSubScript("chrome://global/content/contentAreaUtils.js",
                               caUtils);
    let httpURI = caUtils.makeURI("http://" + aDomain);
    let httpsURI = caUtils.makeURI("https://" + aDomain);
    qm.clearStoragesForURI(httpURI);
    qm.clearStoragesForURI(httpsURI);

    function onContentPrefsRemovalFinished() {
      
      Services.obs.notifyObservers(null, "browser:purge-domain-data", aDomain);
    }

    
    let cps2 = Cc["@mozilla.org/content-pref/service;1"].
               getService(Ci.nsIContentPrefService2);
    cps2.removeBySubdomain(aDomain, null, {
      handleCompletion: function() onContentPrefsRemovalFinished(),
      handleError: function() {}
    });

    
    
    let np = Cc["@mozilla.org/network/predictor;1"].
             getService(Ci.nsINetworkPredictor);
    np.reset();
  }
};
