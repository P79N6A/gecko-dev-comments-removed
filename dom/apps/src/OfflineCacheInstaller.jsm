



"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
const CC = Components.Constructor;

let EXPORTED_SYMBOLS = ["OfflineCacheInstaller"];

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");

let Namespace = CC('@mozilla.org/network/application-cache-namespace;1',
                   'nsIApplicationCacheNamespace',
                   'init');
let makeFile = CC('@mozilla.org/file/local;1',
                'nsIFile',
                'initWithPath');
const nsICache = Ci.nsICache;
const nsIApplicationCache = Ci.nsIApplicationCache;
const applicationCacheService =
  Cc['@mozilla.org/network/application-cache-service;1']
    .getService(Ci.nsIApplicationCacheService);


function debug(aMsg) {
  
}


function enableOfflineCacheForApp(origin, appId) {
  let originURI = Services.io.newURI(origin, null, null);
  let principal = Services.scriptSecurityManager.getAppCodebasePrincipal(
                    originURI, appId, false);
  Services.perms.addFromPrincipal(principal, 'offline-app',
                                  Ci.nsIPermissionManager.ALLOW_ACTION);
  
  Services.perms.addFromPrincipal(principal, 'pin-app',
                                  Ci.nsIPermissionManager.ALLOW_ACTION);
}


function storeCache(applicationCache, url, file, itemType) {
  let session = Services.cache.createSession(applicationCache.clientID,
                                             nsICache.STORE_OFFLINE, true);
  session.asyncOpenCacheEntry(url, nsICache.ACCESS_WRITE, {
    onCacheEntryAvailable: function (cacheEntry, accessGranted, status) {
      cacheEntry.setMetaDataElement('request-method', 'GET');
      cacheEntry.setMetaDataElement('response-head', 'HTTP/1.1 200 OK\r\n');

      let outputStream = cacheEntry.openOutputStream(0);

      
      let inputStream = Cc['@mozilla.org/network/file-input-stream;1']
                          .createInstance(Ci.nsIFileInputStream);
      inputStream.init(file, 1, -1, null);
      let bufferedOutputStream = Cc['@mozilla.org/network/buffered-output-stream;1']
                                   .createInstance(Ci.nsIBufferedOutputStream);
      bufferedOutputStream.init(outputStream, 1024);
      bufferedOutputStream.writeFrom(inputStream, inputStream.available());
      bufferedOutputStream.flush();
      bufferedOutputStream.close();
      outputStream.close();
      inputStream.close();

      cacheEntry.markValid();
      debug (file.path + ' -> ' + url + ' (' + itemType + ')');
      applicationCache.markEntry(url, itemType);
      cacheEntry.close();
    }
  });
}

function readFile(aFile, aCallback) {
  let channel = NetUtil.newChannel(aFile);
  channel.contentType = "pain/text";
  NetUtil.asyncFetch(channel, function(aStream, aResult) {
    if (!Components.isSuccessCode(aResult)) {
      Cu.reportError("OfflineCacheInstaller: Could not read file " + aFile.path);
      if (aCallback)
        aCallback(null);
      return;
    }

    
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                      .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    let data = NetUtil.readInputStreamToString(aStream,
                                               aStream.available());
    aCallback(converter.ConvertToUnicode(data));
  });
}

const OfflineCacheInstaller = {
  installCache: function installCache(app) {
    let cacheDir = makeFile(app.basePath)
    cacheDir.append(app.appId);
    cacheDir.append("cache");
    if (!cacheDir.exists())
      return;

    let cacheManifest = cacheDir.clone();
    cacheManifest.append("manifest.appcache");
    if (!cacheManifest.exists())
      return;

    enableOfflineCacheForApp(app.origin, app.localId);

    
    let appcacheURL = app.origin + "cache/manifest.appcache";

    
    
    
    let groupID = appcacheURL + '#' + app.localId+ '+f';
    let applicationCache = applicationCacheService.createApplicationCache(groupID);
    applicationCache.activate();

    readFile(cacheManifest, function (content) {
      let lines = content.split(/\r?\n/);
      
      
      let urls = [];
      for(let i = 0; i < lines.length; i++) {
        let line = lines[i];
        
        if (/^#/.test(line) || !line.length)
          continue;
        if (line == 'CACHE MANIFEST')
          continue;
        if (line == 'CACHE:')
          continue;
        
        if (line == 'NETWORK:')
          break;

        
        if (line[0] == '/') {
          urls.push(app.origin + line.substring(1));
        
        } else if (line.substr(0, 4) == 'http') {
          urls.push(line);
        } else {
          throw new Error('Invalid line in appcache manifest:\n' + line +
                          '\nFrom: ' + cacheManifest.path);
        }
      }
      urls.forEach(function processCachedFile(url) {
        
        let path = url.replace(/https?:\/\//, '');
        let file = cacheDir.clone();
        let paths = path.split('/');
        paths.forEach(file.append);

        if (!file.exists()) {
          let msg = 'File ' + file.path + ' exists in the manifest but does ' +
                    'not points to a real file.';
          throw new Error(msg);
        }

        let itemType = nsIApplicationCache.ITEM_EXPLICIT;
        storeCache(applicationCache, url, file, itemType);
      });
    });
  }
};

