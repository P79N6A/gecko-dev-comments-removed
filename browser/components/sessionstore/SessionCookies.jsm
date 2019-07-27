



"use strict";

this.EXPORTED_SYMBOLS = ["SessionCookies"];

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, "Utils",
  "resource:///modules/sessionstore/Utils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivacyLevel",
  "resource:///modules/sessionstore/PrivacyLevel.jsm");


const MAX_EXPIRY = Math.pow(2, 62);




this.SessionCookies = Object.freeze({
  update: function (windows) {
    SessionCookiesInternal.update(windows);
  },

  getHostsForWindow: function (window, checkPrivacy = false) {
    return SessionCookiesInternal.getHostsForWindow(window, checkPrivacy);
  },

  restore(cookies) {
    SessionCookiesInternal.restore(cookies);
  }
});




let SessionCookiesInternal = {
  


  _initialized: false,

  








  update: function (windows) {
    this._ensureInitialized();

    for (let window of windows) {
      let cookies = [];

      
      let hosts = this.getHostsForWindow(window, true);

      for (let host of Object.keys(hosts)) {
        let isPinned = hosts[host];

        for (let cookie of CookieStore.getCookiesForHost(host)) {
          
          
          
          if (PrivacyLevel.canSave({isHttps: cookie.secure, isPinned: isPinned})) {
            cookies.push(cookie);
          }
        }
      }

      
      if (cookies.length) {
        window.cookies = cookies;
      } else if ("cookies" in window) {
        delete window.cookies;
      }
    }
  },

  












  getHostsForWindow: function (window, checkPrivacy = false) {
    let hosts = {};

    for (let tab of window.tabs) {
      for (let entry of tab.entries) {
        this._extractHostsFromEntry(entry, hosts, checkPrivacy, tab.pinned);
      }
    }

    return hosts;
  },

  


  restore(cookies) {
    for (let cookie of cookies) {
      let expiry = "expiry" in cookie ? cookie.expiry : MAX_EXPIRY;
      Services.cookies.add(cookie.host, cookie.path || "", cookie.name || "",
                           cookie.value, !!cookie.secure, !!cookie.httponly,
                            true, expiry);
    }
  },

  



  observe: function (subject, topic, data) {
    switch (data) {
      case "added":
      case "changed":
        this._updateCookie(subject);
        break;
      case "deleted":
        this._removeCookie(subject);
        break;
      case "cleared":
        CookieStore.clear();
        break;
      case "batch-deleted":
        this._removeCookies(subject);
        break;
      case "reload":
        CookieStore.clear();
        this._reloadCookies();
        break;
      default:
        throw new Error("Unhandled cookie-changed notification.");
    }
  },

  



  _ensureInitialized: function () {
    if (!this._initialized) {
      this._reloadCookies();
      this._initialized = true;
      Services.obs.addObserver(this, "cookie-changed", false);
    }
  },

  













  _extractHostsFromEntry: function (entry, hosts, checkPrivacy, isPinned) {
    let host = entry._host;
    let scheme = entry._scheme;

    
    
    
    
    if (!host && !scheme) {
      try {
        let uri = Utils.makeURI(entry.url);
        host = uri.host;
        scheme = uri.scheme;
        this._extractHostsFromHostScheme(host, scheme, hosts, checkPrivacy, isPinned);
      }
      catch (ex) { }
    }

    if (entry.children) {
      for (let child of entry.children) {
        this._extractHostsFromEntry(child, hosts, checkPrivacy, isPinned);
      }
    }
  },

  















  _extractHostsFromHostScheme:
    function (host, scheme, hosts, checkPrivacy, isPinned) {
    
    
    if (/https?/.test(scheme) && !hosts[host] &&
        (!checkPrivacy ||
         PrivacyLevel.canSave({isHttps: scheme == "https", isPinned: isPinned}))) {
      
      
      hosts[host] = isPinned;
    } else if (scheme == "file") {
      hosts[host] = true;
    }
  },

  


  _updateCookie: function (cookie) {
    cookie.QueryInterface(Ci.nsICookie2);

    if (cookie.isSession) {
      CookieStore.set(cookie);
    }
  },

  


  _removeCookie: function (cookie) {
    cookie.QueryInterface(Ci.nsICookie2);

    if (cookie.isSession) {
      CookieStore.delete(cookie);
    }
  },

  


  _removeCookies: function (cookies) {
    for (let i = 0; i < cookies.length; i++) {
      this._removeCookie(cookies.queryElementAt(i, Ci.nsICookie2));
    }
  },

  



  _reloadCookies: function () {
    let iter = Services.cookies.enumerator;
    while (iter.hasMoreElements()) {
      this._updateCookie(iter.getNext());
    }
  }
};


















function* getPossibleSubdomainVariants(host) {
  
  yield "." + host;

  
  let parts = host.split(".");
  if (parts.length < 3) {
    return;
  }

  
  let rest = parts.slice(1).join(".");

  
  yield* getPossibleSubdomainVariants(rest);
}





let CookieStore = {
  



























  _hosts: new Map(),

  





  getCookiesForHost: function (host) {
    let cookies = [];

    let appendCookiesForHost = host => {
      if (!this._hosts.has(host)) {
        return;
      }

      for (let pathToNamesMap of this._hosts.get(host).values()) {
        cookies.push(...pathToNamesMap.values());
      }
    }

    
    
    
    
    
    
    
    for (let variant of [host, ...getPossibleSubdomainVariants(host)]) {
      appendCookiesForHost(variant);
    }

    return cookies;
  },

  





  set: function (cookie) {
    let jscookie = {host: cookie.host, value: cookie.value};

    
    if (cookie.path) {
      jscookie.path = cookie.path;
    }

    if (cookie.name) {
      jscookie.name = cookie.name;
    }

    if (cookie.isSecure) {
      jscookie.secure = true;
    }

    if (cookie.isHttpOnly) {
      jscookie.httponly = true;
    }

    if (cookie.expiry < MAX_EXPIRY) {
      jscookie.expiry = cookie.expiry;
    }

    this._ensureMap(cookie).set(cookie.name, jscookie);
  },

  





  delete: function (cookie) {
    this._ensureMap(cookie).delete(cookie.name);
  },

  


  clear: function () {
    this._hosts.clear();
  },

  








  _ensureMap: function (cookie) {
    if (!this._hosts.has(cookie.host)) {
      this._hosts.set(cookie.host, new Map());
    }

    let pathToNamesMap = this._hosts.get(cookie.host);

    if (!pathToNamesMap.has(cookie.path)) {
      pathToNamesMap.set(cookie.path, new Map());
    }

    return pathToNamesMap.get(cookie.path);
  }
};
