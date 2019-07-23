






































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

var EXPORTED_SYMBOLS = [];

Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/AddonManager.jsm");
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
const PREF_EM_CHECK_COMPATIBILITY     = "extensions.checkCompatibility";
const PREF_EM_CHECK_UPDATE_SECURITY   = "extensions.checkUpdateSecurity";
const PREF_EM_UPDATE_URL              = "extensions.update.url";
const PREF_EM_ENABLED_ADDONS          = "extensions.enabledAddons";
const PREF_EM_EXTENSION_FORMAT        = "extensions.";
const PREF_XPI_ENABLED                = "xpinstall.enabled";
const PREF_XPI_WHITELIST_REQUIRED     = "xpinstall.whitelist.required";

const DIR_EXTENSIONS                  = "extensions";
const DIR_STAGE                       = "staged";

const FILE_OLD_DATABASE               = "extensions.rdf";
const FILE_DATABASE                   = "extensions.sqlite";
const FILE_INSTALL_MANIFEST           = "install.rdf";
const FILE_XPI_ADDONS_LIST            = "extensions.ini";

const KEY_PROFILEDIR                  = "ProfD";
const KEY_APPDIR                      = "XCurProcD";

const KEY_APP_PROFILE                 = "app-profile";
const KEY_APP_GLOBAL                  = "app-global";
const KEY_APP_SYSTEM_LOCAL            = "app-system-local";
const KEY_APP_SYSTEM_SHARE            = "app-system-share";
const KEY_APP_SYSTEM_USER             = "app-system-user";

const UNKNOWN_XPCOM_ABI               = "unknownABI";
const PREFIX_ITEM_URI                 = "urn:mozilla:item:";
const XPI_PERMISSION                  = "install";

const TOOLKIT_ID                      = "toolkit@mozilla.org";

const BRANCH_REGEXP                   = /^([^\.]+\.[0-9]+[a-z]*).*/gi;

const DB_SCHEMA                       = 1;
const REQ_VERSION                     = 2;

const PROP_METADATA      = ["id", "version", "type", "internalName", "updateURL",
                            "updateKey", "optionsURL", "aboutURL", "iconURL"]
const PROP_LOCALE_SINGLE = ["name", "description", "creator", "homepageURL"];
const PROP_LOCALE_MULTI  = ["developers", "translators", "contributors"];
const PROP_TARGETAPP     = ["id", "minVersion", "maxVersion"];


const TYPES = {
  extension: 2,
  theme: 4,
  locale: 8,
  bootstrapped: 64
};




