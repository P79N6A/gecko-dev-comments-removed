




































let Ci = Components.interfaces;
let Cc = Components.classes;
let Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PluralForm.jsm");
Cu.import("resource://gre/modules/DownloadUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");

let gFaviconService = Cc["@mozilla.org/browser/favicon-service;1"].
                      getService(Ci.nsIFaviconService);

let gPlacesDatabase = Cc["@mozilla.org/browser/nav-history-service;1"].
                      getService(Ci.nsPIPlacesDatabase).
                      DBConnection.
                      clone(true);

let gSitesStmt = gPlacesDatabase.createAsyncStatement(
                  "SELECT get_unreversed_host(rev_host) AS host " +
                  "FROM moz_places " +
                  "WHERE rev_host > '.' " +
                  "AND visit_count > 0 " +
                  "GROUP BY rev_host " +
                  "ORDER BY MAX(frecency) DESC " +
                  "LIMIT :limit");

let gVisitStmt = gPlacesDatabase.createAsyncStatement(
                  "SELECT SUM(visit_count) AS count " +
                  "FROM moz_places " +
                  "WHERE rev_host = :rev_host");





let TEST_EXACT_PERM_TYPES = ["geo"];




function Site(host) {
  this.host = host;
  this.listitem = null;

  this.httpURI = NetUtil.newURI("http://" + this.host);
  this.httpsURI = NetUtil.newURI("https://" + this.host);
}

Site.prototype = {
  






  getFavicon: function Site_getFavicon(aCallback) {
    function faviconDataCallback(aURI, aDataLen, aData, aMimeType) {
      try {
        aCallback(aURI.spec);
      } catch (e) {
        Cu.reportError("AboutPermissions: " + e);
      }
    }

    
    
    gFaviconService.getFaviconURLForPage(this.httpURI, faviconDataCallback);
    gFaviconService.getFaviconURLForPage(this.httpsURI, faviconDataCallback);
  },

  





  getVisitCount: function Site_getVisitCount(aCallback) {
    let rev_host = this.host.split("").reverse().join("") + ".";
    gVisitStmt.params.rev_host = rev_host;
    gVisitStmt.executeAsync({
      handleResult: function(aResults) {
        let row = aResults.getNextRow();
        let count = row.getResultByName("count") || 0;
        try {
          aCallback(count);
        } catch (e) {
          Cu.reportError("AboutPermissions: " + e);
        }
      },
      handleError: function(aError) {
        Cu.reportError("AboutPermissions: " + aError);
      },
      handleCompletion: function(aReason) {
      }
    });
  },

  










  getPermission: function Site_getPermission(aType, aResultObj) {
    let permissionValue;
    if (TEST_EXACT_PERM_TYPES.indexOf(aType) == -1) {
      permissionValue = Services.perms.testPermission(this.httpURI, aType);
    } else {
      permissionValue = Services.perms.testExactPermission(this.httpURI, aType);
    }
    aResultObj.value = permissionValue;

    return permissionValue != Ci.nsIPermissionManager.UNKNOWN_ACTION;
  },

  









  setPermission: function Site_setPermission(aType, aPerm) {
    
    
    Services.perms.add(this.httpURI, aType, aPerm);
  },

  






  clearPermission: function Site_clearPermission(aType) {
    Services.perms.remove(this.host, aType);
  },

  





  get cookies() {
    let cookies = [];
    let enumerator = Services.cookies.getCookiesFromHost(this.host);
    while (enumerator.hasMoreElements()) {
      let cookie = enumerator.getNext().QueryInterface(Ci.nsICookie2);
      
      
      if (cookie.rawHost == this.host) {
        cookies.push(cookie);
      }
    }
    return cookies;
  },

  


  clearCookies: function Site_clearCookies() {
    this.cookies.forEach(function(aCookie) {
      Services.cookies.remove(aCookie.host, aCookie.name, aCookie.path, false);
    });
  },

  




  get logins() {
    
    
    let httpLogins = Services.logins.findLogins({}, this.httpURI.prePath, "", null);
    let httpsLogins = Services.logins.findLogins({}, this.httpsURI.prePath, "", null);
    return httpLogins.concat(httpsLogins);
  },

  get loginSavingEnabled() {
    
    return Services.logins.getLoginSavingEnabled(this.httpURI.prePath) &&
           Services.logins.getLoginSavingEnabled(this.httpsURI.prePath);
  },

  set loginSavingEnabled(isEnabled) {
    Services.logins.setLoginSavingEnabled(this.httpURI.prePath, isEnabled);
    Services.logins.setLoginSavingEnabled(this.httpsURI.prePath, isEnabled);
  },

  


  forgetSite: function Site_forgetSite() {
    let pb = Cc["@mozilla.org/privatebrowsing;1"].
             getService(Ci.nsIPrivateBrowsingService);
    pb.removeDataFromDomain(this.host);
  }
}







