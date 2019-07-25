






































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

var EXPORTED_SYMBOLS = [];

Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/AddonManager.jsm");
Components.utils.import("resource://gre/modules/AddonRepository.jsm");
Components.utils.import("resource://gre/modules/LightweightThemeManager.jsm");
Components.utils.import("resource://gre/modules/FileUtils.jsm");
Components.utils.import("resource://gre/modules/NetUtil.jsm");

const PREF_DB_SCHEMA                  = "extensions.databaseSchema";
const PREF_INSTALL_CACHE              = "extensions.installCache";
const PREF_BOOTSTRAP_ADDONS           = "extensions.bootstrappedAddons";
const PREF_PENDING_OPERATIONS         = "extensions.pendingOperations";
const PREF_MATCH_OS_LOCALE            = "intl.locale.matchOS";
const PREF_SELECTED_LOCALE            = "general.useragent.locale";
const PREF_EM_DSS_ENABLED             = "extensions.dss.enabled";
const PREF_DSS_SWITCHPENDING          = "extensions.dss.switchPending";
const PREF_DSS_SKIN_TO_SELECT         = "extensions.lastSelectedSkin";
const PREF_GENERAL_SKINS_SELECTEDSKIN = "general.skins.selectedSkin";
const PREF_EM_CHECK_COMPATIBILITY_BASE = "extensions.checkCompatibility";
const PREF_EM_CHECK_UPDATE_SECURITY   = "extensions.checkUpdateSecurity";
const PREF_EM_UPDATE_URL              = "extensions.update.url";
const PREF_EM_ENABLED_ADDONS          = "extensions.enabledAddons";
const PREF_EM_EXTENSION_FORMAT        = "extensions.";
const PREF_EM_ENABLED_SCOPES          = "extensions.enabledScopes";
const PREF_EM_SHOW_MISMATCH_UI        = "extensions.showMismatchUI";
const PREF_EM_DISABLED_ADDONS_LIST    = "extensions.disabledAddons";
const PREF_XPI_ENABLED                = "xpinstall.enabled";
const PREF_XPI_WHITELIST_REQUIRED     = "xpinstall.whitelist.required";
const PREF_XPI_WHITELIST_PERMISSIONS  = "xpinstall.whitelist.add";
const PREF_XPI_BLACKLIST_PERMISSIONS  = "xpinstall.blacklist.add";
const PREF_XPI_UNPACK                 = "extensions.alwaysUnpack";
const PREF_INSTALL_REQUIREBUILTINCERTS = "extensions.install.requireBuiltInCerts";
const PREF_INSTALL_DISTRO_ADDONS      = "extensions.installDistroAddons";
const PREF_BRANCH_INSTALLED_ADDON     = "extensions.installedDistroAddon.";

const URI_EXTENSION_UPDATE_DIALOG     = "chrome://mozapps/content/extensions/update.xul";
const URI_EXTENSION_STRINGS           = "chrome://mozapps/locale/extensions/extensions.properties";

const STRING_TYPE_NAME                = "type.%ID%.name";

const DIR_EXTENSIONS                  = "extensions";
const DIR_STAGE                       = "staged";
const DIR_XPI_STAGE                   = "staged-xpis";
const DIR_TRASH                       = "trash";

const FILE_OLD_DATABASE               = "extensions.rdf";
const FILE_OLD_CACHE                  = "extensions.cache";
const FILE_DATABASE                   = "extensions.sqlite";
const FILE_INSTALL_MANIFEST           = "install.rdf";
const FILE_XPI_ADDONS_LIST            = "extensions.ini";

const KEY_PROFILEDIR                  = "ProfD";
const KEY_APPDIR                      = "XCurProcD";
const KEY_TEMPDIR                     = "TmpD";
const KEY_APP_DISTRIBUTION            = "XREAppDist";

const KEY_APP_PROFILE                 = "app-profile";
const KEY_APP_GLOBAL                  = "app-global";
const KEY_APP_SYSTEM_LOCAL            = "app-system-local";
const KEY_APP_SYSTEM_SHARE            = "app-system-share";
const KEY_APP_SYSTEM_USER             = "app-system-user";

const CATEGORY_UPDATE_PARAMS          = "extension-update-params";

const UNKNOWN_XPCOM_ABI               = "unknownABI";
const XPI_PERMISSION                  = "install";

const PREFIX_ITEM_URI                 = "urn:mozilla:item:";
const RDFURI_ITEM_ROOT                = "urn:mozilla:item:root"
const RDFURI_INSTALL_MANIFEST_ROOT    = "urn:mozilla:install-manifest";
const PREFIX_NS_EM                    = "http://www.mozilla.org/2004/em-rdf#";

const TOOLKIT_ID                      = "toolkit@mozilla.org";

const BRANCH_REGEXP                   = /^([^\.]+\.[0-9]+[a-z]*).*/gi;

const DB_SCHEMA                       = 4;
const REQ_VERSION                     = 2;

#ifdef MOZ_COMPATABILITY_NIGHTLY
const PREF_EM_CHECK_COMPATIBILITY = PREF_EM_CHECK_COMPATIBILITY_BASE +
                                    ".nightly";
#else
const PREF_EM_CHECK_COMPATIBILITY = PREF_EM_CHECK_COMPATIBILITY_BASE + "." +
                                    Services.appinfo.version.replace(BRANCH_REGEXP, "$1");
#endif


const PROP_METADATA      = ["id", "version", "type", "internalName", "updateURL",
                            "updateKey", "optionsURL", "aboutURL", "iconURL",
                            "icon64URL"];
const PROP_LOCALE_SINGLE = ["name", "description", "creator", "homepageURL"];
const PROP_LOCALE_MULTI  = ["developers", "translators", "contributors"];
const PROP_TARGETAPP     = ["id", "minVersion", "maxVersion"];


const DB_METADATA        = ["installDate", "updateDate", "size", "sourceURI",
                            "releaseNotesURI", "applyBackgroundUpdates"];
const DB_BOOL_METADATA   = ["visible", "active", "userDisabled", "appDisabled",
                            "pendingUninstall", "bootstrap", "skinnable",
                            "softDisabled"];

const BOOTSTRAP_REASONS = {
  APP_STARTUP     : 1,
  APP_SHUTDOWN    : 2,
  ADDON_ENABLE    : 3,
  ADDON_DISABLE   : 4,
  ADDON_INSTALL   : 5,
  ADDON_UNINSTALL : 6,
  ADDON_UPGRADE   : 7,
  ADDON_DOWNGRADE : 8
};


const TYPES = {
  extension: 2,
  theme: 4,
  locale: 8,
  multipackage: 32
};

const MSG_JAR_FLUSH = "AddonJarFlush";