var gIDTest = /^(\{[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\}|[a-z0-9-\._]*\@[a-z0-9-\._]+)$/i;







function LOG(str) {
  dump("*** addons.xpi: " + str + "\n");
}







function WARN(str) {
  LOG(str);
}







function ERROR(str) {
  LOG(str);
}





function getLocale() {
  if (Prefs.getBoolPref(PREF_MATCH_OS_LOCALE), false)
    return Services.locale.getLocaleComponentForUserAgent();
  return Prefs.getCharPref(PREF_SELECTED_LOCALE, "en-US");
}








function findClosestLocale(locales) {
  let appLocale = getLocale();

  
  var bestmatch = null;
  
  var bestmatchcount = 0;
  
  var bestpartcount = 0;

  var matchLocales = [appLocale.toLowerCase()];
  

  if (matchLocales[0].substring(0, 3) != "en-")
    matchLocales.push("en-us");

  for each (var locale in matchLocales) {
    var lparts = locale.split("-");
    for each (var localized in locales) {
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








function isUsableAddon(addon) {
  
  if (addon.type == "theme" && addon.internalName == XPIProvider.defaultSkin)
    return true;
  if (XPIProvider.checkCompatibility) {
    if (!addon.isCompatible)
      return false;
  }
  else {
    if (!addon.matchingTargetApplication)
      return false;
  }
  if (XPIProvider.checkUpdateSecurity && !addon.providesUpdatesSecurely)
    return false;
  return addon.blocklistState != Ci.nsIBlocklistService.STATE_BLOCKED;
}












function loadManifestFromRDF(uri, stream) {
  let RDF = Cc["@mozilla.org/rdf/rdf-service;1"].
            getService(Ci.nsIRDFService);
  const RDFURI_INSTALL_MANIFEST_ROOT = "urn:mozilla:install-manifest";
  const PREFIX_NS_EM = "http://www.mozilla.org/2004/em-rdf#";

  function EM_R(property) {
    return RDF.GetResource(PREFIX_NS_EM + property);
  }

  function getValue(literal) {
    if (literal instanceof Ci.nsIRDFLiteral)
      return literal.Value;
    if (literal instanceof Ci.nsIRDFResource)
      return literal.Value;
    if (literal instanceof Ci.nsIRDFInt)
      return literal.Value;
    return null;
  }

  function getProperty(ds, source, property) {
    return getValue(ds.GetTarget(source, EM_R(property), true));
  }

  function getPropertyArray(ds, source, property) {
    let values = [];
    let targets = ds.GetTargets(source, EM_R(property), true);
    while (targets.hasMoreElements())
      values.push(getValue(targets.getNext()));

    return values;
  }

  function readLocale(ds, source, isDefault) {
    let locale = { };
    if (!isDefault) {
      locale.locales = [];
      let targets = ds.GetTargets(source, EM_R("locale"), true);
      while (targets.hasMoreElements())
        locale.locales.push(getValue(targets.getNext()));

      if (locale.locales.length == 0)
        throw new Error("No locales given for localized properties");
    }

    PROP_LOCALE_SINGLE.forEach(function(prop) {
      locale[prop] = getProperty(ds, source, prop);
    });

    PROP_LOCALE_MULTI.forEach(function(prop) {
      locale[prop] = getPropertyArray(ds, source,
                                      prop.substring(0, prop.length - 1));
    });

    return locale;
  }

  let rdfParser = Cc["@mozilla.org/rdf/xml-parser;1"].
                  createInstance(Ci.nsIRDFXMLParser)
  let ds = Cc["@mozilla.org/rdf/datasource;1?name=in-memory-datasource"].
           createInstance(Ci.nsIRDFDataSource);
  let listener = rdfParser.parseAsync(ds, uri);
  let channel = Cc["@mozilla.org/network/input-stream-channel;1"].
                createInstance(Ci.nsIInputStreamChannel);
  channel.setURI(uri);
  channel.contentStream = stream;
  channel.QueryInterface(Ci.nsIChannel);
  channel.contentType = "text/xml";

  listener.onStartRequest(channel, null);

  try {
    let pos = 0;
    let count = stream.available();
    while (count > 0) {
      listener.onDataAvailable(channel, null, stream, pos, count);
      pos += count;
      count = stream.available();
    }
    listener.onStopRequest(channel, null, Components.results.NS_OK);
  }
  catch (e) {
    listener.onStopRequest(channel, null, e.result);
    throw e;
  }

  let root = RDF.GetResource(RDFURI_INSTALL_MANIFEST_ROOT);
  let addon = new AddonInternal();
  PROP_METADATA.forEach(function(prop) {
    addon[prop] = getProperty(ds, root, prop);
  });
  if (!addon.id || !addon.version)
    throw new Error("No ID or version in install manifest");

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
  if (addon.type == "theme" && !addon.internalName)
    throw new Error("Themes must include an internalName property");

  addon.defaultLocale = readLocale(ds, root, true);

  addon.locales = [];
  let targets = ds.GetTargets(root, EM_R("localized"), true);
  while (targets.hasMoreElements()) {
    let target = targets.getNext().QueryInterface(Ci.nsIRDFResource);
    addon.locales.push(readLocale(ds, target, false));
  }

  addon.targetApplications = [];
  targets = ds.GetTargets(root, EM_R("targetApplication"), true);
  while (targets.hasMoreElements()) {
    let target = targets.getNext().QueryInterface(Ci.nsIRDFResource);
    let targetAppInfo = {};
    PROP_TARGETAPP.forEach(function(prop) {
      targetAppInfo[prop] = getProperty(ds, target, prop);
    });
    if (!targetAppInfo.id || !targetAppInfo.minVersion ||
        !targetAppInfo.maxVersion)
      throw new Error("Invalid targetApplication entry in install manifest");
    addon.targetApplications.push(targetAppInfo);
  }

  addon.targetPlatforms = getPropertyArray(ds, root, "targetPlatform");

  
  if (addon.type == "theme") {
    addon.userDisabled = addon.internalName != XPIProvider.selectedSkin;
  }
  else {
    addon.userDisabled = false;
  }
  addon.appDisabled = !isUsableAddon(addon);

  return addon;
}









function loadManifestFromDir(dir) {
  let file = dir.clone();
  file.append(FILE_INSTALL_MANIFEST);
  if (!file.exists() || !file.isFile())
    throw new Error("Directory " + dir.path + " does not contain a valid " +
                    "install manifest");

  let fis = Cc["@mozilla.org/network/file-input-stream;1"].
            createInstance(Ci.nsIFileInputStream);
  fis.init(file, -1, -1, false);
  let bis = Cc["@mozilla.org/network/buffered-input-stream;1"].
            createInstance(Ci.nsIBufferedInputStream);
  bis.init(fis, 4096);

  try {
    let addon = loadManifestFromRDF(Services.io.newFileURI(file), bis);
    addon._sourceBundle = dir.clone().QueryInterface(Ci.nsILocalFile);
    return addon;
  }
  finally {
    bis.close();
    fis.close();
  }
}










function buildJarURI(jarfile, path) {
  let uri = Services.io.newFileURI(jarfile);
  uri = "jar:" + uri.spec + "!/" + path;
  return NetUtil.newURI(uri);
}









function extractFiles(zipFile, dir) {
  function getTargetFile(dir, entry) {
    let target = dir.clone();
    entry.split("/").forEach(function(part) {
      target.append(part);
    });
    return target;
  }

  let zipReader = Cc["@mozilla.org/libjar/zip-reader;1"].
                  createInstance(Ci.nsIZipReader);
  zipReader.open(zipFile);

  try {
    
    let entries = zipReader.findEntries("*/");
    while (entries.hasMore()) {
      var entryName = entries.getNext();
      let target = getTargetFile(dir, entryName);
      if (!target.exists()) {
        try {
          target.create(Ci.nsILocalFile.DIRECTORY_TYPE,
                        FileUtils.PERMS_DIRECTORY);
        }
        catch (e) {
          ERROR("extractFiles: failed to create target directory for " +
                "extraction file = " + target.path + ", exception = " + e +
                "\n");
        }
      }
    }

    entries = zipReader.findEntries(null);
    while (entries.hasMore()) {
      let entryName = entries.getNext();
      let target = getTargetFile(dir, entryName);
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












function verifyZipSigning(zip, principal) {
  var count = 0;
  var entries = zip.findEntries(null);
  while (entries.hasMore()) {
    var entry = entries.getNext();
    
    if (entry.substr(0, 9) == "META-INF/")
      continue;
    
    if (entry.substr(-1) == "/")
      continue;
    count++;
    var entryPrincipal = zip.getCertificatePrincipal(entry);
    if (!entryPrincipal || !principal.equals(entryPrincipal))
      return false;
  }
  return zip.manifestEntriesCount == count;
}
















function escapeAddonURI(addon, uri, updateType, appVersion)
{
  var addonStatus = addon.userDisabled ? "userDisabled" : "userEnabled";

  if (!addon.isCompatible)
    addonStatus += ",incompatible";
  if (addon.blocklistState > 0)
    addonStatus += ",blocklisted";

  try {
    var xpcomABI = Services.appinfo.XPCOMABI;
  } catch (ex) {
    xpcomABI = UNKNOWN_XPCOM_ABI;
  }

  uri = uri.replace(/%ITEM_ID%/g, addon.id);
  uri = uri.replace(/%ITEM_VERSION%/g, addon.version);
  uri = uri.replace(/%ITEM_STATUS%/g, addonStatus);
  uri = uri.replace(/%APP_ID%/g, Services.appinfo.ID);
  uri = uri.replace(/%APP_VERSION%/g, appVersion ? appVersion :
                                                   Services.appinfo.version);
  uri = uri.replace(/%REQ_VERSION%/g, REQ_VERSION);
  uri = uri.replace(/%APP_OS%/g, Services.appinfo.OS);
  uri = uri.replace(/%APP_ABI%/g, xpcomABI);
  uri = uri.replace(/%APP_LOCALE%/g, getLocale());
  uri = uri.replace(/%CURRENT_APP_VERSION%/g, Services.appinfo.version);

  
  if (updateType)
    uri = uri.replace(/%UPDATE_TYPE%/g, updateType);

  
  
  
  let app = addon.matchingTargetApplication;
  if (app)
    var maxVersion = app.maxVersion;
  else
    maxVersion = "";
  uri = uri.replace(/%ITEM_MAXAPPVERSION%/g, maxVersion);

  
  
  var catMan = null;
  uri = uri.replace(/%(\w{3,})%/g, function(match, param) {
    if (!catMan) {
      catMan = Cc["@mozilla.org/categorymanager;1"].
               getService(Ci.nsICategoryManager);
    }

    try {
      var contractID = catMan.getCategoryEntry(CATEGORY_UPDATE_PARAMS, param);
      var paramHandler = Cc[contractID].getService(Ci.nsIPropertyBag2);
      return paramHandler.getPropertyAsAString(param);
    }
    catch(e) {
      return match;
    }
  });

  
  return uri.replace(/\+/g, "%2B");
}













function copyProperties(object, properties, target) {
  if (!target)
    target = {};
  properties.forEach(function(prop) {
    target[prop] = object[prop];
  });
  return target;
}













function copyRowProperties(row, properties, target) {
  if (!target)
    target = {};
  properties.forEach(function(prop) {
    target[prop] = row.getResultByName(prop);
  });
  return target;
}







function resultRows(statement) {
  try {
    while (statement.executeStep())
      yield statement.row;
  }
  finally {
    statement.reset();
  }
}





var Prefs = {
  









  getDefaultCharPref: function(name, defaultValue) {
    try {
      return Services.prefs.getDefaultBranch("").getCharPref(name);
    }
    catch (e) {
    }
    return defaultValue;
  },

  








  getCharPref: function(name, defaultValue) {
    try {
      return Services.prefs.getCharPref(name);
    }
    catch (e) {
    }
    return defaultValue;
  },

  








  getBoolPref: function(name, defaultValue) {
    try {
      return Services.prefs.getBoolPref(name);
    }
    catch (e) {
    }
    return defaultValue;
  },

  








  getIntPref: function(name, defaultValue) {
    try {
      return Services.prefs.getIntPref(name);
    }
    catch (e) {
    }
    return defaultValue;
  }
}

var XPIProvider = {
  
  installLocations: null,
  
  installLocationsByName: null,
  
  installs: null,
  
  defaultSkin: "classic/1.0",
  
  
  selectedSkin: null,
  
  
  checkCompatibilityPref: null,
  
  checkCompatibility: true,
  
  checkUpdateSecurity: true,
  
  bootstrappedAddons: {},
  
  bootstrapScopes: {},
  
  enabledAddons: null,

  


  startup: function XPI_startup() {
    LOG("startup");
    this.installs = [];
    this.installLocations = [];
    this.installLocationsByName = {};

    function addDirectoryInstallLocation(name, key, paths, locked) {
      try {
        var dir = FileUtils.getDir(key, paths);
      }
      catch (e) {
        
        LOG("Skipping unavailable install location " + name);
        return;
      }

      try {
        var location = new DirectoryInstallLocation(name, dir, locked);
      }
      catch (e) {
        WARN("Failed to add directory install location " + name + " " + e);
        return;
      }

      XPIProvider.installLocations.push(location);
      XPIProvider.installLocationsByName[location.name] = location;
    }

    function addRegistryInstallLocation(name, rootkey) {
      try {
        var location = new WinRegInstallLocation(name, rootkey);
      }
      catch (e) {
        WARN("Failed to add registry install location " + name + " " + e);
        return;
      }

      XPIProvider.installLocations.push(location);
      XPIProvider.installLocationsByName[location.name] = location;
    }

    let hasRegistry = ("nsIWindowsRegKey" in Ci);

    
    if (hasRegistry)
      addRegistryInstallLocation("winreg-app-global", Ci.nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE);
    addDirectoryInstallLocation(KEY_APP_SYSTEM_LOCAL, "XRESysLExtPD", [Services.appinfo.ID], true);
    addDirectoryInstallLocation(KEY_APP_SYSTEM_SHARE, "XRESysSExtPD", [Services.appinfo.ID], true);
    addDirectoryInstallLocation(KEY_APP_GLOBAL,       KEY_APPDIR,     [DIR_EXTENSIONS], true);
    if (hasRegistry)
      addRegistryInstallLocation("winreg-app-user", Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER);
    addDirectoryInstallLocation(KEY_APP_SYSTEM_USER,  "XREUSysExt",   [Services.appinfo.ID], true);
    addDirectoryInstallLocation(KEY_APP_PROFILE,      KEY_PROFILEDIR, [DIR_EXTENSIONS], false);

    this.defaultSkin = Prefs.getDefaultCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN,
                                                "classic/1.0");
    this.selectedSkin = Prefs.getCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN,
                                          this.defaultSkin);

    
    if (Prefs.getBoolPref(PREF_DSS_SWITCHPENDING, false)) {
      try {
        this.selectedSkin = Prefs.getCharPref(PREF_DSS_SKIN_TO_SELECT);
        Services.prefs.setCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN,
                                   this.selectedSkin);
        Services.prefs.clearUserPref(PREF_DSS_SKIN_TO_SELECT);
        LOG("Changed skin to " + this.selectedSkin);
      }
      catch (e) {
        ERROR(e);
      }
      Services.prefs.clearUserPref(PREF_DSS_SWITCHPENDING);
    }

    var version = Services.appinfo.version.replace(BRANCH_REGEXP, "$1");
    this.checkCompatibilityPref = PREF_EM_CHECK_COMPATIBILITY + "." + version;
    this.checkCompatibility = Prefs.getBoolPref(this.checkCompatibilityPref,
                                                true)
    this.checkUpdateSecurity = Prefs.getBoolPref(PREF_EM_CHECK_UPDATE_SECURITY,
                                                 true)
    this.enabledAddons = Prefs.getCharPref(PREF_EM_ENABLED_ADDONS, "");

    if ("nsICrashReporter" in Ci &&
        Services.appinfo instanceof Ci.nsICrashReporter) {
      
      try {
        Services.appinfo.annotateCrashReport("Theme", this.selectedSkin);
      } catch (e) { }
      try {
        Services.appinfo.annotateCrashReport("EMCheckCompatibility",
                                             this.checkCompatibility);
      } catch (e) { }
      this.addAddonsToCrashReporter();
    }

    Services.prefs.addObserver(this.checkCompatibilityPref, this, false);
    Services.prefs.addObserver(PREF_EM_CHECK_UPDATE_SECURITY, this, false);
  },

  


  shutdown: function XPI_shutdown() {
    LOG("shutdown");

    Services.prefs.removeObserver(this.checkCompatibilityPref, this);
    Services.prefs.removeObserver(PREF_EM_CHECK_UPDATE_SECURITY, this);

    if (Prefs.getBoolPref(PREF_PENDING_OPERATIONS, false)) {
      XPIDatabase.updateActiveAddons();
      Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, false);
    }
    XPIDatabase.shutdown();
    this.installs = null;

    this.installLocations = null;
    this.installLocationsByName = null;
  },

  


  addAddonsToCrashReporter: function XPI_addAddonsToCrashReporter() {
    if (!("nsICrashReporter" in Ci) ||
        !(Services.appinfo instanceof Ci.nsICrashReporter))
      return;

    let data = this.enabledAddons;
    for (let id in this.bootstrappedAddons)
      data += (data ? "," : "") + id + ":" + this.bootstrappedAddons[id].version;

    try {
      Services.appinfo.annotateCrashReport("Add-ons", data);
    }
    catch (e) { }
  },

  









  getAddonStates: function XPI_getAddonStates(location) {
    let addonStates = {};
    location.addonLocations.forEach(function(dir) {
      let id = location.getIDForLocation(dir);
      addonStates[id] = {
        descriptor: dir.persistentDescriptor,
        mtime: dir.lastModifiedTime
      };
    });

    return addonStates;
  },

  








  getInstallLocationStates: function XPI_getInstallLocationStates() {
    let states = [];
    this.installLocations.forEach(function(location) {
      let addons = location.addonLocations;
      if (addons.length == 0)
        return;

      let locationState = {
        name: location.name,
        addons: this.getAddonStates(location)
      };

      states.push(locationState);
    }, this);
    return states;
  },

  








  processPendingFileChanges: function XPI_processPendingFileChanges(manifests) {
    
    let changed = false;
    this.installLocations.forEach(function(location) {
      manifests[location.name] = {};
      
      if (location.locked)
        return;

      let stagingDir = location.getStagingDir();
      if (!stagingDir || !stagingDir.exists())
        return;

      let entries = stagingDir.directoryEntries;
      while (entries.hasMoreElements()) {
        let stageDirEntry = entries.getNext().QueryInterface(Ci.nsILocalFile);

        
        if (!stageDirEntry.isDirectory()) {
          WARN("Ignoring file: " + stageDirEntry.path);
          continue;
        }

        
        let id = stageDirEntry.leafName;
        if (!gIDTest.test(id)) {
          WARN("Ignoring directory whose name is not a valid add-on ID: " +
               stageDirEntry.path);
          continue;
        }

        
        let manifest = stageDirEntry.clone();
        manifest.append(FILE_INSTALL_MANIFEST);

        
        
        if (!manifest.exists()) {
          LOG("Processing uninstall of " + id + " in " + location.name);
          location.uninstallAddon(id);
          
          changed = true;
          continue;
        }

        LOG("Processing install of " + id + " in " + location.name);
        try {
          var addonInstallDir = location.installAddon(id, stageDirEntry);
        }
        catch (e) {
          ERROR("Failed to install staged add-on " + id + " in " + location.name +
                ": " + e);
          continue;
        }

        manifests[location.name][id] = null;
        changed = true;

        
        
        let jsonfile = stagingDir.clone();
        jsonfile.append(id + ".json");
        if (jsonfile.exists()) {
          LOG("Found updated manifest for " + id + " in " + location.name);
          let fis = Cc["@mozilla.org/network/file-input-stream;1"].
                       createInstance(Ci.nsIFileInputStream);
          let json = Cc["@mozilla.org/dom/json;1"].
                     createInstance(Ci.nsIJSON);

          try {
            fis.init(jsonfile, -1, 0, 0);
            manifests[location.name][id] = json.decodeFromStream(fis,
                                                                 jsonfile.fileSize);
            manifests[location.name][id]._sourceBundle = addonInstallDir;
          }
          catch (e) {
            ERROR("Unable to read add-on manifest for " + id + " in " +
                  location.name + ": " + e);
          }
          finally {
            fis.close();
          }
        }
      }

      try {
        stagingDir.remove(true);
      }
      catch (e) {
        
        LOG("Error removing staging dir " + stagingDir.path + ": " + e);
      }
    });
    return changed;
  },

  














  processFileChanges: function XPI_processFileChanges(state, manifests,
                                                      updateCompatibility,
                                                      migrateData) {
    let visibleAddons = {};

    














    function updateMetadata(installLocation, oldAddon, addonState) {
      LOG("Add-on " + oldAddon.id + " modified in " + installLocation.name);

      
      let newAddon = manifests[installLocation.name][oldAddon.id];

      try {
        
        if (!newAddon) {
          let dir = installLocation.getLocationForID(oldAddon.id);
          newAddon = loadManifestFromDir(dir);
        }

        
        
        if (newAddon.id != oldAddon.id)
          throw new Error("Incorrect id in install manifest");
      }
      catch (e) {
        WARN("Add-on is invalid: " + e);
        XPIDatabase.removeAddonMetadata(oldAddon);
        if (!installLocation.locked)
          installLocation.uninstallAddon(oldAddon.id);
        else
          WARN("Could not uninstall invalid item from locked install location");
        
        if (oldAddon.active) {
          if (oldAddon.type == "bootstrapped")
            delete XPIProvider.bootstrappedAddons[oldAddon.id];
          else
            return true;
        }

        return false;
      }

      
      newAddon._installLocation = installLocation;
      newAddon.userDisabled = oldAddon.userDisabled;
      newAddon.installDate = oldAddon.installDate;
      newAddon.updateDate = addonState.mtime;
      newAddon.visible = !(newAddon.id in visibleAddons);

      
      XPIDatabase.updateAddonMetadata(oldAddon, newAddon, addonState.descriptor);
      if (newAddon.visible) {
        visibleAddons[newAddon.id] = newAddon;
        
        
        
        if ((oldAddon.active && oldAddon.type != "bootstrapped") ||
            (newAddon.active && newAddon.type != "bootstrapped")) {
          return true;
        }
      }

      return false;
    }

    















    function updateVisibilityAndCompatibility(installLocation, oldAddon,
                                              addonState) {
      let changed = false;

      
      if (!(oldAddon.id in visibleAddons)) {
        visibleAddons[oldAddon.id] = oldAddon;

        if (!oldAddon.visible) {
          XPIDatabase.makeAddonVisible(oldAddon);

          
          
          if (oldAddon.type == "bootstrapped" && !oldAddon.appDisabled &&
              !oldAddon.userDisabled) {
            oldAddon.active = true;
            XPIDatabase.updateAddonActive(oldAddon);
            XPIProvider.bootstrappedAddons[oldAddon.id] = {
              version: oldAddon.version,
              descriptor: addonState.descriptor
            };
          }
          else {
            
            changed = true;
          }
        }
      }

      
      if (updateCompatibility) {
        let appDisabled = !isUsableAddon(oldAddon);

        
        if (appDisabled != oldAddon.appDisabled) {
          LOG("Add-on " + oldAddon.id + " changed appDisabled state to " +
              appDisabled);
          XPIDatabase.setAddonProperties(oldAddon, {
            appDisabled: appDisabled
          });

          
          
          if (oldAddon.visible && !oldAddon.userDisabled) {
            if (oldAddon.type == "bootstrapped") {
              
              
              oldAddon.active = !oldAddon.appDisabled;
              XPIDatabase.updateAddonActive(oldAddon);
              if (oldAddon.active) {
                XPIProvider.bootstrappedAddons[oldAddon.id] = {
                  version: oldAddon.version,
                  descriptor: addonState.descriptor
                };
              }
              else {
                delete XPIProvider.bootstrappedAddons[oldAddon.id];
              }
            }
            else {
              changed = true;
            }
          }
        }
      }

      return changed;
    }

    










    function removeMetadata(installLocation, oldAddon) {
      
      LOG("Add-on " + oldAddon.id + " removed from " + installLocation.name);
      XPIDatabase.removeAddonMetadata(oldAddon);
      if (oldAddon.active) {

        
        
        if (oldAddon.type == "theme")
          XPIProvider.enableDefaultTheme();

        
        
        if (oldAddon.type != "bootstrapped")
          return true;

        delete XPIProvider.bootstrappedAddons[oldAddon.id];
      }

      return false;
    }

    














    function addMetadata(installLocation, id, addonState, migrateData) {
      LOG("New add-on " + id + " installed in " + installLocation.name);

      
      let newAddon = manifests[installLocation.name][id];

      try {
        
        if (!newAddon)
          newAddon = loadManifestFromDir(installLocation.getLocationForID(id));
        
        if (newAddon.id != id)
          throw new Error("Incorrect id in install manifest");
      }
      catch (e) {
        WARN("Add-on is invalid: " + e);

        
        
        if (!installLocation.locked)
          installLocation.uninstallAddon(id);
        else
          WARN("Could not uninstall invalid item from locked install location");
        return false;
      }

      
      newAddon._installLocation = installLocation;
      newAddon.visible = !(newAddon.id in visibleAddons);
      newAddon.installDate = addonState.mtime;
      newAddon.updateDate = addonState.mtime;

      
      if (migrateData) {
        newAddon.userDisabled = migrateData.userDisabled;
        if ("installDate" in migrateData)
          newAddon.installDate = migrateData.installDate;
      }

      
      XPIDatabase.addAddonMetadata(newAddon, addonState.descriptor);

      
      if (newAddon.visible) {
        visibleAddons[newAddon.id] = newAddon;
        if (newAddon.type != "bootstrapped")
          return true;

        XPIProvider.bootstrappedAddons[newAddon.id] = {
          version: newAddon.version,
          descriptor: addonState.descriptor
        };
      }

      return false;
    }

    let changed = false;
    let knownLocations = XPIDatabase.getInstallLocations();

    
    
    
    state.reverse().forEach(function(st) {

      
      
      let installLocation = this.installLocationsByName[st.name];
      let addonStates = st.addons;

      
      let pos = knownLocations.indexOf(installLocation.name);
      if (pos >= 0) {
        knownLocations.splice(pos, 1);
        let addons = XPIDatabase.getAddonsInLocation(installLocation.name);
        
        
        addons.forEach(function(oldAddon) {
          
          if (oldAddon.id in addonStates) {
            let addonState = addonStates[oldAddon.id];
            delete addonStates[oldAddon.id];

            
            
            
            if (oldAddon.id in manifests[installLocation.name] ||
                oldAddon.updateDate != addonState.mtime ||
                oldAddon._descriptor != addonState.descriptor) {
              changed = updateMetadata(installLocation, oldAddon, addonState) ||
                        changed;
            }
            else {
              changed = updateVisibilityAndCompatibility(installLocation,
                                                         oldAddon, addonState) ||
                        changed;
            }
          }
          else {
            changed = removeMetadata(installLocation, oldAddon) || changed;
          }
        }, this);
      }

      

      
      let locMigrateData = {};
      if (migrateData && installLocation.name in migrateData)
        locMigrateData = migrateData[installLocation.name];
      for (let id in addonStates) {
        changed = addMetadata(installLocation, id, addonStates[id],
                              locMigrateData[id]) || changed;
      }
    }, this);

    
    
    
    
    knownLocations.forEach(function(location) {
      let addons = XPIDatabase.getAddonsInLocation(location);
      addons.forEach(function(oldAddon) {
        changed = removeMetadata(location, oldAddon) || changed;
      }, this);
    }, this);

    
    cache = JSON.stringify(this.getInstallLocationStates());
    Services.prefs.setCharPref(PREF_INSTALL_CACHE, cache);
    return changed;
  },

  








  checkForChanges: function XPI_checkForChanges(appChanged) {
    LOG("checkForChanges");

    
    
    let manifests = {};
    let changed = this.processPendingFileChanges(manifests);

    
    
    let schema = Prefs.getIntPref(PREF_DB_SCHEMA, 0);

    let migrateData = null;
    let cache = null;
    if (schema != DB_SCHEMA) {
      
      migrateData = XPIDatabase.migrateData(schema);
    }
    else {
      
      let db = FileUtils.getFile(KEY_PROFILEDIR, [FILE_DATABASE], true);
      if (db.exists())
        cache = Prefs.getCharPref(PREF_INSTALL_CACHE, null);
    }

    
    
    this.bootstrappedAddons = JSON.parse(Prefs.getCharPref(PREF_BOOTSTRAP_ADDONS,
                                         "{}"));
    let state = this.getInstallLocationStates();
    if (appChanged || changed || cache == null ||
        cache != JSON.stringify(state)) {
      try {
        changed = this.processFileChanges(state, manifests, appChanged,
                                          migrateData);
      }
      catch (e) {
        ERROR("Error processing file changes: " + e);
      }
    }

    
    
    if (changed || Prefs.getBoolPref(PREF_PENDING_OPERATIONS)) {
      LOG("Restart necessary");
      XPIDatabase.updateActiveAddons();
      Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, false);
      return true;
    }

    LOG("No changes found");

    
    let addonsList = FileUtils.getFile(KEY_PROFILEDIR, [FILE_XPI_ADDONS_LIST],
                                       true);
    if (!addonsList.exists()) {
      LOG("Add-ons list is missing, recreating");
      XPIDatabase.writeAddonsList();
      return true;
    }

    let bootstrappedAddons = this.bootstrappedAddons;
    this.bootstrappedAddons = {};
    for (let id in bootstrappedAddons) {
      let dir = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      dir.persistentDescriptor = bootstrappedAddons[id].descriptor;
      this.activateAddon(id, bootstrappedAddons[id].version, dir, true, false);
    }

    
    
    Services.obs.addObserver({
      observe: function(subject, topic, data) {
        Services.prefs.setCharPref(PREF_BOOTSTRAP_ADDONS,
                                   JSON.stringify(XPIProvider.bootstrappedAddons));
        for (let id in XPIProvider.bootstrappedAddons)
          XPIProvider.deactivateAddon(id, true, false);
        Services.obs.removeObserver(this, "quit-application-granted");
      }
    }, "quit-application-granted", false);

    return false;
  },

  







  supportsMimetype: function XPI_supportsMimetype(mimetype) {
    return mimetype == "application/x-xpinstall";
  },

  




  isInstallEnabled: function XPI_isInstallEnabled() {
    
    return Prefs.getBoolPref(PREF_XPI_ENABLED, true);
  },

  






  isInstallAllowed: function XPI_isInstallAllowed(uri) {
    if (!this.isInstallEnabled())
      return false;

    if (!uri)
      return true;

    
    if (uri.schemeIs("chrome") || uri.schemeIs("file"))
      return true;


    let permission = Services.perms.testPermission(uri, XPI_PERMISSION);
    if (permission == Ci.nsIPermissionManager.DENY_ACTION)
      return false;

    let requireWhitelist = Prefs.getBoolPref(PREF_XPI_WHITELIST_REQUIRED, true);
    if (requireWhitelist && (permission != Ci.nsIPermissionManager.ALLOW_ACTION))
      return false;

    return true;
  },

  

















  getInstallForURL: function XPI_getInstallForURL(url, hash, name, iconURL,
                                                  version, loadGroup, callback) {
    AddonInstall.createDownload(function(install) {
      callback(install.wrapper);
    }, url, hash, name, iconURL, version, loadGroup);
  },

  







  getInstallForFile: function XPI_getInstallForFile(file, callback) {
    AddonInstall.createInstall(function(install) {
      if (install)
        callback(install.wrapper);
      else
        callback(null);
    }, file);
  },

  







  getAddon: function XPI_getAddon(id, callback) {
    XPIDatabase.getVisibleAddonForID(id, function(addon) {
      if (addon)
        callback(createWrapper(addon));
      else
        callback(null);
    });
  },

  







  getAddonsByTypes: function XPI_getAddonsByTypes(types, callback) {
    XPIDatabase.getVisibleAddons(types, function(addons) {
      callback([createWrapper(a) for each (a in addons)]);
    });
  },

  







  getAddonsWithPendingOperations:
  function XPI_getAddonsWithPendingOperations(types, callback) {
    XPIDatabase.getVisibleAddonsWithPendingOperations(types, function(addons) {
      let results = [createWrapper(a) for each (a in addons)];
      XPIProvider.installs.forEach(function(install) {
        if (install.state == AddonManager.STATE_INSTALLED &&
            !(install.addon instanceof DBAddonInternal))
          results.push(createWrapper(install.addon));
      });
      callback(results);
    });
  },

  








  getInstalls: function XPI_getInstalls(types, callback) {
    let results = [];
    this.installs.forEach(function(install) {
      if (!types || types.indexOf(install.type) >= 0)
        results.push(install.wrapper);
    });
    callback(results);
  },

  











  addonChanged: function XPI_addonChanged(id, type, pendingRestart) {
    
    if (type != "theme")
      return;

    if (!id) {
      
      this.enableDefaultTheme();
      return;
    }

    
    
    let previousTheme = null;
    let newSkin = this.defaultSkin;
    let addons = XPIDatabase.getAddonsByType("theme");
    addons.forEach(function(a) {
      if (!a.visible)
        return;
      if (a.id == id)
        newSkin = a.internalName;
      else if (a.userDisabled == false && !a.pendingUninstall)
        previousTheme = a;
    }, this);

    if (pendingRestart) {
      Services.prefs.setBoolPref(PREF_DSS_SWITCHPENDING, true);
      Services.prefs.setCharPref(PREF_DSS_SKIN_TO_SELECT, newSkin);
    }
    else {
      Services.prefs.setCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN, newSkin);
    }
    this.selectedSkin = newSkin;

    
    
    if (previousTheme)
      this.updateAddonDisabledState(previousTheme, true);
  },

  



  enableDefaultTheme: function XPI_enableDefaultTheme() {
    LOG("Activating default theme");
    let addons = XPIDatabase.getAddonsByType("theme");
    addons.forEach(function(a) {
      
      
      if (a.internalName == this.defaultSkin && a.visible)
        this.updateAddonDisabledState(a, false);
    }, this);
  },

  




  observe: function XPI_observe(subject, topic, data) {
    switch (data) {
    case this.checkCompatibilityPref:
    case PREF_EM_CHECK_UPDATE_SECURITY:
      this.checkCompatibility = Prefs.getBoolPref(this.checkCompatibilityPref,
                                                  true);
      this.checkUpdateSecurity = Prefs.getBoolPref(PREF_EM_CHECK_UPDATE_SECURITY,
                                                   true);
      this.updateAllAddonDisabledStates();
      break;
    }
  },

  






  enableRequiresRestart: function XPI_enableRequiresRestart(addon) {
    
    
    if (addon.type == "theme")
      return addon.internalName !=
             Prefs.getCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN);

    return addon.type != "bootstrapped";
  },

  






  disableRequiresRestart: function XPI_disableRequiresRestart(addon) {
    
    
    
    
    if (addon.type == "theme")
      return this.selectedSkin !=
             Prefs.getCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN);

    return addon.type != "bootstrapped";
  },

  






  installRequiresRestart: function XPI_installRequiresRestart(addon) {
    
    if (addon.type == "theme")
      return addon.internalName ==
             Prefs.getCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN);

    return addon.type != "bootstrapped";
  },

  






  uninstallRequiresRestart: function XPI_uninstallRequiresRestart(addon) {
    
    if (addon.type == "theme")
      return addon.internalName ==
             Prefs.getCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN);

    return addon.type != "bootstrapped";
  },

  










  callBootstrapMethod: function XPI_callBootstrapMethod(id, scope, methods) {
    for (let i = 0; i < methods.length; i++) {
      if (methods[i] in scope) {
        LOG("Calling bootstrap method " + methods[i] + " on " + id);

        try {
          scope[methods[i]](id);
        }
        catch (e) {
          WARN("Exception running bootstrap method " + methods[i] + " on " +
               id + ": " + e);
        }
        return;
      }
    }
  },

  















  activateAddon: function XPI_activateAddon(id, version, dir, startup, install) {
    let methods = ["enable"];
    if (startup)
      methods.unshift("startup");
    if (install)
      methods.unshift("install");
    let bootstrap = dir.clone();
    bootstrap.append("bootstrap.js");
    if (bootstrap.exists()) {
      let uri = Services.io.newFileURI(bootstrap);
      let principal = Cc["@mozilla.org/systemprincipal;1"].
                      createInstance(Ci.nsIPrincipal);
      let scope = new Components.utils.Sandbox(principal);
      let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
                   createInstance(Ci.mozIJSSubScriptLoader);

      
      this.bootstrappedAddons[id] = {
        version: version,
        descriptor: dir.persistentDescriptor
      };
      this.bootstrapScopes[id] = scope;
      this.addAddonsToCrashReporter();

      try {
        loader.loadSubScript(uri.spec, scope);
      }
      catch (e) {
        WARN("Error loading bootstrap.js for " + id + ": " + e);
      }

      this.callBootstrapMethod(id, scope, methods);
    }
    else {
      WARN("Bootstrap missing for " + id);
    }
  },

  











  deactivateAddon: function XPI_deactivateAddon(id, shutdown, uninstall) {
    if (!(id in this.bootstrappedAddons)) {
      ERROR("Attempted to deactivate an add-on that was never activated");
      return;
    }
    let scope = this.bootstrapScopes[id];
    delete this.bootstrappedAddons[id];
    delete this.bootstrapScopes[id];

    let methods = ["disable"];
    if (shutdown)
      methods.unshift("shutdown");
    if (uninstall)
      methods.unshift("uninstall");
    this.callBootstrapMethod(id, scope, methods);

    this.addAddonsToCrashReporter();
  },

  


  updateAllAddonDisabledStates: function XPI_updateAllAddonDisabledStates() {
    let addons = XPIDatabase.getAddons();
    addons.forEach(function(addon) {
      this.updateAddonDisabledState(addon);
    }, this);
  },

  











  updateAddonDisabledState: function XPI_updateAddonDisabledState(addon,
                                                                  userDisabled) {
    if (!(addon instanceof DBAddonInternal))
      throw new Error("Can only update addon states for installed addons.");

    if (userDisabled === undefined)
      userDisabled = addon.userDisabled;

    let appDisabled = !isUsableAddon(addon);
    
    if (addon.userDisabled == userDisabled &&
        addon.appDisabled == appDisabled)
      return;

    let wasDisabled = addon.userDisabled || addon.appDisabled;
    let isDisabled = userDisabled || appDisabled;

    
    XPIDatabase.setAddonProperties(addon, {
      userDisabled: userDisabled,
      appDisabled: appDisabled
    });

    
    
    if (!addon.visible || (wasDisabled == isDisabled))
      return;

    
    Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, true);

    let wrapper = createWrapper(addon);
    
    if (isDisabled != addon.active) {
      AddonManagerPrivate.callAddonListeners("onOperationCancelled", wrapper);
    }
    else {
      if (isDisabled) {
        var needsRestart = this.disableRequiresRestart(addon);
        AddonManagerPrivate.callAddonListeners("onDisabling", wrapper,
                                               needsRestart);
      }
      else {
        needsRestart = this.enableRequiresRestart(addon);
        AddonManagerPrivate.callAddonListeners("onEnabling", wrapper,
                                               needsRestart);
      }

      if (!needsRestart) {
        addon.active = !isDisabled;
        XPIDatabase.updateAddonActive(addon);
        if (isDisabled) {
          if (addon.type == "bootstrapped")
            this.deactivateAddon(addon.id, false, false);
          AddonManagerPrivate.callAddonListeners("onDisabled", wrapper);
        }
        else {
          if (addon.type == "bootstrapped") {
            let dir = addon._installLocation.getLocationForID(addon.id);
            this.activateAddon(addon.id, addon.version, dir, false, false);
          }
          AddonManagerPrivate.callAddonListeners("onEnabled", wrapper);
        }
      }
    }

    
    if (addon.type == "theme" && !isDisabled)
      AddonManagerPrivate.notifyAddonChanged(addon.id, addon.type, true);
  },

  








  uninstallAddon: function XPI_uninstallAddon(addon) {
    if (!(addon instanceof DBAddonInternal))
      throw new Error("Can only uninstall installed addons.");

    if (addon._installLocation.locked)
      throw new Error("Cannot uninstall addons from locked install locations");

    
    let requiresRestart = addon.active && this.uninstallRequiresRestart(addon);

    if (requiresRestart) {
      
      
      let stage = addon._installLocation.getStagingDir();
      stage.append(addon.id);
      if (!stage.exists())
        stage.create(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);

      XPIDatabase.setAddonProperties(addon, {
        pendingUninstall: true
      });
      Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, true);
    }

    
    if (!addon.visible)
      return;

    let wrapper = createWrapper(addon);
    AddonManagerPrivate.callAddonListeners("onUninstalling", wrapper,
                                           requiresRestart);

    if (!requiresRestart) {
      if (addon.type == "bootstrapped")
        this.deactivateAddon(addon.id, false, true);
      addon._installLocation.uninstallAddon(addon.id);
      XPIDatabase.removeAddonMetadata(addon);
      AddonManagerPrivate.callAddonListeners("onUninstalled", wrapper);
      
    }

    
    if (addon.type == "theme" && addon.active)
      AddonManagerPrivate.notifyAddonChanged(null, addon.type, requiresRestart);
  },

  





  cancelUninstallAddon: function XPI_cancelUninstallAddon(addon) {
    if (!(addon instanceof DBAddonInternal))
      throw new Error("Can only cancel uninstall for installed addons.");

    let stagedAddon = addon._installLocation.getStagingDir();
    stagedAddon.append(addon.id);
    if (stagedAddon.exists())
      stagedAddon.remove(true);

    XPIDatabase.setAddonProperties(addon, {
      pendingUninstall: false
    });

    if (!addon.visible)
      return;

    Services.prefs.setBoolPref(PREF_PENDING_OPERATIONS, true);

    
    let wrapper = createWrapper(addon);
    AddonManagerPrivate.callAddonListeners("onOperationCancelled", wrapper);

    
    if (addon.type == "theme" && addon.active)
      AddonManagerPrivate.notifyAddonChanged(addon.id, addon.type, false);
  }
};