let PermissionDefaults = {
  UNKNOWN: Ci.nsIPermissionManager.UNKNOWN_ACTION, 
  ALLOW: Ci.nsIPermissionManager.ALLOW_ACTION, 
  DENY: Ci.nsIPermissionManager.DENY_ACTION, 
  SESSION: Ci.nsICookiePermission.ACCESS_SESSION, 

  get password() {
    if (Services.prefs.getBoolPref("signon.rememberSignons")) {
      return this.ALLOW;
    }
    return this.DENY;
  },
  set password(aValue) {
    let value = (aValue == this.ALLOW);
    Services.prefs.setBoolPref("signon.rememberSignons", value);
  },

  
  COOKIE_ACCEPT: 0,
  COOKIE_DENY: 2,
  COOKIE_NORMAL: 0,
  COOKIE_SESSION: 2,

  get cookie() {
    if (Services.prefs.getIntPref("network.cookie.cookieBehavior") == this.COOKIE_DENY) {
      return this.DENY;
    }

    if (Services.prefs.getIntPref("network.cookie.lifetimePolicy") == this.COOKIE_DENY) {
      return this.SESSION;
    }
    return this.ALLOW;
  },
  set cookie(aValue) {
    let value = (aValue == this.DENY) ? this.COOKIE_DENY : this.COOKIE_ACCEPT;
    Services.prefs.setIntPref("network.cookie.cookieBehavior", value);

    let lifetimeValue = aValue == this.SESSION ? this.COOKIE_SESSION :
                                                 this.COOKIE_NORMAL;
    Services.prefs.setIntPref("network.cookie.lifetimePolicy", lifetimeValue);
  },

  get geo() {
    if (!Services.prefs.getBoolPref("geo.enabled")) {
      return this.DENY;
    }
    
    
    return this.UNKNOWN;
  },
  set geo(aValue) {
    let value = (aValue == this.ALLOW);
    Services.prefs.setBoolPref("geo.enabled", value);
  },

  get indexedDB() {
    if (!Services.prefs.getBoolPref("dom.indexedDB.enabled")) {
      return this.DENY;
    }
    
    
    return this.UNKNOWN;
  },
  set indexedDB(aValue) {
    let value = (aValue == this.ALLOW);
    Services.prefs.setBoolPref("dom.indexedDB.enabled", value);
  },

  get popup() {
    if (Services.prefs.getBoolPref("dom.disable_open_during_load")) {
      return this.DENY;
    }
    return this.ALLOW;
  },
  set popup(aValue) {
    let value = (aValue == this.DENY);
    Services.prefs.setBoolPref("dom.disable_open_during_load", value);
  }
}




