



"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
const CC = Components.Constructor;

this.EXPORTED_SYMBOLS = ["OfflineCacheInstaller"];

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");

let Namespace = CC('@mozilla.org/network/application-cache-namespace;1',
                   'nsIApplicationCacheNamespace',
                   'init');
let makeFile = CC('@mozilla.org/file/local;1',
                'nsIFile',
                'initWithPath');
let MutableArray = CC('@mozilla.org/array;1', 'nsIMutableArray');

let {LoadContextInfo} = Cu.import("resource://gre/modules/LoadContextInfo.jsm", {});

const nsICacheStorage = Ci.nsICacheStorage;
const nsIApplicationCache = Ci.nsIApplicationCache;
const applicationCacheService =
  Cc['@mozilla.org/network/application-cache-service;1']
    .getService(Ci.nsIApplicationCacheService);


function debug(aMsg) {
  
}


function enableOfflineCacheForApp(aPrincipal) {
  Services.perms.addFromPrincipal(aPrincipal, 'offline-app',
                                  Ci.nsIPermissionManager.ALLOW_ACTION);
  
  Services.perms.addFromPrincipal(aPrincipal, 'pin-app',
                                  Ci.nsIPermissionManager.ALLOW_ACTION);
}


function storeCache(applicationCache, url, file, itemType, metadata) {
  let storage =
    Services.cache2.appCacheStorage(LoadContextInfo.default, applicationCache);
  let uri = Services.io.newURI(url, null, null);
  let nowGMT = new Date().toGMTString();
  metadata = metadata || {};
  metadata.lastFetched = metadata.lastFetched || nowGMT;
  metadata.lastModified = metadata.lastModified || nowGMT;
  storage.asyncOpenURI(uri, "", nsICacheStorage.OPEN_TRUNCATE, {
    onCacheEntryAvailable:
      function (cacheEntry, isNew, appCache, result) {
        cacheEntry.setMetaDataElement("request-method", "GET");
        cacheEntry.setMetaDataElement("response-head",
          "HTTP/1.1 200 OK\r\n" +
          "Date: " + metadata.lastFetched + "\r\n" +
          "Last-Modified: " + metadata.lastModified + "\r\n" +
          "Cache-Control: no-cache\r\n");

        let outputStream = cacheEntry.openOutputStream(0);

        
        
        let inputStream = Cc["@mozilla.org/network/file-input-stream;1"]
                            .createInstance(Ci.nsIFileInputStream);
        inputStream.init(file, 1, -1, null);
        let bufferedOutputStream =
          Cc["@mozilla.org/network/buffered-output-stream;1"]
            .createInstance(Ci.nsIBufferedOutputStream);
        bufferedOutputStream.init(outputStream, 1024);
        bufferedOutputStream.writeFrom(inputStream, inputStream.available());
        bufferedOutputStream.flush();
        bufferedOutputStream.close();
        inputStream.close();

        cacheEntry.setExpirationTime(0);
        cacheEntry.markValid();
        debug (file.path + " -> " + url + " (" + itemType + ")");
        applicationCache.markEntry(url, itemType);
        cacheEntry.close();
      }
  });
}

