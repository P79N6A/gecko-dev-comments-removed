



"use strict";

let Ci = Components.interfaces;
let Cc = Components.classes;
let Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/DownloadUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/ForgetAboutSite.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
                                  "resource://gre/modules/PluralForm.jsm");

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





let TEST_EXACT_PERM_TYPES = ["geo", "camera", "microphone"];




function Site(host) {
  this.host = host;
  this.listitem = null;

  this.httpURI = NetUtil.newURI("http://" + this.host);
  this.httpsURI = NetUtil.newURI("https://" + this.host);
}

Site.prototype = {
  






  getFavicon: function Site_getFavicon(aCallback) {
    function invokeCallback(aFaviconURI) {
      try {
        
        
        aCallback(gFaviconService.getFaviconLinkForIcon(aFaviconURI).spec);
      } catch (e) {
        Cu.reportError("AboutPermissions: " + e);
      }
    }

    
    gFaviconService.getFaviconURLForPage(this.httpsURI, function (aURI) {
      if (aURI) {
        invokeCallback(aURI);
      } else {
        gFaviconService.getFaviconURLForPage(this.httpURI, function (aURI) {
          if (aURI) {
            invokeCallback(aURI);
          }
        });
      }
    }.bind(this));
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
    
    
    if (aType == "password") {
      aResultObj.value =  this.loginSavingEnabled ?
                          Ci.nsIPermissionManager.ALLOW_ACTION :
                          Ci.nsIPermissionManager.DENY_ACTION;
      return true;
    }

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
    
    
    if (aType == "password") {
      this.loginSavingEnabled = aPerm == Ci.nsIPermissionManager.ALLOW_ACTION;
      return;
    }

    
    
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
    let httpLogins = Services.logins.findLogins({}, this.httpURI.prePath, "", "");
    let httpsLogins = Services.logins.findLogins({}, this.httpsURI.prePath, "", "");
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
    ForgetAboutSite.removeDataFromDomain(this.host);
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
    let value = (aValue != this.DENY);
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

    if (Services.prefs.getIntPref("network.cookie.lifetimePolicy") == this.COOKIE_SESSION) {
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
    let value = (aValue != this.DENY);
    Services.prefs.setBoolPref("geo.enabled", value);
  },

  get indexedDB() {
    if (!Services.prefs.getBoolPref("dom.indexedDB.enabled")) {
      return this.DENY;
    }
    
    
    return this.UNKNOWN;
  },
  set indexedDB(aValue) {
    let value = (aValue != this.DENY);
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
  },

  get fullscreen() {
    if (!Services.prefs.getBoolPref("full-screen-api.enabled")) {
      return this.DENY;
    }
    return this.UNKNOWN;
  },
  set fullscreen(aValue) {
    let value = (aValue != this.DENY);
    Services.prefs.setBoolPref("full-screen-api.enabled", value);
  },
  get push() {
    if (!Services.prefs.getBoolPref("dom.push.enabled")) {
      return this.DENY;
    }
    return this.UNKNOWN;
  },
  set push(aValue) {
    let value = (aValue != this.DENY);
    Services.prefs.setBoolPref("dom.push.enabled", value);
  },
  get camera() this.UNKNOWN,
  get microphone() this.UNKNOWN
};




let AboutPermissions = {
  


  PLACES_SITES_LIMIT: 50,

  


  LIST_BUILD_CHUNK: 5, 
  LIST_BUILD_DELAY: 100, 

  


  _sites: {},

  sitesList: null,
  _selectedSite: null,

  


  _initPlacesDone: false,
  _initServicesDone: false,

  






  _supportedPermissions: ["password", "cookie", "geo", "indexedDB", "popup",
                          "fullscreen", "camera", "microphone", "push"],

  


  _noGlobalAllow: ["geo", "indexedDB", "fullscreen", "camera", "microphone", "push"],

  


  _noGlobalDeny: ["camera", "microphone"],

  _stringBundle: Services.strings.
                 createBundle("chrome://browser/locale/preferences/aboutPermissions.properties"),

  


  init: function() {
    this.sitesList = document.getElementById("sites-list");

    this.getSitesFromPlaces();

    this.enumerateServicesGenerator = this.getEnumerateServicesGenerator();
    setTimeout(this.enumerateServicesDriver.bind(this), this.LIST_BUILD_DELAY);

    
    Services.prefs.addObserver("signon.rememberSignons", this, false);
    Services.prefs.addObserver("network.cookie.", this, false);
    Services.prefs.addObserver("geo.enabled", this, false);
    Services.prefs.addObserver("dom.push.enabled", this, false);
    Services.prefs.addObserver("dom.indexedDB.enabled", this, false);
    Services.prefs.addObserver("dom.disable_open_during_load", this, false);
    Services.prefs.addObserver("full-screen-api.enabled", this, false);
    Services.prefs.addObserver("dom.push.enabled", this, false);

    Services.obs.addObserver(this, "perm-changed", false);
    Services.obs.addObserver(this, "passwordmgr-storage-changed", false);
    Services.obs.addObserver(this, "cookie-changed", false);
    Services.obs.addObserver(this, "browser:purge-domain-data", false);

    this._observersInitialized = true;
    Services.obs.notifyObservers(null, "browser-permissions-preinit", null);
  },

  


  cleanUp: function() {
    if (this._observersInitialized) {
      Services.prefs.removeObserver("signon.rememberSignons", this, false);
      Services.prefs.removeObserver("network.cookie.", this, false);
      Services.prefs.removeObserver("geo.enabled", this, false);
      Services.prefs.removeObserver("dom.push.enabled", this, false);
      Services.prefs.removeObserver("dom.indexedDB.enabled", this, false);
      Services.prefs.removeObserver("dom.disable_open_during_load", this, false);
      Services.prefs.removeObserver("full-screen-api.enabled", this, false);
      Services.prefs.removeObserver("dom.push.enabled", this, false);

      Services.obs.removeObserver(this, "perm-changed");
      Services.obs.removeObserver(this, "passwordmgr-storage-changed");
      Services.obs.removeObserver(this, "cookie-changed");
      Services.obs.removeObserver(this, "browser:purge-domain-data");
    }

    gSitesStmt.finalize();
    gVisitStmt.finalize();
    gPlacesDatabase.asyncClose(null);
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
        AboutPermissions.startSitesListBatch();
        let row;
        while (row = aResults.getNextRow()) {
          let host = row.getResultByName("host");
          AboutPermissions.addHost(host);
        }
        AboutPermissions.endSitesListBatch();
      },
      handleError: function(aError) {
        Cu.reportError("AboutPermissions: " + aError);
      },
      handleCompletion: function(aReason) {
        
        AboutPermissions._initPlacesDone = true;
        if (AboutPermissions._initServicesDone) {
          Services.obs.notifyObservers(null, "browser-permissions-initialized", null);
        }
      }
    });
  },

  


  enumerateServicesDriver: function() {
    if (this.enumerateServicesGenerator.next()) {
      
      let delay = Math.min(this.sitesList.itemCount * 5, this.LIST_BUILD_DELAY);
      setTimeout(this.enumerateServicesDriver.bind(this), delay);
    } else {
      this.enumerateServicesGenerator.close();
      this._initServicesDone = true;
      if (this._initPlacesDone) {
        Services.obs.notifyObservers(null, "browser-permissions-initialized", null);
      }
    }
  },

  



  getEnumerateServicesGenerator: function() {
    let itemCnt = 1;

    let logins = Services.logins.getAllLogins();
    logins.forEach(function(aLogin) {
      if (itemCnt % this.LIST_BUILD_CHUNK == 0) {
        yield true;
      }
      try {
        
        let uri = NetUtil.newURI(aLogin.hostname);
        this.addHost(uri.host);
      } catch (e) {
        
      }
      itemCnt++;
    }, this);

    let disabledHosts = Services.logins.getAllDisabledHosts();
    disabledHosts.forEach(function(aHostname) {
      if (itemCnt % this.LIST_BUILD_CHUNK == 0) {
        yield true;
      }
      try {
        
        let uri = NetUtil.newURI(aHostname);
        this.addHost(uri.host);
      } catch (e) {
        
      }
      itemCnt++;
    }, this);

    let enumerator = Services.perms.enumerator;
    while (enumerator.hasMoreElements()) {
      if (itemCnt % this.LIST_BUILD_CHUNK == 0) {
        yield true;
      }
      let permission = enumerator.getNext().QueryInterface(Ci.nsIPermission);
      
      if (this._supportedPermissions.indexOf(permission.type) != -1) {
        this.addHost(permission.host);
      }
      itemCnt++;
    }

    yield false;
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

    
    let filterValue = document.getElementById("sites-filter").value.toLowerCase();
    item.collapsed = aSite.host.toLowerCase().indexOf(filterValue) == -1;

    (this._listFragment || this.sitesList).appendChild(item);
  },

  startSitesListBatch: function () {
    if (!this._listFragment)
      this._listFragment = document.createDocumentFragment();
  },

  endSitesListBatch: function () {
    if (this._listFragment) {
      this.sitesList.appendChild(this._listFragment);
      this._listFragment = null;
    }
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
    for (let host in this._sites) {
      let site = this._sites[host];
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
    allowItem.hidden = !this._selectedSite &&
                       this._noGlobalAllow.indexOf(aType) != -1;
    let denyItem = document.getElementById(aType + "-" + PermissionDefaults.DENY);
    denyItem.hidden = !this._selectedSite &&
                      this._noGlobalDeny.indexOf(aType) != -1;

    let permissionMenulist = document.getElementById(aType + "-menulist");
    let permissionValue;
    if (!this._selectedSite) {
      
      permissionValue = PermissionDefaults[aType];
      if (aType == "cookie")
	
	
	document.getElementById("cookie-9").hidden = true;
    } else {
      if (aType == "cookie")
        document.getElementById("cookie-9").hidden = false;
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