var gIDTest = /^(\{[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\}|[a-z0-9-\._]*\@[a-z0-9-\._]+)$/i;

["LOG", "WARN", "ERROR"].forEach(function(aName) {
  this.__defineGetter__(aName, function() {
    Components.utils.import("resource://gre/modules/AddonLogging.jsm");

    LogManager.getLogger("addons.xpi", this);
    return this[aName];
  })
}, this);











function SafeInstallOperation() {
  this._installedFiles = [];
  this._createdDirs = [];
}

SafeInstallOperation.prototype = {
  _installedFiles: null,
  _createdDirs: null,

  _installFile: function(aFile, aTargetDirectory, aCopy) {
    let oldFile = aCopy ? null : aFile.clone();
    let newFile = aFile.clone();
    try {
      if (aCopy)
        newFile.copyTo(aTargetDirectory, null);
      else
        newFile.moveTo(aTargetDirectory, null);
    }
    catch (e) {
      ERROR("Failed to " + (aCopy ? "copy" : "move") + " file " + aFile.path +
            " to " + aTargetDirectory.path, e);
      throw e;
    }
    this._installedFiles.push({ oldFile: oldFile, newFile: newFile });
  },

  _installDirectory: function(aDirectory, aTargetDirectory, aCopy) {
    let newDir = aTargetDirectory.clone();
    newDir.append(aDirectory.leafName);
    try {
      newDir.create(Ci.nsILocalFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
    }
    catch (e) {
      ERROR("Failed to create directory " + newDir.path, e);
      throw e;
    }
    this._createdDirs.push(newDir);

    let entries = aDirectory.directoryEntries
                            .QueryInterface(Ci.nsIDirectoryEnumerator);
    let cacheEntries = [];
    try {
      let entry;
      while (entry = entries.nextFile)
        cacheEntries.push(entry);
    }
    finally {
      entries.close();
    }

    cacheEntries.forEach(function(aEntry) {
      try {
        this._installDirEntry(aEntry, newDir, aCopy);
      }
      catch (e) {
        ERROR("Failed to " + (aCopy ? "copy" : "move") + " entry " +
              aEntry.path, e);
        throw e;
      }
    }, this);

    
    if (aCopy)
      return;

    
    
    try {
      aDirectory.permissions = FileUtils.PERMS_DIRECTORY;
      aDirectory.remove(false);
    }
    catch (e) {
      ERROR("Failed to remove directory " + aDirectory.path, e);
      throw e;
    }

    
    
    this._installedFiles.push({ oldFile: aDirectory, newFile: newDir });
  },

  _installDirEntry: function(aDirEntry, aTargetDirectory, aCopy) {
    try {
      if (aDirEntry.isDirectory())
        this._installDirectory(aDirEntry, aTargetDirectory, aCopy);
      else
        this._installFile(aDirEntry, aTargetDirectory, aCopy);
    }
    catch (e) {
      ERROR("Failure " + (aCopy ? "copying" : "moving") + " " + aDirEntry.path +
            " to " + aTargetDirectory.path);
      throw e;
    }
  },

  









  move: function(aFile, aTargetDirectory) {
    try {
      this._installDirEntry(aFile, aTargetDirectory, false);
    }
    catch (e) {
      this.rollback();
      throw e;
    }
  },

  









  copy: function(aFile, aTargetDirectory) {
    try {
      this._installDirEntry(aFile, aTargetDirectory, true);
    }
    catch (e) {
      this.rollback();
      throw e;
    }
  },

  




  rollback: function() {
    while (this._installedFiles.length > 0) {
      let move = this._installedFiles.pop();
      if (move.newFile.isDirectory()) {
        let oldDir = move.oldFile.parent.clone();
        oldDir.append(move.oldFile.leafName);
        oldDir.create(Ci.nsILocalFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
      }
      else if (!move.oldFile) {
        
        move.newFile.remove(true);
      }
      else {
        move.newFile.moveTo(move.oldFile.parent, null);
      }
    }

    while (this._createdDirs.length > 0)
      recursiveRemove(this._createdDirs.pop());
  }
};





function getLocale() {
  if (Prefs.getBoolPref(PREF_MATCH_OS_LOCALE, false))
    return Services.locale.getLocaleComponentForUserAgent();
  let locale = Prefs.getComplexValue(PREF_SELECTED_LOCALE, Ci.nsIPrefLocalizedString);
  if (locale)
    return locale;
  return Prefs.getCharPref(PREF_SELECTED_LOCALE, "en-US");
}








function findClosestLocale(aLocales) {
  let appLocale = getLocale();

  
  var bestmatch = null;
  
  var bestmatchcount = 0;
  
  var bestpartcount = 0;

  var matchLocales = [appLocale.toLowerCase()];
  

  if (matchLocales[0].substring(0, 3) != "en-")
    matchLocales.push("en-us");

  for each (var locale in matchLocales) {
    var lparts = locale.split("-");
    for each (var localized in aLocales) {
      for each (found in localized.locales) {
        found = found.toLowerCase();
        
        if (locale == found)
          return localized;

        var fparts = found.split("-");
        

        if (bestmatch && fparts.length < bestmatchcount)
          continue;

        
        var maxmatchcount = Math.min(fparts.length, lparts.length);
        var matchcount = 0;
        while (matchcount < maxmatchcount &&
               fparts[matchcount] == lparts[matchcount])
          matchcount++;

        

        if (matchcount > bestmatchcount ||
           (matchcount == bestmatchcount && fparts.length < bestpartcount)) {
          bestmatch = localized;
          bestmatchcount = matchcount;
          bestpartcount = fparts.length;
        }
      }
    }
    
    if (bestmatch)
      return bestmatch;
  }
  return null;
}


















function applyBlocklistChanges(aOldAddon, aNewAddon, aOldAppVersion,
                               aOldPlatformVersion) {
  
  aNewAddon.userDisabled = aOldAddon.userDisabled;
  aNewAddon.softDisabled = aOldAddon.softDisabled;

  let bs = Cc["@mozilla.org/extensions/blocklist;1"].
           getService(Ci.nsIBlocklistService);

  let oldBlocklistState = bs.getAddonBlocklistState(aOldAddon.id,
                                                    aOldAddon.version,
                                                    aOldAppVersion,
                                                    aOldPlatformVersion);
  let newBlocklistState = bs.getAddonBlocklistState(aNewAddon.id,
                                                    aNewAddon.version);

  
  
  if (newBlocklistState == oldBlocklistState)
    return;

  if (newBlocklistState == Ci.nsIBlocklistService.STATE_SOFTBLOCKED) {
    if (aNewAddon.type != "theme") {
      
      
      aNewAddon.softDisabled = !aNewAddon.userDisabled;
    }
    else {
      
      aNewAddon.userDisabled = true;
    }
  }
  else {
    
    aNewAddon.softDisabled = false;
  }
}








function isUsableAddon(aAddon) {
  
  if (aAddon.type == "theme" && aAddon.internalName == XPIProvider.defaultSkin)
    return true;

  if (aAddon.blocklistState == Ci.nsIBlocklistService.STATE_BLOCKED)
    return false;

  if (XPIProvider.checkUpdateSecurity && !aAddon.providesUpdatesSecurely)
    return false;

  if (!aAddon.isPlatformCompatible)
    return false;

  if (XPIProvider.checkCompatibility) {
    if (!aAddon.isCompatible)
      return false;
  }
  else {
    if (!aAddon.matchingTargetApplication)
      return false;
  }

  return true;
}

function isAddonDisabled(aAddon) {
  return aAddon.appDisabled || aAddon.softDisabled || aAddon.userDisabled;
}

this.__defineGetter__("gRDF", function() {
  delete this.gRDF;
  return this.gRDF = Cc["@mozilla.org/rdf/rdf-service;1"].
                     getService(Ci.nsIRDFService);
});

function EM_R(aProperty) {
  return gRDF.GetResource(PREFIX_NS_EM + aProperty);
}








function getRDFValue(aLiteral) {
  if (aLiteral instanceof Ci.nsIRDFLiteral)
    return aLiteral.Value;
  if (aLiteral instanceof Ci.nsIRDFResource)
    return aLiteral.Value;
  if (aLiteral instanceof Ci.nsIRDFInt)
    return aLiteral.Value;
  return null;
}












function getRDFProperty(aDs, aResource, aProperty) {
  return getRDFValue(aDs.GetTarget(aResource, EM_R(aProperty), true));
}












function loadManifestFromRDF(aUri, aStream) {
  function getPropertyArray(aDs, aSource, aProperty) {
    let values = [];
    let targets = aDs.GetTargets(aSource, EM_R(aProperty), true);
    while (targets.hasMoreElements())
      values.push(getRDFValue(targets.getNext()));

    return values;
  }

  
















  function readLocale(aDs, aSource, isDefault, aSeenLocales) {
    let locale = { };
    if (!isDefault) {
      locale.locales = [];
      let targets = ds.GetTargets(aSource, EM_R("locale"), true);
      while (targets.hasMoreElements()) {
        let localeName = getRDFValue(targets.getNext());
        if (!localeName) {
          WARN("Ignoring empty locale in localized properties");
          continue;
        }
        if (aSeenLocales.indexOf(localeName) != -1) {
          WARN("Ignoring duplicate locale in localized properties");
          continue;
        }
        aSeenLocales.push(localeName);
        locale.locales.push(localeName);
      }

      if (locale.locales.length == 0) {
        WARN("Ignoring localized properties with no listed locales");
        return null;
      }
    }

    PROP_LOCALE_SINGLE.forEach(function(aProp) {
      locale[aProp] = getRDFProperty(aDs, aSource, aProp);
    });

    PROP_LOCALE_MULTI.forEach(function(aProp) {
      locale[aProp] = getPropertyArray(aDs, aSource,
                                       aProp.substring(0, aProp.length - 1));
    });

    return locale;
  }

  let rdfParser = Cc["@mozilla.org/rdf/xml-parser;1"].
                  createInstance(Ci.nsIRDFXMLParser)
  let ds = Cc["@mozilla.org/rdf/datasource;1?name=in-memory-datasource"].
           createInstance(Ci.nsIRDFDataSource);
  let listener = rdfParser.parseAsync(ds, aUri);
  let channel = Cc["@mozilla.org/network/input-stream-channel;1"].
                createInstance(Ci.nsIInputStreamChannel);
  channel.setURI(aUri);
  channel.contentStream = aStream;
  channel.QueryInterface(Ci.nsIChannel);
  channel.contentType = "text/xml";

  listener.onStartRequest(channel, null);

  try {
    let pos = 0;
    let count = aStream.available();
    while (count > 0) {
      listener.onDataAvailable(channel, null, aStream, pos, count);
      pos += count;
      count = aStream.available();
    }
    listener.onStopRequest(channel, null, Components.results.NS_OK);
  }
  catch (e) {
    listener.onStopRequest(channel, null, e.result);
    throw e;
  }

  let root = gRDF.GetResource(RDFURI_INSTALL_MANIFEST_ROOT);
  let addon = new AddonInternal();
  PROP_METADATA.forEach(function(aProp) {
    addon[aProp] = getRDFProperty(ds, root, aProp);
  });
  addon.unpack = getRDFProperty(ds, root, "unpack") == "true";

  if (!addon.type) {
    addon.type = addon.internalName ? "theme" : "extension";
  }
  else {
    for (let name in TYPES) {
      if (TYPES[name] == addon.type) {
        addon.type = name;
        break;
      }
    }
  }

  if (!(addon.type in TYPES))
    throw new Error("Install manifest specifies unknown type: " + addon.type);

  if (addon.type != "multipackage") {
    if (!addon.id)
      throw new Error("No ID in install manifest");
    if (!gIDTest.test(addon.id))
      throw new Error("Illegal add-on ID " + addon.id);
    if (!addon.version)
      throw new Error("No version in install manifest");
  }

  
  if (addon.type == "extension") {
    addon.bootstrap = getRDFProperty(ds, root, "bootstrap") == "true";
  }
  else {
    
    
    addon.optionsURL = null;
    addon.aboutURL = null;

    if (addon.type == "theme") {
      if (!addon.internalName)
        throw new Error("Themes must include an internalName property");
      addon.skinnable = getRDFProperty(ds, root, "skinnable") == "true";
    }
  }

  addon.defaultLocale = readLocale(ds, root, true);

  let seenLocales = [];
  addon.locales = [];
  let targets = ds.GetTargets(root, EM_R("localized"), true);
  while (targets.hasMoreElements()) {
    let target = targets.getNext().QueryInterface(Ci.nsIRDFResource);
    let locale = readLocale(ds, target, false, seenLocales);
    if (locale)
      addon.locales.push(locale);
  }

  let seenApplications = [];
  addon.targetApplications = [];
  targets = ds.GetTargets(root, EM_R("targetApplication"), true);
  while (targets.hasMoreElements()) {
    let target = targets.getNext().QueryInterface(Ci.nsIRDFResource);
    let targetAppInfo = {};
    PROP_TARGETAPP.forEach(function(aProp) {
      targetAppInfo[aProp] = getRDFProperty(ds, target, aProp);
    });
    if (!targetAppInfo.id || !targetAppInfo.minVersion ||
        !targetAppInfo.maxVersion) {
      WARN("Ignoring invalid targetApplication entry in install manifest");
      continue;
    }
    if (seenApplications.indexOf(targetAppInfo.id) != -1) {
      WARN("Ignoring duplicate targetApplication entry for " + targetAppInfo.id +
           " in install manifest");
      continue;
    }
    seenApplications.push(targetAppInfo.id);
    addon.targetApplications.push(targetAppInfo);
  }

  
  
  let targetPlatforms = getPropertyArray(ds, root, "targetPlatform");
  addon.targetPlatforms = [];
  targetPlatforms.forEach(function(aPlatform) {
    let platform = {
      os: null,
      abi: null
    };

    let pos = aPlatform.indexOf("_");
    if (pos != -1) {
      platform.os = aPlatform.substring(0, pos);
      platform.abi = aPlatform.substring(pos + 1);
    }
    else {
      platform.os = aPlatform;
    }

    addon.targetPlatforms.push(platform);
  });

  
  
  
  if (addon.type == "theme") {
    addon.userDisabled = !!LightweightThemeManager.currentTheme ||
                         addon.internalName != XPIProvider.selectedSkin;
  }
  else {
    addon.userDisabled = false;
    addon.softDisabled = addon.blocklistState == Ci.nsIBlocklistService.STATE_SOFTBLOCKED;
  }

  addon.appDisabled = !isUsableAddon(addon);

  addon.applyBackgroundUpdates = AddonManager.AUTOUPDATE_DEFAULT;

  return addon;
}









function loadManifestFromDir(aDir) {
  function getFileSize(aFile) {
    if (aFile.isSymlink())
      return 0;

    if (!aFile.isDirectory())
      return aFile.fileSize;

    let size = 0;
    let entries = aFile.directoryEntries.QueryInterface(Ci.nsIDirectoryEnumerator);
    let entry;
    while (entry = entries.nextFile)
      size += getFileSize(entry);
    entries.close();
    return size;
  }

  let file = aDir.clone();
  file.append(FILE_INSTALL_MANIFEST);
  if (!file.exists() || !file.isFile())
    throw new Error("Directory " + aDir.path + " does not contain a valid " +
                    "install manifest");

  let fis = Cc["@mozilla.org/network/file-input-stream;1"].
            createInstance(Ci.nsIFileInputStream);
  fis.init(file, -1, -1, false);
  let bis = Cc["@mozilla.org/network/buffered-input-stream;1"].
            createInstance(Ci.nsIBufferedInputStream);
  bis.init(fis, 4096);

  try {
    let addon = loadManifestFromRDF(Services.io.newFileURI(file), bis);
    addon._sourceBundle = aDir.clone().QueryInterface(Ci.nsILocalFile);
    addon.size = getFileSize(aDir);
    return addon;
  }
  finally {
    bis.close();
    fis.close();
  }
}









function loadManifestFromZipReader(aZipReader) {
  let zis = aZipReader.getInputStream(FILE_INSTALL_MANIFEST);
  let bis = Cc["@mozilla.org/network/buffered-input-stream;1"].
            createInstance(Ci.nsIBufferedInputStream);
  bis.init(zis, 4096);

  try {
    let uri = buildJarURI(aZipReader.file, FILE_INSTALL_MANIFEST);
    let addon = loadManifestFromRDF(uri, bis);
    addon._sourceBundle = aZipReader.file;

    addon.size = 0;
    let entries = aZipReader.findEntries(null);
    while (entries.hasMore())
      addon.size += aZipReader.getEntry(entries.getNext()).realSize;

    return addon;
  }
  finally {
    bis.close();
    zis.close();
  }
}









function loadManifestFromZipFile(aXPIFile) {
  let zipReader = Cc["@mozilla.org/libjar/zip-reader;1"].
                  createInstance(Ci.nsIZipReader);
  try {
    zipReader.open(aXPIFile);

    return loadManifestFromZipReader(zipReader);
  }
  finally {
    zipReader.close();
  }
}

function loadManifestFromFile(aFile) {
  if (aFile.isFile())
    return loadManifestFromZipFile(aFile);
  else
    return loadManifestFromDir(aFile);
}










function buildJarURI(aJarfile, aPath) {
  let uri = Services.io.newFileURI(aJarfile);
  uri = "jar:" + uri.spec + "!/" + aPath;
  return NetUtil.newURI(uri);
}







function flushJarCache(aJarFile) {
  Services.obs.notifyObservers(aJarFile, "flush-cache-entry", null);
  Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIChromeFrameMessageManager)
    .sendAsyncMessage(MSG_JAR_FLUSH, aJarFile.path);
}








function getTemporaryFile() {
  let file = FileUtils.getDir(KEY_TEMPDIR, []);
  let random = Math.random().toString(36).replace(/0./, '').substr(-3);
  file.append("tmp-" + random + ".xpi");
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);

  return file;
}









function extractFiles(aZipFile, aDir) {
  function getTargetFile(aDir, entry) {
    let target = aDir.clone();
    entry.split("/").forEach(function(aPart) {
      target.append(aPart);
    });
    return target;
  }

  let zipReader = Cc["@mozilla.org/libjar/zip-reader;1"].
                  createInstance(Ci.nsIZipReader);
  zipReader.open(aZipFile);

  try {
    
    let entries = zipReader.findEntries("*/");
    while (entries.hasMore()) {
      var entryName = entries.getNext();
      let target = getTargetFile(aDir, entryName);
      if (!target.exists()) {
        try {
          target.create(Ci.nsILocalFile.DIRECTORY_TYPE,
                        FileUtils.PERMS_DIRECTORY);
        }
        catch (e) {
          ERROR("extractFiles: failed to create target directory for " +
                "extraction file = " + target.path, e);
        }
      }
    }

    entries = zipReader.findEntries(null);
    while (entries.hasMore()) {
      let entryName = entries.getNext();
      let target = getTargetFile(aDir, entryName);
      if (target.exists())
        continue;

      zipReader.extract(entryName, target);
      target.permissions |= FileUtils.PERMS_FILE;
    }
  }
  finally {
    zipReader.close();
  }
}












function verifyZipSigning(aZip, aPrincipal) {
  var count = 0;
  var entries = aZip.findEntries(null);
  while (entries.hasMore()) {
    var entry = entries.getNext();
    
    if (entry.substr(0, 9) == "META-INF/")
      continue;
    
    if (entry.substr(-1) == "/")
      continue;
    count++;
    var entryPrincipal = aZip.getCertificatePrincipal(entry);
    if (!entryPrincipal || !aPrincipal.equals(entryPrincipal))
      return false;
  }
  return aZip.manifestEntriesCount == count;
}
















function escapeAddonURI(aAddon, aUri, aUpdateType, aAppVersion)
{
  var addonStatus = aAddon.userDisabled || aAddon.softDisabled ? "userDisabled"
                                                               : "userEnabled";

  if (!aAddon.isCompatible)
    addonStatus += ",incompatible";
  if (aAddon.blocklistState == Ci.nsIBlocklistService.STATE_BLOCKED)
    addonStatus += ",blocklisted";
  if (aAddon.blocklistState == Ci.nsIBlocklistService.STATE_SOFTBLOCKED)
    addonStatus += ",softblocked";

  try {
    var xpcomABI = Services.appinfo.XPCOMABI;
  } catch (ex) {
    xpcomABI = UNKNOWN_XPCOM_ABI;
  }

  let uri = aUri.replace(/%ITEM_ID%/g, aAddon.id);
  uri = uri.replace(/%ITEM_VERSION%/g, aAddon.version);
  uri = uri.replace(/%ITEM_STATUS%/g, addonStatus);
  uri = uri.replace(/%APP_ID%/g, Services.appinfo.ID);
  uri = uri.replace(/%APP_VERSION%/g, aAppVersion ? aAppVersion :
                                                    Services.appinfo.version);
  uri = uri.replace(/%REQ_VERSION%/g, REQ_VERSION);
  uri = uri.replace(/%APP_OS%/g, Services.appinfo.OS);
  uri = uri.replace(/%APP_ABI%/g, xpcomABI);
  uri = uri.replace(/%APP_LOCALE%/g, getLocale());
  uri = uri.replace(/%CURRENT_APP_VERSION%/g, Services.appinfo.version);

  
  if (aUpdateType)
    uri = uri.replace(/%UPDATE_TYPE%/g, aUpdateType);

  
  
  
  let app = aAddon.matchingTargetApplication;
  if (app)
    var maxVersion = app.maxVersion;
  else
    maxVersion = "";
  uri = uri.replace(/%ITEM_MAXAPPVERSION%/g, maxVersion);

  
  
  var catMan = null;
  uri = uri.replace(/%(\w{3,})%/g, function(aMatch, aParam) {
    if (!catMan) {
      catMan = Cc["@mozilla.org/categorymanager;1"].
               getService(Ci.nsICategoryManager);
    }

    try {
      var contractID = catMan.getCategoryEntry(CATEGORY_UPDATE_PARAMS, aParam);
      var paramHandler = Cc[contractID].getService(Ci.nsIPropertyBag2);
      return paramHandler.getPropertyAsAString(aParam);
    }
    catch(e) {
      return aMatch;
    }
  });

  
  return uri.replace(/\+/g, "%2B");
}













function copyProperties(aObject, aProperties, aTarget) {
  if (!aTarget)
    aTarget = {};
  aProperties.forEach(function(aProp) {
    aTarget[aProp] = aObject[aProp];
  });
  return aTarget;
}













function copyRowProperties(aRow, aProperties, aTarget) {
  if (!aTarget)
    aTarget = {};
  aProperties.forEach(function(aProp) {
    aTarget[aProp] = aRow.getResultByName(aProp);
  });
  return aTarget;
}







function resultRows(aStatement) {
  try {
    while (stepStatement(aStatement))
      yield aStatement.row;
  }
  finally {
    aStatement.reset();
  }
}











function cleanStagingDir(aDir, aLeafNames) {
  aLeafNames.forEach(function(aName) {
    let file = aDir.clone();
    file.append(aName);
    if (file.exists())
      recursiveRemove(file);
  });

  let dirEntries = aDir.directoryEntries.QueryInterface(Ci.nsIDirectoryEnumerator);
  try {
    if (dirEntries.nextFile)
      return;
  }
  finally {
    dirEntries.close();
  }

  try {
    aDir.permissions = FileUtils.PERMS_DIRECTORY;
    aDir.remove(false);
  }
  catch (e) {
    
  }
}







function recursiveRemove(aFile) {
  aFile.permissions = aFile.isDirectory() ? FileUtils.PERMS_DIRECTORY
                                          : FileUtils.PERMS_FILE;

  try {
    aFile.remove(true);
    return;
  }
  catch (e) {
    if (!aFile.isDirectory()) {
      ERROR("Failed to remove file " + aFile.path, e);
      throw e;
    }
  }

  let entry;
  let dirEntries = aFile.directoryEntries.QueryInterface(Ci.nsIDirectoryEnumerator);
  try {
    while (entry = dirEntries.nextFile)
      recursiveRemove(entry);
    aFile.remove(true);
  }
  finally {
    dirEntries.close();
  }
}









function recursiveLastModifiedTime(aFile) {
  if (aFile.isFile())
    return aFile.lastModifiedTime;

  if (aFile.isDirectory()) {
    let entries = aFile.directoryEntries.QueryInterface(Ci.nsIDirectoryEnumerator);
    let entry, time;
    let maxTime = aFile.lastModifiedTime;
    while (entry = entries.nextFile) {
      time = recursiveLastModifiedTime(entry);
      maxTime = Math.max(time, maxTime);
    }
    entries.close();
    return maxTime;
  }

  
  return 0;
}





var Prefs = {
  









  getDefaultCharPref: function(aName, aDefaultValue) {
    try {
      return Services.prefs.getDefaultBranch("").getCharPref(aName);
    }
    catch (e) {
    }
    return aDefaultValue;
  },

  








  getCharPref: function(aName, aDefaultValue) {
    try {
      return Services.prefs.getCharPref(aName);
    }
    catch (e) {
    }
    return aDefaultValue;
  },

  










  getComplexValue: function(aName, aType, aDefaultValue) {
    try {
      return Services.prefs.getComplexValue(aName, aType).data;
    }
    catch (e) {
    }
    return aDefaultValue;
  },

  








  getBoolPref: function(aName, aDefaultValue) {
    try {
      return Services.prefs.getBoolPref(aName);
    }
    catch (e) {
    }
    return aDefaultValue;
  },

  








  getIntPref: function(aName, defaultValue) {
    try {
      return Services.prefs.getIntPref(aName);
    }
    catch (e) {
    }
    return defaultValue;
  },

  





  clearUserPref: function(aName) {
    if (Services.prefs.prefHasUserValue(aName))
      Services.prefs.clearUserPref(aName);
  }
}

var XPIProvider = {
  
  installLocations: null,
  
  installLocationsByName: null,
  
  installs: null,
  
  defaultSkin: "classic/1.0",
  
  currentSkin: null,
  
  
  
  selectedSkin: null,
  
  checkCompatibility: true,
  
  checkUpdateSecurity: true,
  
  bootstrappedAddons: {},
  
  bootstrapScopes: {},
  
  extensionsActive: false,

  
  
  allAppGlobal: true,
  
  enabledAddons: null,
  
  inactiveAddonIDs: [],
  
  
  
  
  startupChanges: {
    
    appDisabled: []
  },

  















  startup: function XPI_startup(aAppChanged, aOldAppVersion, aOldPlatformVersion) {
    LOG("startup");
    this.installs = [];
    this.installLocations = [];
    this.installLocationsByName = {};

    function addDirectoryInstallLocation(aName, aKey, aPaths, aScope, aLocked) {
      try {
        var dir = FileUtils.getDir(aKey, aPaths);
      }
      catch (e) {
        
        LOG("Skipping unavailable install location " + aName);
        return;
      }

      try {
        var location = new DirectoryInstallLocation(aName, dir, aScope, aLocked);
      }
      catch (e) {
        WARN("Failed to add directory install location " + aName, e);
        return;
      }

      XPIProvider.installLocations.push(location);
      XPIProvider.installLocationsByName[location.name] = location;
    }

    function addRegistryInstallLocation(aName, aRootkey, aScope) {
      try {
        var location = new WinRegInstallLocation(aName, aRootkey, aScope);
      }
      catch (e) {
        WARN("Failed to add registry install location " + aName, e);
        return;
      }

      XPIProvider.installLocations.push(location);
      XPIProvider.installLocationsByName[location.name] = location;
    }

    let hasRegistry = ("nsIWindowsRegKey" in Ci);

    let enabledScopes = Prefs.getIntPref(PREF_EM_ENABLED_SCOPES,
                                         AddonManager.SCOPE_ALL);

    
    if (enabledScopes & AddonManager.SCOPE_SYSTEM) {
      if (hasRegistry) {
        addRegistryInstallLocation("winreg-app-global",
                                   Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE,
                                   AddonManager.SCOPE_SYSTEM);
      }
      addDirectoryInstallLocation(KEY_APP_SYSTEM_LOCAL, "XRESysLExtPD",
                                  [Services.appinfo.ID],
                                  AddonManager.SCOPE_SYSTEM, true);
      addDirectoryInstallLocation(KEY_APP_SYSTEM_SHARE, "XRESysSExtPD",
                                  [Services.appinfo.ID],
                                  AddonManager.SCOPE_SYSTEM, true);
    }

    if (enabledScopes & AddonManager.SCOPE_APPLICATION) {
      addDirectoryInstallLocation(KEY_APP_GLOBAL, KEY_APPDIR,
                                  [DIR_EXTENSIONS],
                                  AddonManager.SCOPE_APPLICATION, true);
    }

    if (enabledScopes & AddonManager.SCOPE_USER) {
      if (hasRegistry) {
        addRegistryInstallLocation("winreg-app-user",
                                   Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                                   AddonManager.SCOPE_USER);
      }
      addDirectoryInstallLocation(KEY_APP_SYSTEM_USER, "XREUSysExt",
                                  [Services.appinfo.ID],
                                  AddonManager.SCOPE_USER, true);
    }

    
    addDirectoryInstallLocation(KEY_APP_PROFILE, KEY_PROFILEDIR,
                                [DIR_EXTENSIONS],
                                AddonManager.SCOPE_PROFILE, false);

    this.defaultSkin = Prefs.getDefaultCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN,
                                                "classic/1.0");
    this.currentSkin = Prefs.getCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN,
                                         this.defaultSkin);
    this.selectedSkin = this.currentSkin;
    this.applyThemeChange();

    this.checkCompatibility = Prefs.getBoolPref(PREF_EM_CHECK_COMPATIBILITY,
                                                true)
    this.checkUpdateSecurity = Prefs.getBoolPref(PREF_EM_CHECK_UPDATE_SECURITY,
                                                 true)
    this.enabledAddons = [];

    Services.prefs.addObserver(PREF_EM_CHECK_COMPATIBILITY, this, false);
    Services.prefs.addObserver(PREF_EM_CHECK_UPDATE_SECURITY, this, false);

    let flushCaches = this.checkForChanges(aAppChanged, aOldAppVersion,
                                           aOldPlatformVersion);

    
    this.applyThemeChange();

    if (Services.prefs.prefHasUserValue(PREF_EM_DISABLED_ADDONS_LIST))
      Services.prefs.clearUserPref(PREF_EM_DISABLED_ADDONS_LIST);

    
    
    
    if (aAppChanged && !this.allAppGlobal) {
      
      if (Prefs.getBoolPref(PREF_EM_SHOW_MISMATCH_UI, true)) {
        this.showMismatchWindow();
        flushCaches = true;
      }
      else if (this.startupChanges.appDisabled.length > 0) {
        
        
        Services.prefs.setCharPref(PREF_EM_DISABLED_ADDONS_LIST,
                                   this.startupChanges.appDisabled.join(","));
      }
    }

    if (flushCaches) {
      
      let xulPrototypeCache = Cc["@mozilla.org/xul/xul-prototype-cache;1"].getService(Ci.nsISupports);
      Services.obs.notifyObservers(null, "startupcache-invalidate", null);

      
      
      
      
      Services.obs.notifyObservers(null, "chrome-flush-skin-caches", null);
      Services.obs.notifyObservers(null, "chrome-flush-caches", null);
    }

    this.enabledAddons = Prefs.getCharPref(PREF_EM_ENABLED_ADDONS, "");
    if ("nsICrashReporter" in Ci &&
        Services.appinfo instanceof Ci.nsICrashReporter) {
      
      try {
        Services.appinfo.annotateCrashReport("Theme", this.currentSkin);
      } catch (e) { }
      try {
        Services.appinfo.annotateCrashReport("EMCheckCompatibility",
                                             this.checkCompatibility);
      } catch (e) { }
      this.addAddonsToCrashReporter();
    }

    for (let id in this.bootstrappedAddons) {
      let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      file.persistentDescriptor = this.bootstrappedAddons[id].descriptor;
      this.callBootstrapMethod(id, this.bootstrappedAddons[id].version, file,
                               "startup", BOOTSTRAP_REASONS.APP_STARTUP);
    }

    
    
    Services.obs.addObserver({
      observe: function(aSubject, aTopic, aData) {
        Services.prefs.setCharPref(PREF_BOOTSTRAP_ADDONS,
                                   JSON.stringify(XPIProvider.bootstrappedAddons));
        for (let id in XPIProvider.bootstrappedAddons) {
          let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
          file.persistentDescriptor = XPIProvider.bootstrappedAddons[id].descriptor;
          XPIProvider.callBootstrapMethod(id, XPIProvider.bootstrappedAddons[id].version,
                                          file, "shutdown",
                                          BOOTSTRAP_REASONS.APP_SHUTDOWN);
        }
        Services.obs.removeObserver(this, "quit-application-granted");
      }
    }, "quit-application-granted", false);

    this.extensionsActive = true;
  },

  


  shutdown: function XPI_shutdown() {
    LOG("shutdown");

    Services.prefs.removeObserver(PREF_EM_CHECK_COMPATIBILITY, this);
    Services.prefs.removeObserver(PREF_EM_CHECK_UPDATE_SECURITY, this);

    this.bootstrappedAddons = {};
    this.bootstrapScopes = {};
    this.enabledAddons = null;
    this.allAppGlobal = true;

    for (let type in this.startupChanges)
      this.startupChanges[type] = [];

    this.inactiveAddonIDs = [];

    
    
    if (Prefs.getBoolPref(PREF_PENDING_OPERATIONS, false)) {
      XPIDatabase.updateActiveAddons();
      XPIDatabase.writeAddonsList();
      Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, false);
    }

    this.installs = null;
    this.installLocations = null;
    this.installLocationsByName = null;

    
    this.extensionsActive = false;

    XPIDatabase.shutdown(function() {
      Services.obs.notifyObservers(null, "xpi-provider-shutdown", null);
    });
  },

  


  applyThemeChange: function XPI_applyThemeChange() {
    if (!Prefs.getBoolPref(PREF_DSS_SWITCHPENDING, false))
      return;

    
    try {
      this.selectedSkin = Prefs.getCharPref(PREF_DSS_SKIN_TO_SELECT);
      Services.prefs.setCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN,
                                 this.selectedSkin);
      Services.prefs.clearUserPref(PREF_DSS_SKIN_TO_SELECT);
      LOG("Changed skin to " + this.selectedSkin);
      this.currentSkin = this.selectedSkin;
    }
    catch (e) {
      ERROR("Error applying theme change", e);
    }
    Services.prefs.clearUserPref(PREF_DSS_SWITCHPENDING);
  },

  


  showMismatchWindow: function XPI_showMismatchWindow() {
    var variant = Cc["@mozilla.org/variant;1"].
                  createInstance(Ci.nsIWritableVariant);
    variant.setFromVariant(this.inactiveAddonIDs);

    
    var features = "chrome,centerscreen,dialog,titlebar,modal";
    var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
             getService(Ci.nsIWindowWatcher);
    ww.openWindow(null, URI_EXTENSION_UPDATE_DIALOG, "", features, variant);

    
    XPIDatabase.writeAddonsList([]);
    Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, false);
  },

  


  addAddonsToCrashReporter: function XPI_addAddonsToCrashReporter() {
    if (!("nsICrashReporter" in Ci) ||
        !(Services.appinfo instanceof Ci.nsICrashReporter))
      return;

    
    
    if (Services.appinfo.inSafeMode)
      return;

    let data = this.enabledAddons;
    for (let id in this.bootstrappedAddons)
      data += (data ? "," : "") + id + ":" + this.bootstrappedAddons[id].version;

    try {
      Services.appinfo.annotateCrashReport("Add-ons", data);
    }
    catch (e) { }
  },

  










  getAddonStates: function XPI_getAddonStates(aLocation) {
    let addonStates = {};
    aLocation.addonLocations.forEach(function(file) {
      let id = aLocation.getIDForLocation(file);
      addonStates[id] = {
        descriptor: file.persistentDescriptor,
        mtime: recursiveLastModifiedTime(file)
      };
    });

    return addonStates;
  },

  








  getInstallLocationStates: function XPI_getInstallLocationStates() {
    let states = [];
    this.installLocations.forEach(function(aLocation) {
      let addons = aLocation.addonLocations;
      if (addons.length == 0)
        return;

      let locationState = {
        name: aLocation.name,
        addons: this.getAddonStates(aLocation)
      };

      states.push(locationState);
    }, this);
    return states;
  },

  








  processPendingFileChanges: function XPI_processPendingFileChanges(aManifests) {
    let changed = false;
    this.installLocations.forEach(function(aLocation) {
      aManifests[aLocation.name] = {};
      
      if (aLocation.locked)
        return;

      let stagedXPIDir = aLocation.getXPIStagingDir();
      let stagingDir = aLocation.getStagingDir();

      if (stagedXPIDir.exists() && stagedXPIDir.isDirectory()) {
        let entries = stagedXPIDir.directoryEntries
                                  .QueryInterface(Ci.nsIDirectoryEnumerator);
        while (entries.hasMoreElements()) {
          let stageDirEntry = entries.nextFile;

          if (!stageDirEntry.isDirectory()) {
            WARN("Ignoring file in XPI staging directory: " + stageDirEntry.path);
            continue;
          }

          
          let stagedXPI = null;
          var xpiEntries = stageDirEntry.directoryEntries
                                        .QueryInterface(Ci.nsIDirectoryEnumerator);
          while (xpiEntries.hasMoreElements()) {
            let file = xpiEntries.nextFile;
            if (!(file instanceof Ci.nsILocalFile))
              continue;
            if (file.isDirectory())
              continue;

            let extension = file.leafName;
            extension = extension.substring(extension.length - 4);

            if (extension != ".xpi" && extension != ".jar")
              continue;

            stagedXPI = file;
          }
          xpiEntries.close();

          if (!stagedXPI)
            continue;

          let addon = null;
          try {
            addon = loadManifestFromZipFile(stagedXPI);
          }
          catch (e) {
            ERROR("Unable to read add-on manifest from " + stagedXPI.path, e);
            continue;
          }

          LOG("Migrating staged install of " + addon.id + " in " + aLocation.name);

          if (addon.unpack || Prefs.getBoolPref(PREF_XPI_UNPACK, false)) {
            let targetDir = stagingDir.clone();
            targetDir.append(addon.id);
            try {
              targetDir.create(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
            }
            catch (e) {
              ERROR("Failed to create staging directory for add-on " + id, e);
              continue;
            }

            try {
              extractFiles(stagedXPI, targetDir);
            }
            catch (e) {
              ERROR("Failed to extract staged XPI for add-on " + id + " in " +
                    aLocation.name, e);
            }
          }
          else {
            try {
              stagedXPI.moveTo(stagingDir, addon.id + ".xpi");
            }
            catch (e) {
              ERROR("Failed to move staged XPI for add-on " + id + " in " +
                    aLocation.name, e);
            }
          }
        }
        entries.close();
      }

      if (stagedXPIDir.exists()) {
        try {
          recursiveRemove(stagedXPIDir);
        }
        catch (e) {
          
          LOG("Error removing XPI staging dir " + stagedXPIDir.path, e);
        }
      }

      if (!stagingDir || !stagingDir.exists() || !stagingDir.isDirectory())
        return;

      entries = stagingDir.directoryEntries
                          .QueryInterface(Ci.nsIDirectoryEnumerator);
      while (entries.hasMoreElements()) {
        let stageDirEntry = entries.getNext().QueryInterface(Ci.nsILocalFile);

        let id = stageDirEntry.leafName;
        if (!stageDirEntry.isDirectory()) {
          if (id.substring(id.length - 4).toLowerCase() == ".xpi") {
            id = id.substring(0, id.length - 4);
          }
          else {
            if (id.substring(id.length - 5).toLowerCase() != ".json")
              WARN("Ignoring file: " + stageDirEntry.path);
            continue;
          }
        }

        
        if (!gIDTest.test(id)) {
          WARN("Ignoring directory whose name is not a valid add-on ID: " +
               stageDirEntry.path);
          continue;
        }

        changed = true;

        if (stageDirEntry.isDirectory()) {
          
          let manifest = stageDirEntry.clone();
          manifest.append(FILE_INSTALL_MANIFEST);

          
          
          if (!manifest.exists()) {
            LOG("Processing uninstall of " + id + " in " + aLocation.name);
            try {
              aLocation.uninstallAddon(id);
            }
            catch (e) {
              ERROR("Failed to uninstall add-on " + id + " in " + aLocation.name, e);
            }
            
            continue;
          }
        }

        aManifests[aLocation.name][id] = null;
        let existingAddonID = id;

        
        
        let jsonfile = stagingDir.clone();
        jsonfile.append(id + ".json");
        if (jsonfile.exists()) {
          LOG("Found updated manifest for " + id + " in " + aLocation.name);
          let fis = Cc["@mozilla.org/network/file-input-stream;1"].
                       createInstance(Ci.nsIFileInputStream);
          let json = Cc["@mozilla.org/dom/json;1"].
                     createInstance(Ci.nsIJSON);

          try {
            fis.init(jsonfile, -1, 0, 0);
            aManifests[aLocation.name][id] = json.decodeFromStream(fis,
                                                                   jsonfile.fileSize);
            existingAddonID = aManifests[aLocation.name][id].existingAddonID || id;
          }
          catch (e) {
            ERROR("Unable to read add-on manifest from " + jsonfile.path, e);
          }
          finally {
            fis.close();
          }
        }

        
        if (!aManifests[aLocation.name][id]) {
          try {
            aManifests[aLocation.name][id] = loadManifestFromFile(stageDirEntry);
            existingAddonID = aManifests[aLocation.name][id].existingAddonID || id;
          }
          catch (e) {
            
            stageDirEntry.remove(true);
            ERROR("Unable to read add-on manifest from " + stageDirEntry.path, e);
          }
        }

        var oldBootstrap = null;
        LOG("Processing install of " + id + " in " + aLocation.name);
        if (existingAddonID in this.bootstrappedAddons) {
          try {
            var existingAddon = aLocation.getLocationForID(existingAddonID);
            if (this.bootstrappedAddons[existingAddonID].descriptor ==
                existingAddon.persistentDescriptor) {
              oldBootstrap = this.bootstrappedAddons[existingAddonID];

              
              
              let oldVersion = aManifests[aLocation.name][id].version;
              let newVersion = oldBootstrap.version;
              let uninstallReason = Services.vc.compare(newVersion, oldVersion) < 0 ?
                                    BOOTSTRAP_REASONS.ADDON_UPGRADE :
                                    BOOTSTRAP_REASONS.ADDON_DOWNGRADE;

              this.callBootstrapMethod(existingAddonID, oldBootstrap.version,
                                       existingAddon, "uninstall", uninstallReason);
              this.unloadBootstrapScope(existingAddonID);
            }
          }
          catch (e) {
          }
        }

        try {
          var addonInstallLocation = aLocation.installAddon(id, stageDirEntry,
                                                            existingAddonID);
          if (aManifests[aLocation.name][id])
            aManifests[aLocation.name][id]._sourceBundle = addonInstallLocation;
        }
        catch (e) {
          ERROR("Failed to install staged add-on " + id + " in " + aLocation.name,
                e);
          delete aManifests[aLocation.name][id];

          if (oldBootstrap) {
            
            this.callBootstrapMethod(existingAddonID, oldBootstrap.version,
                                     existingAddon, "install",
                                     BOOTSTRAP_REASONS.ADDON_INSTALL);
          }
          continue;
        }
      }
      entries.close();

      try {
        recursiveRemove(stagingDir);
      }
      catch (e) {
        
        LOG("Error removing staging dir " + stagingDir.path, e);
      }
    }, this);
    return changed;
  },

  










  installDistributionAddons: function XPI_installDistributionAddons(aManifests) {
    let distroDir;
    try {
      distroDir = FileUtils.getDir(KEY_APP_DISTRIBUTION, [DIR_EXTENSIONS]);
    }
    catch (e) {
      return false;
    }

    if (!distroDir.exists())
      return false;

    if (!distroDir.isDirectory())
      return false;

    let changed = false;
    let profileLocation = this.installLocationsByName[KEY_APP_PROFILE];

    let entries = distroDir.directoryEntries
                           .QueryInterface(Ci.nsIDirectoryEnumerator);
    let entry;
    while (entry = entries.nextFile) {
      
      if (!(entry instanceof Ci.nsILocalFile))
        continue;

      let id = entry.leafName;

      if (entry.isFile()) {
        if (id.substring(id.length - 4).toLowerCase() == ".xpi") {
          id = id.substring(0, id.length - 4);
        }
        else {
          LOG("Ignoring distribution add-on that isn't an XPI: " + entry.path);
          continue;
        }
      }
      else if (!entry.isDirectory()) {
        LOG("Ignoring distribution add-on that isn't a file or directory: " +
            entry.path);
        continue;
      }

      if (!gIDTest.test(id)) {
        LOG("Ignoring distribution add-on whose name is not a valid add-on ID: " +
            entry.path);
        continue;
      }

      let addon;
      try {
        addon = loadManifestFromFile(entry);
      }
      catch (e) {
        WARN("File entry " + entry.path + " contains an invalid add-on", e);
        continue;
      }

      if (addon.id != id) {
        WARN("File entry " + entry.path + " contains an add-on with an " +
             "incorrect ID")
        continue;
      }

      let existingEntry = null;
      try {
        existingEntry = profileLocation.getLocationForID(id);
      }
      catch (e) {
      }

      if (existingEntry) {
        let existingAddon;
        try {
          existingAddon = loadManifestFromFile(existingEntry);

          if (Services.vc.compare(addon.version, existingAddon.version) <= 0)
            continue;
        }
        catch (e) {
          
          WARN("Profile contains an add-on with a bad or missing install " +
               "manifest at " + existingEntry.path + ", overwriting", e);
        }
      }
      else if (Prefs.getBoolPref(PREF_BRANCH_INSTALLED_ADDON + id, false)) {
        continue;
      }

      
      try {
        profileLocation.installAddon(id, entry, null, true);
        LOG("Installed distribution add-on " + id);

        Services.prefs.setBoolPref(PREF_BRANCH_INSTALLED_ADDON + id, true)

        
        
        
        if (!(KEY_APP_PROFILE in aManifests))
          aManifests[KEY_APP_PROFILE] = {};
        aManifests[KEY_APP_PROFILE][id] = addon;
        changed = true;
      }
      catch (e) {
        ERROR("Failed to install distribution add-on " + entry.path, e);
      }
    }

    entries.close();

    return changed;
  },

  





























  processFileChanges: function XPI_processFileChanges(aState, aManifests,
                                                      aUpdateCompatibility,
                                                      aOldAppVersion,
                                                      aOldPlatformVersion,
                                                      aMigrateData,
                                                      aActiveBundles) {
    let visibleAddons = {};
    let oldBootstrappedAddons = this.bootstrappedAddons;
    this.bootstrappedAddons = {};

    














    function updateMetadata(aInstallLocation, aOldAddon, aAddonState) {
      LOG("Add-on " + aOldAddon.id + " modified in " + aInstallLocation.name);

      
      let newAddon = aManifests[aInstallLocation.name][aOldAddon.id];

      try {
        
        if (!newAddon) {
          let file = aInstallLocation.getLocationForID(aOldAddon.id);
          newAddon = loadManifestFromFile(file);
          applyBlocklistChanges(aOldAddon, newAddon);
        }

        
        
        if (newAddon.id != aOldAddon.id)
          throw new Error("Incorrect id in install manifest");
      }
      catch (e) {
        WARN("Add-on is invalid", e);
        XPIDatabase.removeAddonMetadata(aOldAddon);
        if (!aInstallLocation.locked)
          aInstallLocation.uninstallAddon(aOldAddon.id);
        else
          WARN("Could not uninstall invalid item from locked install location");
        
        if (aOldAddon.active)
          return true;

        return false;
      }

      
      newAddon._installLocation = aInstallLocation;
      newAddon.updateDate = aAddonState.mtime;
      newAddon.visible = !(newAddon.id in visibleAddons);

      
      XPIDatabase.updateAddonMetadata(aOldAddon, newAddon, aAddonState.descriptor);
      if (newAddon.visible) {
        visibleAddons[newAddon.id] = newAddon;

        
        
        if (aOldAddon.active && isAddonDisabled(newAddon))
          XPIProvider.enableDefaultTheme();

        
        if (newAddon.active && newAddon.bootstrap) {
          let installReason = Services.vc.compare(aOldAddon.version, newAddon.version) < 0 ?
                              BOOTSTRAP_REASONS.ADDON_UPGRADE :
                              BOOTSTRAP_REASONS.ADDON_DOWNGRADE;

          let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
          file.persistentDescriptor = aAddonState.descriptor;
          XPIProvider.callBootstrapMethod(newAddon.id, newAddon.version, file,
                                          "install", installReason);
          return false;
        }

        
        return true;
      }

      return false;
    }

    















    function updateVisibilityAndCompatibility(aInstallLocation, aOldAddon,
                                              aAddonState) {
      let changed = false;

      
      if (!(aOldAddon.id in visibleAddons)) {
        visibleAddons[aOldAddon.id] = aOldAddon;

        if (!aOldAddon.visible) {
          XPIDatabase.makeAddonVisible(aOldAddon);

          if (aOldAddon.bootstrap) {
            
            let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
            file.persistentDescriptor = aAddonState.descriptor;
            XPIProvider.callBootstrapMethod(aOldAddon.id, aOldAddon.version, file,
                                            "install",
                                            BOOTSTRAP_REASONS.ADDON_INSTALL);

            
            
            if (!isAddonDisabled(aOldAddon)) {
              aOldAddon.active = true;
              XPIDatabase.updateAddonActive(aOldAddon);
            }
            else {
              XPIProvider.unloadBootstrapScope(newAddon.id);
            }
          }
          else {
            
            changed = true;
          }
        }
      }

      
      if (aUpdateCompatibility) {
        
        
        let newAddon = new AddonInternal();
        newAddon.id = aOldAddon.id;
        newAddon.version = aOldAddon.version;
        newAddon.type = aOldAddon.type;
        newAddon.appDisabled = !isUsableAddon(aOldAddon);

        
        if (aOldAddon.type == "theme")
          newAddon.userDisabled = aOldAddon.internalName != XPIProvider.selectedSkin;

        applyBlocklistChanges(aOldAddon, newAddon, aOldAppVersion,
                              aOldPlatformVersion);

        let wasDisabled = isAddonDisabled(aOldAddon);
        let isDisabled = isAddonDisabled(newAddon);

        
        if (aOldAddon.visible && newAddon.appDisabled && !aOldAddon.appDisabled)
          XPIProvider.startupChanges.appDisabled.push(aOldAddon.id);

        
        if (newAddon.appDisabled != aOldAddon.appDisabled ||
            newAddon.userDisabled != aOldAddon.userDisabled ||
            newAddon.softDisabled != aOldAddon.softDisabled) {
          LOG("Add-on " + aOldAddon.id + " changed appDisabled state to " +
              newAddon.appDisabled + ", userDisabled state to " +
              newAddon.userDisabled + " and softDisabled state to " +
              newAddon.softDisabled);
          XPIDatabase.setAddonProperties(aOldAddon, {
            appDisabled: newAddon.appDisabled,
            userDisabled: newAddon.userDisabled,
            softDisabled: newAddon.softDisabled
          });
        }

        
        
        if (aOldAddon.visible && wasDisabled != isDisabled) {
          if (aOldAddon.bootstrap) {
            
            aOldAddon.active = !isDisabled;
            XPIDatabase.updateAddonActive(aOldAddon);
          }
          else {
            changed = true;
          }
        }
      }

      if (aOldAddon.visible && aOldAddon.active && aOldAddon.bootstrap) {
        XPIProvider.bootstrappedAddons[aOldAddon.id] = {
          version: aOldAddon.version,
          descriptor: aAddonState.descriptor
        };
      }

      return changed;
    }

    










    function removeMetadata(aInstallLocation, aOldAddon) {
      
      LOG("Add-on " + aOldAddon.id + " removed from " + aInstallLocation.name);
      XPIDatabase.removeAddonMetadata(aOldAddon);
      if (aOldAddon.active) {

        
        
        if (aOldAddon.type == "theme")
          XPIProvider.enableDefaultTheme();

        
        if (!aOldAddon.bootstrap)
          return true;
      }

      return false;
    }

    














    function addMetadata(aInstallLocation, aId, aAddonState, aMigrateData) {
      LOG("New add-on " + aId + " installed in " + aInstallLocation.name);

      let newAddon = null;
      
      
      if (aInstallLocation.name in aManifests)
        newAddon = aManifests[aInstallLocation.name][aId];

      try {
        
        if (!newAddon) {
          let file = aInstallLocation.getLocationForID(aId);
          newAddon = loadManifestFromFile(file);
        }
        
        if (newAddon.id != aId)
          throw new Error("Incorrect id in install manifest");
      }
      catch (e) {
        WARN("Add-on is invalid", e);

        
        
        if (!aInstallLocation.locked)
          aInstallLocation.uninstallAddon(aId);
        else
          WARN("Could not uninstall invalid item from locked install location");
        return false;
      }

      
      newAddon._installLocation = aInstallLocation;
      newAddon.visible = !(newAddon.id in visibleAddons);
      newAddon.installDate = aAddonState.mtime;
      newAddon.updateDate = aAddonState.mtime;

      
      if (aMigrateData) {
        
        
        if (newAddon.type != "theme")
          newAddon.userDisabled = aMigrateData.userDisabled;
        if ("installDate" in aMigrateData)
          newAddon.installDate = aMigrateData.installDate;
        if ("softDisabled" in aMigrateData)
          newAddon.softDisabled = aMigrateData.softDisabled;

        
        
        
        if (aMigrateData.version == newAddon.version) {
          if ("targetApplications" in aMigrateData)
            newAddon.applyCompatibilityUpdate(aMigrateData, true);
        }

        
        applyBlocklistChanges(newAddon, newAddon, aOldAppVersion,
                              aOldPlatformVersion);
      }

      if (aActiveBundles) {
        
        
        
        if (newAddon.type == "theme")
          newAddon.active = newAddon.internalName == XPIProvider.currentSkin;
        else
          newAddon.active = aActiveBundles.indexOf(aAddonState.descriptor) != -1;

        
        
        if (!newAddon.active && newAddon.visible && !isAddonDisabled(newAddon)) {
          
          if (newAddon.blocklistState == Ci.nsIBlocklistService.STATE_SOFTBLOCKED)
            newAddon.softDisabled = true;
          else
            newAddon.userDisabled = true;
        }
      }
      else {
        newAddon.active = (newAddon.visible && !isAddonDisabled(newAddon))
      }

      try {
        
        XPIDatabase.addAddonMetadata(newAddon, aAddonState.descriptor);
      }
      catch (e) {
        
        
        
        ERROR("Failed to add add-on " + aId + " in " + aInstallLocation.name +
              " to database", e);
        return false;
      }

      if (newAddon.visible) {
        
        if (newAddon._installLocation.name != KEY_APP_GLOBAL)
          XPIProvider.allAppGlobal = false;

        visibleAddons[newAddon.id] = newAddon;

        let installReason = BOOTSTRAP_REASONS.ADDON_INSTALL;

        
        if (newAddon.id in oldBootstrappedAddons) {
          let oldBootstrap = oldBootstrappedAddons[newAddon.id];
          XPIProvider.bootstrappedAddons[newAddon.id] = oldBootstrap;

          installReason = Services.vc.compare(oldBootstrap.version, newAddon.version) < 0 ?
                          BOOTSTRAP_REASONS.ADDON_UPGRADE :
                          BOOTSTRAP_REASONS.ADDON_DOWNGRADE;

          let oldAddonFile = Cc["@mozilla.org/file/local;1"].
                             createInstance(Ci.nsILocalFile);
          oldAddonFile.persistentDescriptor = oldBootstrap.descriptor;
          XPIProvider.callBootstrapMethod(newAddon.id, oldBootstrap.version,
                                          oldAddonFile, "uninstall",
                                          installReason);
          XPIProvider.unloadBootstrapScope(newAddon.id);
        }

        if (!newAddon.bootstrap)
          return true;

        
        let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
        file.persistentDescriptor = aAddonState.descriptor;
        XPIProvider.callBootstrapMethod(newAddon.id, newAddon.version, file,
                                        "install", installReason);
        if (!newAddon.active)
          XPIProvider.unloadBootstrapScope(newAddon.id);
      }

      return false;
    }

    let changed = false;
    let knownLocations = XPIDatabase.getInstallLocations();

    
    
    
    aState.reverse().forEach(function(aSt) {

      
      
      let installLocation = this.installLocationsByName[aSt.name];
      let addonStates = aSt.addons;

      
      let pos = knownLocations.indexOf(installLocation.name);
      if (pos >= 0) {
        knownLocations.splice(pos, 1);
        let addons = XPIDatabase.getAddonsInLocation(installLocation.name);
        
        
        addons.forEach(function(aOldAddon) {
          
          if (aOldAddon.id in addonStates) {
            let addonState = addonStates[aOldAddon.id];
            delete addonStates[aOldAddon.id];

            
            if (aOldAddon.visible && !aOldAddon.active)
              XPIProvider.inactiveAddonIDs.push(aOldAddon.id);

            
            
            
            
            
            if (aOldAddon.id in aManifests[installLocation.name] ||
                aOldAddon.updateDate != addonState.mtime ||
                aOldAddon._descriptor != addonState.descriptor ||
                (aUpdateCompatibility && installLocation.name == KEY_APP_GLOBAL)) {
              changed = updateMetadata(installLocation, aOldAddon, addonState) ||
                        changed;
            }
            else {
              changed = updateVisibilityAndCompatibility(installLocation,
                                                         aOldAddon, addonState) ||
                        changed;
            }
            if (aOldAddon.visible && aOldAddon._installLocation.name != KEY_APP_GLOBAL)
              XPIProvider.allAppGlobal = false;
          }
          else {
            changed = removeMetadata(installLocation, aOldAddon) || changed;
          }
        }, this);
      }

      

      
      let locMigrateData = {};
      if (aMigrateData && installLocation.name in aMigrateData)
        locMigrateData = aMigrateData[installLocation.name];
      for (let id in addonStates) {
        changed = addMetadata(installLocation, id, addonStates[id],
                              locMigrateData[id]) || changed;
      }
    }, this);

    
    
    
    
    knownLocations.forEach(function(aLocation) {
      let addons = XPIDatabase.getAddonsInLocation(aLocation);
      addons.forEach(function(aOldAddon) {
        changed = removeMetadata(aLocation, aOldAddon) || changed;
      }, this);
    }, this);

    
    cache = JSON.stringify(this.getInstallLocationStates());
    Services.prefs.setCharPref(PREF_INSTALL_CACHE, cache);

    return changed;
  },

  



  importPermissions: function XPI_importPermissions() {
    function importList(aPrefBranch, aAction) {
      let list = Services.prefs.getChildList(aPrefBranch, {});
      list.forEach(function(aPref) {
        let hosts = Prefs.getCharPref(aPref, "");
        if (!hosts)
          return;

        hosts.split(",").forEach(function(aHost) {
          Services.perms.add(NetUtil.newURI("http://" + aHost), XPI_PERMISSION,
                             aAction);
        });

        Services.prefs.setCharPref(aPref, "");
      });
    }

    importList(PREF_XPI_WHITELIST_PERMISSIONS,
               Ci.nsIPermissionManager.ALLOW_ACTION);
    importList(PREF_XPI_BLACKLIST_PERMISSIONS,
               Ci.nsIPermissionManager.DENY_ACTION);
  },

  

















  checkForChanges: function XPI_checkForChanges(aAppChanged, aOldAppVersion,
                                                aOldPlatformVersion) {
    LOG("checkForChanges");

    
    if (aAppChanged !== false)
      this.importPermissions();

    
    
    let updateDatabase = aAppChanged;

    
    
    this.bootstrappedAddons = JSON.parse(Prefs.getCharPref(PREF_BOOTSTRAP_ADDONS,
                                         "{}"));

    
    
    
    let manifests = {};
    updateDatabase = this.processPendingFileChanges(manifests) | updateDatabase;

    
    
    
    let hasPendingChanges = Prefs.getBoolPref(PREF_PENDING_OPERATIONS);

    
    updateDatabase |= DB_SCHEMA != Prefs.getIntPref(PREF_DB_SCHEMA, 0);

    
    if (aAppChanged !== false &&
        Prefs.getBoolPref(PREF_INSTALL_DISTRO_ADDONS, true))
      updateDatabase = this.installDistributionAddons(manifests) | updateDatabase;

    let state = this.getInstallLocationStates();

    
    
    let dbFile = FileUtils.getFile(KEY_PROFILEDIR, [FILE_DATABASE], true);
    updateDatabase |= !dbFile.exists();
    if (!updateDatabase) {
      
      let cache = Prefs.getCharPref(PREF_INSTALL_CACHE, null);
      updateDatabase |= cache != JSON.stringify(state);
    }

    if (!updateDatabase) {
      let bootstrapDescriptors = [this.bootstrappedAddons[b].descriptor
                                  for (b in this.bootstrappedAddons)];

      state.forEach(function(aInstallLocationState) {
        for (let id in aInstallLocationState.addons) {
          let pos = bootstrapDescriptors.indexOf(aInstallLocationState.addons[id].descriptor);
          if (pos != -1)
            bootstrapDescriptors.splice(pos, 1);
        }
      });
  
      if (bootstrapDescriptors.length > 0) {
        WARN("Bootstrap state is invalid (missing add-ons: " + bootstrapDescriptors.toSource() + ")");
        updateDatabase = true;
      }
    }

    
    XPIDatabase.beginTransaction();
    try {
      let extensionListChanged = false;
      
      
      if (updateDatabase || hasPendingChanges) {
        let migrateData = XPIDatabase.openConnection(false);

        try {
          extensionListChanged = this.processFileChanges(state, manifests,
                                                         aAppChanged,
                                                         aOldAppVersion,
                                                         aOldPlatformVersion,
                                                         migrateData, null);
        }
        catch (e) {
          ERROR("Error processing file changes", e);
        }
      }

      if (aAppChanged) {
        
        
        if (this.currentSkin != this.defaultSkin) {
          let oldSkin = XPIDatabase.getVisibleAddonForInternalName(this.currentSkin);
          if (!oldSkin || isAddonDisabled(oldSkin))
            this.enableDefaultTheme();
        }

        
        
        let oldCache = FileUtils.getFile(KEY_PROFILEDIR, [FILE_OLD_CACHE], true);
        if (oldCache.exists())
          oldCache.remove(true);
      }

      
      
      if (extensionListChanged || hasPendingChanges) {
        LOG("Updating database with changes to installed add-ons");
        XPIDatabase.updateActiveAddons();
        XPIDatabase.commitTransaction();
        XPIDatabase.writeAddonsList();
        Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, false);
        Services.prefs.setCharPref(PREF_BOOTSTRAP_ADDONS,
                                   JSON.stringify(this.bootstrappedAddons));
        return true;
      }

      LOG("No changes found");
      XPIDatabase.commitTransaction();
    }
    catch (e) {
      ERROR("Error during startup file checks, rolling back any database " +
            "changes", e);
      XPIDatabase.rollbackTransaction();
    }

    
    let addonsList = FileUtils.getFile(KEY_PROFILEDIR, [FILE_XPI_ADDONS_LIST],
                                       true);
    if (!addonsList.exists()) {
      LOG("Add-ons list is missing, recreating");
      XPIDatabase.writeAddonsList();
    }

    return false;
  },

  







  supportsMimetype: function XPI_supportsMimetype(aMimetype) {
    return aMimetype == "application/x-xpinstall";
  },

  




  isInstallEnabled: function XPI_isInstallEnabled() {
    
    return Prefs.getBoolPref(PREF_XPI_ENABLED, true);
  },

  






  isInstallAllowed: function XPI_isInstallAllowed(aUri) {
    if (!this.isInstallEnabled())
      return false;

    if (!aUri)
      return true;

    
    if (aUri.schemeIs("chrome") || aUri.schemeIs("file"))
      return true;


    let permission = Services.perms.testPermission(aUri, XPI_PERMISSION);
    if (permission == Ci.nsIPermissionManager.DENY_ACTION)
      return false;

    let requireWhitelist = Prefs.getBoolPref(PREF_XPI_WHITELIST_REQUIRED, true);
    if (requireWhitelist && (permission != Ci.nsIPermissionManager.ALLOW_ACTION))
      return false;

    return true;
  },

  

















  getInstallForURL: function XPI_getInstallForURL(aUrl, aHash, aName, aIconURL,
                                                  aVersion, aLoadGroup, aCallback) {
    AddonInstall.createDownload(function(aInstall) {
      aCallback(aInstall.wrapper);
    }, aUrl, aHash, aName, aIconURL, aVersion, aLoadGroup);
  },

  







  getInstallForFile: function XPI_getInstallForFile(aFile, aCallback) {
    AddonInstall.createInstall(function(aInstall) {
      if (aInstall)
        aCallback(aInstall.wrapper);
      else
        aCallback(null);
    }, aFile);
  },

  





  removeActiveInstall: function XPI_removeActiveInstall(aInstall) {
    this.installs = this.installs.filter(function(i) i != aInstall);
  },

  







  getAddonByID: function XPI_getAddonByID(aId, aCallback) {
    XPIDatabase.getVisibleAddonForID(aId, function(aAddon) {
      if (aAddon)
        aCallback(createWrapper(aAddon));
      else
        aCallback(null);
    });
  },

  







  getAddonsByTypes: function XPI_getAddonsByTypes(aTypes, aCallback) {
    XPIDatabase.getVisibleAddons(aTypes, function(aAddons) {
      aCallback([createWrapper(a) for each (a in aAddons)]);
    });
  },

  







  getAddonsWithOperationsByTypes:
  function XPI_getAddonsWithOperationsByTypes(aTypes, aCallback) {
    XPIDatabase.getVisibleAddonsWithPendingOperations(aTypes, function(aAddons) {
      let results = [createWrapper(a) for each (a in aAddons)];
      XPIProvider.installs.forEach(function(aInstall) {
        if (aInstall.state == AddonManager.STATE_INSTALLED &&
            !(aInstall.addon instanceof DBAddonInternal))
          results.push(createWrapper(aInstall.addon));
      });
      aCallback(results);
    });
  },

  








  getInstallsByTypes: function XPI_getInstallsByTypes(aTypes, aCallback) {
    let results = [];
    this.installs.forEach(function(aInstall) {
      if (!aTypes || aTypes.indexOf(aInstall.type) >= 0)
        results.push(aInstall.wrapper);
    });
    aCallback(results);
  },

  











  addonChanged: function XPI_addonChanged(aId, aType, aPendingRestart) {
    
    if (aType != "theme")
      return;

    if (!aId) {
      
      this.enableDefaultTheme();
      return;
    }

    
    
    let previousTheme = null;
    let newSkin = this.defaultSkin;
    let addons = XPIDatabase.getAddonsByType("theme");
    addons.forEach(function(aTheme) {
      if (!aTheme.visible)
        return;
      if (aTheme.id == aId)
        newSkin = aTheme.internalName;
      else if (aTheme.userDisabled == false && !aTheme.pendingUninstall)
        previousTheme = aTheme;
    }, this);

    if (aPendingRestart) {
      Services.prefs.setBoolPref(PREF_DSS_SWITCHPENDING, true);
      Services.prefs.setCharPref(PREF_DSS_SKIN_TO_SELECT, newSkin);
    }
    else if (newSkin == this.currentSkin) {
      try {
        Services.prefs.clearUserPref(PREF_DSS_SWITCHPENDING);
      }
      catch (e) { }
      try {
        Services.prefs.clearUserPref(PREF_DSS_SKIN_TO_SELECT);
      }
      catch (e) { }
    }
    else {
      Services.prefs.setCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN, newSkin);
      this.currentSkin = newSkin;
    }
    this.selectedSkin = newSkin;

    
    
    Services.prefs.savePrefFile(null);

    
    
    if (previousTheme)
      this.updateAddonDisabledState(previousTheme, true);
  },

  


  updateAddonAppDisabledStates: function XPI_updateAddonAppDisabledStates() {
    let addons = XPIDatabase.getAddons();
    addons.forEach(function(aAddon) {
      this.updateAddonDisabledState(aAddon);
    }, this);
  },

  



  enableDefaultTheme: function XPI_enableDefaultTheme() {
    LOG("Activating default theme");
    let addon = XPIDatabase.getVisibleAddonForInternalName(this.defaultSkin);
    if (addon) {
      if (addon.userDisabled) {
        this.updateAddonDisabledState(addon, false);
      }
      else if (!this.extensionsActive) {
        
        
        
        Services.prefs.setCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN,
                                   addon.internalName);
        this.currentSkin = this.selectedSkin = addon.internalName;
        Prefs.clearUserPref(PREF_DSS_SKIN_TO_SELECT);
        Prefs.clearUserPref(PREF_DSS_SWITCHPENDING);
      }
      else {
        WARN("Attempting to activate an already active default theme");
      }
    }
    else {
      WARN("Unable to activate the default theme");
    }
  },

  




  observe: function XPI_observe(aSubject, aTopic, aData) {
    switch (aData) {
    case PREF_EM_CHECK_COMPATIBILITY:
    case PREF_EM_CHECK_UPDATE_SECURITY:
      this.checkCompatibility = Prefs.getBoolPref(PREF_EM_CHECK_COMPATIBILITY,
                                                  true);
      this.checkUpdateSecurity = Prefs.getBoolPref(PREF_EM_CHECK_UPDATE_SECURITY,
                                                   true);
      this.updateAllAddonDisabledStates();
      break;
    }
  },

  






  enableRequiresRestart: function XPI_enableRequiresRestart(aAddon) {
    
    
    if (!this.extensionsActive)
      return false;

    
    
    if (Services.appinfo.inSafeMode)
      return false;

    
    if (aAddon.active)
      return false;

    if (aAddon.type == "theme") {
      
      
      if (Prefs.getBoolPref(PREF_EM_DSS_ENABLED))
        return false;

      
      
      
      return aAddon.internalName != this.currentSkin;
    }

    return !aAddon.bootstrap;
  },

  






  disableRequiresRestart: function XPI_disableRequiresRestart(aAddon) {
    
    
    if (!this.extensionsActive)
      return false;

    
    
    if (Services.appinfo.inSafeMode)
      return false;

    
    if (!aAddon.active)
      return false;

    if (aAddon.type == "theme") {
      
      
      if (Prefs.getBoolPref(PREF_EM_DSS_ENABLED))
        return false;

      
      
      
      if (aAddon.internalName != this.defaultSkin)
        return true;

      
      
      
      
      
      
      return this.selectedSkin != this.currentSkin;
    }

    return !aAddon.bootstrap;
  },

  






  installRequiresRestart: function XPI_installRequiresRestart(aAddon) {
    
    
    if (!this.extensionsActive)
      return false;

    
    
    if (Services.appinfo.inSafeMode)
      return false;

    
    
    
    
    if (aAddon instanceof DBAddonInternal)
      return false;

    
    
    if ("_install" in aAddon && aAddon._install) {
      
      
      
      let existingAddon = aAddon._install.existingAddon;
      if (existingAddon && this.uninstallRequiresRestart(existingAddon))
        return true;
    }

    
    
    if (isAddonDisabled(aAddon))
      return false;

    
    
    
    return aAddon.type == "theme" || !aAddon.bootstrap;
  },

  






  uninstallRequiresRestart: function XPI_uninstallRequiresRestart(aAddon) {
    
    
    if (!this.extensionsActive)
      return false;

    
    
    if (Services.appinfo.inSafeMode)
      return false;

    
    
    return this.disableRequiresRestart(aAddon);
  },

  













  loadBootstrapScope: function XPI_loadBootstrapScope(aId, aFile, aVersion) {
    LOG("Loading bootstrap scope from " + aFile.path);
    
    this.bootstrappedAddons[aId] = {
      version: aVersion,
      descriptor: aFile.persistentDescriptor
    };
    this.addAddonsToCrashReporter();

    let principal = Cc["@mozilla.org/systemprincipal;1"].
                    createInstance(Ci.nsIPrincipal);
    this.bootstrapScopes[aId] = new Components.utils.Sandbox(principal);

    let bootstrap = aFile.clone();
    let name = aFile.leafName;
    let spec;

    if (!bootstrap.exists()) {
      ERROR("Attempted to load bootstrap scope from missing directory " + bootstrap.path);
      return;
    }

    if (bootstrap.isDirectory()) {
      bootstrap.append("bootstrap.js");
      let uri = Services.io.newFileURI(bootstrap);
      spec = uri.spec;
    } else {
      spec = buildJarURI(bootstrap, "bootstrap.js").spec;
    }
    if (bootstrap.exists()) {
      let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
                   createInstance(Ci.mozIJSSubScriptLoader);

      try {
        
        
        
        this.bootstrapScopes[aId].__SCRIPT_URI_SPEC__ = spec;
        Components.utils.evalInSandbox(
          "Components.classes['@mozilla.org/moz/jssubscript-loader;1'] \
                     .createInstance(Components.interfaces.mozIJSSubScriptLoader) \
                     .loadSubScript(__SCRIPT_URI_SPEC__);", this.bootstrapScopes[aId], "ECMAv5");
      }
      catch (e) {
        WARN("Error loading bootstrap.js for " + aId, e);
      }

      
      for (let name in BOOTSTRAP_REASONS)
        this.bootstrapScopes[aId][name] = BOOTSTRAP_REASONS[name];
    }
    else {
      WARN("Bootstrap missing for " + aId);
    }
  },

  






  unloadBootstrapScope: function XPI_unloadBootstrapScope(aId) {
    delete this.bootstrapScopes[aId];
    delete this.bootstrappedAddons[aId];
    this.addAddonsToCrashReporter();
  },

  













  callBootstrapMethod: function XPI_callBootstrapMethod(aId, aVersion, aFile,
                                                        aMethod, aReason) {
    
    if (Services.appinfo.inSafeMode)
      return;

    
    if (!(aId in this.bootstrapScopes))
      this.loadBootstrapScope(aId, aFile, aVersion);

    if (!(aMethod in this.bootstrapScopes[aId])) {
      WARN("Add-on " + aId + " is missing bootstrap method " + aMethod);
      return;
    }

    let params = {
      id: aId,
      version: aVersion,
      installPath: aFile.clone()
    };

    LOG("Calling bootstrap method " + aMethod + " on " + aId + " version " +
        aVersion);
    try {
      this.bootstrapScopes[aId][aMethod](params, aReason);
    }
    catch (e) {
      WARN("Exception running bootstrap method " + aMethod + " on " +
           aId, e);
    }
  },

  


  updateAllAddonDisabledStates: function XPI_updateAllAddonDisabledStates() {
    let addons = XPIDatabase.getAddons();
    addons.forEach(function(aAddon) {
      this.updateAddonDisabledState(aAddon);
    }, this);
  },

  














  updateAddonDisabledState: function XPI_updateAddonDisabledState(aAddon,
                                                                  aUserDisabled,
                                                                  aSoftDisabled) {
    if (!(aAddon instanceof DBAddonInternal))
      throw new Error("Can only update addon states for installed addons.");
    if (aUserDisabled !== undefined && aSoftDisabled !== undefined) {
      throw new Error("Cannot change userDisabled and softDisabled at the " +
                      "same time");
    }

    if (aUserDisabled === undefined) {
      aUserDisabled = aAddon.userDisabled;
    }
    else if (!aUserDisabled) {
      
      aSoftDisabled = false;
    }

    
    
    if (aSoftDisabled === undefined || aUserDisabled)
      aSoftDisabled = aAddon.softDisabled;

    let appDisabled = !isUsableAddon(aAddon);
    
    if (aAddon.userDisabled == aUserDisabled &&
        aAddon.appDisabled == appDisabled &&
        aAddon.softDisabled == aSoftDisabled)
      return;

    let wasDisabled = isAddonDisabled(aAddon);
    let isDisabled = aUserDisabled || aSoftDisabled || appDisabled;

    
    XPIDatabase.setAddonProperties(aAddon, {
      userDisabled: aUserDisabled,
      appDisabled: appDisabled,
      softDisabled: aSoftDisabled
    });

    
    
    if (!aAddon.visible || (wasDisabled == isDisabled))
      return;

    
    Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, true);

    let wrapper = createWrapper(aAddon);
    
    if (isDisabled != aAddon.active) {
      AddonManagerPrivate.callAddonListeners("onOperationCancelled", wrapper);
    }
    else {
      if (isDisabled) {
        var needsRestart = this.disableRequiresRestart(aAddon);
        AddonManagerPrivate.callAddonListeners("onDisabling", wrapper,
                                               needsRestart);
      }
      else {
        needsRestart = this.enableRequiresRestart(aAddon);
        AddonManagerPrivate.callAddonListeners("onEnabling", wrapper,
                                               needsRestart);
      }

      if (!needsRestart) {
        aAddon.active = !isDisabled;
        XPIDatabase.updateAddonActive(aAddon);
        if (isDisabled) {
          if (aAddon.bootstrap) {
            let file = aAddon._installLocation.getLocationForID(aAddon.id);
            this.callBootstrapMethod(aAddon.id, aAddon.version, file, "shutdown",
                                     BOOTSTRAP_REASONS.ADDON_DISABLE);
            this.unloadBootstrapScope(aAddon.id);
          }
          AddonManagerPrivate.callAddonListeners("onDisabled", wrapper);
        }
        else {
          if (aAddon.bootstrap) {
            let file = aAddon._installLocation.getLocationForID(aAddon.id);
            this.callBootstrapMethod(aAddon.id, aAddon.version, file, "startup",
                                     BOOTSTRAP_REASONS.ADDON_ENABLE);
          }
          AddonManagerPrivate.callAddonListeners("onEnabled", wrapper);
        }
      }
    }

    
    if (aAddon.type == "theme" && !isDisabled)
      AddonManagerPrivate.notifyAddonChanged(aAddon.id, aAddon.type, needsRestart);
  },

  








  uninstallAddon: function XPI_uninstallAddon(aAddon) {
    if (!(aAddon instanceof DBAddonInternal))
      throw new Error("Can only uninstall installed addons.");

    if (aAddon._installLocation.locked)
      throw new Error("Cannot uninstall addons from locked install locations");

    
    let requiresRestart = this.uninstallRequiresRestart(aAddon);

    if (requiresRestart) {
      
      
      let stage = aAddon._installLocation.getStagingDir();
      stage.append(aAddon.id);
      if (!stage.exists())
        stage.create(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);

      XPIDatabase.setAddonProperties(aAddon, {
        pendingUninstall: true
      });
      Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, true);
    }

    
    if (!aAddon.visible)
      return;

    let wrapper = createWrapper(aAddon);
    AddonManagerPrivate.callAddonListeners("onUninstalling", wrapper,
                                           requiresRestart);

    if (!requiresRestart) {
      if (aAddon.bootstrap) {
        let file = aAddon._installLocation.getLocationForID(aAddon.id);
        if (aAddon.active) {
          this.callBootstrapMethod(aAddon.id, aAddon.version, file, "shutdown",
                                   BOOTSTRAP_REASONS.ADDON_UNINSTALL);
        }
        this.callBootstrapMethod(aAddon.id, aAddon.version, file, "uninstall",
                                 BOOTSTRAP_REASONS.ADDON_UNINSTALL);
        this.unloadBootstrapScope(aAddon.id);
      }
      aAddon._installLocation.uninstallAddon(aAddon.id);
      XPIDatabase.removeAddonMetadata(aAddon);
      AddonManagerPrivate.callAddonListeners("onUninstalled", wrapper);

      
      function revealAddon(aAddon) {
        XPIDatabase.makeAddonVisible(aAddon);

        let wrappedAddon = createWrapper(aAddon);
        AddonManagerPrivate.callAddonListeners("onInstalling", wrappedAddon, false);

        if (!isAddonDisabled(aAddon) && !XPIProvider.enableRequiresRestart(aAddon)) {
          aAddon.active = true;
          XPIDatabase.updateAddonActive(aAddon);
        }

        if (aAddon.bootstrap) {
          let file = aAddon._installLocation.getLocationForID(aAddon.id);
          XPIProvider.callBootstrapMethod(aAddon.id, aAddon.version, file,
                                          "install", BOOTSTRAP_REASONS.ADDON_INSTALL);

          if (aAddon.active) {
            XPIProvider.callBootstrapMethod(aAddon.id, aAddon.version, file,
                                            "startup", BOOTSTRAP_REASONS.ADDON_INSTALL);
          }
          else {
            XPIProvider.unloadBootstrapScope(aAddon.id);
          }
        }

        
        
        AddonManagerPrivate.callAddonListeners("onInstalled", wrappedAddon);
      }

      function checkInstallLocation(aPos) {
        if (aPos < 0)
          return;

        let location = XPIProvider.installLocations[aPos];
        XPIDatabase.getAddonInLocation(aAddon.id, location.name, function(aNewAddon) {
          if (aNewAddon)
            revealAddon(aNewAddon);
          else
            checkInstallLocation(aPos - 1);
        })
      }

      checkInstallLocation(this.installLocations.length - 1);
    }

    
    if (aAddon.type == "theme" && aAddon.active)
      AddonManagerPrivate.notifyAddonChanged(null, aAddon.type, requiresRestart);
  },

  





  cancelUninstallAddon: function XPI_cancelUninstallAddon(aAddon) {
    if (!(aAddon instanceof DBAddonInternal))
      throw new Error("Can only cancel uninstall for installed addons.");

    cleanStagingDir(aAddon._installLocation.getStagingDir(), [aAddon.id]);

    XPIDatabase.setAddonProperties(aAddon, {
      pendingUninstall: false
    });

    if (!aAddon.visible)
      return;

    Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, true);

    
    let wrapper = createWrapper(aAddon);
    AddonManagerPrivate.callAddonListeners("onOperationCancelled", wrapper);

    
    if (aAddon.type == "theme" && aAddon.active)
      AddonManagerPrivate.notifyAddonChanged(aAddon.id, aAddon.type, false);
  }
};