const FIELDS_ADDON = "internal_id, id, location, version, type, internalName, " +
                     "updateURL, updateKey, optionsURL, aboutURL, iconURL, " +
                     "defaultLocale, visible, active, userDisabled, appDisabled, " +
                     "pendingUninstall, descriptor, installDate, updateDate";


function asyncErrorLogger(error) {
  ERROR("SQL error " + error.result + ": " + error.message);
}

var XPIDatabase = {
  
  initialized: false,
  
  statementCache: {},
  
  
  addonCache: [],

  
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
    _readLocaleStrings: "SELECT locale_id, type, value FROM locale_strings " +
                        "WHERE locale_id=:id",

    addAddonMetadata_addon: "INSERT INTO addon VALUES (NULL, :id, :location, " +
                            ":version, :type, :internalName, :updateURL, " +
                            ":updateKey, :optionsURL, :aboutURL, :iconURL, " +
                            ":locale, :visible, :active, :userDisabled," +
                            " :appDisabled, 0, :descriptor, :installDate, " +
                            ":updateDate)",
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

    clearVisibleAddons: "UPDATE addon SET visible=0 WHERE id=:id",
    deactivateThemes: "UPDATE addon SET active=:active WHERE " +
                      "internal_id=:internal_id",

    getActiveAddons: "SELECT " + FIELDS_ADDON + " FROM addon WHERE active=1 AND " +
                     "type<>'theme' AND type<>'bootstrapped'",
    getActiveTheme: "SELECT " + FIELDS_ADDON + " FROM addon WHERE " +
                    "internalName=:internalName AND type='theme'",

    getAddonInLocation: "SELECT " + FIELDS_ADDON + " FROM addon WHERE id=:id " +
                        "AND location=:location",
    getAddons: "SELECT " + FIELDS_ADDON + " FROM addon",
    getAddonsByType: "SELECT " + FIELDS_ADDON + " FROM addon WHERE type=:type",
    getAddonsInLocation: "SELECT " + FIELDS_ADDON + " FROM addon WHERE " +
                         "location=:location",
    getInstallLocations: "SELECT DISTINCT location FROM addon",
    getVisibleAddonForID: "SELECT " + FIELDS_ADDON + " FROM addon WHERE " +
                          "visible=1 AND id=:id",
    getVisibleAddons: "SELECT " + FIELDS_ADDON + " FROM addon WHERE visible=1",
    getVisibleAddonsWithPendingOperations: "SELECT " + FIELDS_ADDON + " FROM " +
                                           "addon WHERE visible=1 " +
                                           "AND (pendingUninstall=1 OR " +
                                           "MAX(userDisabled,appDisabled)=active)",

    makeAddonVisible: "UPDATE addon SET visible=1 WHERE internal_id=:internal_id",
    removeAddonMetadata: "DELETE FROM addon WHERE internal_id=:internal_id",
    
    setActiveAddons: "UPDATE addon SET active=MIN(visible, 1 - userDisabled, " +
                     "1 - appDisabled, 1 - pendingUninstall)",
    setAddonProperties: "UPDATE addon SET userDisabled=:userDisabled, " +
                        "appDisabled=:appDisabled, " +
                        "pendingUninstall=:pendingUninstall WHERE " +
                        "internal_id=:internal_id",
    updateTargetApplications: "UPDATE targetApplication SET " +
                              "minVersion=:minVersion, maxVersion=:maxVersion " +
                              "WHERE addon_internal_id=:internal_id AND id=:id",
  },

  




  openConnection: function XPIDB_openConnection() {
    this.initialized = true;
    let dbfile = FileUtils.getFile(KEY_PROFILEDIR, [FILE_DATABASE], true);
    delete this.connection;
    return this.connection = Services.storage.openDatabase(dbfile);
  },

  


  get connection() {
    return this.openConnection();
  },

  







  migrateData: function XPIDB_migrateData(oldSchema) {
    LOG("Migrating data from schema " + oldSchema);
    let migrateData = {};

    if (oldSchema == 0) {
      
      let rdffile = FileUtils.getFile(KEY_PROFILEDIR, [FILE_OLD_DATABASE], true);
      if (rdffile.exists()) {
        let RDF = Cc["@mozilla.org/rdf/rdf-service;1"].
                  getService(Ci.nsIRDFService);
        const PREFIX_NS_EM = "http://www.mozilla.org/2004/em-rdf#";

        function EM_R(property) {
          return RDF.GetResource(PREFIX_NS_EM + property);
        }
        let ds = RDF.GetDataSourceBlocking(Services.io.newFileURI(rdffile).spec);

        
        ["true", "needs-disable"].forEach(function(val) {
          let sources = ds.GetSources(EM_R("userDisabled"), RDF.GetLiteral(val),
                                      true);
          while (sources.hasMoreElements()) {
            let source = sources.getNext().QueryInterface(Ci.nsIRDFResource);
            let location = ds.GetTarget(source, EM_R("installLocation"), true);
            if (location instanceof Ci.nsIRDFLiteral) {
              if (!(location.Value in migrateData))
                migrateData[location.Value] = {};
              let id = source.ValueUTF8.substring(PREFIX_ITEM_URI.length);
              migrateData[location.Value][id] = {
                userDisabled: true
              }
            }
          }
        });
      }
    }
    else {
      
      
      try {
        var stmt = this.connection.createStatement("SELECT id, location, " +
                                                   "userDisabled, installDate " +
                                                   "FROM addon");
        for (let row in resultRows(stmt)) {
          if (!(row.location in migrateData))
            migrateData[row.location] = {};
          migrateData[row.location][row.id] = {
            installDate: row.installDate,
            userDisabled: row.userDisabled == 1
          };
        }
      }
      catch (e) {
        
        ERROR("Error migrating data: " + e);
      }
      finally {
        stmt.finalize();
      }
      this.connection.close();
    }

    
    let dbfile = FileUtils.getFile(KEY_PROFILEDIR, [FILE_DATABASE], true);
    if (dbfile.exists())
      dbfile.remove(true);
    this.openConnection();
    this.createSchema();

    return migrateData;
  },

  


  shutdown: function XPIDB_shutdown() {
    if (this.initialized) {
      for each (let stmt in this.statementCache)
        stmt.finalize();
      this.statementCache = {};
      this.addonCache = [];
      this.connection.asyncClose();
      this.initialized = false;
      delete this.connection;

      
      
      this.__defineGetter__("connection", function() {
        return this.openConnection();
      });
    }
  },

  










  getStatement: function XPIDB_getStatement(key, sql) {
    if (key in this.statementCache)
      return this.statementCache[key];
    if (!sql)
      sql = this.statements[key];

    try {
      return this.statementCache[key] = this.connection.createStatement(sql);
    }
    catch (e) {
      ERROR("Error creating statement " + key + " (" + sql + ")");
      throw e;
    }
  },

  


  createSchema: function XPIDB_createSchema() {
    LOG("Creating database schema");
    this.connection.createTable("addon",
                                "internal_id INTEGER PRIMARY KEY AUTOINCREMENT, " +
                                "id TEXT, location TEXT, version TEXT, " +
                                "type TEXT, internalName TEXT, updateURL TEXT, " +
                                "updateKey TEXT, optionsURL TEXT, aboutURL TEXT, " +
                                "iconURL TEXT, defaultLocale INTEGER, " +
                                "visible INTEGER, active INTEGER, " +
                                "userDisabled INTEGER, appDisabled INTEGER, " +
                                "pendingUninstall INTEGER, descriptor TEXT, " +
                                "installDate INTEGER, updateDate INTEGER, " +
                                "UNIQUE (id, location)");
    this.connection.createTable("targetApplication",
                                "addon_internal_id INTEGER, " +
                                "id TEXT, minVersion TEXT, maxVersion TEXT, " +
                                "UNIQUE (addon_internal_id, id)");
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
    Services.prefs.setIntPref(PREF_DB_SCHEMA, DB_SCHEMA);
  },

  





  _readLocaleStrings: function XPIDB__readLocaleStrings(locale) {
    let stmt = this.getStatement("_readLocaleStrings");

    stmt.params.id = locale.id;
    for (let row in resultRows(stmt)) {
      if (!(row.type in locale))
        locale[row.type] = [];
      locale[row.type].push(row.value);
    }
  },

  






  _getLocales: function XPIDB__getLocales(addon) {
    let stmt = this.getStatement("_getLocales");

    let locales = [];
    stmt.params.internal_id = addon._internal_id;
    for (let row in resultRows(stmt)) {
      let locale = {
        id: row.id,
        locales: [row.locale]
      };
      copyProperties(row, PROP_LOCALE_SINGLE, locale);
      locales.push(locale);
    }
    locales.forEach(function(locale) {
      this._readLocaleStrings(locale);
    }, this);
    return locales;
  },

  







  _getDefaultLocale: function XPIDB__getDefaultLocale(addon) {
    let stmt = this.getStatement("_getDefaultLocale");

    stmt.params.id = addon._defaultLocale;
    if (!stmt.executeStep())
      throw new Error("Missing default locale for " + addon.id);
    let locale = copyProperties(stmt.row, PROP_LOCALE_SINGLE);
    locale.id = addon._defaultLocale;
    stmt.reset();
    this._readLocaleStrings(locale);
    return locale;
  },

  






  _getTargetApplications: function XPIDB__getTargetApplications(addon) {
    let stmt = this.getStatement("_getTargetApplications");

    stmt.params.internal_id = addon._internal_id;
    return [copyProperties(row, PROP_TARGETAPP) for each (row in resultRows(stmt))];
  },

  







  makeAddonFromRow: function XPIDB_makeAddonFromRow(row) {
    if (this.addonCache[row.internal_id]) {
      let addon = this.addonCache[row.internal_id].get();
      if (addon)
        return addon;
    }

    let addon = new DBAddonInternal();
    addon._internal_id = row.internal_id;
    addon._installLocation = XPIProvider.installLocationsByName[row.location];
    addon._descriptor = row.descriptor;
    copyProperties(row, PROP_METADATA, addon);
    addon._defaultLocale = row.defaultLocale;
    addon.installDate = row.installDate;
    addon.updateDate = row.updateDate;
    ["visible", "active", "userDisabled", "appDisabled",
     "pendingUninstall"].forEach(function(prop) {
      addon[prop] = row[prop] != 0;
    });
    this.addonCache[row.internal_id] = Components.utils.getWeakReference(addon);
    return addon;
  },

  







  fetchAddonMetadata: function XPIDB_fetchAddonMetadata(addon, callback) {
    function readLocaleStrings(locale, callback) {
      let stmt = XPIDatabase.getStatement("_readLocaleStrings");

      stmt.params.id = locale.id;
      stmt.executeAsync({
        handleResult: function(results) {
          let row = null;
          while (row = results.getNextRow()) {
            let type = row.getResultByName("type");
            if (!(type in locale))
              locale[type] = [];
            locale[type].push(row.getResultByName("value"));
          }
        },

        handleError: asyncErrorLogger,

        handleCompletion: function(reason) {
          callback();
        }
      });
    }

    function readDefaultLocale() {
      delete addon.defaultLocale;
      let stmt = XPIDatabase.getStatement("_getDefaultLocale");

      stmt.params.id = addon._defaultLocale;
      stmt.executeAsync({
        handleResult: function(results) {
          addon.defaultLocale = copyRowProperties(results.getNextRow(),
                                                  PROP_LOCALE_SINGLE);
          addon.defaultLocale.id = addon._defaultLocale;
        },

        handleError: asyncErrorLogger,

        handleCompletion: function(reason) {
          if (addon.defaultLocale) {
            readLocaleStrings(addon.defaultLocale, readLocales);
          }
          else {
            ERROR("Missing default locale for " + addon.id);
            readLocales();
          }
        }
      });
    }

    function readLocales() {
      delete addon.locales;
      addon.locales = [];
      let stmt = XPIDatabase.getStatement("_getLocales");

      stmt.params.internal_id = addon._internal_id;
      stmt.executeAsync({
        handleResult: function(results) {
          let row = null;
          while (row = results.getNextRow()) {
            let locale = {
              id: row.getResultByName("id"),
              locales: [row.getResultByName("locale")]
            };
            copyRowProperties(row, PROP_LOCALE_SINGLE, locale);
            addon.locales.push(locale);
          }
        },

        handleError: asyncErrorLogger,

        handleCompletion: function(reason) {
          let pos = 0;
          function readNextLocale() {
            if (pos < addon.locales.length)
              readLocaleStrings(addon.locales[pos++], readNextLocale);
            else
              readTargetApplications();
          }

          readNextLocale();
        }
      });
    }

    function readTargetApplications() {
      delete addon.targetApplications;
      addon.targetApplications = [];
      let stmt = XPIDatabase.getStatement("_getTargetApplications");

      stmt.params.internal_id = addon._internal_id;
      stmt.executeAsync({
        handleResult: function(results) {
          let row = null;
          while (row = results.getNextRow())
            addon.targetApplications.push(copyRowProperties(row, PROP_TARGETAPP));
        },

        handleError: asyncErrorLogger,

        handleCompletion: function(reason) {
          callback(addon);
        }
      });
    }

    readDefaultLocale();
  },

  







  makeAddonFromRowAsync: function XPIDB_makeAddonFromRowAsync(row) {
    let internal_id = row.getResultByName("internal_id");
    if (this.addonCache[internal_id]) {
      let addon = this.addonCache[internal_id].get();
      if (addon)
        return addon;
    }

    let addon = new DBAddonInternal();
    addon._internal_id = internal_id;
    let location = row.getResultByName("location");
    addon._installLocation = XPIProvider.installLocationsByName[location];
    addon._descriptor = row.getResultByName("descriptor");
    copyRowProperties(row, PROP_METADATA, addon);
    addon._defaultLocale = row.getResultByName("defaultLocale");
    addon.installDate = row.getResultByName("installDate");
    addon.updateDate = row.getResultByName("updateDate");
    ["visible", "active", "userDisabled", "appDisabled",
     "pendingUninstall"].forEach(function(prop) {
      addon[prop] = row.getResultByName(prop) != 0;
    });
    this.addonCache[internal_id] = Components.utils.getWeakReference(addon);
    return addon;
  },

  







  getInstallLocations: function XPIDB_getInstallLocations() {
    let stmt = this.getStatement("getInstallLocations");

    return [row.location for each (row in resultRows(stmt))];
  },

  






  getAddonsInLocation: function XPIDB_getAddonsInLocation(location) {
    let stmt = this.getStatement("getAddonsInLocation");

    stmt.params.location = location;
    return [this.makeAddonFromRow(row) for each (row in resultRows(stmt))];
  },

  










  getAddonInLocation: function XPIDB_getAddonInLocation(id, location, callback) {
    let stmt = this.getStatement("getAddonInLocation");

    stmt.params.id = id;
    stmt.params.location = location;
    stmt.executeAsync({
      addon: null,

      handleResult: function(results) {
        this.addon = XPIDatabase.makeAddonFromRowAsync(results.getNextRow());
      },

      handleError: asyncErrorLogger,

      handleCompletion: function(reason) {
        if (this.addon)
          XPIDatabase.fetchAddonMetadata(this.addon, callback);
        else
          callback(null);
      }
    });
  },

  







  getVisibleAddonForID: function XPIDB_getVisibleAddonForID(id, callback) {
    let stmt = this.getStatement("getVisibleAddonForID");

    stmt.params.id = id;
    stmt.executeAsync({
      addon: null,

      handleResult: function(results) {
        this.addon = XPIDatabase.makeAddonFromRowAsync(results.getNextRow());
      },

      handleError: asyncErrorLogger,

      handleCompletion: function(reason) {
        if (this.addon)
          XPIDatabase.fetchAddonMetadata(this.addon, callback);
        else
          callback(null);
      }
    });
  },

  







  getVisibleAddons: function XPIDB_getVisibleAddons(types, callback) {
    let stmt = null;
    if (!types || types.length == 0) {
      stmt = this.getStatement("getVisibleAddons");
    }
    else {
      let sql = "SELECT * FROM addon WHERE visible=1 AND type IN (";
      for (let i = 1; i <= types.length; i++) {
        sql += "?" + i;
        if (i < types.length)
          sql += ",";
      }
      sql += ")";

      
      stmt = this.getStatement("getVisibleAddons_" + types.length, sql);
      for (let i = 0; i < types.length; i++)
        stmt.bindStringParameter(i, types[i]);
    }

    let addons = [];
    stmt.executeAsync({
      handleResult: function(results) {
        let row = null;
        while (row = results.getNextRow())
          addons.push(XPIDatabase.makeAddonFromRowAsync(row));
      },

      handleError: asyncErrorLogger,

      handleCompletion: function(reason) {
        let pos = 0;
        function readNextAddon() {
          if (pos < addons.length)
            XPIDatabase.fetchAddonMetadata(addons[pos++], readNextAddon);
          else
            callback(addons);
        }

        readNextAddon();
      }
    });
  },

  






  getAddonsByType: function XPIDB_getAddonsByType(type) {
    let stmt = this.getStatement("getAddonsByType");

    stmt.params.type = type;
    return [this.makeAddonFromRow(row) for each (row in resultRows(stmt))];;
  },

  







  getVisibleAddonsWithPendingOperations:
    function XPIDB_getVisibleAddonsWithPendingOperations(types, callback) {
    let stmt = null;
    if (!types || types.length == 0) {
      stmt = this.getStatement("getVisibleAddonsWithPendingOperations");
    }
    else {
      let sql = "SELECT * FROM addon WHERE visible=1 AND " +
                "(pendingUninstall=1 OR MAX(userDisabled,appDisabled)=active) " +
                "AND type IN (";
      for (let i = 1; i <= types.length; i++) {
        sql += "?" + i;
        if (i < types.length)
          sql += ",";
      }
      sql += ")";

      
      stmt = this.getStatement("getVisibleAddonsWithPendingOperations_" +
                               types.length, sql);
      for (let i = 0; i < types.length; i++)
        stmt.bindStringParameter(i, types[i]);
    }

    let addons = [];
    stmt.executeAsync({
      handleResult: function(results) {
        let row = null;
        while (row = results.getNextRow())
          addons.push(XPIDatabase.makeAddonFromRowAsync(row));
      },

      handleError: asyncErrorLogger,

      handleCompletion: function(reason) {
        let pos = 0;
        function readNextAddon() {
          if (pos < addons.length)
            XPIDatabase.fetchAddonMetadata(addons[pos++], readNextAddon);
          else
            callback(addons);
        }

        readNextAddon();
      }
    });
  },

  




  getAddons: function XPIDB_getAddons() {
    let stmt = this.getStatement("getAddons");

    return [this.makeAddonFromRow(row) for each (row in resultRows(stmt))];;
  },

  







  addAddonMetadata: function XPIDB_addAddonMetadata(addon, descriptor) {
    let localestmt = this.getStatement("addAddonMetadata_locale");
    let stringstmt = this.getStatement("addAddonMetadata_strings");

    function insertLocale(locale) {
      copyProperties(locale, PROP_LOCALE_SINGLE, localestmt.params);
      localestmt.execute();
      let row = XPIDatabase.connection.lastInsertRowID;

      PROP_LOCALE_MULTI.forEach(function(prop) {
        locale[prop].forEach(function(str) {
          stringstmt.params.locale = row;
          stringstmt.params.type = prop;
          stringstmt.params.value = str;
          stringstmt.execute();
        });
      });
      return row;
    }

    if (addon.visible) {
      let stmt = this.getStatement("clearVisibleAddons");
      stmt.params.id = addon.id;
      stmt.execute();
    }

    let stmt = this.getStatement("addAddonMetadata_addon");

    stmt.params.locale = insertLocale(addon.defaultLocale);
    stmt.params.location = addon._installLocation.name;
    stmt.params.descriptor = descriptor;
    stmt.params.installDate = addon.installDate;
    stmt.params.updateDate = addon.updateDate;
    copyProperties(addon, PROP_METADATA, stmt.params);
    ["visible", "userDisabled", "appDisabled"].forEach(function(prop) {
      stmt.params[prop] = addon[prop] ? 1 : 0;
    });
    stmt.params.active = (addon.visible && !addon.userDisabled &&
                          !addon.appDisabled) ? 1 : 0;
    stmt.execute();
    let internal_id = this.connection.lastInsertRowID;

    stmt = this.getStatement("addAddonMetadata_addon_locale");
    addon.locales.forEach(function(locale) {
      let id = insertLocale(locale);
      locale.locales.forEach(function(name) {
        stmt.params.internal_id = internal_id;
        stmt.params.name = name;
        stmt.params.locale = insertLocale(locale);
        stmt.execute();
      });
    });

    stmt = this.getStatement("addAddonMetadata_targetApplication");

    addon.targetApplications.forEach(function(app) {
      stmt.params.internal_id = internal_id;
      stmt.params.id = app.id;
      stmt.params.minVersion = app.minVersion;
      stmt.params.maxVersion = app.maxVersion;
      stmt.execute();
    });
  },

  










  updateAddonMetadata: function XPIDB_updateAddonMetadata(oldAddon, newAddon,
                                                          descriptor) {
    this.removeAddonMetadata(oldAddon);
    this.addAddonMetadata(newAddon, descriptor);
  },

  







  updateTargetApplications: function XPIDB_updateTargetApplications(addon,
                                                                    targets) {
    let stmt = this.getStatement("updateTargetApplications");
    targets.forEach(function(target) {
      stmt.params.internal_id = addon._internal_id;
      stmt.params.id = target.id;
      stmt.params.minVersion = target.minVersion;
      stmt.params.maxVersion = target.maxVersion;
      stmt.execute();
    });
  },

  





  removeAddonMetadata: function XPIDB_removeAddonMetadata(addon) {
    let stmt = this.getStatement("removeAddonMetadata");
    stmt.params.internal_id = addon._internal_id;
    stmt.execute();
  },

  










  makeAddonVisible: function XPIDB_makeAddonVisible(addon) {
    let stmt = this.getStatement("clearVisibleAddons");
    stmt.params.id = addon.id;
    stmt.execute();

    stmt = this.getStatement("makeAddonVisible");
    stmt.params.internal_id = addon._internal_id;
    stmt.execute();

    addon.visible = true;
  },

  







  setAddonProperties: function XPIDB_setAddonProperties(addon, properties) {
    function convertBoolean(value) {
      return value ? 1 : 0;
    }

    let stmt = this.getStatement("setAddonProperties");
    stmt.params.internal_id = addon._internal_id;

    if ("userDisabled" in properties) {
      stmt.params.userDisabled = convertBoolean(properties.userDisabled);
      addon.userDisabled = properties.userDisabled;
    }
    else {
      stmt.params.userDisabled = convertBoolean(addon.userDisabled);
    }

    if ("appDisabled" in properties) {
      stmt.params.appDisabled = convertBoolean(properties.appDisabled);
      addon.appDisabled = properties.appDisabled;
    }
    else {
      stmt.params.appDisabled = convertBoolean(addon.appDisabled);
    }

    if ("pendingUninstall" in properties) {
      stmt.params.pendingUninstall = convertBoolean(properties.pendingUninstall);
      addon.pendingUninstall = properties.pendingUninstall;
    }
    else {
      stmt.params.pendingUninstall = convertBoolean(addon.pendingUninstall);
    }

    stmt.execute();
  },

  





  updateAddonActive: function XPIDB_updateAddonActive(addon) {
    LOG("Updating add-on state");

    stmt = this.getStatement("deactivateThemes");
    stmt.params.internal_id = addon._internal_id;
    stmt.params.active = addon.active ? 1 : 0;
    stmt.execute();
  },

  


  updateActiveAddons: function XPIDB_updateActiveAddons() {
    LOG("Updating add-on states");
    let stmt = this.getStatement("setActiveAddons");
    stmt.execute();

    this.writeAddonsList();
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
    stmt = this.getStatement("getActiveTheme");
    stmt.params.internalName = XPIProvider.selectedSkin;
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































function AddonInstall(callback, installLocation, url, hash, name, type, iconURL,
                      version, infoURL, existingAddon, loadGroup) {
  this.wrapper = new AddonInstallWrapper(this);
  this.installLocation = installLocation;
  this.sourceURL = url;
  this.loadGroup = loadGroup;
  this.listeners = [];
  this.existingAddon = existingAddon;

  if (url instanceof Ci.nsIFileURL) {
    this.file = url.file.QueryInterface(Ci.nsILocalFile);
    this.state = AddonManager.STATE_DOWNLOADED;
    this.progress = this.file.fileSize;
    this.maxProgress = this.file.fileSize;

    if (this.hash) {
      let crypto = Cc["@mozilla.org/security/hash;1"].
                   createInstance(Ci.nsICryptoHash);
      let fis = Cc["@mozilla.org/network/file-input-stream;1"].
                createInstance(Ci.nsIFileInputStream);
      fis.init(this.file, -1, -1, false);
      crypto.updateFromStream(fis, this.file.fileSize);
      let hash = crypto.finish(true);
      if (hash != this.hash)
        throw new Error("Hash mismatch");
    }

    this.loadManifest();
    this.name = this.addon.selectedLocale.name;
    this.type = this.addon.type;
    this.version = this.addon.version;
    this.iconURL = this.addon.iconURL;

    let self = this;
    XPIDatabase.getVisibleAddonForID(this.addon.id, function(addon) {
      self.existingAddon = addon;

      if (!self.addon.isCompatible) {
        
        this.state = AddonManager.STATE_CHECKING;
        new UpdateChecker(self.addon, {
          onUpdateFinished: function(addon) {
            XPIProvider.installs.push(self);
            AddonManagerPrivate.callInstallListeners("onNewInstall",
                                                     self.listeners,
                                                     self.wrapper);

            callback(self);
          }
        }, AddonManager.UPDATE_WHEN_ADDON_INSTALLED);
      }
      else {
        XPIProvider.installs.push(self);
        AddonManagerPrivate.callInstallListeners("onNewInstall", self.listeners,
                                                 self.wrapper);

        callback(self);
      }
    });
  }
  else {
    this.state = AddonManager.STATE_AVAILABLE;
    this.name = name;
    this.type = type;
    this.version = version;
    this.iconURL = iconURL;
    this.progress = 0;
    this.maxProgress = -1;
    this.hash = hash;

    XPIProvider.installs.push(this);
    AddonManagerPrivate.callInstallListeners("onNewInstall", this.listeners,
                                             this.wrapper);

    callback(this);
  }
}

AddonInstall.prototype = {
  installLocation: null,
  wrapper: null,
  stream: null,
  crypto: null,
  hash: null,
  loadGroup: null,
  listeners: null,

  name: null,
  type: null,
  version: null,
  iconURL: null,
  infoURL: null,
  sourceURL: null,
  file: null,
  certificate: null,
  certName: null,

  existingAddon: null,
  addon: null,

  state: null,
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
      this.channel.cancel(Cr.NS_BINDING_ABORTED);
    case AddonManager.STATE_AVAILABLE:
    case AddonManager.STATE_DOWNLOADED:
      LOG("Cancelling download of " + this.sourceURL.spec);
      this.state = AddonManager.STATE_CANCELLED;
      AddonManagerPrivate.callInstallListeners("onDownloadCancelled",
                                               this.listeners, this.wrapper);
      if (this.file && !(this.sourceURL instanceof Ci.nsIFileURL))
        this.file.remove(true);
      break;
    case AddonManager.STATE_INSTALLED:
      LOG("Cancelling install of " + this.addon.id);
      let stagedAddon = this.installLocation.getStagingDir();
      let stagedJSON = stagedAddon.clone();
      stagedAddon.append(this.addon.id);
      stagedJSON.append(this.addon.id + ".json");
      if (stagedAddon.exists())
        stagedAddon.remove(true);
      if (stagedJSON.exists())
        stagedJSON.remove(true);
      this.state = AddonManager.STATE_CANCELLED;
      AddonManagerPrivate.callInstallListeners("onInstallCancelled",
                                               this.listeners, this.wrapper);
      break;
    default:
      throw new Error("Cannot cancel from this state");
    }
  },

  






  addListener: function AI_addListener(listener) {
    if (!this.listeners.some(function(i) { return i == listener; }))
      this.listeners.push(listener);
  },

  





  removeListener: function AI_removeListener(listener) {
    this.listeners = this.listeners.filter(function(i) {
      return i != listener;
    });
  },

  






  loadManifest: function AI_loadManifest() {
    let zipreader = Cc["@mozilla.org/libjar/zip-reader;1"].
                    createInstance(Ci.nsIZipReader);
    zipreader.open(this.file);

    try {
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
          throw new Error("XPI is incorrectly signed");
        }
      }

      if (!zipreader.hasEntry(FILE_INSTALL_MANIFEST)) {
        zipreader.close();
        throw new Error("Missing install.rdf");
      }

      let zis = zipreader.getInputStream(FILE_INSTALL_MANIFEST);
      let bis = Cc["@mozilla.org/network/buffered-input-stream;1"].
                createInstance(Ci.nsIBufferedInputStream);
      bis.init(zis, 4096);

      try {
        uri = buildJarURI(this.file, FILE_INSTALL_MANIFEST);
        this.addon = loadManifestFromRDF(uri, bis);
        this.addon._sourceBundle = this.file;
        this.addon._install = this;
      }
      finally {
        bis.close();
        zis.close();
      }
    }
    finally {
      zipreader.close();
    }
  },

  observe: function AI_observe(subject, topic, data) {
    
    this.cancel();
  },

  


  startDownload: function AI_startDownload() {
    Components.utils.import("resource://gre/modules/CertUtils.jsm");

    this.state = AddonManager.STATE_DOWNLOADING;
    if (!AddonManagerPrivate.callInstallListeners("onDownloadStarted",
                                                  this.listeners, this.wrapper)) {
      this.state = AddonManager.STATE_CANCELLED;
      AddonManagerPrivate.callInstallListeners("onDownloadCancelled",
                                               this.listeners, this.wrapper)
      return;
    }

    this.crypto = Cc["@mozilla.org/security/hash;1"].
                  createInstance(Ci.nsICryptoHash);
    if (this.hash) {
      [alg, this.hash] = this.hash.split(":", 2);

      try {
        this.crypto.initWithString(alg);
      }
      catch (e) {
        WARN("Unknown hash algorithm " + alg);
        this.state = AddonManager.STATE_DOWNLOAD_FAILED;
        AddonManagerPrivate.callInstallListeners("onDownloadFailed",
                                                 this.listeners, this.wrapper,
                                                 AddonManager.ERROR_INCORRECT_HASH);
        return;
      }
    }
    else {
      
      
      this.crypto.initWithString("sha1");
    }

    try {
      this.file = FileUtils.getDir("TmpD", []);
      let random =  Math.random().toString(36).replace(/0./, '').substr(-3);
      this.file.append("tmp-" + random + ".xpi");
      this.file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
      this.stream = Cc["@mozilla.org/network/file-output-stream;1"].
                    createInstance(Ci.nsIFileOutputStream);
      this.stream.init(this.file, FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE |
                       FileUtils.MODE_TRUNCATE, FileUtils.PERMS_FILE, 0);

      let listener = Cc["@mozilla.org/network/stream-listener-tee;1"].
                     createInstance(Ci.nsIStreamListenerTee);
      listener.init(this, this.stream);
      this.channel = NetUtil.newChannel(this.sourceURL);
      if (this.loadGroup)
        this.channel.loadGroup = this.loadGroup;

      Services.obs.addObserver(this, "network:offline-about-to-go-offline", false);

      
      
      if (!this.hash)
        this.channel.notificationCallbacks = new BadCertHandler();
      this.channel.asyncOpen(listener, null);
    }
    catch (e) {
      WARN("Failed to start download: " + e);
      this.state = AddonManager.STATE_DOWNLOAD_FAILED;
      AddonManagerPrivate.callInstallListeners("onDownloadFailed",
                                               this.listeners, this.wrapper,
                                               AddonManager.ERROR_NETWORK_FAILURE);
    }
  },

  




  onDataAvailable: function AI_onDataAvailable(request, context, inputstream,
                                               offset, count) {
    this.crypto.updateFromStream(inputstream, count);
    this.progress += count;
    if (!AddonManagerPrivate.callInstallListeners("onDownloadProgress",
                                                  this.listeners, this.wrapper)) {
      
    }
  },

  




  onStartRequest: function AI_onStartRequest(request, context) {
    
    
    if (this.loadGroup)
      this.loadGroup.removeRequest(request, null, Cr.NS_BINDING_RETARGETED);

    this.progress = 0;
    if (request instanceof Ci.nsIChannel) {
      try {
        this.maxProgress = request.contentLength;
      }
      catch (e) {
      }
      LOG("Download started for " + this.sourceURL.spec + " to file " +
          this.file.path);
    }
  },

  




  onStopRequest: function AI_onStopRequest(request, context, status) {
    this.stream.close();
    this.channel = null;
    Services.obs.removeObserver(this, "network:offline-about-to-go-offline");

    
    if (status == Cr.NS_BINDING_ABORTED)
      return;

    LOG("Download of " + this.sourceURL.spec + " completed.");

    if (Components.isSuccessCode(status)) {
      if (!(request instanceof Ci.nsIHttpChannel) || request.requestSucceeded) {
        if (!this.hash && (request instanceof Ci.nsIChannel)) {
          try {
            checkCert(request);
          }
          catch (e) {
            this.downloadFailed(AddonManager.ERROR_NETWORK_FAILURE, e);
            return;
          }
        }

        
        function toHexString(charCode)
          ("0" + charCode.toString(16)).slice(-2);

        
        let binary = this.crypto.finish(false);
        let hash = [toHexString(binary.charCodeAt(i)) for (i in binary)].join("")
        this.crypto = null;
        if (this.hash && hash != this.hash) {
          this.downloadFailed(AddonManager.ERROR_INCORRECT_HASH,
                              "Downloaded file hash (" + hash +
                              ") did not match provded hash (" + this.hash + ")");
          return;
        }
        try {
          this.loadManifest();
          this.name = this.addon.selectedLocale.name;
          this.type = this.addon.type;
          this.version = this.addon.version;
          
          
          if (this.addon.isCompatible) {
            this.downloadCompleted();
          }
          else {
            
            this.state = AddonManager.STATE_CHECKING;
            let self = this;
            new UpdateChecker(this.addon, {
              onUpdateFinished: function(addon) {
                self.downloadCompleted();
              }
            }, AddonManager.UPDATE_WHEN_ADDON_INSTALLED);
          }
        }
        catch (e) {
          this.downloadFailed(AddonManager.ERROR_CORRUPT_FILE, e);
        }
      }
      else {
        if (request instanceof Ci.nsIHttpChannel)
          this.downloadFailed(AddonManager.ERROR_NETWORK_FAILURE,
                              request.responseStatus + " " +
                              request.responseStatusText);
        else
          this.downloadFailed(AddonManager.ERROR_NETWORK_FAILURE, status);
      }
    }
    else {
      this.downloadFailed(AddonManager.ERROR_NETWORK_FAILURE, status);
    }
  },

  







  downloadFailed: function(reason, error) {
    WARN("Download failed: " + error + "\n");
    this.state = AddonManager.STATE_DOWNLOAD_FAILED;
    AddonManagerPrivate.callInstallListeners("onDownloadFailed", this.listeners,
                                             this.wrapper, reason);
    this.file.remove(true);
  },

  


  downloadCompleted: function() {
    let self = this;
    XPIDatabase.getVisibleAddonForID(this.addon.id, function(addon) {
      self.existingAddon = addon;
      self.state = AddonManager.STATE_DOWNLOADED;
      if (AddonManagerPrivate.callInstallListeners("onDownloadEnded",
                                                   self.listeners,
                                                   self.wrapper))
        self.install();
    });
  },

  
  
  
  


  startInstall: function AI_startInstall() {
    this.state = AddonManager.STATE_INSTALLING;
    if (!AddonManagerPrivate.callInstallListeners("onInstallStarted",
                                                  this.listeners, this.wrapper)) {
      this.state = AddonManager.STATE_DOWNLOADED;
      AddonManagerPrivate.callInstallListeners("onInstallCancelled",
                                               this.listeners, this.wrapper)
      return;
    }

    let isUpgrade = this.existingAddon &&
                    this.existingAddon._installLocation == this.installLocation;
    let requiresRestart = XPIProvider.installRequiresRestart(this.addon);
    
    
    if (!requiresRestart && this.existingAddon) {
      requiresRestart = this.existingAddon.active &&
                        XPIProvider.disableRequiresRestart(this.existingAddon);
    }

    LOG("Starting install of " + this.sourceURL.spec);
    AddonManagerPrivate.callAddonListeners("onInstalling",
                                           createWrapper(this.addon),
                                           requiresRestart);
    let stagedAddon = this.installLocation.getStagingDir();

    try {
      
      let stagedJSON = stagedAddon.clone();
      stagedAddon.append(this.addon.id);
      if (stagedAddon.exists())
        stagedAddon.remove(true);
      stagedAddon.create(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
      extractFiles(this.file, stagedAddon);

      if (requiresRestart) {
        
        stagedJSON.append(this.addon.id + ".json");
        if (stagedJSON.exists())
          stagedJSON.remove(true);
        let stream = Cc["@mozilla.org/network/file-output-stream;1"].
                     createInstance(Ci.nsIFileOutputStream);
        let converter = Cc["@mozilla.org/intl/converter-output-stream;1"].
                        createInstance(Ci.nsIConverterOutputStream);
        let json = Cc["@mozilla.org/dom/json;1"].
                   createInstance(Ci.nsIJSON);

        try {
          stream.init(stagedJSON, FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE |
                                  FileUtils.MODE_TRUNCATE, FileUtils.PERMS_FILE,
                                 0);
          converter.init(stream, "UTF-8", 0, 0x0000);

          
          let objs = {
            sourceBundle: this.addon._sourceBundle,
            install: this.addon._install
          };
          delete this.addon._sourceBundle;
          delete this.addon._install;
          converter.writeString(json.encode(this.addon));
          this.addon._sourceBundle = objs.sourceBundle;
          this.addon._install = objs.install;
        }
        finally {
          converter.close();
          stream.close();
        }

        LOG("Install of " + this.sourceURL.spec + " completed.");
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
        
        
        

        
        if (this.existingAddon) {
          if (this.existingAddon.active) {
            if (this.existingAddon.type == "bootstrapped")
              XPIProvider.deactivateAddon(this.existingAddon.id, false,
                                          isUpgrade);
            
            if (!isUpgrade) {
              this.existingAddon.active = false;
              XPIDatabase.updateAddonActive(this.existingAddon);
            }
          }
          if (isUpgrade)
            this.installLocation.uninstallAddon(this.existingAddon.id);
        }

        
        let dir = this.installLocation.installAddon(this.addon.id, stagedAddon);

        
        this.addon._installLocation = this.installLocation;
        this.addon.updateDate = dir.lastModifiedTime;
        this.addon.visible = true;
        if (isUpgrade) {
          this.addon.installDate = this.existingAddon.installDate;
          XPIDatabase.updateAddonMetadata(this.existingAddon, this.addon,
                                          dir.persistentDescriptor);
        }
        else {
          this.addon.installDate = this.addon.updateDate;
          XPIDatabase.addAddonMetadata(this.addon, dir.persistentDescriptor);
        }

        
        let self = this;
        XPIDatabase.getAddonInLocation(this.addon.id, this.installLocation.name,
                                       function(a) {
          self.addon = a;
          if (self.addon.active && self.addon.type == "bootstrapped")
            XPIProvider.activateAddon(self.addon.id, self.addon.version, dir, false, true);
          AddonManagerPrivate.callAddonListeners("onInstalled",
                                                 createWrapper(self.addon));

          LOG("Install of " + self.sourceURL.spec + " completed.");
          self.state = AddonManager.STATE_INSTALLED;
          AddonManagerPrivate.callInstallListeners("onInstallEnded",
                                                   self.listeners, self.wrapper,
                                                   createWrapper(self.addon));
        });
      }
    }
    catch (e) {
      WARN("Failed to install: " + e);
      if (stagedAddon.exists())
        stagedAddon.remove(true);
      this.state = AddonManager.STATE_INSTALL_FAILED;
      AddonManagerPrivate.callInstallListeners("onInstallFailed",
                                               this.listeners,
                                               this.wrapper, e);
    }
    finally {
      
      if (!(this.sourceURL instanceof Ci.nsIFileURL))
        this.file.remove(true);
    }
  }
}










