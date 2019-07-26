



Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");

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
    
    try {
        Services.prefs.deleteBranch("geo.wifi.access_token.");
    } catch (e) {}

    PlacesUtils.history.removePagesFromHost(aDomain, true);

    
    let (cs = Cc["@mozilla.org/network/cache-service;1"].
              getService(Ci.nsICacheService)) {
      
      
      try {
        cs.evictEntries(Ci.nsICache.STORE_ANYWHERE);
      } catch (ex) {
        Cu.reportError("Exception thrown while clearing the cache: " +
          ex.toString());
      }
    }

    
    let (imageCache = Cc["@mozilla.org/image/tools;1"].
                      getService(Ci.imgITools).getImgCacheForDocument(null)) {
      try {
        imageCache.clearCache(false); 
      } catch (ex) {
        Cu.reportError("Exception thrown while clearing the image cache: " +
          ex.toString());
      }
    }

    
    let (cm = Cc["@mozilla.org/cookiemanager;1"].
              getService(Ci.nsICookieManager2)) {
      let enumerator = cm.getCookiesFromHost(aDomain);
      while (enumerator.hasMoreElements()) {
        let cookie = enumerator.getNext().QueryInterface(Ci.nsICookie);
        cm.remove(cookie.host, cookie.name, cookie.path, false);
      }
    }

    
    const phInterface = Ci.nsIPluginHost;
    const FLAG_CLEAR_ALL = phInterface.FLAG_CLEAR_ALL;
    let (ph = Cc["@mozilla.org/plugin/host;1"].getService(phInterface)) {
      let tags = ph.getPluginTags();
      for (let i = 0; i < tags.length; i++) {
        try {
          ph.clearSiteData(tags[i], aDomain, FLAG_CLEAR_ALL, -1);
        } catch (e) {
          
        }
      }
    }

    
    let (dm = Cc["@mozilla.org/download-manager;1"].
              getService(Ci.nsIDownloadManager)) {
      
      for (let enumerator of [dm.activeDownloads, dm.activePrivateDownloads]) {
        while (enumerator.hasMoreElements()) {
          let dl = enumerator.getNext().QueryInterface(Ci.nsIDownload);
          if (hasRootDomain(dl.source.host, aDomain)) {
            dl.cancel();
            dl.remove();
          }
        }
      }

      function deleteAllLike(db) {
        
        
        
        let stmt = db.createStatement(
          "DELETE FROM moz_downloads " +
          "WHERE source LIKE ?1 ESCAPE '/' " +
          "AND state NOT IN (?2, ?3, ?4)"
        );
        let pattern = stmt.escapeStringForLIKE(aDomain, "/");
        stmt.bindByIndex(0, "%" + pattern + "%");
        stmt.bindByIndex(1, Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING);
        stmt.bindByIndex(2, Ci.nsIDownloadManager.DOWNLOAD_PAUSED);
        stmt.bindByIndex(3, Ci.nsIDownloadManager.DOWNLOAD_QUEUED);
        try {
          stmt.execute();
        }
        finally {
          stmt.finalize();
        }
      }

      
      deleteAllLike(dm.DBConnection);
      deleteAllLike(dm.privateDBConnection);

      
      
      let os = Cc["@mozilla.org/observer-service;1"].
               getService(Ci.nsIObserverService);
      os.notifyObservers(null, "download-manager-remove-download", null);
    }

    
    let (lm = Cc["@mozilla.org/login-manager;1"].
              getService(Ci.nsILoginManager)) {
      
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
    }

    
    let (pm = Cc["@mozilla.org/permissionmanager;1"].
              getService(Ci.nsIPermissionManager)) {
      
      let enumerator = pm.enumerator;
      while (enumerator.hasMoreElements()) {
        let perm = enumerator.getNext().QueryInterface(Ci.nsIPermission);
        if (hasRootDomain(perm.host, aDomain))
          pm.remove(perm.host, perm.type);
      }
    }

    
    let (cp = Cc["@mozilla.org/content-pref/service;1"].
              getService(Ci.nsIContentPrefService)) {
      let db = cp.DBConnection;
      
      let names = [];
      let stmt = db.createStatement(
        "SELECT name " +
        "FROM groups " +
        "WHERE name LIKE ?1 ESCAPE '/'"
      );
      let pattern = stmt.escapeStringForLIKE(aDomain, "/");
      stmt.bindByIndex(0, "%" + pattern);
      try {
        while (stmt.executeStep())
          if (hasRootDomain(stmt.getString(0), aDomain))
            names.push(stmt.getString(0));
      }
      finally {
        stmt.finalize();
      }

      
      for (let i = 0; i < names.length; i++) {
        let uri = names[i];
        let enumerator = cp.getPrefs(uri, null).enumerator;
        while (enumerator.hasMoreElements()) {
          let pref = enumerator.getNext().QueryInterface(Ci.nsIProperty);
          cp.removePref(uri, pref.name, null);
        }
      }
    }

    
    let (idbm = Cc["@mozilla.org/dom/indexeddb/manager;1"].
                getService(Ci.nsIIndexedDatabaseManager)) {
      
      let caUtils = {};
      let scriptLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
                         getService(Ci.mozIJSSubScriptLoader);
      scriptLoader.loadSubScript("chrome://global/content/contentAreaUtils.js",
                                 caUtils);
      let httpURI = caUtils.makeURI("http://" + aDomain);
      let httpsURI = caUtils.makeURI("https://" + aDomain);
      idbm.clearDatabasesForURI(httpURI);
      idbm.clearDatabasesForURI(httpsURI);
    }

    
    Services.obs.notifyObservers(null, "browser:purge-domain-data", aDomain);
  }
};
