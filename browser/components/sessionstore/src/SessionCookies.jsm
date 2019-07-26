



"use strict";

this.EXPORTED_SYMBOLS = ["SessionCookies"];

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm", this);


const MAX_EXPIRY = Math.pow(2, 62);





this.SessionCookies = Object.freeze({
  getCookiesForHost: function (host) {
    return SessionCookiesInternal.getCookiesForHost(host);
  }
});




let SessionCookiesInternal = {
  


  _initialized: false,

  


  getCookiesForHost: function (host) {
    this._ensureInitialized();
    return CookieStore.getCookiesForHost(host);
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





let CookieStore = {
  






















  _hosts: new Map(),

  





  getCookiesForHost: function (host) {
    if (!this._hosts.has(host)) {
      return [];
    }

    let cookies = [];

    for (let pathToNamesMap of this._hosts.get(host).values()) {
      cookies = cookies.concat([cookie for (cookie of pathToNamesMap.values())]);
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