const FIELDS_ADDON = "internal_id, id, location, version, type, internalName, " +
                     "updateURL, updateKey, optionsURL, aboutURL, iconURL, " +
                     "icon64URL, defaultLocale, visible, active, userDisabled, " +
                     "appDisabled, pendingUninstall, descriptor, installDate, " +
                     "updateDate, applyBackgroundUpdates, bootstrap, skinnable, " +
                     "size, sourceURI, releaseNotesURI, softDisabled";









function logSQLError(aError, aErrorString) {
  ERROR("SQL error " + aError + ": " + aErrorString);
}







function asyncErrorLogger(aError) {
  logSQLError(aError.result, aError.message);
}








function executeStatement(aStatement) {
  try {
    aStatement.execute();
  }
  catch (e) {
    logSQLError(XPIDatabase.connection.lastError,
                XPIDatabase.connection.lastErrorString);
    throw e;
  }
}








function stepStatement(aStatement) {
  try {
    return aStatement.executeStep();
  }
  catch (e) {
    logSQLError(XPIDatabase.connection.lastError,
                XPIDatabase.connection.lastErrorString);
    throw e;
  }
}










function AsyncAddonListCallback(aCallback) {
  this.callback = aCallback;
  this.addons = [];
}

AsyncAddonListCallback.prototype = {
  callback: null,
  complete: false,
  count: 0,
  addons: null,

  handleResult: function(aResults) {
    let row = null;
    while (row = aResults.getNextRow()) {
      this.count++;
      let self = this;
      XPIDatabase.makeAddonFromRowAsync(row, function(aAddon) {
        function completeAddon(aRepositoryAddon) {
          aAddon._repositoryAddon = aRepositoryAddon;
          self.addons.push(aAddon);
          if (self.complete && self.addons.length == self.count)
           self.callback(self.addons);
        }

        if ("getCachedAddonByID" in AddonRepository)
          AddonRepository.getCachedAddonByID(aAddon.id, completeAddon);
        else
          completeAddon(null);
      });
    }
  },

  handleError: asyncErrorLogger,

  handleCompletion: function(aReason) {
    this.complete = true;
    if (this.addons.length == this.count)
      this.callback(this.addons);
  }
};