AddonInstall.createInstall = function(callback, file) {
  let location = XPIProvider.installLocationsByName[KEY_APP_PROFILE];
  let url = Services.io.newFileURI(file);

  try {
    new AddonInstall(callback, location, url);
  }
  catch(e) {
    callback(null);
  }
};



















AddonInstall.createDownload = function(callback, uri, hash, name, iconURL,
                                       version, loadGroup) {
  let location = XPIProvider.installLocationsByName[KEY_APP_PROFILE];
  let url = NetUtil.newURI(uri);
  new AddonInstall(callback, location, url, hash, name, null,
                   iconURL, version, null, null, loadGroup);
};











AddonInstall.createUpdate = function(callback, addon, update) {
  let url = NetUtil.newURI(update.updateURL);
  let infoURL = null;
  if (update.updateInfoURL)
    infoURL = escapeAddonURI(addon, update.updateInfoURL);
  new AddonInstall(callback, addon._installLocation, url, update.updateHash,
                   addon.selectedLocale.name, addon.type,
                   addon.iconURL, update.version, infoURL, addon);
};







function AddonInstallWrapper(install) {
  ["name", "type", "version", "iconURL", "infoURL", "file", "state", "progress",
   "maxProgress", "certificate", "certName"].forEach(function(prop) {
    this.__defineGetter__(prop, function() install[prop]);
  }, this);

  this.__defineGetter__("existingAddon", function() {
    return createWrapper(install.existingAddon);
  });
  this.__defineGetter__("addon", function() createWrapper(install.addon));
  this.__defineGetter__("sourceURL", function() install.sourceURL.spec);

  this.install = function() {
    install.install();
  }

  this.cancel = function() {
    install.cancel();
  }

  this.addListener = function(listener) {
    install.addListener(listener);
  }

  this.removeListener = function(listener) {
    install.removeListener(listener);
  }
}