function readFile(aFile, aPrincipal, aCallback) {

  let channel = NetUtil.newChannel({
    uri: NetUtil.newURI(aFile),
    loadingPrincipal: aPrincipal,
    contentPolicyType: Ci.nsIContentPolicy.TYPE_OTHER});
  channel.contentType = "plain/text";
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

function parseCacheLine(app, urls, line) {
  try {
    let url = Services.io.newURI(line, null, app.origin);
    urls.push(url.spec);
  } catch(e) {
    throw new Error('Unable to parse cache line: ' + line + '(' + e + ')');
  }
}

function parseFallbackLine(app, urls, namespaces, fallbacks, line) {
  let split = line.split(/[ \t]+/);
  if (split.length != 2) {
    throw new Error('Should be made of two URLs seperated with spaces')
  }
  let type = Ci.nsIApplicationCacheNamespace.NAMESPACE_FALLBACK;
  let [ namespace, fallback ] = split;

  
  try {
    namespace = Services.io.newURI(namespace, null, app.origin).spec;
    fallback = Services.io.newURI(fallback, null, app.origin).spec;
  } catch(e) {
    throw new Error('Unable to parse fallback line: ' + line + '(' + e + ')');
  }

  namespaces.push([type, namespace, fallback]);
  fallbacks.push(fallback);
  urls.push(fallback);
}

function parseNetworkLine(namespaces, line) {
  let type = Ci.nsIApplicationCacheNamespace.NAMESPACE_BYPASS;
  if (line[0] == '*' && (line.length == 1 || line[1] == ' '
                                          || line[1] == '\t')) {
    namespaces.push([type, '', '']);
  } else {
    namespaces.push([type, namespace, '']);
  }
}

function parseAppCache(app, path, content) {
  let lines = content.split(/\r?\n/);

  let urls = [];
  let namespaces = [];
  let fallbacks = [];

  let currentSection = 'CACHE';
  for (let i = 0; i < lines.length; i++) {
    let line = lines[i];

    
    if (/^#/.test(line) || !line.length)
      continue;

    
    if (line == 'CACHE MANIFEST')
      continue;
    if (line == 'CACHE:') {
      currentSection = 'CACHE';
      continue;
    } else if (line == 'NETWORK:') {
      currentSection = 'NETWORK';
      continue;
    } else if (line == 'FALLBACK:') {
      currentSection = 'FALLBACK';
      continue;
    }

    
    try {
      if (currentSection == 'CACHE') {
        parseCacheLine(app, urls, line);
      } else if (currentSection == 'NETWORK') {
        parseNetworkLine(namespaces, line);
      } else if (currentSection == 'FALLBACK') {
        parseFallbackLine(app, urls, namespaces, fallbacks, line);
      }
    } catch(e) {
      throw new Error('Invalid ' + currentSection + ' line in appcache ' +
                      'manifest:\n' + e.message +
                      '\nFrom: ' + path +
                      '\nLine ' + i + ': ' + line);
    }
  }

  return {
    urls: urls,
    namespaces: namespaces,
    fallbacks: fallbacks
  };
}

function installCache(app) {
  if (!app.cachePath) {
    return;
  }

  let cacheDir = makeFile(app.cachePath);
  cacheDir.append(app.appId);

  let resourcesMetadata = cacheDir.clone();
  resourcesMetadata.append('resources_metadata.json');

  cacheDir.append('cache');
  if (!cacheDir.exists())
    return;

  let cacheManifest = cacheDir.clone();
  cacheManifest.append('manifest.appcache');
  if (!cacheManifest.exists())
    return;

  let principal = Services.scriptSecurityManager.getAppCodebasePrincipal(
      app.origin, app.localId, false);

  
  
  
  let metadataLoaded;
  if (!resourcesMetadata.exists()) {
    
    dump("OfflineCacheInstaller: App " + app.appId + " does have an app cache" +
         " but does not have a resources_metadata.json file!");
    metadataLoaded = Promise.resolve({});
  } else {
    metadataLoaded = new Promise(
      (resolve, reject) =>
        readFile(resourcesMetadata, principal, content => resolve(JSON.parse(content))));
  }

  metadataLoaded.then(function(metadata) {
    enableOfflineCacheForApp(principal);

    
    let appcacheURL = app.appcache_path;

    
    
    
    let groupID = appcacheURL + '#' + app.localId+ '+f';
    let applicationCache = applicationCacheService.createApplicationCache(groupID);
    applicationCache.activate();

    readFile(cacheManifest, principal, function readAppCache(content) {
      let entries = parseAppCache(app, cacheManifest.path, content);

      entries.urls.forEach(function processCachedFile(url) {
        
        
        
        let path = url.replace(app.origin.spec, '');
        let file = cacheDir.clone();
        let paths = path.split('/');
        paths.forEach(file.append);

        if (!file.exists()) {
          let msg = 'File ' + file.path + ' exists in the manifest but does ' +
                    'not points to a real file.';
          throw new Error(msg);
        }

        let itemType = nsIApplicationCache.ITEM_EXPLICIT;
        if (entries.fallbacks.indexOf(url) > -1) {
          debug('add fallback: ' + url + '\n');
          itemType |= nsIApplicationCache.ITEM_FALLBACK;
        }
        storeCache(applicationCache, url, file, itemType, metadata[path]);
      });

      let array = new MutableArray();
      entries.namespaces.forEach(function processNamespace([type, spec, data]) {
        debug('add namespace: ' + type + ' - ' + spec + ' - ' + data + '\n');
        array.appendElement(new Namespace(type, spec, data), false);
      });
      applicationCache.addNamespaces(array);

      storeCache(applicationCache, appcacheURL, cacheManifest,
                 nsIApplicationCache.ITEM_MANIFEST);
    });
  });
}




this.OfflineCacheInstaller = {
  installCache: installCache
};