var XPIDatabase = {
  
  initialized: false,
  
  statementCache: {},
  
  
  addonCache: [],
  
  transactionCount: 0,

  
  statements: {
    _getDefaultLocale: "SELECT id, name, description, creator, homepageURL " +
                       "FROM locale WHERE id=:id",
    _getLocales: "SELECT addon_locale.locale, locale.id, locale.name, " +
                 "locale.description, locale.creator, locale.homepageURL " +
                 "FROM addon_locale JOIN locale ON " +
                 "addon_locale.locale_id=locale.id WHERE " +
                 "addon_internal_id=:internal_id",
    _getTargetApplications: "SELECT addon_internal_id, id, minVersion, " +
                            "maxVersion FROM targetApplication WHERE " +
                            "addon_internal_id=:internal_id",
    _getTargetPlatforms: "SELECT os, abi FROM targetPlatform WHERE " +
                         "addon_internal_id=:internal_id",
    _readLocaleStrings: "SELECT locale_id, type, value FROM locale_strings " +
                        "WHERE locale_id=:id",

    addAddonMetadata_addon: "INSERT INTO addon VALUES (NULL, :id, :location, " +
                            ":version, :type, :internalName, :updateURL, " +
                            ":updateKey, :optionsURL, :aboutURL, :iconURL, " +
                            ":icon64URL, :locale, :visible, :active, " +
                            ":userDisabled, :appDisabled, :pendingUninstall, " +
                            ":descriptor, :installDate, :updateDate, " +
                            ":applyBackgroundUpdates, :bootstrap, :skinnable, " +
                            ":size, :sourceURI, :releaseNotesURI, :softDisabled)",
    addAddonMetadata_addon_locale: "INSERT INTO addon_locale VALUES " +
                                   "(:internal_id, :name, :locale)",
    addAddonMetadata_locale: "INSERT INTO locale (name, description, creator, " +
                             "homepageURL) VALUES (:name, :description, " +
                             ":creator, :homepageURL)",
    addAddonMetadata_strings: "INSERT INTO locale_strings VALUES (:locale, " +
                              ":type, :value)",
    addAddonMetadata_targetApplication: "INSERT INTO targetApplication VALUES " +
                                        "(:internal_id, :id, :minVersion, " +
                                        ":maxVersion)",
    addAddonMetadata_targetPlatform: "INSERT INTO targetPlatform VALUES " +
                                     "(:internal_id, :os, :abi)",

    clearVisibleAddons: "UPDATE addon SET visible=0 WHERE id=:id",
    updateAddonActive: "UPDATE addon SET active=:active WHERE " +
                       "internal_id=:internal_id",

    getActiveAddons: "SELECT " + FIELDS_ADDON + " FROM addon WHERE active=1 AND " +
                     "type<>'theme' AND bootstrap=0",
    getActiveTheme: "SELECT " + FIELDS_ADDON + " FROM addon WHERE " +
                    "internalName=:internalName AND type='theme'",
    getThemes: "SELECT " + FIELDS_ADDON + " FROM addon WHERE type='theme'",

    getAddonInLocation: "SELECT " + FIELDS_ADDON + " FROM addon WHERE id=:id " +
                        "AND location=:location",
    getAddons: "SELECT " + FIELDS_ADDON + " FROM addon",
    getAddonsByType: "SELECT " + FIELDS_ADDON + " FROM addon WHERE type=:type",
    getAddonsInLocation: "SELECT " + FIELDS_ADDON + " FROM addon WHERE " +
                         "location=:location",
    getInstallLocations: "SELECT DISTINCT location FROM addon",
    getVisibleAddonForID: "SELECT " + FIELDS_ADDON + " FROM addon WHERE " +
                          "visible=1 AND id=:id",
    getVisibleAddoForInternalName: "SELECT " + FIELDS_ADDON + " FROM addon " +
                                   "WHERE visible=1 AND internalName=:internalName",
    getVisibleAddons: "SELECT " + FIELDS_ADDON + " FROM addon WHERE visible=1",
    getVisibleAddonsWithPendingOperations: "SELECT " + FIELDS_ADDON + " FROM " +
                                           "addon WHERE visible=1 " +
                                           "AND (pendingUninstall=1 OR " +
                                           "MAX(userDisabled,appDisabled)=active)",

    makeAddonVisible: "UPDATE addon SET visible=1 WHERE internal_id=:internal_id",
    removeAddonMetadata: "DELETE FROM addon WHERE internal_id=:internal_id",
    
    setActiveAddons: "UPDATE addon SET active=MIN(visible, 1 - userDisabled, " +
                     "1 - softDisabled, 1 - appDisabled)",
    setAddonProperties: "UPDATE addon SET userDisabled=:userDisabled, " +
                        "appDisabled=:appDisabled, " +
                        "softDisabled=:softDisabled, " +
                        "pendingUninstall=:pendingUninstall, " +
                        "applyBackgroundUpdates=:applyBackgroundUpdates WHERE " +
                        "internal_id=:internal_id",
    updateTargetApplications: "UPDATE targetApplication SET " +
                              "minVersion=:minVersion, maxVersion=:maxVersion " +
                              "WHERE addon_internal_id=:internal_id AND id=:id",

    createSavepoint: "SAVEPOINT 'default'",
    releaseSavepoint: "RELEASE SAVEPOINT 'default'",
    rollbackSavepoint: "ROLLBACK TO SAVEPOINT 'default'"
  },

  








  beginTransaction: function XPIDB_beginTransaction() {
    if (this.initialized)
      this.getStatement("createSavepoint").execute();
    this.transactionCount++;
  },

  



  commitTransaction: function XPIDB_commitTransaction() {
    if (this.transactionCount == 0) {
      ERROR("Attempt to commit one transaction too many.");
      return;
    }

    if (this.initialized)
      this.getStatement("releaseSavepoint").execute();
    this.transactionCount--;
  },

  



  rollbackTransaction: function XPIDB_rollbackTransaction() {
    if (this.transactionCount == 0) {
      ERROR("Attempt to rollback one transaction too many.");
      return;
    }

    if (this.initialized) {
      this.getStatement("rollbackSavepoint").execute();
      this.getStatement("releaseSavepoint").execute();
    }
    this.transactionCount--;
  },

  








  openDatabaseFile: function XPIDB_openDatabaseFile(aDBFile) {
    LOG("Opening database");
    let connection = null;

    
    try {
      connection = Services.storage.openUnsharedDatabase(aDBFile);
    }
    catch (e) {
      ERROR("Failed to open database (1st attempt)", e);
      try {
        aDBFile.remove(true);
      }
      catch (e) {
        ERROR("Failed to remove database that could not be opened", e);
      }
      try {
        connection = Services.storage.openUnsharedDatabase(aDBFile);
      }
      catch (e) {
        ERROR("Failed to open database (2nd attempt)", e);

        
        
        
        return Services.storage.openSpecialDatabase("memory");
      }
    }

    connection.executeSimpleSQL("PRAGMA synchronous = FULL");
    connection.executeSimpleSQL("PRAGMA locking_mode = EXCLUSIVE");

    return connection;
  },

  








  openConnection: function XPIDB_openConnection(aRebuildOnError) {
    this.initialized = true;
    let dbfile = FileUtils.getFile(KEY_PROFILEDIR, [FILE_DATABASE], true);
    delete this.connection;

    this.connection = this.openDatabaseFile(dbfile);

    let migrateData = null;
    
    
    let schemaVersion = this.connection.schemaVersion;
    if (schemaVersion != DB_SCHEMA) {
      
      
      
      if (schemaVersion != 0) {
        LOG("Migrating data from schema " + schemaVersion);
        migrateData = this.getMigrateDataFromDatabase();

        
        this.connection.close();
        try {
          if (dbfile.exists())
            dbfile.remove(true);

          
          this.connection = this.openDatabaseFile(dbfile);
        }
        catch (e) {
          ERROR("Failed to remove old database", e);
          
          
          this.connection = Services.storage.openSpecialDatabase("memory");
        }
      }
      else if (Prefs.getIntPref(PREF_DB_SCHEMA, 0) == 0) {
        
        LOG("Migrating data from extensions.rdf");
        migrateData = this.getMigrateDataFromRDF();
      }

      
      this.createSchema();

      if (aRebuildOnError) {
        let activeBundles = this.getActiveBundles();
        WARN("Rebuilding add-ons database from installed extensions.");
        this.beginTransaction();
        try {
          let state = XPIProvider.getInstallLocationStates();
          XPIProvider.processFileChanges(state, {}, false, undefined, undefined,
                                         migrateData, activeBundles)
          
          Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, true);
          this.commitTransaction();
        }
        catch (e) {
          ERROR("Error processing file changes", e);
          this.rollbackTransaction();
        }
      }
    }

    
    
    
    
    
    if (this.connection.databaseFile) {
      Services.prefs.setIntPref(PREF_DB_SCHEMA, DB_SCHEMA);
    }
    else {
      try {
        Services.prefs.clearUserPref(PREF_DB_SCHEMA);
      }
      catch (e) {
        
      }
    }
    Services.prefs.savePrefFile(null);

    
    for (let i = 0; i < this.transactionCount; i++)
      this.connection.executeSimpleSQL("SAVEPOINT 'default'");
    return migrateData;
  },

  


  get connection() {
    this.openConnection(true);
    return this.connection;
  },

  







  getActiveBundles: function XPIDB_getActiveBundles() {
    let bundles = [];

    let addonsList = FileUtils.getFile(KEY_PROFILEDIR, [FILE_XPI_ADDONS_LIST],
                                       true);

    let iniFactory = Cc["@mozilla.org/xpcom/ini-parser-factory;1"].
                     getService(Ci.nsIINIParserFactory);
    let parser = iniFactory.createINIParser(addonsList);

    let keys = parser.getKeys("ExtensionDirs");

    while (keys.hasMore())
      bundles.push(parser.getString("ExtensionDirs", keys.getNext()));

    
    for (let id in XPIProvider.bootstrappedAddons)
      bundles.push(XPIProvider.bootstrappedAddons[id].descriptor);

    return bundles;
  },

  





  getMigrateDataFromRDF: function XPIDB_getMigrateDataFromRDF(aDbWasMissing) {
    let migrateData = {};

    
    let rdffile = FileUtils.getFile(KEY_PROFILEDIR, [FILE_OLD_DATABASE], true);
    if (rdffile.exists()) {
      let ds = gRDF.GetDataSourceBlocking(Services.io.newFileURI(rdffile).spec);
      let root = Cc["@mozilla.org/rdf/container;1"].
                 createInstance(Ci.nsIRDFContainer);
      root.Init(ds, gRDF.GetResource(RDFURI_ITEM_ROOT));
      let elements = root.GetElements();
      while (elements.hasMoreElements()) {
        let source = elements.getNext().QueryInterface(Ci.nsIRDFResource);

        let location = getRDFProperty(ds, source, "installLocation");
        if (location) {
          if (!(location in migrateData))
            migrateData[location] = {};
          let id = source.ValueUTF8.substring(PREFIX_ITEM_URI.length);
          migrateData[location][id] = {
            version: getRDFProperty(ds, source, "version"),
            userDisabled: false,
            targetApplications: []
          }

          let disabled = getRDFProperty(ds, source, "userDisabled");
          if (disabled == "true" || disabled == "needs-disable")
            migrateData[location][id].userDisabled = true;

          let targetApps = ds.GetTargets(source, EM_R("targetApplication"),
                                         true);
          while (targetApps.hasMoreElements()) {
            let targetApp = targetApps.getNext()
                                      .QueryInterface(Ci.nsIRDFResource);
            let appInfo = {
              id: getRDFProperty(ds, targetApp, "id")
            };

            let minVersion = getRDFProperty(ds, targetApp, "updatedMinVersion");
            if (minVersion) {
              appInfo.minVersion = minVersion;
              appInfo.maxVersion = getRDFProperty(ds, targetApp, "updatedMaxVersion");
            }
            else {
              appInfo.minVersion = getRDFProperty(ds, targetApp, "minVersion");
              appInfo.maxVersion = getRDFProperty(ds, targetApp, "maxVersion");
            }
            migrateData[location][id].targetApplications.push(appInfo);
          }
        }
      }
    }

    return migrateData;
  },

  





  getMigrateDataFromDatabase: function XPIDB_getMigrateDataFromDatabase() {
    let migrateData = {};

    
    
    try {
      
      
      var sql = [];
      sql.push("SELECT internal_id, id, location, userDisabled, " +
               "softDisabled, installDate, version FROM addon");
      sql.push("SELECT internal_id, id, location, userDisabled, installDate, " +
               "version FROM addon");

      var stmt = null;
      if (!sql.some(function(aSql) {
        try {
          stmt = this.connection.createStatement(aSql);
          return true;
        }
        catch (e) {
          return false;
        }
      }, this)) {
        ERROR("Unable to read anything useful from the database");
        return migrateData;
      }

      for (let row in resultRows(stmt)) {
        if (!(row.location in migrateData))
          migrateData[row.location] = {};
        migrateData[row.location][row.id] = {
          internal_id: row.internal_id,
          version: row.version,
          installDate: row.installDate,
          userDisabled: row.userDisabled == 1,
          targetApplications: []
        };

        if ("softDisabled" in row)
          migrateData[row.location][row.id].softDisabled = row.softDisabled == 1;
      }

      var taStmt = this.connection.createStatement("SELECT id, minVersion, " +
                                                   "maxVersion FROM " +
                                                   "targetApplication WHERE " +
                                                   "addon_internal_id=:internal_id");

      for (let location in migrateData) {
        for (let id in migrateData[location]) {
          taStmt.params.internal_id = migrateData[location][id].internal_id;
          delete migrateData[location][id].internal_id;
          for (let row in resultRows(taStmt)) {
            migrateData[location][id].targetApplications.push({
              id: row.id,
              minVersion: row.minVersion,
              maxVersion: row.maxVersion
            });
          }
        }
      }
    }
    catch (e) {
      
      ERROR("Error migrating data", e);
    }
    finally {
      if (taStmt)
        taStmt.finalize();
      if (stmt)
        stmt.finalize();
    }

    return migrateData;
  },

  


  shutdown: function XPIDB_shutdown(aCallback) {
    if (this.initialized) {
      for each (let stmt in this.statementCache)
        stmt.finalize();
      this.statementCache = {};
      this.addonCache = [];

      if (this.transactionCount > 0) {
        ERROR(this.transactionCount + " outstanding transactions, rolling back.");
        while (this.transactionCount > 0)
          this.rollbackTransaction();
      }

      this.initialized = false;
      let connection = this.connection;
      delete this.connection;

      
      
      this.__defineGetter__("connection", function() {
        this.openConnection(true);
        return this.connection;
      });

      connection.asyncClose(aCallback);
    }
    else {
      if (aCallback)
        aCallback();
    }
  },

  










  getStatement: function XPIDB_getStatement(aKey, aSql) {
    if (aKey in this.statementCache)
      return this.statementCache[aKey];
    if (!aSql)
      aSql = this.statements[aKey];

    try {
      return this.statementCache[aKey] = this.connection.createStatement(aSql);
    }
    catch (e) {
      ERROR("Error creating statement " + aKey + " (" + aSql + ")");
      throw e;
    }
  },

  


  createSchema: function XPIDB_createSchema() {
    LOG("Creating database schema");
    this.beginTransaction();

    
    try {
      this.connection.createTable("addon",
                                  "internal_id INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                  "id TEXT, location TEXT, version TEXT, " +
                                  "type TEXT, internalName TEXT, updateURL TEXT, " +
                                  "updateKey TEXT, optionsURL TEXT, aboutURL TEXT, " +
                                  "iconURL TEXT, icon64URL TEXT, " +
                                  "defaultLocale INTEGER, " +
                                  "visible INTEGER, active INTEGER, " +
                                  "userDisabled INTEGER, appDisabled INTEGER, " +
                                  "pendingUninstall INTEGER, descriptor TEXT, " +
                                  "installDate INTEGER, updateDate INTEGER, " +
                                  "applyBackgroundUpdates INTEGER, " +
                                  "bootstrap INTEGER, skinnable INTEGER, " +
                                  "size INTEGER, sourceURI TEXT, " +
                                  "releaseNotesURI TEXT, softDisabled INTEGER, " +
                                  "UNIQUE (id, location)");
      this.connection.createTable("targetApplication",
                                  "addon_internal_id INTEGER, " +
                                  "id TEXT, minVersion TEXT, maxVersion TEXT, " +
                                  "UNIQUE (addon_internal_id, id)");
      this.connection.createTable("targetPlatform",
                                  "addon_internal_id INTEGER, " +
                                  "os, abi TEXT, " +
                                  "UNIQUE (addon_internal_id, os, abi)");
      this.connection.createTable("addon_locale",
                                  "addon_internal_id INTEGER, "+
                                  "locale TEXT, locale_id INTEGER, " +
                                  "UNIQUE (addon_internal_id, locale)");
      this.connection.createTable("locale",
                                  "id INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                  "name TEXT, description TEXT, creator TEXT, " +
                                  "homepageURL TEXT");
      this.connection.createTable("locale_strings",
                                  "locale_id INTEGER, type TEXT, value TEXT");
      this.connection.executeSimpleSQL("CREATE TRIGGER delete_addon AFTER DELETE " +
        "ON addon BEGIN " +
        "DELETE FROM targetApplication WHERE addon_internal_id=old.internal_id; " +
        "DELETE FROM targetPlatform WHERE addon_internal_id=old.internal_id; " +
        "DELETE FROM addon_locale WHERE addon_internal_id=old.internal_id; " +
        "DELETE FROM locale WHERE id=old.defaultLocale; " +
        "END");
      this.connection.executeSimpleSQL("CREATE TRIGGER delete_addon_locale AFTER " +
        "DELETE ON addon_locale WHEN NOT EXISTS " +
        "(SELECT * FROM addon_locale WHERE locale_id=old.locale_id) BEGIN " +
        "DELETE FROM locale WHERE id=old.locale_id; " +
        "END");
      this.connection.executeSimpleSQL("CREATE TRIGGER delete_locale AFTER " +
        "DELETE ON locale BEGIN " +
        "DELETE FROM locale_strings WHERE locale_id=old.id; " +
        "END");
      this.connection.schemaVersion = DB_SCHEMA;
      this.commitTransaction();
    }
    catch (e) {
      ERROR("Failed to create database schema", e);
      logSQLError(this.connection.lastError, this.connection.lastErrorString);
      this.rollbackTransaction();
      this.connection.close();
      this.connection = null;
      throw e;
    }
  },

  





  _readLocaleStrings: function XPIDB__readLocaleStrings(aLocale) {
    let stmt = this.getStatement("_readLocaleStrings");

    stmt.params.id = aLocale.id;
    for (let row in resultRows(stmt)) {
      if (!(row.type in aLocale))
        aLocale[row.type] = [];
      aLocale[row.type].push(row.value);
    }
  },

  






  _getLocales: function XPIDB__getLocales(aAddon) {
    let stmt = this.getStatement("_getLocales");

    let locales = [];
    stmt.params.internal_id = aAddon._internal_id;
    for (let row in resultRows(stmt)) {
      let locale = {
        id: row.id,
        locales: [row.locale]
      };
      copyProperties(row, PROP_LOCALE_SINGLE, locale);
      locales.push(locale);
    }
    locales.forEach(function(aLocale) {
      this._readLocaleStrings(aLocale);
    }, this);
    return locales;
  },

  







  _getDefaultLocale: function XPIDB__getDefaultLocale(aAddon) {
    let stmt = this.getStatement("_getDefaultLocale");

    stmt.params.id = aAddon._defaultLocale;
    if (!stepStatement(stmt))
      throw new Error("Missing default locale for " + aAddon.id);
    let locale = copyProperties(stmt.row, PROP_LOCALE_SINGLE);
    locale.id = aAddon._defaultLocale;
    stmt.reset();
    this._readLocaleStrings(locale);
    return locale;
  },

  






  _getTargetApplications: function XPIDB__getTargetApplications(aAddon) {
    let stmt = this.getStatement("_getTargetApplications");

    stmt.params.internal_id = aAddon._internal_id;
    return [copyProperties(row, PROP_TARGETAPP) for each (row in resultRows(stmt))];
  },

  






  _getTargetPlatforms: function XPIDB__getTargetPlatforms(aAddon) {
    let stmt = this.getStatement("_getTargetPlatforms");

    stmt.params.internal_id = aAddon._internal_id;
    return [copyProperties(row, ["os", "abi"]) for each (row in resultRows(stmt))];
  },

  







  makeAddonFromRow: function XPIDB_makeAddonFromRow(aRow) {
    if (this.addonCache[aRow.internal_id]) {
      let addon = this.addonCache[aRow.internal_id].get();
      if (addon)
        return addon;
    }

    let addon = new DBAddonInternal();
    addon._internal_id = aRow.internal_id;
    addon._installLocation = XPIProvider.installLocationsByName[aRow.location];
    addon._descriptor = aRow.descriptor;
    addon._defaultLocale = aRow.defaultLocale;
    copyProperties(aRow, PROP_METADATA, addon);
    copyProperties(aRow, DB_METADATA, addon);
    DB_BOOL_METADATA.forEach(function(aProp) {
      addon[aProp] = aRow[aProp] != 0;
    });
    try {
      addon._sourceBundle = addon._installLocation.getLocationForID(addon.id);
    }
    catch (e) {
      
      
      
    }

    this.addonCache[aRow.internal_id] = Components.utils.getWeakReference(addon);
    return addon;
  },

  







  fetchAddonMetadata: function XPIDB_fetchAddonMetadata(aAddon) {
    function readLocaleStrings(aLocale, aCallback) {
      let stmt = XPIDatabase.getStatement("_readLocaleStrings");

      stmt.params.id = aLocale.id;
      stmt.executeAsync({
        handleResult: function(aResults) {
          let row = null;
          while (row = aResults.getNextRow()) {
            let type = row.getResultByName("type");
            if (!(type in aLocale))
              aLocale[type] = [];
            aLocale[type].push(row.getResultByName("value"));
          }
        },

        handleError: asyncErrorLogger,

        handleCompletion: function(aReason) {
          aCallback();
        }
      });
    }

    function readDefaultLocale() {
      delete aAddon.defaultLocale;
      let stmt = XPIDatabase.getStatement("_getDefaultLocale");

      stmt.params.id = aAddon._defaultLocale;
      stmt.executeAsync({
        handleResult: function(aResults) {
          aAddon.defaultLocale = copyRowProperties(aResults.getNextRow(),
                                                   PROP_LOCALE_SINGLE);
          aAddon.defaultLocale.id = aAddon._defaultLocale;
        },

        handleError: asyncErrorLogger,

        handleCompletion: function(aReason) {
          if (aAddon.defaultLocale) {
            readLocaleStrings(aAddon.defaultLocale, readLocales);
          }
          else {
            ERROR("Missing default locale for " + aAddon.id);
            readLocales();
          }
        }
      });
    }

    function readLocales() {
      delete aAddon.locales;
      aAddon.locales = [];
      let stmt = XPIDatabase.getStatement("_getLocales");

      stmt.params.internal_id = aAddon._internal_id;
      stmt.executeAsync({
        handleResult: function(aResults) {
          let row = null;
          while (row = aResults.getNextRow()) {
            let locale = {
              id: row.getResultByName("id"),
              locales: [row.getResultByName("locale")]
            };
            copyRowProperties(row, PROP_LOCALE_SINGLE, locale);
            aAddon.locales.push(locale);
          }
        },

        handleError: asyncErrorLogger,

        handleCompletion: function(aReason) {
          let pos = 0;
          function readNextLocale() {
            if (pos < aAddon.locales.length)
              readLocaleStrings(aAddon.locales[pos++], readNextLocale);
            else
              readTargetApplications();
          }

          readNextLocale();
        }
      });
    }

    function readTargetApplications() {
      delete aAddon.targetApplications;
      aAddon.targetApplications = [];
      let stmt = XPIDatabase.getStatement("_getTargetApplications");

      stmt.params.internal_id = aAddon._internal_id;
      stmt.executeAsync({
        handleResult: function(aResults) {
          let row = null;
          while (row = aResults.getNextRow())
            aAddon.targetApplications.push(copyRowProperties(row, PROP_TARGETAPP));
        },

        handleError: asyncErrorLogger,

        handleCompletion: function(aReason) {
          readTargetPlatforms();
        }
      });
    }

    function readTargetPlatforms() {
      delete aAddon.targetPlatforms;
      aAddon.targetPlatforms = [];
      let stmt = XPIDatabase.getStatement("_getTargetPlatforms");

      stmt.params.internal_id = aAddon._internal_id;
      stmt.executeAsync({
        handleResult: function(aResults) {
          let row = null;
          while (row = aResults.getNextRow())
            aAddon.targetPlatforms.push(copyRowProperties(row, ["os", "abi"]));
        },

        handleError: asyncErrorLogger,

        handleCompletion: function(aReason) {
          let callbacks = aAddon._pendingCallbacks;
          delete aAddon._pendingCallbacks;
          callbacks.forEach(function(aCallback) {
            aCallback(aAddon);
          });
        }
      });
    }

    readDefaultLocale();
  },

  







  makeAddonFromRowAsync: function XPIDB_makeAddonFromRowAsync(aRow, aCallback) {
    let internal_id = aRow.getResultByName("internal_id");
    if (this.addonCache[internal_id]) {
      let addon = this.addonCache[internal_id].get();
      if (addon) {
        
        
        
        if ("_pendingCallbacks" in addon)
          addon._pendingCallbacks.push(aCallback);
        else
          aCallback(addon);
        return;
      }
    }

    let addon = new DBAddonInternal();
    addon._internal_id = internal_id;
    let location = aRow.getResultByName("location");
    addon._installLocation = XPIProvider.installLocationsByName[location];
    addon._descriptor = aRow.getResultByName("descriptor");
    copyRowProperties(aRow, PROP_METADATA, addon);
    addon._defaultLocale = aRow.getResultByName("defaultLocale");
    copyRowProperties(aRow, DB_METADATA, addon);
    DB_BOOL_METADATA.forEach(function(aProp) {
      addon[aProp] = aRow.getResultByName(aProp) != 0;
    });
    try {
      addon._sourceBundle = addon._installLocation.getLocationForID(addon.id);
    }
    catch (e) {
      
      
      
    }

    this.addonCache[internal_id] = Components.utils.getWeakReference(addon);
    addon._pendingCallbacks = [aCallback];
    this.fetchAddonMetadata(addon);
  },

  







  getInstallLocations: function XPIDB_getInstallLocations() {
    let stmt = this.getStatement("getInstallLocations");

    return [row.location for each (row in resultRows(stmt))];
  },

  






  getAddonsInLocation: function XPIDB_getAddonsInLocation(aLocation) {
    let stmt = this.getStatement("getAddonsInLocation");

    stmt.params.location = aLocation;
    return [this.makeAddonFromRow(row) for each (row in resultRows(stmt))];
  },

  










  getAddonInLocation: function XPIDB_getAddonInLocation(aId, aLocation, aCallback) {
    let stmt = this.getStatement("getAddonInLocation");

    stmt.params.id = aId;
    stmt.params.location = aLocation;
    stmt.executeAsync(new AsyncAddonListCallback(function(aAddons) {
      if (aAddons.length == 0) {
        aCallback(null);
        return;
      }
      
      
      if (aAddons.length > 1)
        ERROR("Multiple addons with ID " + aId + " found in location " + aLocation);
      aCallback(aAddons[0]);
    }));
  },

  







  getVisibleAddonForID: function XPIDB_getVisibleAddonForID(aId, aCallback) {
    let stmt = this.getStatement("getVisibleAddonForID");

    stmt.params.id = aId;
    stmt.executeAsync(new AsyncAddonListCallback(function(aAddons) {
      if (aAddons.length == 0) {
        aCallback(null);
        return;
      }
      
      
      if (aAddons.length > 1)
        ERROR("Multiple visible addons with ID " + aId + " found");
      aCallback(aAddons[0]);
    }));
  },

  







  getVisibleAddons: function XPIDB_getVisibleAddons(aTypes, aCallback) {
    let stmt = null;
    if (!aTypes || aTypes.length == 0) {
      stmt = this.getStatement("getVisibleAddons");
    }
    else {
      let sql = "SELECT * FROM addon WHERE visible=1 AND type IN (";
      for (let i = 1; i <= aTypes.length; i++) {
        sql += "?" + i;
        if (i < aTypes.length)
          sql += ",";
      }
      sql += ")";

      
      stmt = this.getStatement("getVisibleAddons_" + aTypes.length, sql);
      for (let i = 0; i < aTypes.length; i++)
        stmt.bindByIndex(i, aTypes[i]);
    }

    stmt.executeAsync(new AsyncAddonListCallback(aCallback));
  },

  






  getAddonsByType: function XPIDB_getAddonsByType(aType) {
    let stmt = this.getStatement("getAddonsByType");

    stmt.params.type = aType;
    return [this.makeAddonFromRow(row) for each (row in resultRows(stmt))];;
  },

  






  getVisibleAddonForInternalName: function XPIDB_getVisibleAddonForInternalName(aInternalName) {
    let stmt = this.getStatement("getVisibleAddoForInternalName");

    let addon = null;
    stmt.params.internalName = aInternalName;

    if (stepStatement(stmt))
      addon = this.makeAddonFromRow(stmt.row);

    stmt.reset();
    return addon;
  },

  







  getVisibleAddonsWithPendingOperations:
    function XPIDB_getVisibleAddonsWithPendingOperations(aTypes, aCallback) {
    let stmt = null;
    if (!aTypes || aTypes.length == 0) {
      stmt = this.getStatement("getVisibleAddonsWithPendingOperations");
    }
    else {
      let sql = "SELECT * FROM addon WHERE visible=1 AND " +
                "(pendingUninstall=1 OR MAX(userDisabled,appDisabled)=active) " +
                "AND type IN (";
      for (let i = 1; i <= aTypes.length; i++) {
        sql += "?" + i;
        if (i < aTypes.length)
          sql += ",";
      }
      sql += ")";

      
      stmt = this.getStatement("getVisibleAddonsWithPendingOperations_" +
                               aTypes.length, sql);
      for (let i = 0; i < aTypes.length; i++)
        stmt.bindByIndex(i, aTypes[i]);
    }

    stmt.executeAsync(new AsyncAddonListCallback(aCallback));
  },

  




  getAddons: function XPIDB_getAddons() {
    let stmt = this.getStatement("getAddons");

    return [this.makeAddonFromRow(row) for each (row in resultRows(stmt))];;
  },

  







  addAddonMetadata: function XPIDB_addAddonMetadata(aAddon, aDescriptor) {
    this.beginTransaction();

    
    try {
      let localestmt = this.getStatement("addAddonMetadata_locale");
      let stringstmt = this.getStatement("addAddonMetadata_strings");

      function insertLocale(aLocale) {
        copyProperties(aLocale, PROP_LOCALE_SINGLE, localestmt.params);
        executeStatement(localestmt);
        let row = XPIDatabase.connection.lastInsertRowID;

        PROP_LOCALE_MULTI.forEach(function(aProp) {
          aLocale[aProp].forEach(function(aStr) {
            stringstmt.params.locale = row;
            stringstmt.params.type = aProp;
            stringstmt.params.value = aStr;
            executeStatement(stringstmt);
          });
        });
        return row;
      }

      if (aAddon.visible) {
        let stmt = this.getStatement("clearVisibleAddons");
        stmt.params.id = aAddon.id;
        executeStatement(stmt);
      }

      let stmt = this.getStatement("addAddonMetadata_addon");

      stmt.params.locale = insertLocale(aAddon.defaultLocale);
      stmt.params.location = aAddon._installLocation.name;
      stmt.params.descriptor = aDescriptor;
      copyProperties(aAddon, PROP_METADATA, stmt.params);
      copyProperties(aAddon, DB_METADATA, stmt.params);
      DB_BOOL_METADATA.forEach(function(aProp) {
        stmt.params[aProp] = aAddon[aProp] ? 1 : 0;
      });
      executeStatement(stmt);
      let internal_id = this.connection.lastInsertRowID;

      stmt = this.getStatement("addAddonMetadata_addon_locale");
      aAddon.locales.forEach(function(aLocale) {
        let id = insertLocale(aLocale);
        aLocale.locales.forEach(function(aName) {
          stmt.params.internal_id = internal_id;
          stmt.params.name = aName;
          stmt.params.locale = insertLocale(aLocale);
          executeStatement(stmt);
        });
      });

      stmt = this.getStatement("addAddonMetadata_targetApplication");

      aAddon.targetApplications.forEach(function(aApp) {
        stmt.params.internal_id = internal_id;
        stmt.params.id = aApp.id;
        stmt.params.minVersion = aApp.minVersion;
        stmt.params.maxVersion = aApp.maxVersion;
        executeStatement(stmt);
      });

      stmt = this.getStatement("addAddonMetadata_targetPlatform");

      aAddon.targetPlatforms.forEach(function(aPlatform) {
        stmt.params.internal_id = internal_id;
        stmt.params.os = aPlatform.os;
        stmt.params.abi = aPlatform.abi;
        executeStatement(stmt);
      });

      this.commitTransaction();
    }
    catch (e) {
      this.rollbackTransaction();
      throw e;
    }
  },

  










  updateAddonMetadata: function XPIDB_updateAddonMetadata(aOldAddon, aNewAddon,
                                                          aDescriptor) {
    this.beginTransaction();

    
    try {
      this.removeAddonMetadata(aOldAddon);
      aNewAddon.installDate = aOldAddon.installDate;
      aNewAddon.applyBackgroundUpdates = aOldAddon.applyBackgroundUpdates;
      aNewAddon.active = (aNewAddon.visible && !aNewAddon.userDisabled &&
                          !aNewAddon.appDisabled)
      this.addAddonMetadata(aNewAddon, aDescriptor);
      this.commitTransaction();
    }
    catch (e) {
      this.rollbackTransaction();
      throw e;
    }
  },

  







  updateTargetApplications: function XPIDB_updateTargetApplications(aAddon,
                                                                    aTargets) {
    this.beginTransaction();

    
    try {
      let stmt = this.getStatement("updateTargetApplications");
      aTargets.forEach(function(aTarget) {
        stmt.params.internal_id = aAddon._internal_id;
        stmt.params.id = aTarget.id;
        stmt.params.minVersion = aTarget.minVersion;
        stmt.params.maxVersion = aTarget.maxVersion;
        executeStatement(stmt);
      });
      this.commitTransaction();
    }
    catch (e) {
      this.rollbackTransaction();
      throw e;
    }
  },

  





  removeAddonMetadata: function XPIDB_removeAddonMetadata(aAddon) {
    let stmt = this.getStatement("removeAddonMetadata");
    stmt.params.internal_id = aAddon._internal_id;
    executeStatement(stmt);
  },

  








  makeAddonVisible: function XPIDB_makeAddonVisible(aAddon) {
    let stmt = this.getStatement("clearVisibleAddons");
    stmt.params.id = aAddon.id;
    executeStatement(stmt);

    stmt = this.getStatement("makeAddonVisible");
    stmt.params.internal_id = aAddon._internal_id;
    executeStatement(stmt);

    aAddon.visible = true;
  },

  







  setAddonProperties: function XPIDB_setAddonProperties(aAddon, aProperties) {
    function convertBoolean(value) {
      return value ? 1 : 0;
    }

    let stmt = this.getStatement("setAddonProperties");
    stmt.params.internal_id = aAddon._internal_id;

    ["userDisabled", "appDisabled", "softDisabled",
     "pendingUninstall"].forEach(function(aProp) {
      if (aProp in aProperties) {
        stmt.params[aProp] = convertBoolean(aProperties[aProp]);
        aAddon[aProp] = aProperties[aProp];
      }
      else {
        stmt.params[aProp] = convertBoolean(aAddon[aProp]);
      }
    });

    if ("applyBackgroundUpdates" in aProperties) {
      stmt.params.applyBackgroundUpdates = aProperties.applyBackgroundUpdates;
      aAddon.applyBackgroundUpdates = aProperties.applyBackgroundUpdates;
    }
    else {
      stmt.params.applyBackgroundUpdates = aAddon.applyBackgroundUpdates;
    }

    executeStatement(stmt);
  },

  





  updateAddonActive: function XPIDB_updateAddonActive(aAddon) {
    LOG("Updating add-on state");

    let stmt = this.getStatement("updateAddonActive");
    stmt.params.internal_id = aAddon._internal_id;
    stmt.params.active = aAddon.active ? 1 : 0;
    executeStatement(stmt);
  },

  


  updateActiveAddons: function XPIDB_updateActiveAddons() {
    LOG("Updating add-on states");
    let stmt = this.getStatement("setActiveAddons");
    executeStatement(stmt);

    
    
    
    
    this.addonCache = [];
  },

  


  writeAddonsList: function XPIDB_writeAddonsList() {
    LOG("Writing add-ons list");
    Services.appinfo.invalidateCachesOnRestart();
    let addonsList = FileUtils.getFile(KEY_PROFILEDIR, [FILE_XPI_ADDONS_LIST],
                                       true);

    let enabledAddons = [];
    let text = "[ExtensionDirs]\r\n";
    let count = 0;

    let stmt = this.getStatement("getActiveAddons");

    for (let row in resultRows(stmt)) {
      text += "Extension" + (count++) + "=" + row.descriptor + "\r\n";
      enabledAddons.push(row.id + ":" + row.version);
    }

    
    
    text += "\r\n[ThemeDirs]\r\n";
    if (Prefs.getBoolPref(PREF_EM_DSS_ENABLED)) {
      stmt = this.getStatement("getThemes");
    }
    else {
      stmt = this.getStatement("getActiveTheme");
      stmt.params.internalName = XPIProvider.selectedSkin;
    }
    count = 0;
    for (let row in resultRows(stmt)) {
      text += "Extension" + (count++) + "=" + row.descriptor + "\r\n";
      enabledAddons.push(row.id + ":" + row.version);
    }

    var fos = FileUtils.openSafeFileOutputStream(addonsList);
    fos.write(text, text.length);
    FileUtils.closeSafeFileOutputStream(fos);

    Services.prefs.setCharPref(PREF_EM_ENABLED_ADDONS, enabledAddons.join(","));
  }
};

