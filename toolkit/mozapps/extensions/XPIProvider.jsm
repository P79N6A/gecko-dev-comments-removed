



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

this.EXPORTED_SYMBOLS = [];

Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/AddonManager.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AddonRepository",
                                  "resource://gre/modules/AddonRepository.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ChromeManifestParser",
                                  "resource://gre/modules/ChromeManifestParser.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "LightweightThemeManager",
                                  "resource://gre/modules/LightweightThemeManager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");

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
const PREF_EM_UPDATE_URL              = "extensions.update.url";
const PREF_EM_UPDATE_BACKGROUND_URL   = "extensions.update.background.url";
const PREF_EM_ENABLED_ADDONS          = "extensions.enabledAddons";
const PREF_EM_EXTENSION_FORMAT        = "extensions.";
const PREF_EM_ENABLED_SCOPES          = "extensions.enabledScopes";
const PREF_EM_AUTO_DISABLED_SCOPES    = "extensions.autoDisableScopes";
const PREF_EM_SHOW_MISMATCH_UI        = "extensions.showMismatchUI";
const PREF_XPI_ENABLED                = "xpinstall.enabled";
const PREF_XPI_WHITELIST_REQUIRED     = "xpinstall.whitelist.required";
const PREF_XPI_WHITELIST_PERMISSIONS  = "xpinstall.whitelist.add";
const PREF_XPI_BLACKLIST_PERMISSIONS  = "xpinstall.blacklist.add";
const PREF_XPI_UNPACK                 = "extensions.alwaysUnpack";
const PREF_INSTALL_REQUIREBUILTINCERTS = "extensions.install.requireBuiltInCerts";
const PREF_INSTALL_DISTRO_ADDONS      = "extensions.installDistroAddons";
const PREF_BRANCH_INSTALLED_ADDON     = "extensions.installedDistroAddon.";
const PREF_SHOWN_SELECTION_UI         = "extensions.shownSelectionUI";

const PREF_EM_MIN_COMPAT_APP_VERSION      = "extensions.minCompatibleAppVersion";
const PREF_EM_MIN_COMPAT_PLATFORM_VERSION = "extensions.minCompatiblePlatformVersion";

const URI_EXTENSION_SELECT_DIALOG     = "chrome://mozapps/content/extensions/selectAddons.xul";
const URI_EXTENSION_UPDATE_DIALOG     = "chrome://mozapps/content/extensions/update.xul";
const URI_EXTENSION_STRINGS           = "chrome://mozapps/locale/extensions/extensions.properties";

const STRING_TYPE_NAME                = "type.%ID%.name";

const DIR_EXTENSIONS                  = "extensions";
const DIR_STAGE                       = "staged";
const DIR_XPI_STAGE                   = "staged-xpis";
const DIR_TRASH                       = "trash";

const FILE_DATABASE                   = "extensions.sqlite";
const FILE_OLD_CACHE                  = "extensions.cache";
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

const UNKNOWN_XPCOM_ABI               = "unknownABI";
const XPI_PERMISSION                  = "install";

const PREFIX_ITEM_URI                 = "urn:mozilla:item:";
const RDFURI_ITEM_ROOT                = "urn:mozilla:item:root"
const RDFURI_INSTALL_MANIFEST_ROOT    = "urn:mozilla:install-manifest";
const PREFIX_NS_EM                    = "http://www.mozilla.org/2004/em-rdf#";

const TOOLKIT_ID                      = "toolkit@mozilla.org";


#expand const DB_SCHEMA                       = __MOZ_EXTENSIONS_DB_SCHEMA__;


const PROP_METADATA      = ["id", "version", "type", "internalName", "updateURL",
                            "updateKey", "optionsURL", "optionsType", "aboutURL",
                            "iconURL", "icon64URL"];
const PROP_LOCALE_SINGLE = ["name", "description", "creator", "homepageURL"];
const PROP_LOCALE_MULTI  = ["developers", "translators", "contributors"];
const PROP_TARGETAPP     = ["id", "minVersion", "maxVersion"];




const DB_MIGRATE_METADATA= ["installDate", "userDisabled", "softDisabled",
                            "sourceURI", "applyBackgroundUpdates",
                            "releaseNotesURI", "isForeignInstall", "syncGUID"];



const STATIC_BLOCKLIST_PATTERNS = [
  { creator: "Mozilla Corp.",
    level: Ci.nsIBlocklistService.STATE_BLOCKED,
    blockID: "i162" },
  { creator: "Mozilla.org",
    level: Ci.nsIBlocklistService.STATE_BLOCKED,
    blockID: "i162" }
];


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
  multipackage: 32,
  dictionary: 64
};

const COMPATIBLE_BY_DEFAULT_TYPES = {
  extension: true,
  dictionary: true
};

const MSG_JAR_FLUSH = "AddonJarFlush";

var gGlobalScope = this;