let AboutPermissions = {
  

  
  PLACES_SITES_LIMIT: 50,

  


  _sites: {},

  sitesList: null,
  _selectedSite: null,

  






  _supportedPermissions: ["password", "cookie", "geo", "indexedDB", "popup"],

  


  _noGlobalAllow: ["geo", "indexedDB"],

  _stringBundle: Services.strings.
                 createBundle("chrome://browser/locale/preferences/aboutPermissions.properties"),

  


  init: function() {
    this.sitesList = document.getElementById("sites-list");

    this.getSitesFromPlaces();
    this.enumerateServices();

    
    Services.prefs.addObserver("signon.rememberSignons", this, false);
    Services.prefs.addObserver("network.cookie.", this, false);
    Services.prefs.addObserver("geo.enabled", this, false);
    Services.prefs.addObserver("dom.indexedDB.enabled", this, false);
    Services.prefs.addObserver("dom.disable_open_during_load", this, false);

    Services.obs.addObserver(this, "perm-changed", false);
    Services.obs.addObserver(this, "passwordmgr-storage-changed", false);
    Services.obs.addObserver(this, "cookie-changed", false);
    Services.obs.addObserver(this, "browser:purge-domain-data", false);
    
    this._observersInitialized = true;
  },

  


  cleanUp: function() {
    if (this._observersInitialized) {
      Services.prefs.removeObserver("signon.rememberSignons", this, false);
      Services.prefs.removeObserver("network.cookie.", this, false);
      Services.prefs.removeObserver("geo.enabled", this, false);
      Services.prefs.removeObserver("dom.indexedDB.enabled", this, false);
      Services.prefs.removeObserver("dom.disable_open_during_load", this, false);

      Services.obs.removeObserver(this, "perm-changed", false);
      Services.obs.removeObserver(this, "passwordmgr-storage-changed", false);
      Services.obs.removeObserver(this, "cookie-changed", false);
      Services.obs.removeObserver(this, "browser:purge-domain-data", false);
    }

    gSitesStmt.finalize();
    gVisitStmt.finalize();
  },

  observe: function (aSubject, aTopic, aData) {
    switch(aTopic) {
      case "perm-changed":
        
        if (!this._selectedSite) {
          break;
        }
        
        if (!aSubject) {
          this._supportedPermissions.forEach(function(aType){
            this.updatePermission(aType);
          }, this);
          break;
        }
        let permission = aSubject.QueryInterface(Ci.nsIPermission);
        
        
        
        if (this._supportedPermissions.indexOf(permission.type) != -1) {
          this.updatePermission(permission.type);
        }
        break;
      case "nsPref:changed":
        this._supportedPermissions.forEach(function(aType){
          this.updatePermission(aType);
        }, this);
        break;
      case "passwordmgr-storage-changed":
        this.updatePermission("password");
        if (this._selectedSite) {
          this.updatePasswordsCount();
        }
        break;
      case "cookie-changed":
        if (this._selectedSite) {
          this.updateCookiesCount();
        }
        break;
      case "browser:purge-domain-data":
        this.deleteFromSitesList(aData);
        break;
    }
  },

  



  getSitesFromPlaces: function() {
    gSitesStmt.params.limit = this.PLACES_SITES_LIMIT;
    gSitesStmt.executeAsync({
      handleResult: function(aResults) {
        let row;
        while (row = aResults.getNextRow()) {
          let host = row.getResultByName("host");
          AboutPermissions.addHost(host);
        }
      },
      handleError: function(aError) {
        Cu.reportError("AboutPermissions: " + aError);
      },
      handleCompletion: function(aReason) {
        
        Services.obs.notifyObservers(null, "browser-permissions-initialized", null);
      }
    });
  },

  



  enumerateServices: function() {
    let logins = Services.logins.getAllLogins();
    logins.forEach(function(aLogin) {
      try {
        
        let uri = NetUtil.newURI(aLogin.hostname);
        this.addHost(uri.host);
      } catch (e) {
        
      }
    }, this);

    let disabledHosts = Services.logins.getAllDisabledHosts();
    disabledHosts.forEach(function(aHostname) {
      try {
        
        let uri = NetUtil.newURI(aHostname);
        this.addHost(uri.host);
      } catch (e) {
        
      }
    }, this);

    let (enumerator = Services.perms.enumerator) {
      while (enumerator.hasMoreElements()) {
        let permission = enumerator.getNext().QueryInterface(Ci.nsIPermission);
        
        if (this._supportedPermissions.indexOf(permission.type) != -1) {
          this.addHost(permission.host);
        }
      }
    }
  },

  





  addHost: function(aHost) {
    if (aHost in this._sites) {
      return;
    }
    let site = new Site(aHost);
    this._sites[aHost] = site;
    this.addToSitesList(site);
  },

  





  addToSitesList: function(aSite) {
    let item = document.createElement("richlistitem");
    item.setAttribute("class", "site");
    item.setAttribute("value", aSite.host);

    aSite.getFavicon(function(aURL) {
      item.setAttribute("favicon", aURL);
    });
    aSite.listitem = item;

    this.sitesList.appendChild(item);    
  },

  


  filterSitesList: function() {
    let siteItems = this.sitesList.children;
    let filterValue = document.getElementById("sites-filter").value.toLowerCase();

    if (filterValue == "") {
      for (let i = 0; i < siteItems.length; i++) {
        siteItems[i].collapsed = false;
      }
      return;
    }

    for (let i = 0; i < siteItems.length; i++) {
      let siteValue = siteItems[i].value.toLowerCase();
      siteItems[i].collapsed = siteValue.indexOf(filterValue) == -1;
    }
  },

  



  forgetSite: function() {
    this._selectedSite.forgetSite();
  },

  






  deleteFromSitesList: function(aHost) {
    for each (let site in this._sites) {
      if (site.host.hasRootDomain(aHost)) {
        if (site == this._selectedSite) {
          
          this.sitesList.selectedItem = document.getElementById("all-sites-item");
        }

        this.sitesList.removeChild(site.listitem);
        delete this._sites[site.host];
      }
    }    
  },

  


  onSitesListSelect: function(event) {
    if (event.target.selectedItem.id == "all-sites-item") {
      
      document.getElementById("site-label").value = "";
      this.manageDefaultPermissions();
      return;
    }

    let host = event.target.value;
    let site = this._selectedSite = this._sites[host];
    document.getElementById("site-label").value = host;
    document.getElementById("header-deck").selectedPanel =
      document.getElementById("site-header");

    this.updateVisitCount();
    this.updatePermissionsBox();
  },

  



  manageDefaultPermissions: function() {
    this._selectedSite = null;

    document.getElementById("header-deck").selectedPanel =
      document.getElementById("defaults-header");

    this.updatePermissionsBox();
  },

  


  updatePermissionsBox: function() {
    this._supportedPermissions.forEach(function(aType){
      this.updatePermission(aType);
    }, this);

    this.updatePasswordsCount();
    this.updateCookiesCount();
  },

  







  updatePermission: function(aType) {
    let allowItem = document.getElementById(aType + "-" + PermissionDefaults.ALLOW);
    if (!this._selectedSite &&
        this._noGlobalAllow.indexOf(aType) != -1) {
      allowItem.hidden = true;
      return;
    }

    allowItem.hidden = false;

    let permissionMenulist = document.getElementById(aType + "-menulist");
    let permissionValue;    
    if (!this._selectedSite) {

      
      permissionValue = PermissionDefaults[aType];
    } else if (aType == "password") {
      
      
      permissionValue = this._selectedSite.loginSavingEnabled ?
                        PermissionDefaults.ALLOW : PermissionDefaults.DENY;
    } else {
      let result = {};
      permissionValue = this._selectedSite.getPermission(aType, result) ?
                        result.value : PermissionDefaults[aType];
    }

    permissionMenulist.selectedItem = document.getElementById(aType + "-" + permissionValue);
  },

  onPermissionCommand: function(event) {
    let permissionType = event.currentTarget.getAttribute("type");
    let permissionValue = event.target.value;

    if (!this._selectedSite) {
      
      PermissionDefaults[permissionType] = permissionValue;
    } else if (permissionType == "password") {
      let isEnabled = permissionValue == PermissionDefaults.ALLOW;
      this._selectedSite.loginSavingEnabled = isEnabled;
    } else {
      this._selectedSite.setPermission(permissionType, permissionValue);
    }
  },

  updateVisitCount: function() {
    this._selectedSite.getVisitCount(function(aCount) {
      let visitForm = AboutPermissions._stringBundle.GetStringFromName("visitCount");
      let visitLabel = PluralForm.get(aCount, visitForm)
                                  .replace("#1", aCount);
      document.getElementById("site-visit-count").value = visitLabel;
    });  
  },

  updatePasswordsCount: function() {
    if (!this._selectedSite) {
      document.getElementById("passwords-count").hidden = true;
      document.getElementById("passwords-manage-all-button").hidden = false;
      return;
    }

    let passwordsCount = this._selectedSite.logins.length;
    let passwordsForm = this._stringBundle.GetStringFromName("passwordsCount");
    let passwordsLabel = PluralForm.get(passwordsCount, passwordsForm)
                                   .replace("#1", passwordsCount);

    document.getElementById("passwords-label").value = passwordsLabel;
    document.getElementById("passwords-manage-button").disabled = (passwordsCount < 1);
    document.getElementById("passwords-manage-all-button").hidden = true;
    document.getElementById("passwords-count").hidden = false;
  },

  


  managePasswords: function() {
    let selectedHost = "";
    if (this._selectedSite) {
      selectedHost = this._selectedSite.host;
    }

    let win = Services.wm.getMostRecentWindow("Toolkit:PasswordManager");
    if (win) {
      win.setFilter(selectedHost);
      win.focus();
    } else {
      window.openDialog("chrome://passwordmgr/content/passwordManager.xul",
                        "Toolkit:PasswordManager", "", {filterString : selectedHost});
    }
  },

  updateCookiesCount: function() {
    if (!this._selectedSite) {
      document.getElementById("cookies-count").hidden = true;
      document.getElementById("cookies-clear-all-button").hidden = false;
      document.getElementById("cookies-manage-all-button").hidden = false;
      return;
    }

    let cookiesCount = this._selectedSite.cookies.length;
    let cookiesForm = this._stringBundle.GetStringFromName("cookiesCount");
    let cookiesLabel = PluralForm.get(cookiesCount, cookiesForm)
                                 .replace("#1", cookiesCount);

    document.getElementById("cookies-label").value = cookiesLabel;
    document.getElementById("cookies-clear-button").disabled = (cookiesCount < 1);
    document.getElementById("cookies-manage-button").disabled = (cookiesCount < 1);
    document.getElementById("cookies-clear-all-button").hidden = true;
    document.getElementById("cookies-manage-all-button").hidden = true;
    document.getElementById("cookies-count").hidden = false;
  },

  


  clearCookies: function() {
    if (!this._selectedSite) {
      return;
    }
    let site = this._selectedSite;
    site.clearCookies(site.cookies);
    this.updateCookiesCount();
  },

  


  manageCookies: function() {
    let selectedHost = "";
    if (this._selectedSite) {
      selectedHost = this._selectedSite.host;
    }

    let win = Services.wm.getMostRecentWindow("Browser:Cookies");
    if (win) {
      win.gCookiesWindow.setFilter(selectedHost);
      win.focus();
    } else {
      window.openDialog("chrome://browser/content/preferences/cookies.xul",
                        "Browser:Cookies", "", {filterString : selectedHost});
    }
  }
}


String.prototype.hasRootDomain = function hasRootDomain(aDomain) {
  let index = this.indexOf(aDomain);
  if (index == -1)
    return false;

  if (this == aDomain)
    return true;

  let prevChar = this[index - 1];
  return (index == (this.length - aDomain.length)) &&
         (prevChar == "." || prevChar == "/");
}