function getHashStringForCrypto(aCrypto) {
  
  function toHexString(charCode)
    ("0" + charCode.toString(16)).slice(-2);

  
  let binary = aCrypto.finish(false);
  return [toHexString(binary.charCodeAt(i)) for (i in binary)].join("").toLowerCase()
}































function AddonInstall(aCallback, aInstallLocation, aUrl, aHash, aName, aType,
                      aIconURL, aVersion, aReleaseNotesURI, aExistingAddon,
                      aLoadGroup) {
  this.wrapper = new AddonInstallWrapper(this);
  this.installLocation = aInstallLocation;
  this.sourceURI = aUrl;
  this.releaseNotesURI = aReleaseNotesURI;
  if (aHash) {
    let hashSplit = aHash.toLowerCase().split(":");
    this.originalHash = {
      algorithm: hashSplit[0],
      data: hashSplit[1]
    };
  }
  this.hash = this.originalHash;
  this.loadGroup = aLoadGroup;
  this.listeners = [];
  this.existingAddon = aExistingAddon;
  this.error = 0;
  if (aLoadGroup)
    this.window = aLoadGroup.notificationCallbacks
                            .getInterface(Ci.nsIDOMWindow);
  else
    this.window = null;

  if (aUrl instanceof Ci.nsIFileURL) {
    this.file = aUrl.file.QueryInterface(Ci.nsILocalFile);

    if (!this.file.exists()) {
      WARN("XPI file " + this.file.path + " does not exist");
      this.state = AddonManager.STATE_DOWNLOAD_FAILED;
      this.error = AddonManager.ERROR_NETWORK_FAILURE;
      aCallback(this);
      return;
    }

    this.state = AddonManager.STATE_DOWNLOADED;
    this.progress = this.file.fileSize;
    this.maxProgress = this.file.fileSize;

    if (this.hash) {
      let crypto = Cc["@mozilla.org/security/hash;1"].
                   createInstance(Ci.nsICryptoHash);
      try {
        crypto.initWithString(this.hash.algorithm);
      }
      catch (e) {
        WARN("Unknown hash algorithm " + this.hash.algorithm);
        this.state = AddonManager.STATE_DOWNLOAD_FAILED;
        this.error = AddonManager.ERROR_INCORRECT_HASH;
        aCallback(this);
        return;
      }

      let fis = Cc["@mozilla.org/network/file-input-stream;1"].
                createInstance(Ci.nsIFileInputStream);
      fis.init(this.file, -1, -1, false);
      crypto.updateFromStream(fis, this.file.fileSize);
      let calculatedHash = getHashStringForCrypto(crypto);
      if (calculatedHash != this.hash.data) {
        WARN("File hash (" + calculatedHash + ") did not match provided hash (" +
             this.hash.data + ")");
        this.state = AddonManager.STATE_DOWNLOAD_FAILED;
        this.error = AddonManager.ERROR_INCORRECT_HASH;
        aCallback(this);
        return;
      }
    }

    try {
      let self = this;
      this.loadManifest(function() {
        XPIDatabase.getVisibleAddonForID(self.addon.id, function(aAddon) {
          self.existingAddon = aAddon;
          if (aAddon)
            applyBlocklistChanges(aAddon, self.addon);
          self.addon.updateDate = Date.now();
          self.addon.installDate = aAddon ? aAddon.installDate : self.addon.updateDate;

          if (!self.addon.isCompatible) {
            
            self.state = AddonManager.STATE_CHECKING;
            new UpdateChecker(self.addon, {
              onUpdateFinished: function(aAddon) {
                self.state = AddonManager.STATE_DOWNLOADED;
                XPIProvider.installs.push(self);
                AddonManagerPrivate.callInstallListeners("onNewInstall",
                                                         self.listeners,
                                                         self.wrapper);

                aCallback(self);
              }
            }, AddonManager.UPDATE_WHEN_ADDON_INSTALLED);
          }
          else {
            XPIProvider.installs.push(self);
            AddonManagerPrivate.callInstallListeners("onNewInstall",
                                                     self.listeners,
                                                     self.wrapper);

            aCallback(self);
          }
        });
      });
    }
    catch (e) {
      WARN("Invalid XPI", e);
      this.state = AddonManager.STATE_DOWNLOAD_FAILED;
      this.error = AddonManager.ERROR_CORRUPT_FILE;
      aCallback(this);
      return;
    }
  }
  else {
    this.state = AddonManager.STATE_AVAILABLE;
    this.name = aName;
    this.type = aType;
    this.version = aVersion;
    this.iconURL = aIconURL;
    this.progress = 0;
    this.maxProgress = -1;

    XPIProvider.installs.push(this);
    AddonManagerPrivate.callInstallListeners("onNewInstall", this.listeners,
                                             this.wrapper);

    aCallback(this);
  }
}