AddonInstallWrapper.prototype = {};
















function UpdateChecker(addon, listener, reason, appVersion, platformVersion) {
  if (!listener || !reason)
    throw Cr.NS_ERROR_INVALID_ARG;

  Components.utils.import("resource://gre/modules/AddonUpdateChecker.jsm");

  this.addon = addon;
  this.listener = listener;
  this.appVersion = appVersion;
  this.platformVersion = platformVersion;

  let updateURL = addon.updateURL ? addon.updateURL :
                                    Services.prefs.getCharPref(PREF_EM_UPDATE_URL);

  const UPDATE_TYPE_COMPATIBILITY = 32;
  const UPDATE_TYPE_NEWVERSION = 64;

  reason |= UPDATE_TYPE_COMPATIBILITY;
  if ("onUpdateAvailable" in this.listener)
    reason |= UPDATE_TYPE_NEWVERSION;

  let url = escapeAddonURI(addon, updateURL, reason, appVersion);
  AddonUpdateChecker.checkForUpdates(addon.id, addon.type, addon.updateKey, url, this);
}

UpdateChecker.prototype = {
  addon: null,
  listener: null,
  appVersion: null,
  platformVersion: null,

  





  onUpdateCheckComplete: function UC_onUpdateCheckComplete(updates) {
    let AUC = AddonUpdateChecker;
    let compatUpdate = AUC.getCompatibilityUpdate(updates, this.addon.version,
                                                  this.appVersion,
                                                  this.platformVersion);
    if (compatUpdate && this.addon.applyCompatibilityUpdate(compatUpdate)) {
      if ("onCompatibilityUpdated" in this.listener)
        this.listener.onCompatibilityUpdated(createWrapper(this.addon));
    }

    let update = AUC.getNewestCompatibleUpdate(updates,
                                               this.appVersion,
                                               this.platformVersion);
    if (update && Services.vc.compare(this.addon.version, update.version) < 0) {
      if ("onUpdateAvailable" in this.listener) {
        let self = this;
        AddonInstall.createUpdate(function(install) {
          self.listener.onUpdateAvailable(createWrapper(self.addon),
                                          install.wrapper);
          if ("onUpdateFinished" in self.listener)
            self.listener.onUpdateFinished(createWrapper(self.addon));
        }, this.addon, update);
      }
      else if ("onUpdateFinished" in this.listener) {
        this.listener.onUpdateFinished(createWrapper(this.addon));
      }
    }
    else {
      if ("onNoUpdateAvailable" in this.listener)
        this.listener.onNoUpdateAvailable(createWrapper(this.addon));
      if ("onUpdateFinished" in this.listener)
        this.listener.onUpdateFinished(createWrapper(this.addon));
    }
  },

  





  onUpdateCheckError: function UC_onUpdateCheckError(error) {
    if ("onNoUpdateAvailable" in this.listener)
      this.listener.onNoUpdateAvailable(createWrapper(this.addon), error);
    if ("onUpdateFinished" in this.listener)
      this.listener.onUpdateFinished(createWrapper(this.addon));
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
    let app = this.matchingTargetApplication;
    if (!app)
      return false;

    let version;
    if (app.id == Services.appinfo.ID)
      version = Services.appinfo.version;
    else if (app.id == TOOLKIT_ID)
      version = Services.appinfo.platformVersion

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

  applyCompatibilityUpdate: function(update) {
    let changed = false;
    this.targetApplications.forEach(function(ta) {
      update.targetApplications.forEach(function(updateTarget) {
        if (ta.id == updateTarget.id && (ta.minVersion != updateTarget.minVersion ||
                                         ta.maxVersion != updateTarget.maxVersion)) {
          ta.minVersion = updateTarget.minVersion;
          ta.maxVersion = updateTarget.maxVersion;
          changed = true;
        }
      });
    });
    this.appDisabled = !isUsableAddon(this);
    return changed;
  }
};







function DBAddonInternal() {
  this.__defineGetter__("targetApplications", function() {
    delete this.targetApplications;
    return this.targetApplications = XPIDatabase._getTargetApplications(this);
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
  applyCompatibilityUpdate: function(update) {
    let changes = [];
    this.targetApplications.forEach(function(ta) {
      update.targetApplications.forEach(function(updateTarget) {
        if (ta.id == updateTarget.id && (ta.minVersion != updateTarget.minVersion ||
                                         ta.maxVersion != updateTarget.maxVersion)) {
          ta.minVersion = updateTarget.minVersion;
          ta.maxVersion = updateTarget.maxVersion;
          changes.push(ta);
        }
      });
    });
    XPIDatabase.updateTargetApplications(this, changes);
    XPIProvider.updateAddonDisabledState(this);
    return changes.length > 0;
  }
}

DBAddonInternal.prototype.__proto__ = AddonInternal.prototype;








function createWrapper(addon) {
  if (!addon)
    return null;
  if (!addon.wrapper)
    addon.wrapper = new AddonWrapper(addon);
  return addon.wrapper;
}





function AddonWrapper(addon) {
  ["id", "version", "type", "optionsURL", "aboutURL", "isCompatible",
   "providesUpdatesSecurely", "blocklistState", "appDisabled",
   "userDisabled"].forEach(function(prop) {
     this.__defineGetter__(prop, function() addon[prop]);
  }, this);

  ["installDate", "updateDate"].forEach(function(prop) {
    this.__defineGetter__(prop, function() new Date(addon[prop]));
  }, this);

  this.__defineGetter__("iconURL", function() {
      return addon.active ? addon.iconURL : null;
  });

  PROP_LOCALE_SINGLE.forEach(function(prop) {
    this.__defineGetter__(prop, function() {
      if (addon.active) {
        try {
          let pref = PREF_EM_EXTENSION_FORMAT + addon.id + "." + prop;
          let value = Services.prefs.getComplexValue(pref,
                                                     Ci.nsIPrefLocalizedString);
          if (value.data)
            return value.data;
        }
        catch (e) {
        }
      }
      return addon.selectedLocale[prop];
    });
  }, this);

  PROP_LOCALE_MULTI.forEach(function(prop) {
    this.__defineGetter__(prop, function() {
      if (addon.active) {
        let pref = PREF_EM_EXTENSION_FORMAT + addon.id + "." +
                   prop.substring(0, prop.length - 1);
        let list = Services.prefs.getChildList(pref, {});
        if (list.length > 0) {
          let results = [];
          list.forEach(function(pref) {
            let value = Services.prefs.getComplexValue(pref,
                                                       Ci.nsIPrefLocalizedString);
            if (value.data)
              results.push(value.data);
          });
          return results;
        }
      }

      return addon.selectedLocale[prop];

    });
  }, this);

  this.__defineGetter__("screenshots", function() {
    return [];
  });

  this.__defineGetter__("updateAutomatically", function() {
    return addon.updateAutomatically;
  });
  this.__defineSetter__("updateAutomatically", function(val) {
    
    addon.updateAutomatically = val;
  });

  this.__defineGetter__("install", function() {
    if (!("_install" in addon) || !addon._install)
      return null;
    return addon._install.wrapper;
  });

  this.__defineGetter__("pendingUpgrade", function() {
    return createWrapper(addon.pendingUpgrade);
  });

  this.__defineGetter__("pendingOperations", function() {
    let pending = 0;
    if (!(addon instanceof DBAddonInternal))
      pending |= AddonManager.PENDING_INSTALL;
    else if (addon.pendingUninstall)
      pending |= AddonManager.PENDING_UNINSTALL;

    if (addon.active && (addon.userDisabled || addon.appDisabled))
      pending |= AddonManager.PENDING_DISABLE;
    else if (!addon.active && (!addon.userDisabled && !addon.appDisabled))
      pending |= AddonManager.PENDING_ENABLE;

    if (addon.pendingUpgrade)
      pending |= AddonManager.PENDING_UPGRADE;

    return pending;
  });

  this.__defineGetter__("permissions", function() {
    let permissions = 0;
    if (!addon.appDisabled) {
      if (addon.userDisabled)
        permissions |= AddonManager.PERM_CAN_ENABLE;
      else if (addon.type != "theme")
        permissions |= AddonManager.PERM_CAN_DISABLE;
    }
    if (addon._installLocation) {
      if (!addon._installLocation.locked) {
        permissions |= AddonManager.PERM_CAN_UPGRADE;
        if (!addon.pendingUninstall)
          permissions |= AddonManager.PERM_CAN_UNINSTALL;
      }
    }
    return permissions;
  });

  this.__defineGetter__("isActive", function() addon.active);
  this.__defineSetter__("userDisabled", function(val) {
    if (addon.type == "theme" && val)
      throw new Error("Cannot disable the active theme");

    if (addon instanceof DBAddonInternal)
      XPIProvider.updateAddonDisabledState(addon, val);
    else
      addon.userDisabled = val;
  });

  this.uninstall = function() {
    if (!(addon instanceof DBAddonInternal))
      throw new Error("Cannot uninstall an add-on that isn't installed");
    if (addon.pendingUninstall)
      throw new Error("Add-on is already marked to be uninstalled");
    XPIProvider.uninstallAddon(addon);
  };

  this.cancelUninstall = function() {
    if (!(addon instanceof DBAddonInternal))
      throw new Error("Cannot cancel uninstall for an add-on that isn't installed");
    if (!addon.pendingUninstall)
      throw new Error("Add-on is not marked to be uninstalled");
    XPIProvider.cancelUninstallAddon(addon);
  };

  this.findUpdates = function(listener, reason, appVersion, platformVersion) {
    new UpdateChecker(addon, listener, reason, appVersion, platformVersion);
  };

  this.hasResource = function(path) {
    let bundle = null;
    if (addon instanceof DBAddonInternal) {
      bundle = addon._sourceBundle = addon._installLocation
                                          .getLocationForID(addon.id);
    }
    else {
      bundle = addon._sourceBundle.clone();
    }

    if (bundle.isDirectory()) {
      bundle.append(path);
      return bundle.exists();
    }

    let zipReader = Cc["@mozilla.org/libjar/zip-reader;1"].
                    createInstance(Ci.nsIZipReader);
    zipReader.open(bundle);
    let result = zipReader.hasEntry(path);
    zipReader.close();
    return result;
  },

  this.getResourceURL = function(path) {
    let bundle = null;
    if (addon instanceof DBAddonInternal) {
      bundle = addon._sourceBundle = addon._installLocation
                                          .getLocationForID(addon.id);
    }
    else {
      bundle = addon._sourceBundle.clone();
    }

    if (bundle.isDirectory()) {
      bundle.append(path);
      return Services.io.newFileURI(bundle).spec;
    }

    return buildJarURI(bundle, path).spec;
  }
}

AddonWrapper.prototype = {
  get screenshots() {
    return [];
  }
};

























function DirectoryInstallLocation(name, directory, locked) {
  this._name = name;
  this.locked = locked;
  this._directory = directory;
  this._IDToDirMap = {};
  this._DirToIDMap = {};

  if (!directory.exists())
    return;
  if (!directory.isDirectory())
    throw new Error("Location must be a directory.");

  this._readAddons();
}

DirectoryInstallLocation.prototype = {
  _name       : "",
  _directory   : null,
  _IDToDirMap : null,  
  _DirToIDMap : null,  

  






  _readDirectoryFromFile: function DirInstallLocation__readDirectoryFromFile(file) {
    let fis = Cc["@mozilla.org/network/file-input-stream;1"].
              createInstance(Ci.nsIFileInputStream);
    fis.init(file, -1, -1, false);
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
        linkedDirectory.setRelativeDescriptor(file.parent, line.value);
      }

      return linkedDirectory;
    }
    return null;
  },

  


  _readAddons: function DirInstallLocation__readAddons() {
    try {
    let entries = this._directory.directoryEntries
                                 .QueryInterface(Ci.nsIDirectoryEnumerator);
    let entry;
    while (entry = entries.nextFile) {
      
      if (!entry instanceof Ci.nsILocalFile)
        continue;

      let id = entry.leafName;

      if (id == DIR_STAGE)
        continue;

      if (!gIDTest.test(id)) {
        LOG("Ignoring file entry whose name is not a valid add-on ID: " +
             entry.path);
        continue;
      }

      
      entry = this._directory.clone().QueryInterface(Ci.nsILocalFile);
      entry.append(id);
      if (entry.isFile()) {
        newEntry = this._readDirectoryFromFile(entry);
        if (!newEntry || !newEntry.exists() || !newEntry.isDirectory()) {
          WARN("File pointer " + entry.path + " points to an invalid " +
               "directory " + newEntry.path);
          continue;
        }
        entry = newEntry;
      }
      else if (!entry.isDirectory()) {
        LOG("Ignoring entry which isn't a directory: " + entry.path);
        continue;
      }

      this._IDToDirMap[id] = entry;
      this._DirToIDMap[entry.path] = id;
    }
    entries.close();
    }
    catch (e) {
      ERROR(e);
    }
  },

  


  get name() {
    return this._name;
  },

  


  get addonLocations() {
    let locations = [];
    for (let id in this._IDToDirMap) {
      locations.push(this._IDToDirMap[id].clone()
                         .QueryInterface(Ci.nsILocalFile));
    }
    return locations;
  },

  





  getStagingDir: function DirInstallLocation_getStagingDir() {
    let dir = this._directory.clone();
    dir.append(DIR_STAGE);
    return dir;
  },

  








  installAddon: function DirInstallLocation_installAddon(id, source) {
    let dir = this._directory.clone().QueryInterface(Ci.nsILocalFile);
    dir.append(id);
    if (dir.exists())
      dir.remove(true);

    source = source.clone();
    source.moveTo(this._directory, id);
    this._DirToIDMap[dir.path] = id;
    this._IDToDirMap[id] = dir;

    return dir;
  },

  






  uninstallAddon: function DirInstallLocation_uninstallAddon(id) {
    let dir = this._directory.clone();
    dir.append(id);

    delete this._DirToIDMap[dir.path];
    delete this._IDToDirMap[id];

    if (!dir.exists())
      throw new Error("Attempt to uninstall unknown add-on " + id);

    dir.remove(true);
  },

  







  getIDForLocation: function DirInstallLocation_getIDForLocation(dir) {
    if (dir.path in this._DirToIDMap)
      return this._DirToIDMap[dir.path];
    throw new Error("Unknown add-on location " + dir.path);
  },

  







  getLocationForID: function DirInstallLocation_getLocationForID(id) {
    if (id in this._IDToDirMap)
      return this._IDToDirMap[id].clone().QueryInterface(Ci.nsILocalFile);
    throw new Error("Unknown add-on ID " + id);
  }
};

#ifdef XP_WIN











function WinRegInstallLocation(name, rootKey) {
  this.locked = true;
  this._name = name;
  this._rootKey = rootKey;
  this._IDToDirMap = {};
  this._DirToIDMap = {};

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
  _IDToDirMap : null,  
  _DirToIDMap : null,  

  


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

  






  _readAddons: function RegInstallLocation__readAddons(key) {
    let count = key.valueCount;
    for (let i = 0; i < count; ++i) {
      let id = key.getValueName(i);

      let dir = Cc["@mozilla.org/file/local;1"].
                createInstance(Ci.nsILocalFile);
      dir.initWithPath(key.readStringValue(id));

      if (dir.exists() && dir.isDirectory()) {
        this._IDToDirMap[id] = dir;
        this._DirToIDMap[dir.path] = id;
      }
      else {
        WARN("Ignoring missing add-on in " + dir.path);
      }
    }
  },

  


  get name() {
    return this._name;
  },

  


  get addonLocations() {
    let locations = [];
    for (let id in this._IDToDirMap) {
      locations.push(this._IDToDirMap[id].clone()
                         .QueryInterface(Ci.nsILocalFile));
    }
    return locations;
  },

  







  getIDForLocation: function RegInstallLocation_getIDForLocation(file) {
    if (file.path in this._DirToIDMap)
      return this._DirToIDMap[file.path];
    throw new Error("Unknown add-on location");
  },

  






  getLocationForID: function RegInstallLocation_getLocationForID(id) {
    if (id in this._IDToDirMap)
      return this._IDToDirMap[id].clone().QueryInterface(Ci.nsILocalFile);
    throw new Error("Unknown add-on ID");
  }
};
#endif

AddonManagerPrivate.registerProvider(XPIProvider);
