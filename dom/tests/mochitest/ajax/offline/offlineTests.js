
netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

var Cc = Components.classes;
var Ci = Components.interfaces;

const kNetBase = 2152398848; 
var NS_ERROR_CACHE_KEY_NOT_FOUND = kNetBase + 61;
var NS_ERROR_CACHE_KEY_WAIT_FOR_VALIDATION = kNetBase + 64;


function OfflineCacheContents(urls) {
  this.urls = urls;
  this.contents = {};
}

OfflineCacheContents.prototype = {
QueryInterface: function(iid) {
    if (!iid.equals(Ci.nsISupports) &&
        !iid.equals(Ci.nsICacheListener)) {
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
    return this;
  },
onCacheEntryAvailable: function(desc, accessGranted, status) {
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

    if (!desc) {
      this.fetch(this.callback);
      return;
    }

    var stream = desc.QueryInterface(Ci.nsICacheEntryDescriptor).openInputStream(0);
    var sstream = Components.classes["@mozilla.org/scriptableinputstream;1"]
                 .createInstance(Components.interfaces.nsIScriptableInputStream);
    sstream.init(stream);
    this.contents[desc.key] = sstream.read(sstream.available());
    sstream.close();
    desc.close();
    this.fetch(this.callback);
  },

fetch: function(callback)
{
  this.callback = callback;
  if (this.urls.length == 0) {
    callback(this.contents);
    return;
  }

  var url = this.urls.shift();
  var self = this;

  var cacheSession = OfflineTest.getActiveSession();
  cacheSession.asyncOpenCacheEntry(url, Ci.nsICache.ACCESS_READ, this);
}
};

var OfflineTest = {

_slaveWindow: null,


_masterWindow: null,


_pathOverrides: [],

setupChild: function()
{
  if (window.parent.OfflineTest.hasSlave()) {
    return false;
  }

  this._slaveWindow = null;
  this._masterWindow = window.top;

  return true;
},



setup: function()
{
  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

  if (!window.opener || !window.opener.OfflineTest ||
      !window.opener.OfflineTest._isMaster) {
    
    
    
    
    
    
    var pm = Cc["@mozilla.org/permissionmanager;1"]
      .getService(Ci.nsIPermissionManager);
    var uri = Cc["@mozilla.org/network/io-service;1"]
      .getService(Ci.nsIIOService)
      .newURI(window.location.href, null, null);
    pm.add(uri, "offline-app", Ci.nsIPermissionManager.ALLOW_ACTION);

    
    
    this._isMaster = true;
    this._slaveWindow = window.open(window.location, "offlinetest");

    this._slaveWindow._OfflineSlaveWindow = true;

    return false;
  }

  this._masterWindow = window.opener;

  return true;
},

teardown: function()
{
  

  netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");

  var pm = Cc["@mozilla.org/permissionmanager;1"]
           .getService(Ci.nsIPermissionManager);
  var uri = Cc["@mozilla.org/network/io-service;1"]
            .getService(Ci.nsIIOService)
            .newURI(window.location.href, null, null);
  pm.remove(uri.host, "offline-app");

  
  for (override in this._pathOverrides)
    this.deleteData(this._pathOverrides[override]);

  this.clear();
},

finish: function()
{
  SimpleTest.finish();

  if (this._masterWindow) {
    this._masterWindow.OfflineTest.finish();
    window.close();
  }
},

hasSlave: function()
{
  return (this._slaveWindow != null);
},




ok: function(condition, name, diag)
{
  return this._masterWindow.SimpleTest.ok(condition, name, diag);
},

is: function(a, b, name)
{
  return this._masterWindow.SimpleTest.is(a, b, name);
},

isnot: function(a, b, name)
{
  return this._masterWindow.SimpleTest.isnot(a, b, name);
},

todo: function(a, name)
{
  return this._masterWindow.SimpleTest.todo(a, name);
},

clear: function()
{
  
  var applicationCache = this.getActiveCache();
  if (applicationCache) {
    applicationCache.discard();
  }
},

failEvent: function(e)
{
  OfflineTest.ok(false, "Unexpected event: " + e.type);
},



waitForAdd: function(url, onFinished) {
  
  var numChecks = 20;
  var waitFunc = function() {
    var cacheSession = OfflineTest.getActiveSession();
    var entry;
    try {
      var entry = cacheSession.openCacheEntry(url, Ci.nsICache.ACCESS_READ, false);
    } catch (e) {
    }

    if (entry) {
      entry.close();
      onFinished();
      return;
    }

    if (--numChecks == 0) {
      onFinished();
      return;
    }

    setTimeout(OfflineTest.priv(waitFunc), 500);
  }

  setTimeout(this.priv(waitFunc), 500);
},

getManifestUrl: function()
{
  return window.top.document.documentElement.getAttribute("manifest");
},

getActiveCache: function()
{
  
  
  var serv = Cc["@mozilla.org/network/application-cache-service;1"]
             .getService(Ci.nsIApplicationCacheService);
  return serv.getActiveCache(this.getManifestUrl());
},

getActiveSession: function()
{
  var cache = this.getActiveCache();
  if (!cache) return null;

  var cacheService = Cc["@mozilla.org/network/cache-service;1"]
                     .getService(Ci.nsICacheService);
  return cacheService.createSession(cache.clientID,
                                    Ci.nsICache.STORE_OFFLINE,
                                    true);
},

priv: function(func)
{
  var self = this;
  return function() {
    netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
    func(arguments);
  }
},

checkCustomCache: function(group, url, expectEntry)
{
  var serv = Cc["@mozilla.org/network/application-cache-service;1"]
             .getService(Ci.nsIApplicationCacheService);
  var cache = serv.getActiveCache(group);
  var cacheSession = null;
  if (cache) {
    var cacheService = Cc["@mozilla.org/network/cache-service;1"]
                       .getService(Ci.nsICacheService);
    cacheSession = cacheService.createSession(cache.clientID,
                                      Ci.nsICache.STORE_OFFLINE,
                                      true);
  }

  this._checkCache(cacheSession, url, expectEntry);
},

checkCache: function(url, expectEntry)
{
  var cacheSession = this.getActiveSession();
  this._checkCache(cacheSession, url, expectEntry);
},

_checkCache: function(cacheSession, url, expectEntry)
{
  if (!cacheSession) {
    if (expectEntry) {
      this.ok(false, url + " should exist in the offline cache");
    } else {
      this.ok(true, url + " should not exist in the offline cache");
    }
    return;
  }

  try {
    var entry = cacheSession.openCacheEntry(url, Ci.nsICache.ACCESS_READ, false);
    if (expectEntry) {
      this.ok(true, url + " should exist in the offline cache");
    } else {
      this.ok(false, url + " should not exist in the offline cache");
    }
    entry.close();
  } catch (e) {
    if (e.result == NS_ERROR_CACHE_KEY_NOT_FOUND) {
      if (expectEntry) {
        this.ok(false, url + " should exist in the offline cache");
      } else {
        this.ok(true, url + " should not exist in the offline cache");
      }
    } else if (e.result == NS_ERROR_CACHE_KEY_WAIT_FOR_VALIDATION) {
      
      if (expectEntry) {
        this.ok(true, url + " should exist in the offline cache");
      } else {
        this.ok(false, url + " should not exist in the offline cache");
      }
    } else {
      throw e;
    }
  }
},

putData: function(serverPath, contentType, data)
{
  if (!data.length)
    throw "Data length mush be specified";

  var client = new XMLHttpRequest();
  client.open("PUT", serverPath, false);
  client.setRequestHeader("Content-Type", contentType);
  client.send(data);

  this._pathOverrides.push(serverPath);
},

deleteData: function(serverPath)
{
  delete this._pathOverrides[serverPath];

  var client = new XMLHttpRequest();
  client.open("DELETE", serverPath, false);
  client.send();
}

};