AddonInstall.prototype = {
  installLocation: null,
  wrapper: null,
  stream: null,
  crypto: null,
  originalHash: null,
  hash: null,
  loadGroup: null,
  badCertHandler: null,
  listeners: null,
  restartDownload: false,

  name: null,
  type: null,
  version: null,
  iconURL: null,
  releaseNotesURI: null,
  sourceURI: null,
  file: null,
  ownsTempFile: false,
  certificate: null,
  certName: null,

  linkedInstalls: null,
  existingAddon: null,
  addon: null,

  state: null,
  error: null,
  progress: null,
  maxProgress: null,

  





  install: function AI_install() {
    switch (this.state) {
    case AddonManager.STATE_AVAILABLE:
      this.startDownload();
      break;
    case AddonManager.STATE_DOWNLOADED:
      this.startInstall();
      break;
    case AddonManager.STATE_DOWNLOAD_FAILED:
    case AddonManager.STATE_INSTALL_FAILED:
    case AddonManager.STATE_CANCELLED:
      this.removeTemporaryFile();
      this.state = AddonManager.STATE_AVAILABLE;
      this.error = 0;
      this.progress = 0;
      this.maxProgress = -1;
      this.hash = this.originalHash;
      XPIProvider.installs.push(this);
      this.startDownload();
      break;
    case AddonManager.STATE_DOWNLOADING:
    case AddonManager.STATE_CHECKING:
    case AddonManager.STATE_INSTALLING:
      
      return;
    default:
      throw new Error("Cannot start installing from this state");
    }
  },

  




  cancel: function AI_cancel() {
    switch (this.state) {
    case AddonManager.STATE_DOWNLOADING:
      if (this.channel)
        this.channel.cancel(Cr.NS_BINDING_ABORTED);
    case AddonManager.STATE_AVAILABLE:
    case AddonManager.STATE_DOWNLOADED:
      LOG("Cancelling download of " + this.sourceURI.spec);
      this.state = AddonManager.STATE_CANCELLED;
      XPIProvider.removeActiveInstall(this);
      AddonManagerPrivate.callInstallListeners("onDownloadCancelled",
                                               this.listeners, this.wrapper);
      this.removeTemporaryFile();
      break;
    case AddonManager.STATE_INSTALLED:
      LOG("Cancelling install of " + this.addon.id);
      let xpi = this.installLocation.getStagingDir();
      xpi.append(this.addon.id + ".xpi");
      flushJarCache(xpi);
      cleanStagingDir(this.installLocation.getStagingDir(),
                      [this.addon.id, this.addon.id + ".xpi",
                       this.addon.id + ".json"]);
      this.state = AddonManager.STATE_CANCELLED;
      XPIProvider.removeActiveInstall(this);

      if (this.existingAddon) {
        delete this.existingAddon.pendingUpgrade;
        this.existingAddon.pendingUpgrade = null;
      }

      AddonManagerPrivate.callAddonListeners("onOperationCancelled", createWrapper(this.addon));

      AddonManagerPrivate.callInstallListeners("onInstallCancelled",
                                               this.listeners, this.wrapper);
      break;
    default:
      throw new Error("Cannot cancel install of " + this.sourceURI.spec +
                      " from this state (" + this.state + ")");
    }
  },

  






  addListener: function AI_addListener(aListener) {
    if (!this.listeners.some(function(i) { return i == aListener; }))
      this.listeners.push(aListener);
  },

  





  removeListener: function AI_removeListener(aListener) {
    this.listeners = this.listeners.filter(function(i) {
      return i != aListener;
    });
  },

  


  removeTemporaryFile: function AI_removeTemporaryFile() {
    
    if (!this.ownsTempFile)
      return;

    try {
      this.file.remove(true);
      this.ownsTempFile = false;
    }
    catch (e) {
      WARN("Failed to remove temporary file " + this.file.path, e);
    }
  },

  



  updateAddonURIs: function AI_updateAddonURIs() {
    this.addon.sourceURI = this.sourceURI.spec;
    if (this.releaseNotesURI)
      this.addon.releaseNotesURI = this.releaseNotesURI.spec;
  },

  













  loadMultipackageManifests: function AI_loadMultipackageManifests(aZipReader,
                                                                   aCallback) {
    let files = [];
    let entries = aZipReader.findEntries("(*.[Xx][Pp][Ii]|*.[Jj][Aa][Rr])");
    while (entries.hasMore()) {
      let entryName = entries.getNext();
      var target = getTemporaryFile();
      try {
        aZipReader.extract(entryName, target);
        files.push(target);
      }
      catch (e) {
        WARN("Failed to extract " + entryName + " from multi-package " +
             "XPI", e);
        target.remove(false);
      }
    }

    aZipReader.close();

    if (files.length == 0) {
      throw new Error("Multi-package XPI does not contain any packages " +
                      "to install");
    }

    let addon = null;

    
    
    while (files.length > 0) {
      this.removeTemporaryFile();
      this.file = files.shift();
      this.ownsTempFile = true;
      try {
        addon = loadManifestFromZipFile(this.file);
        break;
      }
      catch (e) {
        WARN(this.file.leafName + " cannot be installed from multi-package " +
             "XPI", e);
      }
    }

    if (!addon) {
      
      aCallback();
      return;
    }

    this.addon = addon;

    this.updateAddonURIs();

    this.addon._install = this;
    this.name = this.addon.selectedLocale.name;
    this.type = this.addon.type;
    this.version = this.addon.version;

    
    
    
    
    

    
    if (files.length > 0) {
      this.linkedInstalls = [];
      let count = 0;
      let self = this;
      files.forEach(function(file) {
        AddonInstall.createInstall(function(aInstall) {
          
          if (aInstall.state == AddonManager.STATE_DOWNLOAD_FAILED) {
            
            file.remove(true);
          }
          else {
            
            aInstall.ownsTempFile = true;

            self.linkedInstalls.push(aInstall)

            aInstall.sourceURI = self.sourceURI;
            aInstall.releaseNotesURI = self.releaseNotesURI;
            aInstall.updateAddonURIs();
          }

          count++;
          if (count == files.length)
            aCallback();
        }, file);
      }, this);
    }
    else {
      aCallback();
    }
  },

  








  loadManifest: function AI_loadManifest(aCallback) {
    function addRepositoryData(aAddon) {
      
      AddonRepository.getCachedAddonByID(aAddon.id, function(aRepoAddon) {
        if (aRepoAddon) {
          aAddon._repositoryAddon = aRepoAddon;
          aCallback();
          return;
        }

        
        AddonRepository.cacheAddons([aAddon.id], function() {
          AddonRepository.getCachedAddonByID(aAddon.id, function(aRepoAddon) {
            aAddon._repositoryAddon = aRepoAddon;
            aCallback();
          });
        });
      });
    }

    let zipreader = Cc["@mozilla.org/libjar/zip-reader;1"].
                    createInstance(Ci.nsIZipReader);
    try {
      zipreader.open(this.file);
    }
    catch (e) {
      zipreader.close();
      throw e;
    }

    let principal = zipreader.getCertificatePrincipal(null);
    if (principal && principal.hasCertificate) {
      LOG("Verifying XPI signature");
      if (verifyZipSigning(zipreader, principal)) {
        let x509 = principal.certificate;
        if (x509 instanceof Ci.nsIX509Cert)
          this.certificate = x509;
        if (this.certificate && this.certificate.commonName.length > 0)
          this.certName = this.certificate.commonName;
        else
          this.certName = principal.prettyName;
      }
      else {
        zipreader.close();
        throw new Error("XPI is incorrectly signed");
      }
    }

    try {
      this.addon = loadManifestFromZipReader(zipreader);
    }
    catch (e) {
      zipreader.close();
      throw e;
    }

    if (this.addon.type == "multipackage") {
      let self = this;
      this.loadMultipackageManifests(zipreader, function() {
        addRepositoryData(self.addon);
      });
      return;
    }

    zipreader.close();

    this.updateAddonURIs();

    this.addon._install = this;
    this.name = this.addon.selectedLocale.name;
    this.type = this.addon.type;
    this.version = this.addon.version;

    
    
    
    
    

    addRepositoryData(this.addon);
  },

  observe: function AI_observe(aSubject, aTopic, aData) {
    
    this.cancel();
  },

  


  startDownload: function AI_startDownload() {
    this.state = AddonManager.STATE_DOWNLOADING;
    if (!AddonManagerPrivate.callInstallListeners("onDownloadStarted",
                                                  this.listeners, this.wrapper)) {
      this.state = AddonManager.STATE_CANCELLED;
      XPIProvider.removeActiveInstall(this);
      AddonManagerPrivate.callInstallListeners("onDownloadCancelled",
                                               this.listeners, this.wrapper)
      return;
    }

    
    if (this.state != AddonManager.STATE_DOWNLOADING)
      return;

    if (this.channel) {
      
      
      LOG("Waiting for previous download to complete");
      this.restartDownload = true;
      return;
    }

    this.openChannel();
  },

  openChannel: function AI_openChannel() {
    this.restartDownload = false;

    try {
      this.file = getTemporaryFile();
      this.ownsTempFile = true;
      this.stream = Cc["@mozilla.org/network/file-output-stream;1"].
                    createInstance(Ci.nsIFileOutputStream);
      this.stream.init(this.file, FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE |
                       FileUtils.MODE_TRUNCATE, FileUtils.PERMS_FILE, 0);
    }
    catch (e) {
      WARN("Failed to start download", e);
      this.state = AddonManager.STATE_DOWNLOAD_FAILED;
      this.error = AddonManager.ERROR_FILE_ACCESS;
      XPIProvider.removeActiveInstall(this);
      AddonManagerPrivate.callInstallListeners("onDownloadFailed",
                                               this.listeners, this.wrapper);
      return;
    }

    let listener = Cc["@mozilla.org/network/stream-listener-tee;1"].
                   createInstance(Ci.nsIStreamListenerTee);
    listener.init(this, this.stream);
    try {
      Components.utils.import("resource://gre/modules/CertUtils.jsm");
      let requireBuiltIn = Prefs.getBoolPref(PREF_INSTALL_REQUIREBUILTINCERTS, true);
      this.badCertHandler = new BadCertHandler(!requireBuiltIn);

      this.channel = NetUtil.newChannel(this.sourceURI);
      this.channel.notificationCallbacks = this;
      if (this.channel instanceof Ci.nsIHttpChannelInternal)
        this.channel.forceAllowThirdPartyCookie = true;
      this.channel.asyncOpen(listener, null);

      Services.obs.addObserver(this, "network:offline-about-to-go-offline", false);
    }
    catch (e) {
      WARN("Failed to start download", e);
      this.state = AddonManager.STATE_DOWNLOAD_FAILED;
      this.error = AddonManager.ERROR_NETWORK_FAILURE;
      XPIProvider.removeActiveInstall(this);
      AddonManagerPrivate.callInstallListeners("onDownloadFailed",
                                               this.listeners, this.wrapper);
    }
  },

  




  onDataAvailable: function AI_onDataAvailable(aRequest, aContext, aInputstream,
                                               aOffset, aCount) {
    this.crypto.updateFromStream(aInputstream, aCount);
    this.progress += aCount;
    if (!AddonManagerPrivate.callInstallListeners("onDownloadProgress",
                                                  this.listeners, this.wrapper)) {
      
    }
  },

  





  asyncOnChannelRedirect: function(aOldChannel, aNewChannel, aFlags, aCallback) {
    if (!this.hash && aOldChannel.originalURI.schemeIs("https") &&
        aOldChannel instanceof Ci.nsIHttpChannel) {
      try {
        let hashStr = aOldChannel.getResponseHeader("X-Target-Digest");
        let hashSplit = hashStr.toLowerCase().split(":");
        this.hash = {
          algorithm: hashSplit[0],
          data: hashSplit[1]
        };
      }
      catch (e) {
      }
    }

    
    
    if (!this.hash)
      this.badCertHandler.asyncOnChannelRedirect(aOldChannel, aNewChannel, aFlags, aCallback);
    else
      aCallback.onRedirectVerifyCallback(Cr.NS_OK);

    this.channel = aNewChannel;
  },

  




  onStartRequest: function AI_onStartRequest(aRequest, aContext) {
    this.crypto = Cc["@mozilla.org/security/hash;1"].
                  createInstance(Ci.nsICryptoHash);
    if (this.hash) {
      try {
        this.crypto.initWithString(this.hash.algorithm);
      }
      catch (e) {
        WARN("Unknown hash algorithm " + this.hash.algorithm);
        this.state = AddonManager.STATE_DOWNLOAD_FAILED;
        this.error = AddonManager.ERROR_INCORRECT_HASH;
        XPIProvider.removeActiveInstall(this);
        AddonManagerPrivate.callInstallListeners("onDownloadFailed",
                                                 this.listeners, this.wrapper);
        aRequest.cancel(Cr.NS_BINDING_ABORTED);
        return;
      }
    }
    else {
      
      
      this.crypto.initWithString("sha1");
    }

    this.progress = 0;
    if (aRequest instanceof Ci.nsIChannel) {
      try {
        this.maxProgress = aRequest.contentLength;
      }
      catch (e) {
      }
      LOG("Download started for " + this.sourceURI.spec + " to file " +
          this.file.path);
    }
  },

  




  onStopRequest: function AI_onStopRequest(aRequest, aContext, aStatus) {
    this.stream.close();
    this.channel = null;
    this.badCerthandler = null;
    Services.obs.removeObserver(this, "network:offline-about-to-go-offline");

    
    if (aStatus == Cr.NS_BINDING_ABORTED) {
      this.removeTemporaryFile();
      if (this.restartDownload)
        this.openChannel();
      return;
    }

    LOG("Download of " + this.sourceURI.spec + " completed.");

    if (Components.isSuccessCode(aStatus)) {
      if (!(aRequest instanceof Ci.nsIHttpChannel) || aRequest.requestSucceeded) {
        if (!this.hash && (aRequest instanceof Ci.nsIChannel)) {
          try {
            checkCert(aRequest,
                      !Prefs.getBoolPref(PREF_INSTALL_REQUIREBUILTINCERTS, true));
          }
          catch (e) {
            this.downloadFailed(AddonManager.ERROR_NETWORK_FAILURE, e);
            return;
          }
        }

        
        let calculatedHash = getHashStringForCrypto(this.crypto);
        this.crypto = null;
        if (this.hash && calculatedHash != this.hash.data) {
          this.downloadFailed(AddonManager.ERROR_INCORRECT_HASH,
                              "Downloaded file hash (" + calculatedHash +
                              ") did not match provided hash (" + this.hash.data + ")");
          return;
        }
        try {
          let self = this;
          this.loadManifest(function() {
            if (self.addon.isCompatible) {
              self.downloadCompleted();
            }
            else {
              
              self.state = AddonManager.STATE_CHECKING;
              new UpdateChecker(self.addon, {
                onUpdateFinished: function(aAddon) {
                  self.downloadCompleted();
                }
              }, AddonManager.UPDATE_WHEN_ADDON_INSTALLED);
            }
          });
        }
        catch (e) {
          this.downloadFailed(AddonManager.ERROR_CORRUPT_FILE, e);
        }
      }
      else {
        if (aRequest instanceof Ci.nsIHttpChannel)
          this.downloadFailed(AddonManager.ERROR_NETWORK_FAILURE,
                              aRequest.responseStatus + " " +
                              aRequest.responseStatusText);
        else
          this.downloadFailed(AddonManager.ERROR_NETWORK_FAILURE, aStatus);
      }
    }
    else {
      this.downloadFailed(AddonManager.ERROR_NETWORK_FAILURE, aStatus);
    }
  },

  







  downloadFailed: function(aReason, aError) {
    WARN("Download failed", aError);
    this.state = AddonManager.STATE_DOWNLOAD_FAILED;
    this.error = aReason;
    XPIProvider.removeActiveInstall(this);
    AddonManagerPrivate.callInstallListeners("onDownloadFailed", this.listeners,
                                             this.wrapper);

    
    
    if (this.state == AddonManager.STATE_DOWNLOAD_FAILED)
      this.removeTemporaryFile();
  },

  


  downloadCompleted: function() {
    let self = this;
    XPIDatabase.getVisibleAddonForID(this.addon.id, function(aAddon) {
      if (aAddon)
        self.existingAddon = aAddon;

      self.state = AddonManager.STATE_DOWNLOADED;
      self.addon.updateDate = Date.now();

      if (self.existingAddon) {
        self.addon.existingAddonID = self.existingAddon.id;
        self.addon.installDate = self.existingAddon.installDate;
        applyBlocklistChanges(self.existingAddon, self.addon);
      }
      else {
        self.addon.installDate = self.addon.updateDate;
      }

      if (AddonManagerPrivate.callInstallListeners("onDownloadEnded",
                                                   self.listeners,
                                                   self.wrapper)) {
        
        if (self.state != AddonManager.STATE_DOWNLOADED)
          return;

        self.install();

        if (self.linkedInstalls) {
          self.linkedInstalls.forEach(function(aInstall) {
            aInstall.install();
          });
        }
      }
    });
  },

  
  
  
  


  startInstall: function AI_startInstall() {
    this.state = AddonManager.STATE_INSTALLING;
    if (!AddonManagerPrivate.callInstallListeners("onInstallStarted",
                                                  this.listeners, this.wrapper)) {
      this.state = AddonManager.STATE_DOWNLOADED;
      XPIProvider.removeActiveInstall(this);
      AddonManagerPrivate.callInstallListeners("onInstallCancelled",
                                               this.listeners, this.wrapper)
      return;
    }

    
    
    XPIProvider.installs.forEach(function(aInstall) {
      if (aInstall.state == AddonManager.STATE_INSTALLED &&
          aInstall.installLocation == this.installLocation &&
          aInstall.addon.id == this.addon.id)
        aInstall.cancel();
    }, this);

    let isUpgrade = this.existingAddon &&
                    this.existingAddon._installLocation == this.installLocation;
    let requiresRestart = XPIProvider.installRequiresRestart(this.addon);

    LOG("Starting install of " + this.sourceURI.spec);
    AddonManagerPrivate.callAddonListeners("onInstalling",
                                           createWrapper(this.addon),
                                           requiresRestart);
    let stagedAddon = this.installLocation.getStagingDir();

    try {
      
      if (this.addon.unpack || Prefs.getBoolPref(PREF_XPI_UNPACK, false)) {
        LOG("Addon " + this.addon.id + " will be installed as " +
            "an unpacked directory");
        stagedAddon.append(this.addon.id);
        if (stagedAddon.exists())
          recursiveRemove(stagedAddon);
        stagedAddon.create(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
        extractFiles(this.file, stagedAddon);
      }
      else {
        LOG("Addon " + this.addon.id + " will be installed as " +
            "a packed xpi");
        stagedAddon.append(this.addon.id + ".xpi");
        if (stagedAddon.exists())
          stagedAddon.remove(true);
        this.file.copyTo(this.installLocation.getStagingDir(),
                         this.addon.id + ".xpi");
      }

      if (requiresRestart) {
        
        this.addon._sourceBundle = stagedAddon;

        
        let stagedJSON = stagedAddon.clone();
        stagedJSON.leafName = this.addon.id + ".json";
        if (stagedJSON.exists())
          stagedJSON.remove(true);
        let stream = Cc["@mozilla.org/network/file-output-stream;1"].
                     createInstance(Ci.nsIFileOutputStream);
        let converter = Cc["@mozilla.org/intl/converter-output-stream;1"].
                        createInstance(Ci.nsIConverterOutputStream);

        try {
          stream.init(stagedJSON, FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE |
                                  FileUtils.MODE_TRUNCATE, FileUtils.PERMS_FILE,
                                 0);
          converter.init(stream, "UTF-8", 0, 0x0000);
          converter.writeString(JSON.stringify(this.addon));
        }
        finally {
          converter.close();
          stream.close();
        }

        LOG("Install of " + this.sourceURI.spec + " completed.");
        this.state = AddonManager.STATE_INSTALLED;
        if (isUpgrade) {
          delete this.existingAddon.pendingUpgrade;
          this.existingAddon.pendingUpgrade = this.addon;
        }
        AddonManagerPrivate.callInstallListeners("onInstallEnded",
                                                 this.listeners, this.wrapper,
                                                 createWrapper(this.addon));
      }
      else {
        
        XPIProvider.removeActiveInstall(this);

        
        
        

        
        let reason = BOOTSTRAP_REASONS.ADDON_INSTALL;
        if (this.existingAddon) {
          if (Services.vc.compare(this.existingAddon.version, this.addon.version) < 0)
            reason = BOOTSTRAP_REASONS.ADDON_UPGRADE;
          else
            reason = BOOTSTRAP_REASONS.ADDON_DOWNGRADE;

          if (this.existingAddon.bootstrap) {
            let file = this.existingAddon._installLocation
                           .getLocationForID(this.existingAddon.id);
            if (this.existingAddon.active) {
              XPIProvider.callBootstrapMethod(this.existingAddon.id,
                                              this.existingAddon.version,
                                              file, "shutdown", reason);
            }
            XPIProvider.callBootstrapMethod(this.existingAddon.id,
                                            this.existingAddon.version,
                                            file, "uninstall", reason);
            XPIProvider.unloadBootstrapScope(this.existingAddon.id);
          }

          if (!isUpgrade && this.existingAddon.active) {
            this.existingAddon.active = false;
            XPIDatabase.updateAddonActive(this.existingAddon);
          }
        }

        
        let existingAddonID = this.existingAddon ? this.existingAddon.id : null;
        let file = this.installLocation.installAddon(this.addon.id, stagedAddon,
                                                     existingAddonID);
        cleanStagingDir(stagedAddon.parent, []);

        
        this.addon._sourceBundle = file;
        this.addon._installLocation = this.installLocation;
        this.addon.updateDate = recursiveLastModifiedTime(file);
        this.addon.visible = true;
        if (isUpgrade) {
          XPIDatabase.updateAddonMetadata(this.existingAddon, this.addon,
                                          file.persistentDescriptor);
        }
        else {
          this.addon.installDate = this.addon.updateDate;
          this.addon.active = (this.addon.visible && !isAddonDisabled(this.addon))
          XPIDatabase.addAddonMetadata(this.addon, file.persistentDescriptor);
        }

        
        let self = this;
        XPIDatabase.getAddonInLocation(this.addon.id, this.installLocation.name,
                                       function(a) {
          self.addon = a;
          if (self.addon.bootstrap) {
            XPIProvider.callBootstrapMethod(self.addon.id, self.addon.version,
                                            file, "install", reason);
            if (self.addon.active) {
              XPIProvider.callBootstrapMethod(self.addon.id, self.addon.version,
                                              file, "startup", reason);
            }
            else {
              XPIProvider.unloadBootstrapScope(self.addon.id);
            }
          }
          AddonManagerPrivate.callAddonListeners("onInstalled",
                                                 createWrapper(self.addon));

          LOG("Install of " + self.sourceURI.spec + " completed.");
          self.state = AddonManager.STATE_INSTALLED;
          AddonManagerPrivate.callInstallListeners("onInstallEnded",
                                                   self.listeners, self.wrapper,
                                                   createWrapper(self.addon));
        });
      }
    }
    catch (e) {
      WARN("Failed to install", e);
      if (stagedAddon.exists())
        recursiveRemove(stagedAddon);
      this.state = AddonManager.STATE_INSTALL_FAILED;
      this.error = AddonManager.ERROR_FILE_ACCESS;
      XPIProvider.removeActiveInstall(this);
      AddonManagerPrivate.callInstallListeners("onInstallFailed",
                                               this.listeners,
                                               this.wrapper);
    }
    finally {
      this.removeTemporaryFile();
    }
  },

  getInterface: function(iid) {
    if (iid.equals(Ci.nsIAuthPrompt2)) {
      var factory = Cc["@mozilla.org/prompter;1"].
                    getService(Ci.nsIPromptFactory);
      return factory.getPrompt(this.window, Ci.nsIAuthPrompt);
    }
    else if (iid.equals(Ci.nsIChannelEventSink)) {
      return this;
    }

    return this.badCertHandler.getInterface(iid);
  }
}










AddonInstall.createInstall = function(aCallback, aFile) {
  let location = XPIProvider.installLocationsByName[KEY_APP_PROFILE];
  let url = Services.io.newFileURI(aFile);

  try {
    new AddonInstall(aCallback, location, url);
  }
  catch(e) {
    ERROR("Error creating install", e);
    aCallback(null);
  }
};



















AddonInstall.createDownload = function(aCallback, aUri, aHash, aName, aIconURL,
                                       aVersion, aLoadGroup) {
  let location = XPIProvider.installLocationsByName[KEY_APP_PROFILE];
  let url = NetUtil.newURI(aUri);
  new AddonInstall(aCallback, location, url, aHash, aName, null,
                   aIconURL, aVersion, null, null, aLoadGroup);
};











AddonInstall.createUpdate = function(aCallback, aAddon, aUpdate) {
  let url = NetUtil.newURI(aUpdate.updateURL);
  let releaseNotesURI = null;
  try {
    if (aUpdate.updateInfoURL)
      releaseNotesURI = NetUtil.newURI(escapeAddonURI(aAddon, aUpdate.updateInfoURL));
  }
  catch (e) {
    
  }
  new AddonInstall(aCallback, aAddon._installLocation, url, aUpdate.updateHash,
                   aAddon.selectedLocale.name, aAddon.type,
                   aAddon.iconURL, aUpdate.version, releaseNotesURI, aAddon);
};







function AddonInstallWrapper(aInstall) {
  ["name", "type", "version", "iconURL", "releaseNotesURI", "file", "state", "error",
   "progress", "maxProgress", "certificate", "certName"].forEach(function(aProp) {
    this.__defineGetter__(aProp, function() aInstall[aProp]);
  }, this);

  this.__defineGetter__("existingAddon", function() {
    return createWrapper(aInstall.existingAddon);
  });
  this.__defineGetter__("addon", function() createWrapper(aInstall.addon));
  this.__defineGetter__("sourceURI", function() aInstall.sourceURI);

  this.__defineGetter__("linkedInstalls", function() {
    if (!aInstall.linkedInstalls)
      return null;
    return [i.wrapper for each (i in aInstall.linkedInstalls)];
  });

  this.install = function() {
    aInstall.install();
  }

  this.cancel = function() {
    aInstall.cancel();
  }

  this.addListener = function(listener) {
    aInstall.addListener(listener);
  }

  this.removeListener = function(listener) {
    aInstall.removeListener(listener);
  }
}

AddonInstallWrapper.prototype = {};
















function UpdateChecker(aAddon, aListener, aReason, aAppVersion, aPlatformVersion) {
  if (!aListener || !aReason)
    throw Cr.NS_ERROR_INVALID_ARG;

  Components.utils.import("resource://gre/modules/AddonUpdateChecker.jsm");

  this.addon = aAddon;
  this.listener = aListener;
  this.appVersion = aAppVersion;
  this.platformVersion = aPlatformVersion;
  this.syncCompatibility = (aReason == AddonManager.UPDATE_WHEN_NEW_APP_INSTALLED);

  let updateURL = aAddon.updateURL ? aAddon.updateURL :
                                     Services.prefs.getCharPref(PREF_EM_UPDATE_URL);

  const UPDATE_TYPE_COMPATIBILITY = 32;
  const UPDATE_TYPE_NEWVERSION = 64;

  aReason |= UPDATE_TYPE_COMPATIBILITY;
  if ("onUpdateAvailable" in this.listener)
    aReason |= UPDATE_TYPE_NEWVERSION;

  let url = escapeAddonURI(aAddon, updateURL, aReason, aAppVersion);
  AddonUpdateChecker.checkForUpdates(aAddon.id, aAddon.type, aAddon.updateKey,
                                     url, this);
}

UpdateChecker.prototype = {
  addon: null,
  listener: null,
  appVersion: null,
  platformVersion: null,
  syncCompatibility: null,

  






  callListener: function(aMethod) {
    if (!(aMethod in this.listener))
      return;

    let args = Array.slice(arguments, 1);
    try {
      this.listener[aMethod].apply(this.listener, args);
    }
    catch (e) {
      LOG("Exception calling UpdateListener method " + aMethod + ": " + e);
    }
  },

  





  onUpdateCheckComplete: function UC_onUpdateCheckComplete(aUpdates) {
    let AUC = AddonUpdateChecker;

    
    let compatUpdate = AUC.getCompatibilityUpdate(aUpdates, this.addon.version,
                                                  this.syncCompatibility);

    
    if (compatUpdate)
      this.addon.applyCompatibilityUpdate(compatUpdate, this.syncCompatibility);

    
    
    
    if ((this.appVersion &&
         Services.vc.compare(this.appVersion, Services.appinfo.version) != 0) ||
        (this.platformVersion &&
         Services.vc.compare(this.platformVersion, Services.appinfo.platformVersion) != 0)) {
      compatUpdate = AUC.getCompatibilityUpdate(aUpdates, this.addon.version,
                                                false, this.appVersion,
                                                this.platformVersion);
    }

    if (compatUpdate)
      this.callListener("onCompatibilityUpdateAvailable", createWrapper(this.addon));
    else
      this.callListener("onNoCompatibilityUpdateAvailable", createWrapper(this.addon));

    function sendUpdateAvailableMessages(aSelf, aInstall) {
      if (aInstall) {
        aSelf.callListener("onUpdateAvailable", createWrapper(aSelf.addon),
                           aInstall.wrapper);
      }
      else {
        aSelf.callListener("onNoUpdateAvailable", createWrapper(aSelf.addon));
      }
      aSelf.callListener("onUpdateFinished", createWrapper(aSelf.addon),
                         AddonManager.UPDATE_STATUS_NO_ERROR);
    }

    let update = AUC.getNewestCompatibleUpdate(aUpdates,
                                               this.appVersion,
                                               this.platformVersion);

    if (update && Services.vc.compare(this.addon.version, update.version) < 0) {
      for (let i = 0; i < XPIProvider.installs.length; i++) {
        
        if (XPIProvider.installs[i].existingAddon != this.addon ||
            XPIProvider.installs[i].version != update.version)
          continue;

        
        
        
        if (XPIProvider.installs[i].state == AddonManager.STATE_AVAILABLE)
          sendUpdateAvailableMessages(this, XPIProvider.installs[i]);
        else
          sendUpdateAvailableMessages(this, null);
        return;
      }

      let self = this;
      AddonInstall.createUpdate(function(aInstall) {
        sendUpdateAvailableMessages(self, aInstall);
      }, this.addon, update);
    }
    else {
      sendUpdateAvailableMessages(this, null);
    }
  },

  





  onUpdateCheckError: function UC_onUpdateCheckError(aError) {
    this.callListener("onNoCompatibilityUpdateAvailable", createWrapper(this.addon));
    this.callListener("onNoUpdateAvailable", createWrapper(this.addon));
    this.callListener("onUpdateFinished", createWrapper(this.addon), aError);
  }
};






function AddonInternal() {
}

AddonInternal.prototype = {
  _selectedLocale: null,
  active: false,
  visible: false,
  userDisabled: false,
  appDisabled: false,
  softDisabled: false,
  sourceURI: null,
  releaseNotesURI: null,

  get selectedLocale() {
    if (this._selectedLocale)
      return this._selectedLocale;
    let locale = findClosestLocale(this.locales);
    this._selectedLocale = locale ? locale : this.defaultLocale;
    return this._selectedLocale;
  },

  get providesUpdatesSecurely() {
    return !!(this.updateKey || !this.updateURL ||
              this.updateURL.substring(0, 6) == "https:");
  },

  get isCompatible() {
    return this.isCompatibleWith();
  },

  get isPlatformCompatible() {
    if (this.targetPlatforms.length == 0)
      return true;

    let matchedOS = false;

    
    
    let needsABI = false;

    
    let abi = null;
    try {
      abi = Services.appinfo.XPCOMABI;
    }
    catch (e) { }

    for (let i = 0; i < this.targetPlatforms.length; i++) {
      let platform = this.targetPlatforms[i];
      if (platform.os == Services.appinfo.OS) {
        if (platform.abi) {
          needsABI = true;
          if (platform.abi === abi)
            return true;
        }
        else {
          matchedOS = true;
        }
      }
    }

    return matchedOS && !needsABI;
  },

  isCompatibleWith: function(aAppVersion, aPlatformVersion) {
    let app = this.matchingTargetApplication;
    if (!app)
      return false;

    if (!aAppVersion)
      aAppVersion = Services.appinfo.version;
    if (!aPlatformVersion)
      aPlatformVersion = Services.appinfo.platformVersion;

    let version;
    if (app.id == Services.appinfo.ID)
      version = aAppVersion;
    else if (app.id == TOOLKIT_ID)
      version = aPlatformVersion

    return (Services.vc.compare(version, app.minVersion) >= 0) &&
           (Services.vc.compare(version, app.maxVersion) <= 0)
  },

  get matchingTargetApplication() {
    let app = null;
    for (let i = 0; i < this.targetApplications.length; i++) {
      if (this.targetApplications[i].id == Services.appinfo.ID)
        return this.targetApplications[i];
      if (this.targetApplications[i].id == TOOLKIT_ID)
        app = this.targetApplications[i];
    }
    return app;
  },

  get blocklistState() {
    let bs = Cc["@mozilla.org/extensions/blocklist;1"].
             getService(Ci.nsIBlocklistService);
    return bs.getAddonBlocklistState(this.id, this.version);
  },

  get blocklistURL() {
    let bs = Cc["@mozilla.org/extensions/blocklist;1"].
             getService(Ci.nsIBlocklistService);
    return bs.getAddonBlocklistURL(this.id, this.version);
  },

  applyCompatibilityUpdate: function(aUpdate, aSyncCompatibility) {
    this.targetApplications.forEach(function(aTargetApp) {
      aUpdate.targetApplications.forEach(function(aUpdateTarget) {
        if (aTargetApp.id == aUpdateTarget.id && (aSyncCompatibility ||
            Services.vc.compare(aTargetApp.maxVersion, aUpdateTarget.maxVersion) < 0)) {
          aTargetApp.minVersion = aUpdateTarget.minVersion;
          aTargetApp.maxVersion = aUpdateTarget.maxVersion;
        }
      });
    });
    this.appDisabled = !isUsableAddon(this);
  },

  












  toJSON: function(aKey) {
    let obj = {};
    for (let prop in this) {
      
      if (prop.substring(0, 1) == "_")
        continue;

      
      if (this.__lookupGetter__(prop))
        continue;

      
      if (this.__lookupSetter__(prop))
        continue;

      
      if (typeof this[prop] == "function")
        continue;

      obj[prop] = this[prop];
    }

    return obj;
  }
};







function DBAddonInternal() {
  this.__defineGetter__("targetApplications", function() {
    delete this.targetApplications;
    return this.targetApplications = XPIDatabase._getTargetApplications(this);
  });

  this.__defineGetter__("targetPlatforms", function() {
    delete this.targetPlatforms;
    return this.targetPlatforms = XPIDatabase._getTargetPlatforms(this);
  });

  this.__defineGetter__("locales", function() {
    delete this.locales;
    return this.locales = XPIDatabase._getLocales(this);
  });

  this.__defineGetter__("defaultLocale", function() {
    delete this.defaultLocale;
    return this.defaultLocale = XPIDatabase._getDefaultLocale(this);
  });

  this.__defineGetter__("pendingUpgrade", function() {
    delete this.pendingUpgrade;
    for (let i = 0; i < XPIProvider.installs.length; i++) {
      let install = XPIProvider.installs[i];
      if (install.state == AddonManager.STATE_INSTALLED &&
          !(install.addon instanceof DBAddonInternal) &&
          install.addon.id == this.id &&
          install.installLocation == this._installLocation) {
        return this.pendingUpgrade = install.addon;
      }
    };
  });
}

DBAddonInternal.prototype = {
  applyCompatibilityUpdate: function(aUpdate, aSyncCompatibility) {
    let changes = [];
    this.targetApplications.forEach(function(aTargetApp) {
      aUpdate.targetApplications.forEach(function(aUpdateTarget) {
        if (aTargetApp.id == aUpdateTarget.id && (aSyncCompatibility ||
            Services.vc.compare(aTargetApp.maxVersion, aUpdateTarget.maxVersion) < 0)) {
          aTargetApp.minVersion = aUpdateTarget.minVersion;
          aTargetApp.maxVersion = aUpdateTarget.maxVersion;
          changes.push(aUpdateTarget);
        }
      });
    });
    try {
      XPIDatabase.updateTargetApplications(this, changes);
    }
    catch (e) {
      
      ERROR("Failed to update target application info in the database for " +
            "add-on " + this.id, e);
      return;
    }
    XPIProvider.updateAddonDisabledState(this);
  }
}

DBAddonInternal.prototype.__proto__ = AddonInternal.prototype;








function createWrapper(aAddon) {
  if (!aAddon)
    return null;
  if (!aAddon._wrapper)
    aAddon._wrapper = new AddonWrapper(aAddon);
  return aAddon._wrapper;
}





function AddonWrapper(aAddon) {
  function chooseValue(aObj, aProp) {
    let repositoryAddon = aAddon._repositoryAddon;
    let objValue = aObj[aProp];

    if (repositoryAddon && (aProp in repositoryAddon) &&
        (objValue === undefined || objValue === null)) {
      return [repositoryAddon[aProp], true];
    }

    return [objValue, false];
  }

  ["id", "version", "type", "isCompatible", "isPlatformCompatible",
   "providesUpdatesSecurely", "blocklistState", "blocklistURL", "appDisabled",
   "softDisabled", "skinnable", "size"].forEach(function(aProp) {
     this.__defineGetter__(aProp, function() aAddon[aProp]);
  }, this);

  ["fullDescription", "developerComments", "eula", "supportURL",
   "contributionURL", "contributionAmount", "averageRating", "reviewCount",
   "reviewURL", "totalDownloads", "weeklyDownloads", "dailyUsers",
   "repositoryStatus"].forEach(function(aProp) {
    this.__defineGetter__(aProp, function() {
      if (aAddon._repositoryAddon)
        return aAddon._repositoryAddon[aProp];

      return null;
    });
  }, this);

  ["optionsURL", "aboutURL"].forEach(function(aProp) {
    this.__defineGetter__(aProp, function() {
      return this.isActive ? aAddon[aProp] : null;
    });
  }, this);

  ["installDate", "updateDate"].forEach(function(aProp) {
    this.__defineGetter__(aProp, function() new Date(aAddon[aProp]));
  }, this);

  ["sourceURI", "releaseNotesURI"].forEach(function(aProp) {
    this.__defineGetter__(aProp, function() {
      let target = chooseValue(aAddon, aProp)[0];
      if (!target)
        return null;
      return NetUtil.newURI(target);
    });
  }, this);

  
  
  ["icon", "icon64"].forEach(function(aProp) {
    this.__defineGetter__(aProp + "URL", function() {
      if (this.isActive && aAddon[aProp + "URL"])
        return aAddon[aProp + "URL"];

      if (this.hasResource(aProp + ".png"))
        return this.getResourceURI(aProp + ".png").spec;

      if (aAddon._repositoryAddon)
        return aAddon._repositoryAddon[aProp + "URL"];

      return null;
    }, this);
  }, this);

  PROP_LOCALE_SINGLE.forEach(function(aProp) {
    this.__defineGetter__(aProp, function() {
      
      if (aProp == "creator" &&
          aAddon._repositoryAddon && aAddon._repositoryAddon.creator) {
        return aAddon._repositoryAddon.creator;
      }

      let result = null;

      if (aAddon.active) {
        try {
          let pref = PREF_EM_EXTENSION_FORMAT + aAddon.id + "." + aProp;
          let value = Services.prefs.getComplexValue(pref,
                                                     Ci.nsIPrefLocalizedString);
          if (value.data)
            result = value.data;
        }
        catch (e) {
        }
      }

      if (result == null)
        [result, ] = chooseValue(aAddon.selectedLocale, aProp);

      if (aProp == "creator")
        return result ? new AddonManagerPrivate.AddonAuthor(result) : null;

      return result;
    });
  }, this);

  PROP_LOCALE_MULTI.forEach(function(aProp) {
    this.__defineGetter__(aProp, function() {
      let results = null;
      let usedRepository = false;

      if (aAddon.active) {
        let pref = PREF_EM_EXTENSION_FORMAT + aAddon.id + "." +
                   aProp.substring(0, aProp.length - 1);
        let list = Services.prefs.getChildList(pref, {});
        if (list.length > 0) {
          list.sort();
          results = [];
          list.forEach(function(aPref) {
            let value = Services.prefs.getComplexValue(aPref,
                                                       Ci.nsIPrefLocalizedString);
            if (value.data)
              results.push(value.data);
          });
        }
      }

      if (results == null)
        [results, usedRepository] = chooseValue(aAddon.selectedLocale, aProp);

      if (results && !usedRepository) {
        results = results.map(function(aResult) {
          return new AddonManagerPrivate.AddonAuthor(aResult);
        });
      }

      return results;
    });
  }, this);

  this.__defineGetter__("screenshots", function() {
    let repositoryAddon = aAddon._repositoryAddon;
    if (repositoryAddon && ("screenshots" in repositoryAddon)) {
      let repositoryScreenshots = repositoryAddon.screenshots;
      if (repositoryScreenshots && repositoryScreenshots.length > 0)
        return repositoryScreenshots;
    }

    if (aAddon.type == "theme" && this.hasResource("preview.png")) {
      let url = this.getResourceURI("preview.png").spec;
      return [new AddonManagerPrivate.AddonScreenshot(url)];
    }

    return null;
  });

  this.__defineGetter__("applyBackgroundUpdates", function() {
    return aAddon.applyBackgroundUpdates;
  });
  this.__defineSetter__("applyBackgroundUpdates", function(val) {
    if (val != AddonManager.AUTOUPDATE_DEFAULT &&
        val != AddonManager.AUTOUPDATE_DISABLE &&
        val != AddonManager.AUTOUPDATE_ENABLE) {
      val = val ? AddonManager.AUTOUPDATE_DEFAULT :
                  AddonManager.AUTOUPDATE_DISABLE;
    }

    if (val == aAddon.applyBackgroundUpdates)
      return val;

    XPIDatabase.setAddonProperties(aAddon, {
      applyBackgroundUpdates: val
    });
    AddonManagerPrivate.callAddonListeners("onPropertyChanged", this, ["applyBackgroundUpdates"]);

    return val;
  });

  this.__defineGetter__("install", function() {
    if (!("_install" in aAddon) || !aAddon._install)
      return null;
    return aAddon._install.wrapper;
  });

  this.__defineGetter__("pendingUpgrade", function() {
    return createWrapper(aAddon.pendingUpgrade);
  });

  this.__defineGetter__("scope", function() {
    if (aAddon._installLocation)
      return aAddon._installLocation.scope;

    return AddonManager.SCOPE_PROFILE;
  });

  this.__defineGetter__("pendingOperations", function() {
    let pending = 0;
    if (!(aAddon instanceof DBAddonInternal)) {
      
      
      
      if (!aAddon._install || aAddon._install.state == AddonManager.STATE_INSTALLING ||
          aAddon._install.state == AddonManager.STATE_INSTALLED)
        pending |= AddonManager.PENDING_INSTALL;
    }
    else if (aAddon.pendingUninstall) {
      pending |= AddonManager.PENDING_UNINSTALL;
    }

    if (aAddon.active && isAddonDisabled(aAddon))
      pending |= AddonManager.PENDING_DISABLE;
    else if (!aAddon.active && !isAddonDisabled(aAddon))
      pending |= AddonManager.PENDING_ENABLE;

    if (aAddon.pendingUpgrade)
      pending |= AddonManager.PENDING_UPGRADE;

    return pending;
  });

  this.__defineGetter__("operationsRequiringRestart", function() {
    let ops = 0;
    if (XPIProvider.installRequiresRestart(aAddon))
      ops |= AddonManager.OP_NEEDS_RESTART_INSTALL;
    if (XPIProvider.uninstallRequiresRestart(aAddon))
      ops |= AddonManager.OP_NEEDS_RESTART_UNINSTALL;
    if (XPIProvider.enableRequiresRestart(aAddon))
      ops |= AddonManager.OP_NEEDS_RESTART_ENABLE;
    if (XPIProvider.disableRequiresRestart(aAddon))
      ops |= AddonManager.OP_NEEDS_RESTART_DISABLE;

    return ops;
  });

  this.__defineGetter__("permissions", function() {
    let permissions = 0;

    
    if (!(aAddon instanceof DBAddonInternal))
      return permissions;

    if (!aAddon.appDisabled) {
      if (this.userDisabled)
        permissions |= AddonManager.PERM_CAN_ENABLE;
      else if (aAddon.type != "theme")
        permissions |= AddonManager.PERM_CAN_DISABLE;
    }

    
    
    if (!aAddon._installLocation.locked && !aAddon.pendingUninstall) {
      
      if (!aAddon._installLocation.isLinkedAddon(aAddon.id))
        permissions |= AddonManager.PERM_CAN_UPGRADE;

      permissions |= AddonManager.PERM_CAN_UNINSTALL;
    }
    return permissions;
  });

  this.__defineGetter__("isActive", function() {
    if (Services.appinfo.inSafeMode)
      return false;
    return aAddon.active;
  });

  this.__defineGetter__("userDisabled", function() {
    return aAddon.softDisabled || aAddon.userDisabled;
  });
  this.__defineSetter__("userDisabled", function(val) {
    if (val == this.userDisabled)
      return val;

    if (aAddon instanceof DBAddonInternal) {
      if (aAddon.type == "theme" && val) {
        if (aAddon.internalName == XPIProvider.defaultSkin)
          throw new Error("Cannot disable the default theme");
        XPIProvider.enableDefaultTheme();
      }
      else {
        XPIProvider.updateAddonDisabledState(aAddon, val);
      }
    }
    else {
      aAddon.userDisabled = val;
      
      if (!val)
        aAddon.softDisabled = false;
    }

    return val;
  });

  this.__defineSetter__("softDisabled", function(val) {
    if (val == aAddon.softDisabled)
      return val;

    if (aAddon instanceof DBAddonInternal) {
      
      if (aAddon.type == "theme" && val && !aAddon.userDisabled) {
        if (aAddon.internalName == XPIProvider.defaultSkin)
          throw new Error("Cannot disable the default theme");
        XPIProvider.enableDefaultTheme();
      }
      else {
        XPIProvider.updateAddonDisabledState(aAddon, undefined, val);
      }
    }
    else {
      
      if (!aAddon.userDisabled)
        aAddon.softDisabled = val;
    }

    return val;
  });

  this.isCompatibleWith = function(aAppVersion, aPlatformVersion) {
    return aAddon.isCompatibleWith(aAppVersion, aPlatformVersion);
  };

  this.uninstall = function() {
    if (!(aAddon instanceof DBAddonInternal))
      throw new Error("Cannot uninstall an add-on that isn't installed");
    if (aAddon.pendingUninstall)
      throw new Error("Add-on is already marked to be uninstalled");
    XPIProvider.uninstallAddon(aAddon);
  };

  this.cancelUninstall = function() {
    if (!(aAddon instanceof DBAddonInternal))
      throw new Error("Cannot cancel uninstall for an add-on that isn't installed");
    if (!aAddon.pendingUninstall)
      throw new Error("Add-on is not marked to be uninstalled");
    XPIProvider.cancelUninstallAddon(aAddon);
  };

  this.findUpdates = function(aListener, aReason, aAppVersion, aPlatformVersion) {
    new UpdateChecker(aAddon, aListener, aReason, aAppVersion, aPlatformVersion);
  };

  this.hasResource = function(aPath) {
    let bundle = aAddon._sourceBundle.clone();

    if (bundle.isDirectory()) {
      if (aPath) {
        aPath.split("/").forEach(function(aPart) {
          bundle.append(aPart);
        });
      }
      return bundle.exists();
    }

    let zipReader = Cc["@mozilla.org/libjar/zip-reader;1"].
                    createInstance(Ci.nsIZipReader);
    zipReader.open(bundle);
    let result = zipReader.hasEntry(aPath);
    zipReader.close();
    return result;
  },

  this.getResourceURI = function(aPath) {
    let bundle = aAddon._sourceBundle.clone();

    if (bundle.isDirectory()) {
      if (aPath) {
        aPath.split("/").forEach(function(aPart) {
          bundle.append(aPart);
        });
      }
      return Services.io.newFileURI(bundle);
    }

    if (!aPath)
      return Services.io.newFileURI(bundle);
    return buildJarURI(bundle, aPath);
  }
}



























function DirectoryInstallLocation(aName, aDirectory, aScope, aLocked) {
  this._name = aName;
  this.locked = aLocked;
  this._directory = aDirectory;
  this._scope = aScope
  this._IDToFileMap = {};
  this._FileToIDMap = {};
  this._linkedAddons = [];

  if (!aDirectory.exists())
    return;
  if (!aDirectory.isDirectory())
    throw new Error("Location must be a directory.");

  this._readAddons();
}

DirectoryInstallLocation.prototype = {
  _name       : "",
  _directory   : null,
  _IDToFileMap : null,  
  _FileToIDMap : null,  

  






  _readDirectoryFromFile: function DirInstallLocation__readDirectoryFromFile(aFile) {
    let fis = Cc["@mozilla.org/network/file-input-stream;1"].
              createInstance(Ci.nsIFileInputStream);
    fis.init(aFile, -1, -1, false);
    let line = { value: "" };
    if (fis instanceof Ci.nsILineInputStream)
      fis.readLine(line);
    fis.close();
    if (line.value) {
      let linkedDirectory = Cc["@mozilla.org/file/local;1"].
                            createInstance(Ci.nsILocalFile);

      try {
        linkedDirectory.initWithPath(line.value);
      }
      catch (e) {
        linkedDirectory.setRelativeDescriptor(aFile.parent, line.value);
      }

      if (!linkedDirectory.exists()) {
        WARN("File pointer " + aFile.path + " points to " + linkedDirectory.path +
             " which does not exist");
        return null;
      }

      if (!linkedDirectory.isDirectory()) {
        WARN("File pointer " + aFile.path + " points to " + linkedDirectory.path +
             " which is not a directory");
        return null;
      }

      return linkedDirectory;
    }

    WARN("File pointer " + aFile.path + " does not contain a path");
    return null;
  },

  


  _readAddons: function DirInstallLocation__readAddons() {
    let entries = this._directory.directoryEntries
                                 .QueryInterface(Ci.nsIDirectoryEnumerator);
    let entry;
    while (entry = entries.nextFile) {
      
      if (!(entry instanceof Ci.nsILocalFile))
        continue;

      let id = entry.leafName;

      if (id == DIR_STAGE || id == DIR_XPI_STAGE || id == DIR_TRASH)
        continue;

      let directLoad = false;
      if (entry.isFile() &&
          id.substring(id.length - 4).toLowerCase() == ".xpi") {
        directLoad = true;
        id = id.substring(0, id.length - 4);
      }

      if (!gIDTest.test(id)) {
        LOG("Ignoring file entry whose name is not a valid add-on ID: " +
             entry.path);
        continue;
      }

      if (entry.isFile() && !directLoad) {
        newEntry = this._readDirectoryFromFile(entry);
        if (!newEntry) {
          LOG("Deleting stale pointer file " + entry.path);
          entry.remove(true);
          continue;
        }

        entry = newEntry;
        this._linkedAddons.push(id);
      }

      this._IDToFileMap[id] = entry;
      this._FileToIDMap[entry.path] = id;
    }
    entries.close();
  },

  


  get name() {
    return this._name;
  },

  


  get scope() {
    return this._scope;
  },

  


  get addonLocations() {
    let locations = [];
    for (let id in this._IDToFileMap) {
      locations.push(this._IDToFileMap[id].clone()
                         .QueryInterface(Ci.nsILocalFile));
    }
    return locations;
  },

  





  getStagingDir: function DirInstallLocation_getStagingDir() {
    let dir = this._directory.clone();
    dir.append(DIR_STAGE);
    return dir;
  },

  





  getXPIStagingDir: function DirInstallLocation_getXPIStagingDir() {
    let dir = this._directory.clone();
    dir.append(DIR_XPI_STAGE);
    return dir;
  },

  







  getTrashDir: function DirInstallLocation_getTrashDir() {
    let trashDir = this._directory.clone();
    trashDir.append(DIR_TRASH);
    if (trashDir.exists())
      recursiveRemove(trashDir);
    trashDir.create(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
    return trashDir;
  },

  













  installAddon: function DirInstallLocation_installAddon(aId, aSource,
                                                         aExistingAddonID,
                                                         aCopy) {
    let trashDir = this.getTrashDir();

    let transaction = new SafeInstallOperation();

    let self = this;
    function moveOldAddon(aId) {
      let file = self._directory.clone().QueryInterface(Ci.nsILocalFile);
      file.append(aId);

      if (file.exists())
        transaction.move(file, trashDir);

      file = self._directory.clone().QueryInterface(Ci.nsILocalFile);
      file.append(aId + ".xpi");
      if (file.exists()) {
        flushJarCache(file);
        transaction.move(file, trashDir);
      }
    }

    
    
    try {
      moveOldAddon(aId);
      if (aExistingAddonID && aExistingAddonID != aId)
        moveOldAddon(aExistingAddonID);

      if (aCopy) {
        transaction.copy(aSource, this._directory);
      }
      else {
        if (aSource.isFile())
          flushJarCache(aSource);

        transaction.move(aSource, this._directory);
      }
    }
    finally {
      
      
      try {
        recursiveRemove(trashDir);
      }
      catch (e) {
        WARN("Failed to remove trash directory when installing " + aId, e);
      }
    }

    let newFile = this._directory.clone().QueryInterface(Ci.nsILocalFile);
    newFile.append(aSource.leafName);
    newFile.lastModifiedTime = Date.now();
    this._FileToIDMap[newFile.path] = aId;
    this._IDToFileMap[aId] = newFile;

    if (aExistingAddonID && aExistingAddonID != aId &&
        aExistingAddonID in this._IDToFileMap) {
      delete this._FileToIDMap[this._IDToFileMap[aExistingAddonID]];
      delete this._IDToFileMap[aExistingAddonID];
    }

    return newFile;
  },

  






  uninstallAddon: function DirInstallLocation_uninstallAddon(aId) {
    let file = this._IDToFileMap[aId];
    if (!file) {
      WARN("Attempted to remove " + aId + " from " +
           this._name + " but it was already gone");
      return;
    }

    file = this._directory.clone();
    file.append(aId);
    if (!file.exists())
      file.leafName += ".xpi";

    if (!file.exists()) {
      WARN("Attempted to remove " + aId + " from " +
           this._name + " but it was already gone");

      delete this._FileToIDMap[file.path];
      delete this._IDToFileMap[aId];
      return;
    }

    let trashDir = this.getTrashDir();

    if (file.leafName != aId)
      flushJarCache(file);

    let transaction = new SafeInstallOperation();

    try {
      transaction.move(file, trashDir);
    }
    finally {
      
      
      try {
        recursiveRemove(trashDir);
      }
      catch (e) {
        WARN("Failed to remove trash directory when uninstalling " + aId, e);
      }
    }

    delete this._FileToIDMap[file.path];
    delete this._IDToFileMap[aId];
  },

  







  getIDForLocation: function DirInstallLocation_getIDForLocation(aFile) {
    if (aFile.path in this._FileToIDMap)
      return this._FileToIDMap[aFile.path];
    throw new Error("Unknown add-on location " + aFile.path);
  },

  







  getLocationForID: function DirInstallLocation_getLocationForID(aId) {
    if (aId in this._IDToFileMap)
      return this._IDToFileMap[aId].clone().QueryInterface(Ci.nsILocalFile);
    throw new Error("Unknown add-on ID " + aId);
  },

  






  isLinkedAddon: function(aId) {
    return this._linkedAddons.indexOf(aId) != -1;
  }
};

#ifdef XP_WIN












function WinRegInstallLocation(aName, aRootKey, aScope) {
  this.locked = true;
  this._name = aName;
  this._rootKey = aRootKey;
  this._scope = aScope;
  this._IDToFileMap = {};
  this._FileToIDMap = {};

  let path = this._appKeyPath + "\\Extensions";
  let key = Cc["@mozilla.org/windows-registry-key;1"].
            createInstance(Ci.nsIWindowsRegKey);

  
  
  try {
    key.open(this._rootKey, path, Ci.nsIWindowsRegKey.ACCESS_READ);
  }
  catch (e) {
    return;
  }

  this._readAddons(key);
  key.close();
}

WinRegInstallLocation.prototype = {
  _name       : "",
  _rootKey    : null,
  _scope      : null,
  _IDToFileMap : null,  
  _FileToIDMap : null,  

  


  get _appKeyPath() {
    let appVendor = Services.appinfo.vendor;
    let appName = Services.appinfo.name;

#ifdef MOZ_THUNDERBIRD
    
    if (appVendor == "")
      appVendor = "Mozilla";
#endif

    
    if (appVendor != "")
      appVendor += "\\";

    return "SOFTWARE\\" + appVendor + appName;
  },

  






  _readAddons: function RegInstallLocation__readAddons(aKey) {
    let count = aKey.valueCount;
    for (let i = 0; i < count; ++i) {
      let id = aKey.getValueName(i);

      let file = Cc["@mozilla.org/file/local;1"].
                createInstance(Ci.nsILocalFile);
      file.initWithPath(aKey.readStringValue(id));

      if (!file.exists()) {
        WARN("Ignoring missing add-on in " + file.path);
        continue;
      }

      this._IDToFileMap[id] = file;
      this._FileToIDMap[file.path] = id;
    }
  },

  


  get name() {
    return this._name;
  },

  


  get scope() {
    return this._scope;
  },

  


  get addonLocations() {
    let locations = [];
    for (let id in this._IDToFileMap) {
      locations.push(this._IDToFileMap[id].clone()
                         .QueryInterface(Ci.nsILocalFile));
    }
    return locations;
  },

  







  getIDForLocation: function RegInstallLocation_getIDForLocation(aFile) {
    if (aFile.path in this._FileToIDMap)
      return this._FileToIDMap[aFile.path];
    throw new Error("Unknown add-on location");
  },

  






  getLocationForID: function RegInstallLocation_getLocationForID(aId) {
    if (aId in this._IDToFileMap)
      return this._IDToFileMap[aId].clone().QueryInterface(Ci.nsILocalFile);
    throw new Error("Unknown add-on ID");
  },

  


  isLinkedAddon: function(aId) {
    return true;
  }
};
#endif

AddonManagerPrivate.registerProvider(XPIProvider, [
  new AddonManagerPrivate.AddonType("extension", URI_EXTENSION_STRINGS,
                                    STRING_TYPE_NAME,
                                    AddonManager.VIEW_TYPE_LIST, 4000),
  new AddonManagerPrivate.AddonType("theme", URI_EXTENSION_STRINGS,
                                    STRING_TYPE_NAME,
                                    AddonManager.VIEW_TYPE_LIST, 5000),
  new AddonManagerPrivate.AddonType("locale", URI_EXTENSION_STRINGS,
                                    STRING_TYPE_NAME,
                                    AddonManager.VIEW_TYPE_LIST, 2000,
                                    AddonManager.TYPE_UI_HIDE_EMPTY)
]);