var gIDTest = /^(\{[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\}|[a-z0-9-\._]*\@[a-z0-9-\._]+)$/i;

["LOG", "WARN", "ERROR"].forEach(function(aName) {
  this.__defineGetter__(aName, function logFuncGetter() {
    Components.utils.import("resource://gre/modules/AddonLogging.jsm");

    LogManager.getLogger("addons.xpi", this);
    return this[aName];
  })
}, this);


const LAZY_OBJECTS = ["XPIDatabase"];

var gLazyObjectsLoaded = false;

function loadLazyObjects() {
  let scope = {};
  Services.scriptloader.loadSubScript("resource://gre/modules/XPIProviderUtils.js",
                                      scope);
  scope.XPIProvider = XPIProvider;

  for (let name of LAZY_OBJECTS) {
    delete gGlobalScope[name];
    gGlobalScope[name] = scope[name];
  }
  gLazyObjectsLoaded = true;
  return scope;
}

for (let name of LAZY_OBJECTS) {
  gGlobalScope.__defineGetter__(name, function lazyObjectGetter() {
    let objs = loadLazyObjects();
    return objs[name];
  });
}


function findMatchingStaticBlocklistItem(aAddon) {
  for (let item of STATIC_BLOCKLIST_PATTERNS) {
    if ("creator" in item && typeof item.creator == "string") {
      if ((aAddon.defaultLocale && aAddon.defaultLocale.creator == item.creator) ||
          (aAddon.selectedLocale && aAddon.selectedLocale.creator == item.creator)) {
        return item;
      }
    }
  }
  return null;
}










function setFilePermissions(aFile, aPermissions) {
  try {
    aFile.permissions = aPermissions;
  }
  catch (e) {
    WARN("Failed to set permissions " + aPermissions.toString(8) + " on " +
         aFile.path, e);
  }
}











function SafeInstallOperation() {
  this._installedFiles = [];
  this._createdDirs = [];
}

SafeInstallOperation.prototype = {
  _installedFiles: null,
  _createdDirs: null,

  _installFile: function SIO_installFile(aFile, aTargetDirectory, aCopy) {
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

  _installDirectory: function SIO_installDirectory(aDirectory, aTargetDirectory, aCopy) {
    let newDir = aTargetDirectory.clone();
    newDir.append(aDirectory.leafName);
    try {
      newDir.create(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
    }
    catch (e) {
      ERROR("Failed to create directory " + newDir.path, e);
      throw e;
    }
    this._createdDirs.push(newDir);

    
    
    
    
    let entries = getDirectoryEntries(aDirectory, true);
    entries.forEach(function(aEntry) {
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
      setFilePermissions(aDirectory, FileUtils.PERMS_DIRECTORY);
      aDirectory.remove(false);
    }
    catch (e) {
      ERROR("Failed to remove directory " + aDirectory.path, e);
      throw e;
    }

    
    
    this._installedFiles.push({ oldFile: aDirectory, newFile: newDir });
  },

  _installDirEntry: function SIO_installDirEntry(aDirEntry, aTargetDirectory, aCopy) {
    let isDir = null;

    try {
      isDir = aDirEntry.isDirectory();
    }
    catch (e) {
      
      
      
      if (e.result == Cr.NS_ERROR_FILE_TARGET_DOES_NOT_EXIST)
        return;

      ERROR("Failure " + (aCopy ? "copying" : "moving") + " " + aDirEntry.path +
            " to " + aTargetDirectory.path);
      throw e;
    }

    try {
      if (isDir)
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

  









  move: function SIO_move(aFile, aTargetDirectory) {
    try {
      this._installDirEntry(aFile, aTargetDirectory, false);
    }
    catch (e) {
      this.rollback();
      throw e;
    }
  },

  









  copy: function SIO_copy(aFile, aTargetDirectory) {
    try {
      this._installDirEntry(aFile, aTargetDirectory, true);
    }
    catch (e) {
      this.rollback();
      throw e;
    }
  },

  




  rollback: function SIO_rollback() {
    while (this._installedFiles.length > 0) {
      let move = this._installedFiles.pop();
      if (move.newFile.isDirectory()) {
        let oldDir = move.oldFile.parent.clone();
        oldDir.append(move.oldFile.leafName);
        oldDir.create(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
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
      for each (let found in localized.locales) {
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

  if (AddonManager.checkUpdateSecurity && !aAddon.providesUpdatesSecurely)
    return false;

  if (!aAddon.isPlatformCompatible)
    return false;

  if (AddonManager.checkCompatibility) {
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

this.__defineGetter__("gRDF", function gRDFGetter() {
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
    let type = addon.type;
    addon.type = null;
    for (let name in TYPES) {
      if (TYPES[name] == type) {
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

  addon.strictCompatibility = !(addon.type in COMPATIBLE_BY_DEFAULT_TYPES) ||
                              getRDFProperty(ds, root, "strictCompatibility") == "true";

  
  if (addon.type == "extension") {
    addon.bootstrap = getRDFProperty(ds, root, "bootstrap") == "true";
    if (addon.optionsType &&
        addon.optionsType != AddonManager.OPTIONS_TYPE_DIALOG &&
        addon.optionsType != AddonManager.OPTIONS_TYPE_INLINE &&
        addon.optionsType != AddonManager.OPTIONS_TYPE_TAB &&
        addon.optionsType != AddonManager.OPTIONS_TYPE_INLINE_INFO) {
      throw new Error("Install manifest specifies unknown type: " + addon.optionsType);
    }
  }
  else {
    
    if (addon.type == "dictionary" || addon.type == "locale")
      addon.bootstrap = true;

    
    
    addon.optionsURL = null;
    addon.optionsType = null;
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

  addon.applyBackgroundUpdates = AddonManager.AUTOUPDATE_DEFAULT;

  
  
  let storage = Services.storage;

  
  
  let rng = Cc["@mozilla.org/security/random-generator;1"].
            createInstance(Ci.nsIRandomGenerator);
  let bytes = rng.generateRandomBytes(9);
  let byte_string = [String.fromCharCode(byte) for each (byte in bytes)]
                    .join("");
  
  addon.syncGUID = btoa(byte_string).replace(/\+/g, '-')
                                    .replace(/\//g, '_');

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
    while ((entry = entries.nextFile))
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
    addon._sourceBundle = aDir.clone();
    addon.size = getFileSize(aDir);

    file = aDir.clone();
    file.append("chrome.manifest");
    let chromeManifest = ChromeManifestParser.parseSync(Services.io.newFileURI(file));
    addon.hasBinaryComponents = ChromeManifestParser.hasType(chromeManifest,
                                                             "binary-component");

    addon.appDisabled = !isUsableAddon(addon);
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

    
    if (addon.unpack) {
      uri = buildJarURI(aZipReader.file, "chrome.manifest");
      let chromeManifest = ChromeManifestParser.parseSync(uri);
      addon.hasBinaryComponents = ChromeManifestParser.hasType(chromeManifest,
                                                               "binary-component");
    } else {
      addon.hasBinaryComponents = false;
    }

    addon.appDisabled = !isUsableAddon(addon);
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














function getURIForResourceInFile(aFile, aPath) {
  if (aFile.isDirectory()) {
    let resource = aFile.clone();
    if (aPath) {
      aPath.split("/").forEach(function(aPart) {
        resource.append(aPart);
      });
    }
    return NetUtil.newURI(resource);
  }

  return buildJarURI(aFile, aPath);
}










function buildJarURI(aJarfile, aPath) {
  let uri = Services.io.newFileURI(aJarfile);
  uri = "jar:" + uri.spec + "!/" + aPath;
  return NetUtil.newURI(uri);
}







function flushJarCache(aJarFile) {
  Services.obs.notifyObservers(aJarFile, "flush-cache-entry", null);
  Cc["@mozilla.org/globalmessagemanager;1"].getService(Ci.nsIMessageBroadcaster)
    .broadcastAsyncMessage(MSG_JAR_FLUSH, aJarFile.path);
}

function flushStartupCache() {
  
  Services.obs.notifyObservers(null, "startupcache-invalidate", null);
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
          target.create(Ci.nsIFile.DIRECTORY_TYPE,
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
      try {
        target.permissions |= FileUtils.PERMS_FILE;
      }
      catch (e) {
        WARN("Failed to set permissions " + aPermissions.toString(8) + " on " +
             target.path, e);
      }
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
  let uri = AddonManager.escapeAddonURI(aAddon, aUri, aAppVersion);

  
  if (aUpdateType)
    uri = uri.replace(/%UPDATE_TYPE%/g, aUpdateType);

  
  
  
  let app = aAddon.matchingTargetApplication;
  if (app)
    var maxVersion = app.maxVersion;
  else
    maxVersion = "";
  uri = uri.replace(/%ITEM_MAXAPPVERSION%/g, maxVersion);

  let compatMode = "normal";
  if (!AddonManager.checkCompatibility)
    compatMode = "ignore";
  else if (AddonManager.strictCompatibility)
    compatMode = "strict";
  uri = uri.replace(/%COMPATIBILITY_MODE%/g, compatMode);

  return uri;
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
    setFilePermissions(aDir, FileUtils.PERMS_DIRECTORY);
    aDir.remove(false);
  }
  catch (e) {
    WARN("Failed to remove staging dir", e);
    
  }
}







function recursiveRemove(aFile) {
  let isDir = null;

  try {
    isDir = aFile.isDirectory();
  }
  catch (e) {
    
    
    
    if (e.result == Cr.NS_ERROR_FILE_TARGET_DOES_NOT_EXIST)
      return;

    throw e;
  }

  setFilePermissions(aFile, isDir ? FileUtils.PERMS_DIRECTORY
                                  : FileUtils.PERMS_FILE);

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

  
  
  
  
  let entries = getDirectoryEntries(aFile, true);
  entries.forEach(recursiveRemove);

  try {
    aFile.remove(true);
  }
  catch (e) {
    ERROR("Failed to remove empty directory " + aFile.path, e);
    throw e;
  }
}









function recursiveLastModifiedTime(aFile) {
  if (aFile.isFile())
    return aFile.lastModifiedTime;

  if (aFile.isDirectory()) {
    let entries = aFile.directoryEntries.QueryInterface(Ci.nsIDirectoryEnumerator);
    let entry, time;
    let maxTime = aFile.lastModifiedTime;
    while ((entry = entries.nextFile)) {
      time = recursiveLastModifiedTime(entry);
      maxTime = Math.max(time, maxTime);
    }
    entries.close();
    return maxTime;
  }

  
  return 0;
}










function getDirectoryEntries(aDir, aSortEntries) {
  let dirEnum;
  try {
    dirEnum = aDir.directoryEntries.QueryInterface(Ci.nsIDirectoryEnumerator);
    let entries = [];
    while (dirEnum.hasMoreElements())
      entries.push(dirEnum.nextFile);

    if (aSortEntries) {
      entries.sort(function sortDirEntries(a, b) {
        return a.path > b.path ? -1 : 1;
      });
    }

    return entries
  }
  catch (e) {
    return null;
  }
  finally {
    dirEnum.close();
  }
}





var Prefs = {
  









  getDefaultCharPref: function Prefs_getDefaultCharPref(aName, aDefaultValue) {
    try {
      return Services.prefs.getDefaultBranch("").getCharPref(aName);
    }
    catch (e) {
    }
    return aDefaultValue;
  },

  








  getCharPref: function Prefs_getCharPref(aName, aDefaultValue) {
    try {
      return Services.prefs.getCharPref(aName);
    }
    catch (e) {
    }
    return aDefaultValue;
  },

  










  getComplexValue: function Prefs_getComplexValue(aName, aType, aDefaultValue) {
    try {
      return Services.prefs.getComplexValue(aName, aType).data;
    }
    catch (e) {
    }
    return aDefaultValue;
  },

  








  getBoolPref: function Prefs_getBoolPref(aName, aDefaultValue) {
    try {
      return Services.prefs.getBoolPref(aName);
    }
    catch (e) {
    }
    return aDefaultValue;
  },

  








  getIntPref: function Prefs_getIntPref(aName, defaultValue) {
    try {
      return Services.prefs.getIntPref(aName);
    }
    catch (e) {
    }
    return defaultValue;
  },

  





  clearUserPref: function Prefs_clearUserPref(aName) {
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
  
  minCompatibleAppVersion: null,
  
  minCompatiblePlatformVersion: null,
  
  bootstrappedAddons: {},
  
  bootstrapScopes: {},
  
  extensionsActive: false,

  
  
  allAppGlobal: true,
  
  enabledAddons: null,
  
  inactiveAddonIDs: [],
  
  unpackedAddons: 0,

  















  startup: function XPI_startup(aAppChanged, aOldAppVersion, aOldPlatformVersion) {
    LOG("startup");
    this.installs = [];
    this.installLocations = [];
    this.installLocationsByName = {};

    AddonManagerPrivate.recordTimestamp("XPI_startup_begin");

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

    this.minCompatibleAppVersion = Prefs.getCharPref(PREF_EM_MIN_COMPAT_APP_VERSION,
                                                     null);
    this.minCompatiblePlatformVersion = Prefs.getCharPref(PREF_EM_MIN_COMPAT_PLATFORM_VERSION,
                                                          null);
    this.enabledAddons = [];

    Services.prefs.addObserver(PREF_EM_MIN_COMPAT_APP_VERSION, this, false);
    Services.prefs.addObserver(PREF_EM_MIN_COMPAT_PLATFORM_VERSION, this, false);

    let flushCaches = this.checkForChanges(aAppChanged, aOldAppVersion,
                                           aOldPlatformVersion);

    
    this.applyThemeChange();

    
    
    
    if (aAppChanged && !this.allAppGlobal &&
        Prefs.getBoolPref(PREF_EM_SHOW_MISMATCH_UI, true)) {
      this.showUpgradeUI();
      flushCaches = true;
    }
    else if (aAppChanged === undefined) {
      
      Services.prefs.setBoolPref(PREF_SHOWN_SELECTION_UI, true);
    }

    if (flushCaches) {
      flushStartupCache();

      
      
      
      
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
                                             AddonManager.checkCompatibility);
      } catch (e) { }
      this.addAddonsToCrashReporter();
    }

    AddonManagerPrivate.recordTimestamp("XPI_bootstrap_addons_begin");
    for (let id in this.bootstrappedAddons) {
      let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
      file.persistentDescriptor = this.bootstrappedAddons[id].descriptor;
      this.callBootstrapMethod(id, this.bootstrappedAddons[id].version,
                               this.bootstrappedAddons[id].type, file,
                               "startup", BOOTSTRAP_REASONS.APP_STARTUP);
    }
    AddonManagerPrivate.recordTimestamp("XPI_bootstrap_addons_end");

    
    
    Services.obs.addObserver({
      observe: function shutdownObserver(aSubject, aTopic, aData) {
        for (let id in XPIProvider.bootstrappedAddons) {
          let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
          file.persistentDescriptor = XPIProvider.bootstrappedAddons[id].descriptor;
          XPIProvider.callBootstrapMethod(id, XPIProvider.bootstrappedAddons[id].version,
                                          XPIProvider.bootstrappedAddons[id].type, file, "shutdown",
                                          BOOTSTRAP_REASONS.APP_SHUTDOWN);
        }
        Services.obs.removeObserver(this, "quit-application-granted");
      }
    }, "quit-application-granted", false);

    AddonManagerPrivate.recordTimestamp("XPI_startup_end");

    this.extensionsActive = true;
  },

  


  shutdown: function XPI_shutdown() {
    LOG("shutdown");

    this.bootstrappedAddons = {};
    this.bootstrapScopes = {};
    this.enabledAddons = null;
    this.allAppGlobal = true;

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

    if (gLazyObjectsLoaded) {
      XPIDatabase.shutdown(function shutdownCallback() {
        Services.obs.notifyObservers(null, "xpi-provider-shutdown", null);
      });
    }
    else {
      Services.obs.notifyObservers(null, "xpi-provider-shutdown", null);
    }
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

  


  showUpgradeUI: function XPI_showUpgradeUI() {
    
    Services.startup.interrupted = true;

    if (!Prefs.getBoolPref(PREF_SHOWN_SELECTION_UI, false)) {
      
      var features = "chrome,centerscreen,dialog,titlebar,modal";
      Services.ww.openWindow(null, URI_EXTENSION_SELECT_DIALOG, "", features, null);
      Services.prefs.setBoolPref(PREF_SHOWN_SELECTION_UI, true);
    }
    else {
      var variant = Cc["@mozilla.org/variant;1"].
                    createInstance(Ci.nsIWritableVariant);
      variant.setFromVariant(this.inactiveAddonIDs);
  
      
      var features = "chrome,centerscreen,dialog,titlebar,modal";
      var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
               getService(Ci.nsIWindowWatcher);
      ww.openWindow(null, URI_EXTENSION_UPDATE_DIALOG, "", features, variant);
    }

    
    XPIDatabase.writeAddonsList();
    Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, false);
  },

  


  persistBootstrappedAddons: function XPI_persistBootstrappedAddons() {
    Services.prefs.setCharPref(PREF_BOOTSTRAP_ADDONS,
                               JSON.stringify(this.bootstrappedAddons));
  },

  


  addAddonsToCrashReporter: function XPI_addAddonsToCrashReporter() {
    if (!("nsICrashReporter" in Ci) ||
        !(Services.appinfo instanceof Ci.nsICrashReporter))
      return;

    
    
    if (Services.appinfo.inSafeMode)
      return;

    let data = this.enabledAddons;
    for (let id in this.bootstrappedAddons) {
      data += (data ? "," : "") + encodeURIComponent(id) + ":" +
              encodeURIComponent(this.bootstrappedAddons[id].version);
    }

    try {
      Services.appinfo.annotateCrashReport("Add-ons", data);
    }
    catch (e) { }
    
    const TelemetryPing = Cc["@mozilla.org/base/telemetry-ping;1"].getService(Ci.nsITelemetryPing);
    TelemetryPing.setAddOns(data);
  },

  










  getAddonStates: function XPI_getAddonStates(aLocation) {
    let addonStates = {};
    aLocation.addonLocations.forEach(function(file) {
      let id = aLocation.getIDForLocation(file);
      addonStates[id] = {
        descriptor: file.persistentDescriptor,
        mtime: recursiveLastModifiedTime(file)
      };
      try {
        
        file.append(FILE_INSTALL_MANIFEST);
        let rdfTime = file.lastModifiedTime;
        addonStates[id].rdfTime = rdfTime;
        this.unpackedAddons += 1;
      }
      catch (e) { }
    }, this);

    return addonStates;
  },

  








  getInstallLocationStates: function XPI_getInstallLocationStates() {
    let states = [];
    this.unpackedAddons = 0;
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

      let seenFiles = [];
      
      
      
      
      let stagingDirEntries = getDirectoryEntries(stagingDir, true);
      for (let stageDirEntry of stagingDirEntries) {
        let id = stageDirEntry.leafName;

        let isDir;
        try {
          isDir = stageDirEntry.isDirectory();
        }
        catch (e if e.result == Cr.NS_ERROR_FILE_TARGET_DOES_NOT_EXIST) {
          
          
          
          continue;
        }

        if (!isDir) {
          if (id.substring(id.length - 4).toLowerCase() == ".xpi") {
            id = id.substring(0, id.length - 4);
          }
          else {
            if (id.substring(id.length - 5).toLowerCase() != ".json") {
              WARN("Ignoring file: " + stageDirEntry.path);
              seenFiles.push(stageDirEntry.leafName);
            }
            continue;
          }
        }

        
        if (!gIDTest.test(id)) {
          WARN("Ignoring directory whose name is not a valid add-on ID: " +
               stageDirEntry.path);
          seenFiles.push(stageDirEntry.leafName);
          continue;
        }

        changed = true;

        if (isDir) {
          
          let manifest = stageDirEntry.clone();
          manifest.append(FILE_INSTALL_MANIFEST);

          
          
          if (!manifest.exists()) {
            LOG("Processing uninstall of " + id + " in " + aLocation.name);
            try {
              aLocation.uninstallAddon(id);
              seenFiles.push(stageDirEntry.leafName);
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

        try {
          aManifests[aLocation.name][id] = loadManifestFromFile(stageDirEntry);
        }
        catch (e) {
          ERROR("Unable to read add-on manifest from " + stageDirEntry.path, e);
          
          seenFiles.push(stageDirEntry.leafName);
          seenFiles.push(jsonfile.leafName);
          continue;
        }

        
        
        if (jsonfile.exists()) {
          LOG("Found updated metadata for " + id + " in " + aLocation.name);
          let fis = Cc["@mozilla.org/network/file-input-stream;1"].
                       createInstance(Ci.nsIFileInputStream);
          let json = Cc["@mozilla.org/dom/json;1"].
                     createInstance(Ci.nsIJSON);

          try {
            fis.init(jsonfile, -1, 0, 0);
            let metadata = json.decodeFromStream(fis, jsonfile.fileSize);
            aManifests[aLocation.name][id].importMetadata(metadata);
          }
          catch (e) {
            
            
            
            ERROR("Unable to read metadata from " + jsonfile.path, e);
          }
          finally {
            fis.close();
          }
        }
        seenFiles.push(jsonfile.leafName);

        existingAddonID = aManifests[aLocation.name][id].existingAddonID || id;

        var oldBootstrap = null;
        LOG("Processing install of " + id + " in " + aLocation.name);
        if (existingAddonID in this.bootstrappedAddons) {
          try {
            var existingAddon = aLocation.getLocationForID(existingAddonID);
            if (this.bootstrappedAddons[existingAddonID].descriptor ==
                existingAddon.persistentDescriptor) {
              oldBootstrap = this.bootstrappedAddons[existingAddonID];

              
              
              let newVersion = aManifests[aLocation.name][id].version;
              let oldVersion = oldBootstrap.version;
              let uninstallReason = Services.vc.compare(oldVersion, newVersion) < 0 ?
                                    BOOTSTRAP_REASONS.ADDON_UPGRADE :
                                    BOOTSTRAP_REASONS.ADDON_DOWNGRADE;

              this.callBootstrapMethod(existingAddonID, oldBootstrap.version,
                                       oldBootstrap.type, existingAddon, "uninstall", uninstallReason,
                                       { newVersion: newVersion });
              this.unloadBootstrapScope(existingAddonID);
              flushStartupCache();
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
          
          AddonInstall.createStagedInstall(aLocation, stageDirEntry,
                                           aManifests[aLocation.name][id]);
          
          seenFiles.pop();

          delete aManifests[aLocation.name][id];

          if (oldBootstrap) {
            
            this.callBootstrapMethod(existingAddonID, oldBootstrap.version,
                                     oldBootstrap.type, existingAddon, "install",
                                     BOOTSTRAP_REASONS.ADDON_INSTALL);
          }
          continue;
        }
      }

      try {
        cleanStagingDir(stagingDir, seenFiles);
      }
      catch (e) {
        
        LOG("Error cleaning staging dir " + stagingDir.path, e);
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
    while ((entry = entries.nextFile)) {

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
                                                      aOldPlatformVersion) {
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

          
          
          
          
          newAddon.pendingUninstall = aOldAddon.pendingUninstall;
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
        
        AddonManagerPrivate.addStartupChange(AddonManager.STARTUP_CHANGE_CHANGED,
                                             newAddon.id);

        
        
        if (aOldAddon.active && isAddonDisabled(newAddon))
          XPIProvider.enableDefaultTheme();

        
        if (newAddon.active && newAddon.bootstrap) {
          
          flushStartupCache();

          let installReason = Services.vc.compare(aOldAddon.version, newAddon.version) < 0 ?
                              BOOTSTRAP_REASONS.ADDON_UPGRADE :
                              BOOTSTRAP_REASONS.ADDON_DOWNGRADE;

          let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
          file.persistentDescriptor = aAddonState.descriptor;
          XPIProvider.callBootstrapMethod(newAddon.id, newAddon.version, newAddon.type, file,
                                          "install", installReason, { oldVersion: aOldAddon.version });
          return false;
        }

        return true;
      }

      return false;
    }

    













    function updateDescriptor(aInstallLocation, aOldAddon, aAddonState) {
      LOG("Add-on " + aOldAddon.id + " moved to " + aAddonState.descriptor);

      aOldAddon._descriptor = aAddonState.descriptor;
      aOldAddon.visible = !(aOldAddon.id in visibleAddons);

      
      XPIDatabase.setAddonDescriptor(aOldAddon, aAddonState.descriptor);
      if (aOldAddon.visible) {
        visibleAddons[aOldAddon.id] = aOldAddon;

        if (aOldAddon.bootstrap && aOldAddon.active) {
          let bootstrap = oldBootstrappedAddons[aOldAddon.id];
          bootstrap.descriptor = aAddonState.descriptor;
          XPIProvider.bootstrappedAddons[aOldAddon.id] = bootstrap;
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
          
          AddonManagerPrivate.addStartupChange(AddonManager.STARTUP_CHANGE_CHANGED,
                                               aOldAddon.id);
          XPIDatabase.makeAddonVisible(aOldAddon);

          if (aOldAddon.bootstrap) {
            
            let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
            file.persistentDescriptor = aAddonState.descriptor;
            XPIProvider.callBootstrapMethod(aOldAddon.id, aOldAddon.version, aOldAddon.type, file,
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
        newAddon.syncGUID = aOldAddon.syncGUID;
        newAddon.version = aOldAddon.version;
        newAddon.type = aOldAddon.type;
        newAddon.appDisabled = !isUsableAddon(aOldAddon);

        
        if (aOldAddon.type == "theme")
          newAddon.userDisabled = aOldAddon.internalName != XPIProvider.selectedSkin;

        applyBlocklistChanges(aOldAddon, newAddon, aOldAppVersion,
                              aOldPlatformVersion);

        let wasDisabled = isAddonDisabled(aOldAddon);
        let isDisabled = isAddonDisabled(newAddon);

        
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
          
          
          let change = isDisabled ? AddonManager.STARTUP_CHANGE_DISABLED
                                  : AddonManager.STARTUP_CHANGE_ENABLED;
          AddonManagerPrivate.addStartupChange(change, aOldAddon.id);
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
          type: aOldAddon.type,
          descriptor: aAddonState.descriptor
        };
      }

      return changed;
    }

    










    function removeMetadata(aInstallLocation, aOldAddon) {
      
      LOG("Add-on " + aOldAddon.id + " removed from " + aInstallLocation.name);
      XPIDatabase.removeAddonMetadata(aOldAddon);

      
      if (aOldAddon.visible) {
        AddonManagerPrivate.addStartupChange(AddonManager.STARTUP_CHANGE_UNINSTALLED,
                                             aOldAddon.id);
      }
      else if (AddonManager.getStartupChanges(AddonManager.STARTUP_CHANGE_INSTALLED)
                           .indexOf(aOldAddon.id) != -1) {
        AddonManagerPrivate.addStartupChange(AddonManager.STARTUP_CHANGE_CHANGED,
                                             aOldAddon.id);
      }

      if (aOldAddon.active) {
        
        
        if (aOldAddon.type == "theme")
          XPIProvider.enableDefaultTheme();

        return true;
      }

      return false;
    }

    



















    function addMetadata(aInstallLocation, aId, aAddonState, aMigrateData) {
      LOG("New add-on " + aId + " installed in " + aInstallLocation.name);

      let newAddon = null;
      let sameVersion = false;
      
      
      if (aInstallLocation.name in aManifests)
        newAddon = aManifests[aInstallLocation.name][aId];

      
      
      
      let isNewInstall = (!!newAddon) || (!XPIDatabase.activeBundles && !aMigrateData);

      
      
      let isDetectedInstall = isNewInstall && !newAddon;

      
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
      newAddon.foreignInstall = isDetectedInstall;

      if (aMigrateData) {
        
        LOG("Migrating data from old database");

        DB_MIGRATE_METADATA.forEach(function(aProp) {
          
          
          if (aProp == "userDisabled" && newAddon.type == "theme")
            return;

          if (aProp in aMigrateData)
            newAddon[aProp] = aMigrateData[aProp];
        });

        
        
        newAddon.foreignInstall |= aInstallLocation.name != KEY_APP_PROFILE;

        
        
        
        if (aMigrateData.version == newAddon.version) {
          LOG("Migrating compatibility info");
          sameVersion = true;
          if ("targetApplications" in aMigrateData)
            newAddon.applyCompatibilityUpdate(aMigrateData, true);
        }

        
        applyBlocklistChanges(newAddon, newAddon, aOldAppVersion,
                              aOldPlatformVersion);
      }

      
      if (newAddon.type == "theme" && newAddon.internalName == XPIProvider.defaultSkin)
        newAddon.foreignInstall = false;

      if (isDetectedInstall && newAddon.foreignInstall) {
        
        
        let disablingScopes = Prefs.getIntPref(PREF_EM_AUTO_DISABLED_SCOPES, 0);
        if (aInstallLocation.scope & disablingScopes)
          newAddon.userDisabled = true;
      }

      
      
      if (!isNewInstall && XPIDatabase.activeBundles) {
        
        if (newAddon.type == "theme")
          newAddon.active = newAddon.internalName == XPIProvider.currentSkin;
        else
          newAddon.active = XPIDatabase.activeBundles.indexOf(aAddonState.descriptor) != -1;

        
        
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
        
        if (isDetectedInstall) {
          
          
          if (AddonManager.getStartupChanges(AddonManager.STARTUP_CHANGE_UNINSTALLED)
                          .indexOf(newAddon.id) != -1) {
            AddonManagerPrivate.addStartupChange(AddonManager.STARTUP_CHANGE_CHANGED,
                                                 newAddon.id);
          }
          else {
            AddonManagerPrivate.addStartupChange(AddonManager.STARTUP_CHANGE_INSTALLED,
                                                 newAddon.id);
          }
        }

        
        if (newAddon._installLocation.name != KEY_APP_GLOBAL)
          XPIProvider.allAppGlobal = false;

        visibleAddons[newAddon.id] = newAddon;

        let installReason = BOOTSTRAP_REASONS.ADDON_INSTALL;
        let extraParams = {};

        
        if (newAddon.id in oldBootstrappedAddons) {
          let oldBootstrap = oldBootstrappedAddons[newAddon.id];
          extraParams.oldVersion = oldBootstrap.version;
          XPIProvider.bootstrappedAddons[newAddon.id] = oldBootstrap;

          
          
          
          if (sameVersion || !isNewInstall)
            return false;

          installReason = Services.vc.compare(oldBootstrap.version, newAddon.version) < 0 ?
                          BOOTSTRAP_REASONS.ADDON_UPGRADE :
                          BOOTSTRAP_REASONS.ADDON_DOWNGRADE;

          let oldAddonFile = Cc["@mozilla.org/file/local;1"].
                             createInstance(Ci.nsIFile);
          oldAddonFile.persistentDescriptor = oldBootstrap.descriptor;

          XPIProvider.callBootstrapMethod(newAddon.id, oldBootstrap.version,
                                          oldBootstrap.type, oldAddonFile, "uninstall",
                                          installReason, { newVersion: newAddon.version });
          XPIProvider.unloadBootstrapScope(newAddon.id);

          
          
          if (newAddon.bootstrap)
            flushStartupCache();
        }

        if (!newAddon.bootstrap)
          return true;

        
        let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
        file.persistentDescriptor = aAddonState.descriptor;
        XPIProvider.callBootstrapMethod(newAddon.id, newAddon.version, newAddon.type, file,
                                        "install", installReason, extraParams);
        if (!newAddon.active)
          XPIProvider.unloadBootstrapScope(newAddon.id);
      }

      return false;
    }

    let changed = false;
    let knownLocations = XPIDatabase.getInstallLocations();

    
    let modifiedUnpacked = 0;
    let modifiedExManifest = 0;
    let modifiedXPI = 0;

    
    
    
    aState.reverse().forEach(function(aSt) {

      
      
      let installLocation = this.installLocationsByName[aSt.name];
      let addonStates = aSt.addons;

      
      let pos = knownLocations.indexOf(installLocation.name);
      if (pos >= 0) {
        knownLocations.splice(pos, 1);
        let addons = XPIDatabase.getAddonsInLocation(installLocation.name);
        
        
        addons.forEach(function(aOldAddon) {
          
          
          if (AddonManager.getStartupChanges(AddonManager.STARTUP_CHANGE_INSTALLED)
                          .indexOf(aOldAddon.id) != -1) {
            AddonManagerPrivate.addStartupChange(AddonManager.STARTUP_CHANGE_CHANGED,
                                                 aOldAddon.id);
          }

          
          if (aOldAddon.id in addonStates) {
            let addonState = addonStates[aOldAddon.id];
            delete addonStates[aOldAddon.id];

            
            if (aOldAddon.visible && !aOldAddon.active)
              XPIProvider.inactiveAddonIDs.push(aOldAddon.id);

            
            
            if ((addonState.rdfTime) && (aOldAddon.updateDate != addonState.mtime)) {
              modifiedUnpacked += 1;
              if (aOldAddon.updateDate >= addonState.rdfTime)
                modifiedExManifest += 1;
            }
            else if (aOldAddon.updateDate != addonState.mtime) {
              modifiedXPI += 1;
            }

            
            
            
            
            if (aOldAddon.id in aManifests[installLocation.name] ||
                aOldAddon.updateDate != addonState.mtime ||
                (aUpdateCompatibility && installLocation.name == KEY_APP_GLOBAL)) {
              changed = updateMetadata(installLocation, aOldAddon, addonState) ||
                        changed;
            }
            else if (aOldAddon._descriptor != addonState.descriptor) {
              changed = updateDescriptor(installLocation, aOldAddon, addonState) ||
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
      if (XPIDatabase.migrateData && installLocation.name in XPIDatabase.migrateData)
        locMigrateData = XPIDatabase.migrateData[installLocation.name];
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

    
    AddonManagerPrivate.recordSimpleMeasure("modifiedUnpacked", modifiedUnpacked);
    if (modifiedUnpacked > 0)
      AddonManagerPrivate.recordSimpleMeasure("modifiedExceptInstallRDF", modifiedExManifest);
    AddonManagerPrivate.recordSimpleMeasure("modifiedXPI", modifiedXPI);

    
    let cache = JSON.stringify(this.getInstallLocationStates());
    Services.prefs.setCharPref(PREF_INSTALL_CACHE, cache);
    this.persistBootstrappedAddons();

    
    XPIDatabase.migrateData = null;

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

    
    
    try {
      this.bootstrappedAddons = JSON.parse(Prefs.getCharPref(PREF_BOOTSTRAP_ADDONS,
                                           "{}"));
    } catch (e) {
      WARN("Error parsing enabled bootstrapped extensions cache", e);
    }

    
    
    
    let manifests = {};
    updateDatabase = this.processPendingFileChanges(manifests) || updateDatabase;

    
    
    
    let hasPendingChanges = Prefs.getBoolPref(PREF_PENDING_OPERATIONS);

    
    updateDatabase = updateDatabase || DB_SCHEMA != Prefs.getIntPref(PREF_DB_SCHEMA, 0);

    
    if (aAppChanged !== false &&
        Prefs.getBoolPref(PREF_INSTALL_DISTRO_ADDONS, true))
      updateDatabase = this.installDistributionAddons(manifests) || updateDatabase;

    
    let telemetryCaptureTime = new Date();
    let state = this.getInstallLocationStates();
    let telemetry = Services.telemetry;
    telemetry.getHistogramById("CHECK_ADDONS_MODIFIED_MS").add(new Date() - telemetryCaptureTime);

    if (!updateDatabase) {
      
      let cache = Prefs.getCharPref(PREF_INSTALL_CACHE, null);
      updateDatabase = cache != JSON.stringify(state);
    }

    
    
    
    
    
    let dbFile = FileUtils.getFile(KEY_PROFILEDIR, [FILE_DATABASE], true);
    if (!dbFile.exists())
      updateDatabase = state.length > 0;

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

    
    let transationBegun = false;
    try {
      let extensionListChanged = false;
      
      
      if (updateDatabase || hasPendingChanges) {
        XPIDatabase.beginTransaction();
        transationBegun = true;
        XPIDatabase.openConnection(false, true);

        try {
          extensionListChanged = this.processFileChanges(state, manifests,
                                                         aAppChanged,
                                                         aOldAppVersion,
                                                         aOldPlatformVersion);
        }
        catch (e) {
          ERROR("Error processing file changes", e);
        }
      }
      AddonManagerPrivate.recordSimpleMeasure("installedUnpacked", this.unpackedAddons);

      if (aAppChanged) {
        
        
        if (this.currentSkin != this.defaultSkin) {
          if (!transationBegun) {
            XPIDatabase.beginTransaction();
            transationBegun = true;
          }
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
        if (!transationBegun) {
          XPIDatabase.beginTransaction();
          transationBegun = true;
        }
        XPIDatabase.updateActiveAddons();
        XPIDatabase.commitTransaction();
        XPIDatabase.writeAddonsList();
        Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, false);
        Services.prefs.setCharPref(PREF_BOOTSTRAP_ADDONS,
                                   JSON.stringify(this.bootstrappedAddons));
        return true;
      }

      LOG("No changes found");
      if (transationBegun)
        XPIDatabase.commitTransaction();
    }
    catch (e) {
      ERROR("Error during startup file checks, rolling back any database " +
            "changes", e);
      if (transationBegun)
        XPIDatabase.rollbackTransaction();
    }

    
    let addonsList = FileUtils.getFile(KEY_PROFILEDIR, [FILE_XPI_ADDONS_LIST],
                                       true);
    if (addonsList.exists() == (state.length == 0)) {
      LOG("Add-ons list is invalid, rebuilding");
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

  

















  getInstallForURL: function XPI_getInstallForURL(aUrl, aHash, aName, aIcons,
                                                  aVersion, aLoadGroup, aCallback) {
    AddonInstall.createDownload(function getInstallForURL_createDownload(aInstall) {
      aCallback(aInstall.wrapper);
    }, aUrl, aHash, aName, aIcons, aVersion, aLoadGroup);
  },

  







  getInstallForFile: function XPI_getInstallForFile(aFile, aCallback) {
    AddonInstall.createInstall(function getInstallForFile_createInstall(aInstall) {
      if (aInstall)
        aCallback(aInstall.wrapper);
      else
        aCallback(null);
    }, aFile);
  },

  





  removeActiveInstall: function XPI_removeActiveInstall(aInstall) {
    this.installs = this.installs.filter(function installFilter(i) i != aInstall);
  },

  







  getAddonByID: function XPI_getAddonByID(aId, aCallback) {
    XPIDatabase.getVisibleAddonForID (aId, function getAddonByID_getVisibleAddonForID(aAddon) {
      if (aAddon)
        aCallback(createWrapper(aAddon));
      else
        aCallback(null);
    });
  },

  







  getAddonsByTypes: function XPI_getAddonsByTypes(aTypes, aCallback) {
    XPIDatabase.getVisibleAddons(aTypes, function getAddonsByTypes_getVisibleAddons(aAddons) {
      aCallback([createWrapper(a) for each (a in aAddons)]);
    });
  },

  







  getAddonBySyncGUID: function XPI_getAddonBySyncGUID(aGUID, aCallback) {
    XPIDatabase.getAddonBySyncGUID(aGUID, function getAddonBySyncGUID_getAddonBySyncGUID(aAddon) {
      if (aAddon)
        aCallback(createWrapper(aAddon));
      else
        aCallback(null);
    });
  },

  







  getAddonsWithOperationsByTypes:
  function XPI_getAddonsWithOperationsByTypes(aTypes, aCallback) {
    XPIDatabase.getVisibleAddonsWithPendingOperations(aTypes,
      function getAddonsWithOpsByTypes_getVisibleAddonsWithPendingOps(aAddons) {
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

  





  updateAddonRepositoryData: function XPI_updateAddonRepositoryData(aCallback) {
    let self = this;
    XPIDatabase.getVisibleAddons(null, function UARD_getVisibleAddonsCallback(aAddons) {
      let pending = aAddons.length;
      if (pending == 0) {
        aCallback();
        return;
      }

      function notifyComplete() {
        if (--pending == 0)
          aCallback();
      }

      aAddons.forEach(function UARD_forEachCallback(aAddon) {
        AddonRepository.getCachedAddonByID(aAddon.id,
                                           function UARD_getCachedAddonCallback(aRepoAddon) {
          if (aRepoAddon) {
            aAddon._repositoryAddon = aRepoAddon;
            aAddon.compatibilityOverrides = aRepoAddon.compatibilityOverrides;
            self.updateAddonDisabledState(aAddon);
          }

          notifyComplete();
        });
      });
    });
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
    case PREF_EM_MIN_COMPAT_APP_VERSION:
    case PREF_EM_MIN_COMPAT_PLATFORM_VERSION:
      this.minCompatibleAppVersion = Prefs.getCharPref(PREF_EM_MIN_COMPAT_APP_VERSION,
                                                       null);
      this.minCompatiblePlatformVersion = Prefs.getCharPref(PREF_EM_MIN_COMPAT_PLATFORM_VERSION,
                                                            null);
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

  















  loadBootstrapScope: function XPI_loadBootstrapScope(aId, aFile, aVersion, aType) {
    
    this.bootstrappedAddons[aId] = {
      version: aVersion,
      type: aType,
      descriptor: aFile.persistentDescriptor
    };
    this.persistBootstrappedAddons();
    this.addAddonsToCrashReporter();

    
    if (aType == "locale") {
      this.bootstrapScopes[aId] = null;
      return;
    }

    LOG("Loading bootstrap scope from " + aFile.path);

    let principal = Cc["@mozilla.org/systemprincipal;1"].
                    createInstance(Ci.nsIPrincipal);

    if (!aFile.exists()) {
      this.bootstrapScopes[aId] = new Components.utils.Sandbox(principal,
                                                               {sandboxName: aFile.path});
      ERROR("Attempted to load bootstrap scope from missing directory " + aFile.path);
      return;
    }

    let uri = getURIForResourceInFile(aFile, "bootstrap.js").spec;
    this.bootstrapScopes[aId] = new Components.utils.Sandbox(principal,
                                                             {sandboxName: uri});

    let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
                 createInstance(Ci.mozIJSSubScriptLoader);

    try {
      
      for (let name in BOOTSTRAP_REASONS)
        this.bootstrapScopes[aId][name] = BOOTSTRAP_REASONS[name];

      
      const features = [ "Worker", "ChromeWorker" ];

      for (let feature of features)
        this.bootstrapScopes[aId][feature] = gGlobalScope[feature];

      
      
      
      if (aType == "dictionary") {
        this.bootstrapScopes[aId].__SCRIPT_URI_SPEC__ =
            "resource://gre/modules/SpellCheckDictionaryBootstrap.js"
      } else {
        this.bootstrapScopes[aId].__SCRIPT_URI_SPEC__ = uri;
      }
      Components.utils.evalInSandbox(
        "Components.classes['@mozilla.org/moz/jssubscript-loader;1'] \
                   .createInstance(Components.interfaces.mozIJSSubScriptLoader) \
                   .loadSubScript(__SCRIPT_URI_SPEC__);", this.bootstrapScopes[aId], "ECMAv5");
    }
    catch (e) {
      WARN("Error loading bootstrap.js for " + aId, e);
    }
  },

  






  unloadBootstrapScope: function XPI_unloadBootstrapScope(aId) {
    delete this.bootstrapScopes[aId];
    delete this.bootstrappedAddons[aId];
    this.persistBootstrappedAddons();
    this.addAddonsToCrashReporter();
  },

  


















  callBootstrapMethod: function XPI_callBootstrapMethod(aId, aVersion, aType, aFile,
                                                        aMethod, aReason, aExtraParams) {
    
    if (Services.appinfo.inSafeMode)
      return;

    if (aMethod == "startup")
      Components.manager.addBootstrappedManifestLocation(aFile);

    try {
      
      if (!(aId in this.bootstrapScopes))
        this.loadBootstrapScope(aId, aFile, aVersion, aType);

      
      if (aType == "locale")
        return;

      if (!(aMethod in this.bootstrapScopes[aId])) {
        WARN("Add-on " + aId + " is missing bootstrap method " + aMethod);
        return;
      }

      let params = {
        id: aId,
        version: aVersion,
        installPath: aFile.clone(),
        resourceURI: getURIForResourceInFile(aFile, "")
      };

      if (aExtraParams) {
        for (let key in aExtraParams) {
          params[key] = aExtraParams[key];
        }
      }

      LOG("Calling bootstrap method " + aMethod + " on " + aId + " version " +
          aVersion);
      try {
        this.bootstrapScopes[aId][aMethod](params, aReason);
      }
      catch (e) {
        WARN("Exception running bootstrap method " + aMethod + " on " +
             aId, e);
      }
    }
    finally {
      if (aMethod == "shutdown")
        Components.manager.removeBootstrappedManifestLocation(aFile);
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

    
    
    let appDisabledChanged = aAddon.appDisabled != appDisabled;

    
    XPIDatabase.setAddonProperties(aAddon, {
      userDisabled: aUserDisabled,
      appDisabled: appDisabled,
      softDisabled: aSoftDisabled
    });

    if (appDisabledChanged) {
      AddonManagerPrivate.callAddonListeners("onPropertyChanged",
                                            aAddon,
                                            ["appDisabled"]);
    }

    
    
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
            this.callBootstrapMethod(aAddon.id, aAddon.version, aAddon.type, file, "shutdown",
                                     BOOTSTRAP_REASONS.ADDON_DISABLE);
            this.unloadBootstrapScope(aAddon.id);
          }
          AddonManagerPrivate.callAddonListeners("onDisabled", wrapper);
        }
        else {
          if (aAddon.bootstrap) {
            let file = aAddon._installLocation.getLocationForID(aAddon.id);
            this.callBootstrapMethod(aAddon.id, aAddon.version, aAddon.type, file, "startup",
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

    if ("_hasResourceCache" in aAddon)
      aAddon._hasResourceCache = new Map();

    
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
        XPIProvider.callBootstrapMethod(aAddon.id, aAddon.version, aAddon.type, file,
                                        "install", BOOTSTRAP_REASONS.ADDON_INSTALL);

        if (aAddon.active) {
          XPIProvider.callBootstrapMethod(aAddon.id, aAddon.version, aAddon.type, file,
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
      XPIDatabase.getAddonInLocation(aAddon.id, location.name,
        function checkInstallLocation_getAddonInLocation(aNewAddon) {
        if (aNewAddon)
          revealAddon(aNewAddon);
        else
          checkInstallLocation(aPos - 1);
      })
    }

    if (!requiresRestart) {
      if (aAddon.bootstrap) {
        let file = aAddon._installLocation.getLocationForID(aAddon.id);
        if (aAddon.active) {
          this.callBootstrapMethod(aAddon.id, aAddon.version, aAddon.type, file,
                                   "shutdown",
                                   BOOTSTRAP_REASONS.ADDON_UNINSTALL);
        }

        this.callBootstrapMethod(aAddon.id, aAddon.version, aAddon.type, file,
                                 "uninstall",
                                 BOOTSTRAP_REASONS.ADDON_UNINSTALL);
        this.unloadBootstrapScope(aAddon.id);
        flushStartupCache();
      }
      aAddon._installLocation.uninstallAddon(aAddon.id);
      XPIDatabase.removeAddonMetadata(aAddon);
      AddonManagerPrivate.callAddonListeners("onUninstalled", wrapper);

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


function getHashStringForCrypto(aCrypto) {
  
  function toHexString(charCode)
    ("0" + charCode.toString(16)).slice(-2);

  
  let binary = aCrypto.finish(false);
  return [toHexString(binary.charCodeAt(i)) for (i in binary)].join("").toLowerCase()
}




















function AddonInstall(aInstallLocation, aUrl, aHash, aReleaseNotesURI,
                      aExistingAddon, aLoadGroup) {
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
  this.icons = {};
  this.existingAddon = aExistingAddon;
  this.error = 0;
  if (aLoadGroup)
    this.window = aLoadGroup.notificationCallbacks
                            .getInterface(Ci.nsIDOMWindow);
  else
    this.window = null;
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
  icons: null,
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

  





  initStagedInstall: function AI_initStagedInstall(aManifest) {
    this.name = aManifest.name;
    this.type = aManifest.type;
    this.version = aManifest.version;
    this.icons = aManifest.icons;
    this.releaseNotesURI = aManifest.releaseNotesURI ?
                           NetUtil.newURI(aManifest.releaseNotesURI) :
                           null
    this.sourceURI = aManifest.sourceURI ?
                     NetUtil.newURI(aManifest.sourceURI) :
                     null;
    this.file = null;
    this.addon = aManifest;

    this.state = AddonManager.STATE_INSTALLED;

    XPIProvider.installs.push(this);
  },

  





  initLocalInstall: function AI_initLocalInstall(aCallback) {
    this.file = this.sourceURI.QueryInterface(Ci.nsIFileURL).file;

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
      this.loadManifest(function  initLocalInstall_loadManifest() {
        XPIDatabase.getVisibleAddonForID(self.addon.id, function initLocalInstall_getVisibleAddon(aAddon) {
          self.existingAddon = aAddon;
          if (aAddon)
            applyBlocklistChanges(aAddon, self.addon);
          self.addon.updateDate = Date.now();
          self.addon.installDate = aAddon ? aAddon.installDate : self.addon.updateDate;

          if (!self.addon.isCompatible) {
            
            self.state = AddonManager.STATE_CHECKING;
            new UpdateChecker(self.addon, {
              onUpdateFinished: function updateChecker_onUpdateFinished(aAddon) {
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
  },

  













  initAvailableDownload: function AI_initAvailableDownload(aName, aType, aIcons, aVersion, aCallback) {
    this.state = AddonManager.STATE_AVAILABLE;
    this.name = aName;
    this.type = aType;
    this.version = aVersion;
    this.icons = aIcons;
    this.progress = 0;
    this.maxProgress = -1;

    XPIProvider.installs.push(this);
    AddonManagerPrivate.callInstallListeners("onNewInstall", this.listeners,
                                             this.wrapper);

    aCallback(this);
  },

  





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
    if (!this.listeners.some(function addListener_matchListener(i) { return i == aListener; }))
      this.listeners.push(aListener);
  },

  





  removeListener: function AI_removeListener(aListener) {
    this.listeners = this.listeners.filter(function removeListener_filterListener(i) {
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
        AddonInstall.createInstall(function loadMultipackageManifests_createInstall(aInstall) {
          
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
      
      AddonRepository.getCachedAddonByID(aAddon.id, function loadManifest_getCachedAddonByID(aRepoAddon) {
        if (aRepoAddon) {
          aAddon._repositoryAddon = aRepoAddon;
          aAddon.compatibilityOverrides = aRepoAddon.compatibilityOverrides;
          aAddon.appDisabled = !isUsableAddon(aAddon);
          aCallback();
          return;
        }

        
        AddonRepository.cacheAddons([aAddon.id], function loadManifest_cacheAddons() {
          AddonRepository.getCachedAddonByID(aAddon.id, function loadManifest_getCachedAddonByID(aRepoAddon) {
            aAddon._repositoryAddon = aRepoAddon;
            aAddon.compatibilityOverrides = aRepoAddon ?
                                              aRepoAddon.compatibilityOverrides :
                                              null;
            aAddon.appDisabled = !isUsableAddon(aAddon);
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
      this.loadMultipackageManifests(zipreader, function loadManifest_loadMultipackageManifests() {
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

  





  asyncOnChannelRedirect: function AI_asyncOnChannelRedirect(aOldChannel, aNewChannel, aFlags, aCallback) {
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
          this.loadManifest(function onStopRequest_loadManifest() {
            if (self.addon.isCompatible) {
              self.downloadCompleted();
            }
            else {
              
              self.state = AddonManager.STATE_CHECKING;
              new UpdateChecker(self.addon, {
                onUpdateFinished: function onStopRequest_onUpdateFinished(aAddon) {
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

  







  downloadFailed: function AI_downloadFailed(aReason, aError) {
    WARN("Download failed", aError);
    this.state = AddonManager.STATE_DOWNLOAD_FAILED;
    this.error = aReason;
    XPIProvider.removeActiveInstall(this);
    AddonManagerPrivate.callInstallListeners("onDownloadFailed", this.listeners,
                                             this.wrapper);

    
    
    if (this.state == AddonManager.STATE_DOWNLOAD_FAILED)
      this.removeTemporaryFile();
  },

  


  downloadCompleted: function AI_downloadCompleted() {
    let self = this;
    XPIDatabase.getVisibleAddonForID(this.addon.id, function downloadCompleted_getVisibleAddonForID(aAddon) {
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
                                              this.existingAddon.type, file,
                                              "shutdown", reason,
                                              { newVersion: this.addon.version });
            }

            XPIProvider.callBootstrapMethod(this.existingAddon.id,
                                            this.existingAddon.version,
                                            this.existingAddon.type, file,
                                            "uninstall", reason,
                                            { newVersion: this.addon.version });
            XPIProvider.unloadBootstrapScope(this.existingAddon.id);
            flushStartupCache();
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
                                       function startInstall_getAddonInLocation(a) {
          self.addon = a;
          let extraParams = {};
          if (self.existingAddon) {
            extraParams.oldVersion = self.existingAddon.version;
          }

          if (self.addon.bootstrap) {
            XPIProvider.callBootstrapMethod(self.addon.id, self.addon.version,
                                            self.addon.type, file, "install",
                                            reason, extraParams);
          }

          AddonManagerPrivate.callAddonListeners("onInstalled",
                                                 createWrapper(self.addon));

          LOG("Install of " + self.sourceURI.spec + " completed.");
          self.state = AddonManager.STATE_INSTALLED;
          AddonManagerPrivate.callInstallListeners("onInstallEnded",
                                                   self.listeners, self.wrapper,
                                                   createWrapper(self.addon));

          if (self.addon.bootstrap) {
            if (self.addon.active) {
              XPIProvider.callBootstrapMethod(self.addon.id, self.addon.version,
                                              self.addon.type, file, "startup",
                                              reason, extraParams);
            }
            else {
              XPIProvider.unloadBootstrapScope(self.addon.id);
            }
          }
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

  getInterface: function AI_getInterface(iid) {
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










AddonInstall.createStagedInstall = function AI_createStagedInstall(aInstallLocation, aDir, aManifest) {
  let url = Services.io.newFileURI(aDir);

  let install = new AddonInstall(aInstallLocation, aDir);
  install.initStagedInstall(aManifest);
};










AddonInstall.createInstall = function AI_createInstall(aCallback, aFile) {
  let location = XPIProvider.installLocationsByName[KEY_APP_PROFILE];
  let url = Services.io.newFileURI(aFile);

  try {
    let install = new AddonInstall(location, url);
    install.initLocalInstall(aCallback);
  }
  catch(e) {
    ERROR("Error creating install", e);
    aCallback(null);
  }
};



















AddonInstall.createDownload = function AI_createDownload(aCallback, aUri, aHash, aName, aIcons,
                                       aVersion, aLoadGroup) {
  let location = XPIProvider.installLocationsByName[KEY_APP_PROFILE];
  let url = NetUtil.newURI(aUri);

  let install = new AddonInstall(location, url, aHash, null, null, aLoadGroup);
  if (url instanceof Ci.nsIFileURL)
    install.initLocalInstall(aCallback);
  else
    install.initAvailableDownload(aName, null, aIcons, aVersion, aCallback);
};











AddonInstall.createUpdate = function AI_createUpdate(aCallback, aAddon, aUpdate) {
  let url = NetUtil.newURI(aUpdate.updateURL);
  let releaseNotesURI = null;
  try {
    if (aUpdate.updateInfoURL)
      releaseNotesURI = NetUtil.newURI(escapeAddonURI(aAddon, aUpdate.updateInfoURL));
  }
  catch (e) {
    
  }

  let install = new AddonInstall(aAddon._installLocation, url,
                                 aUpdate.updateHash, releaseNotesURI, aAddon);
  if (url instanceof Ci.nsIFileURL) {
    install.initLocalInstall(aCallback);
  }
  else {
    install.initAvailableDownload(aAddon.selectedLocale.name, aAddon.type,
                                  aAddon.icons, aUpdate.version, aCallback);
  }
};







function AddonInstallWrapper(aInstall) {
#ifdef MOZ_EM_DEBUG
  this.__defineGetter__("__AddonInstallInternal__", function AIW_debugGetter() {
    return aInstall;
  });
#endif

  ["name", "type", "version", "icons", "releaseNotesURI", "file", "state", "error",
   "progress", "maxProgress", "certificate", "certName"].forEach(function(aProp) {
    this.__defineGetter__(aProp, function AIW_propertyGetter() aInstall[aProp]);
  }, this);

  this.__defineGetter__("iconURL", function AIW_iconURL() aInstall.icons[32]);

  this.__defineGetter__("existingAddon", function AIW_existingAddonGetter() {
    return createWrapper(aInstall.existingAddon);
  });
  this.__defineGetter__("addon", function AIW_addonGetter() createWrapper(aInstall.addon));
  this.__defineGetter__("sourceURI", function AIW_sourceURIGetter() aInstall.sourceURI);

  this.__defineGetter__("linkedInstalls", function AIW_linkedInstallsGetter() {
    if (!aInstall.linkedInstalls)
      return null;
    return [i.wrapper for each (i in aInstall.linkedInstalls)];
  });

  this.install = function AIW_install() {
    aInstall.install();
  }

  this.cancel = function AIW_cancel() {
    aInstall.cancel();
  }

  this.addListener = function AIW_addListener(listener) {
    aInstall.addListener(listener);
  }

  this.removeListener = function AIW_removeListener(listener) {
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

  let updateURL = aAddon.updateURL;
  if (!updateURL) {
    if (aReason == AddonManager.UPDATE_WHEN_PERIODIC_UPDATE &&
        Services.prefs.getPrefType(PREF_EM_UPDATE_BACKGROUND_URL) == Services.prefs.PREF_STRING) {
      updateURL = Services.prefs.getCharPref(PREF_EM_UPDATE_BACKGROUND_URL);
    } else {
      updateURL = Services.prefs.getCharPref(PREF_EM_UPDATE_URL);
    }
  }

  const UPDATE_TYPE_COMPATIBILITY = 32;
  const UPDATE_TYPE_NEWVERSION = 64;

  aReason |= UPDATE_TYPE_COMPATIBILITY;
  if ("onUpdateAvailable" in this.listener)
    aReason |= UPDATE_TYPE_NEWVERSION;

  let url = escapeAddonURI(aAddon, updateURL, aReason, aAppVersion);
  AddonUpdateChecker.checkForUpdates(aAddon.id, aAddon.updateKey,
                                     url, this);
}

UpdateChecker.prototype = {
  addon: null,
  listener: null,
  appVersion: null,
  platformVersion: null,
  syncCompatibility: null,

  






  callListener: function UC_callListener(aMethod, ...aArgs) {
    if (!(aMethod in this.listener))
      return;

    try {
      this.listener[aMethod].apply(this.listener, aArgs);
    }
    catch (e) {
      LOG("Exception calling UpdateListener method " + aMethod + ": " + e);
    }
  },

  





  onUpdateCheckComplete: function UC_onUpdateCheckComplete(aUpdates) {
    let AUC = AddonUpdateChecker;

    let ignoreMaxVersion = false;
    let ignoreStrictCompat = false;
    if (!AddonManager.checkCompatibility) {
      ignoreMaxVersion = true;
      ignoreStrictCompat = true;
    } else if (this.addon.type in COMPATIBLE_BY_DEFAULT_TYPES &&
               !AddonManager.strictCompatibility &&
               !this.addon.strictCompatibility &&
               !this.addon.hasBinaryComponents) {
      ignoreMaxVersion = true;
    }

    
    let compatUpdate = AUC.getCompatibilityUpdate(aUpdates, this.addon.version,
                                                  this.syncCompatibility,
                                                  null, null,
                                                  ignoreMaxVersion,
                                                  ignoreStrictCompat);
    
    if (compatUpdate)
      this.addon.applyCompatibilityUpdate(compatUpdate, this.syncCompatibility);

    
    
    
    if ((this.appVersion &&
         Services.vc.compare(this.appVersion, Services.appinfo.version) != 0) ||
        (this.platformVersion &&
         Services.vc.compare(this.platformVersion, Services.appinfo.platformVersion) != 0)) {
      compatUpdate = AUC.getCompatibilityUpdate(aUpdates, this.addon.version,
                                                false, this.appVersion,
                                                this.platformVersion,
                                                ignoreMaxVersion,
                                                ignoreStrictCompat);
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

    let compatOverrides = AddonManager.strictCompatibility ?
                            null :
                            this.addon.compatibilityOverrides;

    let update = AUC.getNewestCompatibleUpdate(aUpdates,
                                               this.appVersion,
                                               this.platformVersion,
                                               ignoreMaxVersion,
                                               ignoreStrictCompat,
                                               compatOverrides);

    if (update && Services.vc.compare(this.addon.version, update.version) < 0) {
      for (let currentInstall of XPIProvider.installs) {
        
        if (currentInstall.existingAddon != this.addon ||
            currentInstall.version != update.version)
          continue;

        
        
        
        if (currentInstall.state == AddonManager.STATE_AVAILABLE)
          sendUpdateAvailableMessages(this, currentInstall);
        else
          sendUpdateAvailableMessages(this, null);
        return;
      }

      let self = this;
      AddonInstall.createUpdate(function onUpdateCheckComplete_createUpdate(aInstall) {
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
  foreignInstall: false,

  get isForeignInstall() {
    return this.foreignInstall;
  },
  set isForeignInstall(aVal) {
    this.foreignInstall = aVal;
  },

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

    for (let platform of this.targetPlatforms) {
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

  isCompatibleWith: function AddonInternal_isCompatibleWith(aAppVersion, aPlatformVersion) {
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

    
    
    if (this.type in COMPATIBLE_BY_DEFAULT_TYPES &&
        !AddonManager.strictCompatibility && !this.strictCompatibility &&
        !this.hasBinaryComponents) {

      
      
      if (this._repositoryAddon &&
          this._repositoryAddon.compatibilityOverrides) {
        let overrides = this._repositoryAddon.compatibilityOverrides;
        let override = AddonRepository.findMatchingCompatOverride(this.version,
                                                                  overrides);
        if (override && override.type == "incompatible")
          return false;
      }

      
      let minCompatVersion;
      if (app.id == Services.appinfo.ID)
        minCompatVersion = XPIProvider.minCompatibleAppVersion;
      else if (app.id == TOOLKIT_ID)
        minCompatVersion = XPIProvider.minCompatiblePlatformVersion;

      if (minCompatVersion &&
          Services.vc.compare(minCompatVersion, app.maxVersion) > 0)
        return false;

      return Services.vc.compare(version, app.minVersion) >= 0;
    }

    return (Services.vc.compare(version, app.minVersion) >= 0) &&
           (Services.vc.compare(version, app.maxVersion) <= 0)
  },

  get matchingTargetApplication() {
    let app = null;
    for (let targetApp of this.targetApplications) {
      if (targetApp.id == Services.appinfo.ID)
        return targetApp;
      if (targetApp.id == TOOLKIT_ID)
        app = targetApp;
    }
    return app;
  },

  get blocklistState() {
    let staticItem = findMatchingStaticBlocklistItem(this);
    if (staticItem)
      return staticItem.level;

    let bs = Cc["@mozilla.org/extensions/blocklist;1"].
             getService(Ci.nsIBlocklistService);
    return bs.getAddonBlocklistState(this.id, this.version);
  },

  get blocklistURL() {
    let staticItem = findMatchingStaticBlocklistItem(this);
    if (staticItem) {
      let url = Services.urlFormatter.formatURLPref("extensions.blocklist.itemURL");
      return url.replace(/%blockID%/g, staticItem.blockID);
    }

    let bs = Cc["@mozilla.org/extensions/blocklist;1"].
             getService(Ci.nsIBlocklistService);
    return bs.getAddonBlocklistURL(this.id, this.version);
  },

  applyCompatibilityUpdate: function AddonInternal_applyCompatibilityUpdate(aUpdate, aSyncCompatibility) {
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

  












  toJSON: function AddonInternal_toJSON(aKey) {
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
  },

  







  importMetadata: function AddonInternal_importMetaData(aObj) {
    ["syncGUID", "targetApplications", "userDisabled", "softDisabled",
     "existingAddonID", "sourceURI", "releaseNotesURI", "installDate",
     "updateDate", "applyBackgroundUpdates", "compatibilityOverrides"]
    .forEach(function(aProp) {
      if (!(aProp in aObj))
        return;

      this[aProp] = aObj[aProp];
    }, this);

    
    this.appDisabled = !isUsableAddon(this);
  }
};







function DBAddonInternal() {
  this.__defineGetter__("targetApplications", function DBA_targetApplicationsGetter() {
    delete this.targetApplications;
    return this.targetApplications = XPIDatabase._getTargetApplications(this);
  });

  this.__defineGetter__("targetPlatforms", function DBA_targetPlatformsGetter() {
    delete this.targetPlatforms;
    return this.targetPlatforms = XPIDatabase._getTargetPlatforms(this);
  });

  this.__defineGetter__("locales", function DBA_localesGetter() {
    delete this.locales;
    return this.locales = XPIDatabase._getLocales(this);
  });

  this.__defineGetter__("defaultLocale", function DBA_defaultLocaleGetter() {
    delete this.defaultLocale;
    return this.defaultLocale = XPIDatabase._getDefaultLocale(this);
  });

  this.__defineGetter__("pendingUpgrade", function DBA_pendingUpgradeGetter() {
    delete this.pendingUpgrade;
    for (let install of XPIProvider.installs) {
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
  applyCompatibilityUpdate: function DBA_applyCompatibilityUpdate(aUpdate, aSyncCompatibility) {
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

XPIProvider.DBAddonInternal = DBAddonInternal;









function createWrapper(aAddon) {
  if (!aAddon)
    return null;
  if (!aAddon._wrapper) {
    aAddon._hasResourceCache = new Map();
    aAddon._wrapper = new AddonWrapper(aAddon);
  }
  return aAddon._wrapper;
}





function AddonWrapper(aAddon) {
#ifdef MOZ_EM_DEBUG
  this.__defineGetter__("__AddonInternal__", function AW_debugGetter() {
    return aAddon;
  });
#endif

  function chooseValue(aObj, aProp) {
    let repositoryAddon = aAddon._repositoryAddon;
    let objValue = aObj[aProp];

    if (repositoryAddon && (aProp in repositoryAddon) &&
        (objValue === undefined || objValue === null)) {
      return [repositoryAddon[aProp], true];
    }

    return [objValue, false];
  }

  ["id", "syncGUID", "version", "type", "isCompatible", "isPlatformCompatible",
   "providesUpdatesSecurely", "blocklistState", "blocklistURL", "appDisabled",
   "softDisabled", "skinnable", "size", "foreignInstall", "hasBinaryComponents",
   "strictCompatibility", "compatibilityOverrides"].forEach(function(aProp) {
     this.__defineGetter__(aProp, function AddonWrapper_propertyGetter() aAddon[aProp]);
  }, this);

  ["fullDescription", "developerComments", "eula", "supportURL",
   "contributionURL", "contributionAmount", "averageRating", "reviewCount",
   "reviewURL", "totalDownloads", "weeklyDownloads", "dailyUsers",
   "repositoryStatus"].forEach(function(aProp) {
    this.__defineGetter__(aProp, function AddonWrapper_repoPropertyGetter() {
      if (aAddon._repositoryAddon)
        return aAddon._repositoryAddon[aProp];

      return null;
    });
  }, this);

  this.__defineGetter__("aboutURL", function AddonWrapper_aboutURLGetter() {
    return this.isActive ? aAddon["aboutURL"] : null;
  });

  ["installDate", "updateDate"].forEach(function(aProp) {
    this.__defineGetter__(aProp, function AddonWrapper_datePropertyGetter() new Date(aAddon[aProp]));
  }, this);

  ["sourceURI", "releaseNotesURI"].forEach(function(aProp) {
    this.__defineGetter__(aProp, function AddonWrapper_URIPropertyGetter() {
      let target = chooseValue(aAddon, aProp)[0];
      if (!target)
        return null;
      return NetUtil.newURI(target);
    });
  }, this);

  this.__defineGetter__("optionsURL", function AddonWrapper_optionsURLGetter() {
    if (this.isActive && aAddon.optionsURL)
      return aAddon.optionsURL;

    if (this.isActive && this.hasResource("options.xul"))
      return this.getResourceURI("options.xul").spec;

    return null;
  }, this);

  this.__defineGetter__("optionsType", function AddonWrapper_optionsTypeGetter() {
    if (!this.isActive)
      return null;

    let hasOptionsXUL = this.hasResource("options.xul");
    let hasOptionsURL = !!this.optionsURL;

    if (aAddon.optionsType) {
      switch (parseInt(aAddon.optionsType, 10)) {
      case AddonManager.OPTIONS_TYPE_DIALOG:
      case AddonManager.OPTIONS_TYPE_TAB:
        return hasOptionsURL ? aAddon.optionsType : null;
      case AddonManager.OPTIONS_TYPE_INLINE:
      case AddonManager.OPTIONS_TYPE_INLINE_INFO:
        return (hasOptionsXUL || hasOptionsURL) ? aAddon.optionsType : null;
      }
      return null;
    }

    if (hasOptionsXUL)
      return AddonManager.OPTIONS_TYPE_INLINE;

    if (hasOptionsURL)
      return AddonManager.OPTIONS_TYPE_DIALOG;

    return null;
  }, this);

  this.__defineGetter__("iconURL", function AddonWrapper_iconURLGetter() {
    return this.icons[32];
  }, this);

  this.__defineGetter__("icon64URL", function AddonWrapper_icon64URLGetter() {
    return this.icons[64];
  }, this);

  this.__defineGetter__("icons", function AddonWrapper_iconsGetter() {
    let icons = {};
    if (aAddon._repositoryAddon) {
      for (let size in aAddon._repositoryAddon.icons) {
        icons[size] = aAddon._repositoryAddon.icons[size];
      }
    }
    if (this.isActive && aAddon.iconURL) {
      icons[32] = aAddon.iconURL;
    } else if (this.hasResource("icon.png")) {
      icons[32] = this.getResourceURI("icon.png").spec;
    }
    if (this.isActive && aAddon.icon64URL) {
      icons[64] = aAddon.icon64URL;
    } else if (this.hasResource("icon64.png")) {
      icons[64] = this.getResourceURI("icon64.png").spec;
    }
    Object.freeze(icons);
    return icons;
  }, this);

  PROP_LOCALE_SINGLE.forEach(function(aProp) {
    this.__defineGetter__(aProp, function AddonWrapper_singleLocaleGetter() {
      
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
    this.__defineGetter__(aProp, function AddonWrapper_multiLocaleGetter() {
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
        results = results.map(function mapResult(aResult) {
          return new AddonManagerPrivate.AddonAuthor(aResult);
        });
      }

      return results;
    });
  }, this);

  this.__defineGetter__("screenshots", function AddonWrapper_screenshotsGetter() {
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

  this.__defineGetter__("applyBackgroundUpdates", function AddonWrapper_applyBackgroundUpdatesGetter() {
    return aAddon.applyBackgroundUpdates;
  });
  this.__defineSetter__("applyBackgroundUpdates", function AddonWrapper_applyBackgroundUpdatesSetter(val) {
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

  this.__defineSetter__("syncGUID", function AddonWrapper_syncGUIDGetter(val) {
    if (aAddon.syncGUID == val)
      return val;

    if (aAddon instanceof DBAddonInternal)
      XPIDatabase.setAddonSyncGUID(aAddon, val);

    aAddon.syncGUID = val;

    return val;
  });

  this.__defineGetter__("install", function AddonWrapper_installGetter() {
    if (!("_install" in aAddon) || !aAddon._install)
      return null;
    return aAddon._install.wrapper;
  });

  this.__defineGetter__("pendingUpgrade", function AddonWrapper_pendingUpgradeGetter() {
    return createWrapper(aAddon.pendingUpgrade);
  });

  this.__defineGetter__("scope", function AddonWrapper_scopeGetter() {
    if (aAddon._installLocation)
      return aAddon._installLocation.scope;

    return AddonManager.SCOPE_PROFILE;
  });

  this.__defineGetter__("pendingOperations", function AddonWrapper_pendingOperationsGetter() {
    let pending = 0;
    if (!(aAddon instanceof DBAddonInternal)) {
      
      
      
      
      if (!aAddon._install || aAddon._install.state == AddonManager.STATE_INSTALLING ||
          aAddon._install.state == AddonManager.STATE_INSTALLED)
        return AddonManager.PENDING_INSTALL;
    }
    else if (aAddon.pendingUninstall) {
      
      
      return AddonManager.PENDING_UNINSTALL;
    }

    if (aAddon.active && isAddonDisabled(aAddon))
      pending |= AddonManager.PENDING_DISABLE;
    else if (!aAddon.active && !isAddonDisabled(aAddon))
      pending |= AddonManager.PENDING_ENABLE;

    if (aAddon.pendingUpgrade)
      pending |= AddonManager.PENDING_UPGRADE;

    return pending;
  });

  this.__defineGetter__("operationsRequiringRestart", function AddonWrapper_operationsRequiringRestartGetter() {
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

  this.__defineGetter__("permissions", function AddonWrapper_permisionsGetter() {
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

  this.__defineGetter__("isActive", function AddonWrapper_isActiveGetter() {
    if (Services.appinfo.inSafeMode)
      return false;
    return aAddon.active;
  });

  this.__defineGetter__("userDisabled", function AddonWrapper_userDisabledGetter() {
    return aAddon.softDisabled || aAddon.userDisabled;
  });
  this.__defineSetter__("userDisabled", function AddonWrapper_userDisabledSetter(val) {
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

  this.__defineSetter__("softDisabled", function AddonWrapper_softDisabledSetter(val) {
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

  this.isCompatibleWith = function AddonWrapper_isCompatiblewith(aAppVersion, aPlatformVersion) {
    return aAddon.isCompatibleWith(aAppVersion, aPlatformVersion);
  };

  this.uninstall = function AddonWrapper_uninstall() {
    if (!(aAddon instanceof DBAddonInternal))
      throw new Error("Cannot uninstall an add-on that isn't installed");
    if (aAddon.pendingUninstall)
      throw new Error("Add-on is already marked to be uninstalled");
    XPIProvider.uninstallAddon(aAddon);
  };

  this.cancelUninstall = function AddonWrapper_cancelUninstall() {
    if (!(aAddon instanceof DBAddonInternal))
      throw new Error("Cannot cancel uninstall for an add-on that isn't installed");
    if (!aAddon.pendingUninstall)
      throw new Error("Add-on is not marked to be uninstalled");
    XPIProvider.cancelUninstallAddon(aAddon);
  };

  this.findUpdates = function AddonWrapper_findUpdates(aListener, aReason, aAppVersion, aPlatformVersion) {
    new UpdateChecker(aAddon, aListener, aReason, aAppVersion, aPlatformVersion);
  };

  this.hasResource = function AddonWrapper_hasResource(aPath) {
    if (aAddon._hasResourceCache.has(aPath))
      return aAddon._hasResourceCache.get(aPath);

    let bundle = aAddon._sourceBundle.clone();

    
    
    try {
      var isDir = bundle.isDirectory();
    } catch (e) {
      aAddon._hasResourceCache.set(aPath, false);
      return false;
    }

    if (isDir) {
      if (aPath) {
        aPath.split("/").forEach(function(aPart) {
          bundle.append(aPart);
        });
      }
      let result = bundle.exists();
      aAddon._hasResourceCache.set(aPath, result);
      return result;
    }

    let zipReader = Cc["@mozilla.org/libjar/zip-reader;1"].
                    createInstance(Ci.nsIZipReader);
    try {
      zipReader.open(bundle);
      let result = zipReader.hasEntry(aPath);
      aAddon._hasResourceCache.set(aPath, result);
      return result;
    }
    catch (e) {
      aAddon._hasResourceCache.set(aPath, false);
      return false;
    }
    finally {
      zipReader.close();
    }
  },

  










  this.getResourceURI = function AddonWrapper_getResourceURI(aPath) {
    if (!aPath)
      return NetUtil.newURI(aAddon._sourceBundle);

    return getURIForResourceInFile(aAddon._sourceBundle, aPath);
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
                            createInstance(Ci.nsIFile);

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
    
    
    
    let entries = getDirectoryEntries(this._directory);
    for (let entry of entries) {
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
        let newEntry = this._readDirectoryFromFile(entry);
        if (!newEntry) {
          LOG("Deleting stale pointer file " + entry.path);
          try {
            entry.remove(true);
          }
          catch (e) {
            WARN("Failed to remove stale pointer file " + entry.path, e);
            
          }
          continue;
        }

        entry = newEntry;
        this._linkedAddons.push(id);
      }

      this._IDToFileMap[id] = entry;
      this._FileToIDMap[entry.path] = id;
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
      locations.push(this._IDToFileMap[id].clone());
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
      let file = self._directory.clone();
      file.append(aId);

      if (file.exists())
        transaction.move(file, trashDir);

      file = self._directory.clone();
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

    let newFile = this._directory.clone();
    newFile.append(aSource.leafName);
    try {
      newFile.lastModifiedTime = Date.now();
    } catch (e)  {
      WARN("failed to set lastModifiedTime on " + newFile.path, e);
    }
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
      return this._IDToFileMap[aId].clone();
    throw new Error("Unknown add-on ID " + aId);
  },

  






  isLinkedAddon: function DirInstallLocation__isLinkedAddon(aId) {
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
                createInstance(Ci.nsIFile);
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
      locations.push(this._IDToFileMap[id].clone());
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
      return this._IDToFileMap[aId].clone();
    throw new Error("Unknown add-on ID");
  },

  


  isLinkedAddon: function RegInstallLocation_isLinkedAddon(aId) {
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
  new AddonManagerPrivate.AddonType("dictionary", URI_EXTENSION_STRINGS,
                                    STRING_TYPE_NAME,
                                    AddonManager.VIEW_TYPE_LIST, 7000,
                                    AddonManager.TYPE_UI_HIDE_EMPTY),
  new AddonManagerPrivate.AddonType("locale", URI_EXTENSION_STRINGS,
                                    STRING_TYPE_NAME,
                                    AddonManager.VIEW_TYPE_LIST, 8000,
                                    AddonManager.TYPE_UI_HIDE_EMPTY)
]);
