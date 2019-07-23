

















































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/LightweightThemeManager.jsm");
Components.utils.import("resource://gre/modules/FileUtils.jsm");

const PREF_EM_CHECK_COMPATIBILITY     = "extensions.checkCompatibility";
const PREF_EM_CHECK_UPDATE_SECURITY   = "extensions.checkUpdateSecurity";
const PREF_EM_LAST_APP_VERSION        = "extensions.lastAppVersion";
const PREF_EM_ENABLED_ITEMS           = "extensions.enabledItems";
const PREF_UPDATE_COUNT               = "extensions.update.count";
const PREF_UPDATE_DEFAULT_URL         = "extensions.update.url";
const PREF_EM_NEW_ADDONS_LIST         = "extensions.newAddons";
const PREF_EM_DISABLED_ADDONS_LIST    = "extensions.disabledAddons";
const PREF_EM_SHOW_MISMATCH_UI        = "extensions.showMismatchUI";
const PREF_EM_IGNOREMTIMECHANGES      = "extensions.ignoreMTimeChanges";
const PREF_EM_DISABLEDOBSOLETE        = "extensions.disabledObsolete";
const PREF_EM_EXTENSION_FORMAT        = "extensions.%UUID%.";
const PREF_EM_ITEM_UPDATE_ENABLED     = "extensions.%UUID%.update.enabled";
const PREF_EM_UPDATE_ENABLED          = "extensions.update.enabled";
const PREF_EM_ITEM_UPDATE_URL         = "extensions.%UUID%.update.url";
const PREF_EM_DSS_ENABLED             = "extensions.dss.enabled";
const PREF_DSS_SWITCHPENDING          = "extensions.dss.switchPending";
const PREF_DSS_SKIN_TO_SELECT         = "extensions.lastSelectedSkin";
const PREF_LWTHEME_TO_SELECT          = "extensions.lwThemeToSelect";
const PREF_GENERAL_SKINS_SELECTEDSKIN = "general.skins.selectedSkin";
const PREF_EM_LOGGING_ENABLED         = "extensions.logging.enabled";
const PREF_EM_UPDATE_INTERVAL         = "extensions.update.interval";
const PREF_UPDATE_NOTIFYUSER          = "extensions.update.notifyUser";
const PREF_MATCH_OS_LOCALE            = "intl.locale.matchOS";
const PREF_SELECTED_LOCALE            = "general.useragent.locale";

const DIR_EXTENSIONS                  = "extensions";
const DIR_CHROME                      = "chrome";
const DIR_STAGE                       = "staged-xpis";
const FILE_EXTENSIONS                 = "extensions.rdf";
const FILE_EXTENSION_MANIFEST         = "extensions.ini";
const FILE_EXTENSIONS_STARTUP_CACHE   = "extensions.cache";
const FILE_EXTENSIONS_LOG             = "extensions.log";
const FILE_INSTALL_MANIFEST           = "install.rdf";
const FILE_CHROME_MANIFEST            = "chrome.manifest";

const UNKNOWN_XPCOM_ABI               = "unknownABI";

const TOOLKIT_ID                      = "toolkit@mozilla.org"

const KEY_PROFILEDIR                  = "ProfD";
const KEY_PROFILEDS                   = "ProfDS";
const KEY_APPDIR                      = "XCurProcD";
const KEY_TEMPDIR                     = "TmpD";

const EM_ACTION_REQUESTED_TOPIC       = "em-action-requested";
const EM_ITEM_INSTALLED               = "item-installed";
const EM_ITEM_UPGRADED                = "item-upgraded";
const EM_ITEM_UNINSTALLED             = "item-uninstalled";
const EM_ITEM_ENABLED                 = "item-enabled";
const EM_ITEM_DISABLED                = "item-disabled";
const EM_ITEM_CANCEL                  = "item-cancel-action";

const OP_NONE                         = "";
const OP_NEEDS_INSTALL                = "needs-install";
const OP_NEEDS_UPGRADE                = "needs-upgrade";
const OP_NEEDS_UNINSTALL              = "needs-uninstall";
const OP_NEEDS_ENABLE                 = "needs-enable";
const OP_NEEDS_DISABLE                = "needs-disable";

const KEY_APP_PROFILE                 = "app-profile";
const KEY_APP_GLOBAL                  = "app-global";
const KEY_APP_SYSTEM_LOCAL            = "app-system-local";
const KEY_APP_SYSTEM_SHARE            = "app-system-share";
const KEY_APP_SYSTEM_USER             = "app-system-user";

const CATEGORY_INSTALL_LOCATIONS      = "extension-install-locations";
const CATEGORY_UPDATE_PARAMS          = "extension-update-params";

const PREFIX_NS_EM                    = "http://www.mozilla.org/2004/em-rdf#";
const PREFIX_ITEM_URI                 = "urn:mozilla:item:";
const PREFIX_EXTENSION                = "urn:mozilla:extension:";
const PREFIX_THEME                    = "urn:mozilla:theme:";
const RDFURI_INSTALL_MANIFEST_ROOT    = "urn:mozilla:install-manifest";
const RDFURI_ITEM_ROOT                = "urn:mozilla:item:root"
const RDFURI_DEFAULT_THEME            = "urn:mozilla:item:{972ce4c6-7e08-4474-a285-3208198ce6fd}";
const XMLURI_PARSE_ERROR              = "http://www.mozilla.org/newlayout/xml/parsererror.xml"

const URI_GENERIC_ICON_XPINSTALL      = "chrome://mozapps/skin/xpinstall/xpinstallItemGeneric.png";
const URI_GENERIC_ICON_THEME          = "chrome://mozapps/skin/extensions/themeGeneric.png";
const URI_XPINSTALL_CONFIRM_DIALOG    = "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul";
const URI_EXTENSIONS_PROPERTIES       = "chrome://mozapps/locale/extensions/extensions.properties";
const URI_BRAND_PROPERTIES            = "chrome://branding/locale/brand.properties";
const URI_DOWNLOADS_PROPERTIES        = "chrome://mozapps/locale/downloads/downloads.properties";
const URI_EXTENSION_UPDATE_DIALOG     = "chrome://mozapps/content/extensions/update.xul";
const URI_EXTENSION_LIST_DIALOG       = "chrome://mozapps/content/extensions/list.xul";

const URI_EXTENSION_MANAGER           = "chrome://mozapps/content/extensions/extensions.xul";
const FEATURES_EXTENSION_MANAGER      = "chrome,menubar,extra-chrome,toolbar,dialog=no,resizable";
const FEATURES_EXTENSION_UPDATES      = "chrome,centerscreen,extra-chrome,dialog,resizable,modal";





const MAX_PUBLIC_UPDATE_WHEN          = 15;
const UPDATE_WHEN_PERIODIC_UPDATE     = 16;
const UPDATE_WHEN_ADDON_INSTALLED     = 17;




const UPDATE_TYPE_COMPATIBILITY       = 32;
const UPDATE_TYPE_NEWVERSION          = 64;

const INSTALLERROR_SUCCESS               = 0;
const INSTALLERROR_INVALID_VERSION       = -1;
const INSTALLERROR_INVALID_GUID          = -2;
const INSTALLERROR_INCOMPATIBLE_VERSION  = -3;
const INSTALLERROR_PHONING_HOME          = -4;
const INSTALLERROR_INCOMPATIBLE_PLATFORM = -5;
const INSTALLERROR_BLOCKLISTED           = -6;
const INSTALLERROR_INSECURE_UPDATE       = -7;
const INSTALLERROR_INVALID_MANIFEST      = -8;
const INSTALLERROR_RESTRICTED            = -9;
const INSTALLERROR_SOFTBLOCKED           = -10;

const REQ_VERSION = 2;

var gApp  = null;
var gPref = null;
var gRDF  = null;
var gOS   = null;
var gCheckCompatibilityPref;
var gEmSingleton          = null;
var gBlocklist            = null;
var gXPCOMABI             = null;
var gOSTarget             = null;
var gConsole              = null;
var gInstallManifestRoot  = null;
var gVersionChecker       = null;
var gLoggingEnabled       = null;
var gCheckCompatibility   = true;
var gCheckUpdateSecurity  = true;
var gLocale               = "en-US";
var gFirstRun             = false;
var gAllowFlush           = true;
var gDSNeedsFlush         = false;
var gManifestNeedsFlush   = false;
var gDefaultTheme         = "classic/1.0";




var gIDTest = /^(\{[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\}|[a-z0-9-\._]*\@[a-z0-9-\._]+)$/i;

var gBranchVersion = /^([^\.]+\.[0-9]+[a-z]*).*/gi;


XPCOMUtils.defineLazyGetter(this, "gCertUtils", function() {
  let temp = { };
  Components.utils.import("resource://gre/modules/CertUtils.jsm", temp);
  return temp;
});





function getVersionChecker() {
  if (!gVersionChecker) {
    gVersionChecker = Cc["@mozilla.org/xpcom/version-comparator;1"].
                      getService(Ci.nsIVersionComparator);
  }
  return gVersionChecker;
}

var BundleManager = {
  





  getBundle: function BundleManager_getBundle(bundleURI) {
    var sbs = Cc["@mozilla.org/intl/stringbundle;1"].
              getService(Ci.nsIStringBundleService);
    return sbs.createBundle(bundleURI);
  },

  _appName: "",

  


  get appName() {
    if (!this._appName) {
      var brandBundle = this.getBundle(URI_BRAND_PROPERTIES)
      this._appName = brandBundle.GetStringFromName("brandShortName");
    }
    return this._appName;
  }
};





function EM_NS(property) {
  return PREFIX_NS_EM + property;
}

function EM_R(property) {
  return gRDF.GetResource(EM_NS(property));
}

function EM_L(literal) {
  return gRDF.GetLiteral(literal);
}

function EM_I(integer) {
  return gRDF.GetIntLiteral(integer);
}

function EM_D(integer) {
  return gRDF.GetDateLiteral(integer);
}













function getPref(func, preference, defaultValue) {
  try {
    return gPref[func](preference);
  }
  catch (e) {
  }
  return defaultValue;
}









function getContainer(datasource, root) {
  var ctr = Cc["@mozilla.org/rdf/container;1"].
            createInstance(Ci.nsIRDFContainer);
  ctr.Init(datasource, root);
  return ctr;
}








function getResourceForID(id) {
  return gRDF.GetResource(PREFIX_ITEM_URI + id);
}





function makeItem(id, version, locationKey, minVersion, maxVersion, name,
                  updateURL, updateHash, iconURL, updateRDF, updateKey, type, 
                  targetAppID) {
  var item = new UpdateItem();
  item.init(id, version, locationKey, minVersion, maxVersion, name,
            updateURL, updateHash, iconURL, updateRDF, updateKey, type,
            targetAppID);
  return item;
}









function getDescriptorFromFile(itemLocation, installLocation) {
  var baseDir = installLocation.location;

  if (baseDir && baseDir.contains(itemLocation, true)) {
    return "rel%" + itemLocation.getRelativeDescriptor(baseDir);
  }

  return "abs%" + itemLocation.persistentDescriptor;
}

function getAbsoluteDescriptor(itemLocation) {
  return itemLocation.persistentDescriptor;
}











function getFileFromDescriptor(descriptor, installLocation) {
  var location = Cc["@mozilla.org/file/local;1"].
                 createInstance(Ci.nsILocalFile);

  var m = descriptor.match(/^(abs|rel)\%(.*)$/);
  if (!m)
    throw Cr.NS_ERROR_INVALID_ARG;

  if (m[1] == "rel") {
    location.setRelativeDescriptor(installLocation.location, m[2]);
  }
  else {
    location.persistentDescriptor = m[2];
  }

  return location;
}







function fileIsItemPackage(file) {
  var fileURL = getURIFromFile(file);
  if (fileURL instanceof Ci.nsIURL)
    var extension = fileURL.fileExtension.toLowerCase();
  return extension == "xpi" || extension == "jar";
}










function removeDirRecursive(dir) {
  try {
    dir.remove(true);
    return;
  }
  catch (e) {
  }

  var dirEntries = dir.directoryEntries;
  while (dirEntries.hasMoreElements()) {
    var entry = dirEntries.getNext().QueryInterface(Ci.nsIFile);

    if (entry.isDirectory()) {
      removeDirRecursive(entry);
    }
    else {
      entry.permissions = FileUtils.PERMS_FILE;
      entry.remove(false);
    }
  }
  dir.permissions = FileUtils.PERMS_DIRECTORY;
  dir.remove(true);
}







function LOG(string) {
  if (gLoggingEnabled) {
    dump("*** EM_LOG *** " + string + "\n");
    if (gConsole)
      gConsole.logStringMessage(string);
  }
}







function WARN(string) {
  if (gLoggingEnabled)
    dump("*** EM_WARN *** " + string + "\n");
  if (gConsole) {
    var message = Cc["@mozilla.org/scripterror;1"].
                  createInstance(Ci.nsIScriptError);
    message.init(string, null, null, 0, 0, Ci.nsIScriptError.warningFlag,
                 "component javascript");
    gConsole.logMessage(message);
  }
}





  
function ERROR(string) {
  if (gLoggingEnabled)
    dump("*** EM_ERROR *** " + string + "\n");
  if (gConsole) {
    var message = Cc["@mozilla.org/scripterror;1"].
                  createInstance(Ci.nsIScriptError);
    message.init(string, null, null, 0, 0, Ci.nsIScriptError.errorFlag,
                 "component javascript");
    gConsole.logMessage(message);
  }
  try {
    var tstamp = new Date();
    var logfile = FileUtils.getFile(KEY_PROFILEDIR, [FILE_EXTENSIONS_LOG]);
    var stream = Cc["@mozilla.org/network/file-output-stream;1"].
                 createInstance(Ci.nsIFileOutputStream);
    stream.init(logfile, 0x02 | 0x08 | 0x10, 0666, 0); 
    var writer = Cc["@mozilla.org/intl/converter-output-stream;1"].
                 createInstance(Ci.nsIConverterOutputStream);
    writer.init(stream, "UTF-8", 0, 0x0000);
    string = tstamp.toLocaleFormat("%Y-%m-%d %H:%M:%S - ") + string;
    writer.writeString(string + "\n");
    writer.close();
  }
  catch (e) { }
}








function getRandomFileName(fileName) {
  var extensionDelimiter = fileName.lastIndexOf(".");
  var prefix = fileName.substr(0, extensionDelimiter);
  var suffix = fileName.substr(extensionDelimiter);

  var characters = "abcdefghijklmnopqrstuvwxyz0123456789";
  var nameString = prefix + "-";
  for (var i = 0; i < 3; ++i) {
    var index = Math.round((Math.random()) * characters.length);
    nameString += characters.charAt(index);
  }
  return nameString + "." + suffix;
}









function getItemPrefix(type) {
  if (type & Ci.nsIUpdateItem.TYPE_EXTENSION)
    return PREFIX_EXTENSION;
  else if (type & Ci.nsIUpdateItem.TYPE_THEME)
    return PREFIX_THEME;
  return PREFIX_ITEM_URI;
}









function stripPrefix(string, prefix) {
  return string.substr(prefix.length);
}







function getURLSpecFromFile(file) {
  var ioServ = Cc["@mozilla.org/network/io-service;1"].
               getService(Ci.nsIIOService);
  var fph = ioServ.getProtocolHandler("file")
                  .QueryInterface(Ci.nsIFileProtocolHandler);
  return fph.getURLSpecFromFile(file);
}







function newURI(spec) {
  var ioServ = Cc["@mozilla.org/network/io-service;1"].
               getService(Ci.nsIIOService);
  return ioServ.newURI(spec, null, null);
}







function getURIFromFile(file) {
  var ioServ = Cc["@mozilla.org/network/io-service;1"].
               getService(Ci.nsIIOService);
  return ioServ.newFileURI(file);
}




function inSafeMode() {
  return gApp.inSafeMode;
}








function stringData(literalOrResource) {
  if (literalOrResource instanceof Ci.nsIRDFLiteral)
    return literalOrResource.Value;
  if (literalOrResource instanceof Ci.nsIRDFResource)
    return literalOrResource.Value;
  return undefined;
}







function intData(literal) {
  if (literal instanceof Ci.nsIRDFInt)
    return literal.Value;
  return undefined;
}










function getManifestProperty(installManifest, property) {
  var target = installManifest.GetTarget(gInstallManifestRoot,
                                         gRDF.GetResource(EM_NS(property)), true);
  var val = stringData(target);
  return val === undefined ? intData(target) : val;
}









function getAddonTypeFromInstallManifest(installManifest) {
  var target = installManifest.GetTarget(gInstallManifestRoot,
                                         gRDF.GetResource(EM_NS("type")), true);
  if (target) {
    var type = stringData(target);
    return type === undefined ? intData(target) : parseInt(type);
  }

  
  
  
  if (getManifestProperty(installManifest, "internalName") !== undefined)
    return Ci.nsIUpdateItem.TYPE_THEME;

  
  return Ci.nsIUpdateItem.TYPE_EXTENSION;
}






function showIncompatibleError(installData) {
  var extensionStrings = BundleManager.getBundle(URI_EXTENSIONS_PROPERTIES);
  var params = [extensionStrings.GetStringFromName("type-" + installData.type)];
  var title = extensionStrings.formatStringFromName("incompatibleTitle",
                                                    params, params.length);
  params = [installData.name, installData.version, BundleManager.appName,
            gApp.version];
  var message = extensionStrings.formatStringFromName("incompatibleMessage",
                                                      params, params.length);
  var ps = Cc["@mozilla.org/embedcomp/prompt-service;1"].
           getService(Ci.nsIPromptService);
  ps.alert(null, title, message);
}










function showMessage(titleKey, titleParams, messageKey, messageParams) {
  var extensionStrings = BundleManager.getBundle(URI_EXTENSIONS_PROPERTIES);
  if (titleParams && titleParams.length > 0) {
    var title = extensionStrings.formatStringFromName(titleKey, titleParams,
                                                      titleParams.length);
  }
  else
    title = extensionStrings.GetStringFromName(titleKey);

  if (messageParams && messageParams.length > 0) {
    var message = extensionStrings.formatStringFromName(messageKey, messageParams,
                                                        messageParams.length);
  }
  else
    message = extensionStrings.GetStringFromName(messageKey);
  var ps = Cc["@mozilla.org/embedcomp/prompt-service;1"].
           getService(Ci.nsIPromptService);
  ps.alert(null, title, message);
}









function showBlocklistMessage(item, softblocked) {
  var params = Cc["@mozilla.org/embedcomp/dialogparam;1"].
               createInstance(Ci.nsIDialogParamBlock);
  params.SetInt(0, softblocked ? 1 : 0);
  params.SetInt(1, 0);
  params.SetNumberStrings(1);
  params.SetString(0, item.name + " " + item.version);

  var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
           getService(Ci.nsIWindowMediator);
  var win = wm.getMostRecentWindow("Extension:Manager");
  var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  ww.openWindow(win, URI_EXTENSION_LIST_DIALOG, "",
                "chrome,centerscreen,modal,dialog,titlebar", params);

  return params.GetInt(1) == 0 ? false : true;
}







function getZipReaderForFile(zipFile) {
  try {
    var zipReader = Cc["@mozilla.org/libjar/zip-reader;1"].
                    createInstance(Ci.nsIZipReader);
    zipReader.open(zipFile);
  }
  catch (e) {
    zipReader.close();
    throw e;
  }
  return zipReader;
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












function extractRDFFileToTempDir(zipFile, fileName, suppressErrors) {
  var file = FileUtils.getFile(KEY_TEMPDIR, [getRandomFileName(fileName)]);
  try {
    var zipReader = getZipReaderForFile(zipFile);
    zipReader.extract(fileName, file);
    zipReader.close();
  }
  catch (e) {
    if (!suppressErrors) {
      showMessage("missingFileTitle", [], "missingFileMessage",
                  [BundleManager.appName, fileName]);
      throw e;
    }
  }
  return file;
}







function getInstallManifest(file) {
  var uri = getURIFromFile(file);
  try {
    var fis = Cc["@mozilla.org/network/file-input-stream;1"].
              createInstance(Ci.nsIFileInputStream);
    fis.init(file, -1, -1, false);
    var bis = Cc["@mozilla.org/network/buffered-input-stream;1"].
              createInstance(Ci.nsIBufferedInputStream);
    bis.init(fis, 4096);
    
    var rdfParser = Cc["@mozilla.org/rdf/xml-parser;1"].
                    createInstance(Ci.nsIRDFXMLParser)
    var ds = Cc["@mozilla.org/rdf/datasource;1?name=in-memory-datasource"].
             createInstance(Ci.nsIRDFDataSource);
    var listener = rdfParser.parseAsync(ds, uri);
    var channel = Cc["@mozilla.org/network/input-stream-channel;1"].
                  createInstance(Ci.nsIInputStreamChannel);
    channel.setURI(uri);
    channel.contentStream = bis;
    channel.QueryInterface(Ci.nsIChannel);
    channel.contentType = "text/xml";
  
    listener.onStartRequest(channel, null);
    try {
      var pos = 0;
      var count = bis.available();
      while (count > 0) {
        listener.onDataAvailable(channel, null, bis, pos, count);
        pos += count;
        count = bis.available();
      }
      listener.onStopRequest(channel, null, Components.results.NS_OK);
      bis.close();
      fis.close();

      var arcs = ds.ArcLabelsOut(gInstallManifestRoot);
      if (arcs.hasMoreElements())
        return ds;
    }
    catch (e) {
      listener.onStopRequest(channel, null, e.result);
      bis.close();
      fis.close();
    }
  }
  catch (e) { }

  var url = uri.QueryInterface(Ci.nsIURL);
  showMessage("malformedTitle", [], "malformedMessage",
              [BundleManager.appName, url.fileName]);
  return null;
}








function findClosestLocalizedResource(aDataSource, aResource) {
  var localizedProp = EM_R("localized");
  var localeProp = EM_R("locale");

  
  var bestmatch = null;
  
  var bestmatchcount = 0;
  
  var bestpartcount = 0;

  var locales = [gLocale.toLowerCase()];
  

  if (locales[0].substring(0, 3) != "en-")
    locales.push("en-us");

  for each (var locale in locales) {
    var lparts = locale.split("-");
    var localizations = aDataSource.GetTargets(aResource, localizedProp, true);
    while (localizations.hasMoreElements()) {
      var localized = localizations.getNext().QueryInterface(Ci.nsIRDFNode);
      var list = aDataSource.GetTargets(localized, localeProp, true);
      while (list.hasMoreElements()) {
        var found = stringData(list.getNext().QueryInterface(Ci.nsIRDFNode));
        if (!found)
          continue;

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
    




function ArrayEnumerator(aItems) {
  if (aItems) {
    for (var i = 0; i < aItems.length; ++i) {
      if (!aItems[i])
        aItems.splice(i--, 1);
    }
    this._contents = aItems;
  } else {
    this._contents = [];
  }
}

ArrayEnumerator.prototype = {
  _index: 0,

  hasMoreElements: function ArrayEnumerator_hasMoreElements() {
    return this._index < this._contents.length;
  },

  getNext: function ArrayEnumerator_getNext() {
    return this._contents[this._index++];
  }
};







function FileEnumerator(files) {
  if (files) {
    for (var i = 0; i < files.length; ++i) {
      if (!files[i])
        files.splice(i--, 1);
    }
    this._contents = files;
  } else {
    this._contents = [];
  }
}

FileEnumerator.prototype = {
  _index: 0,

  


  get nextFile() {
    if (this._index < this._contents.length)
      return this._contents[this._index++];
    return null;
  },

  


  close: function FileEnumerator_close() {
  }
};

















function DirectoryInstallLocation(name, location, restricted, priority, independent) {
  this._name = name;
  if (location.exists()) {
    if (!location.isDirectory())
      throw new Error("location must be a directoy!");
  }
  else {
    try {
      location.create(Ci.nsILocalFile.DIRECTORY_TYPE, 0775);
    }
    catch (e) {
      LOG("DirectoryInstallLocation: failed to create location " +
          " directory = " + location.path + ", exception = " + e + "\n");
    }
  }

  this._location = location;
  this._locationToIDMap = {};
  this._restricted = restricted;
  this._priority = priority;
  this._independent = independent;
}
DirectoryInstallLocation.prototype = {
  _name           : "",
  _location       : null,
  _locationToIDMap: null,
  _restricted     : false,
  _priority       : 0,
  _independent    : false,
  _canAccess      : null,

  


  get name() {
    return this._name;
  },

  





  _readDirectoryFromFile: function DirInstallLocation__readDirectoryFromFile(file) {
    var fis = Cc["@mozilla.org/network/file-input-stream;1"].
              createInstance(Ci.nsIFileInputStream);
    fis.init(file, -1, -1, false);
    var line = { value: "" };
    if (fis instanceof Ci.nsILineInputStream)
      fis.readLine(line);
    fis.close();
    if (line.value) {
      var linkedDirectory = Cc["@mozilla.org/file/local;1"].
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

  


  get itemLocations() {
    var locations = [];
    if (!this._location.exists())
      return new FileEnumerator(locations);

    try {
      var entries = this._location.directoryEntries.QueryInterface(Ci.nsIDirectoryEnumerator);
      while (true) {
        var entry = entries.nextFile;
        if (!entry)
          break;
        entry instanceof Ci.nsILocalFile;
        if (!entry.isDirectory() && gIDTest.test(entry.leafName)) {
          var linkedDirectory = this._readDirectoryFromFile(entry);
          if (linkedDirectory && linkedDirectory.exists() &&
              linkedDirectory.isDirectory()) {
            locations.push(linkedDirectory);
            this._locationToIDMap[linkedDirectory.persistentDescriptor] = entry.leafName;
          }
        }
        else
          locations.push(entry);
      }
      entries.close();
    }
    catch (e) {
    }
    return new FileEnumerator(locations);
  },

  








  getIDForLocation: function DirInstallLocation_getIDForLocation(file) {
    var section = file.leafName;
    var filePD = file.persistentDescriptor;
    if (filePD in this._locationToIDMap)
      section = this._locationToIDMap[filePD];

    if (gIDTest.test(section))
      return RegExp.$1;
    return undefined;
  },

  


  get location() {
    return this._location.clone();
  },

  


  get restricted() {
    return this._restricted;
  },

  


  get canAccess() {
    if (this._canAccess != null)
      return this._canAccess;

    if (!this.location.exists()) {
      this._canAccess = false;
      return false;
    }

    var testFile = this.location;
    testFile.append("Access Privileges Test");
    try {
      testFile.createUnique(Ci.nsILocalFile.DIRECTORY_TYPE,
                            FileUtils.PERMS_DIRECTORY);
      testFile.remove(false);
      this._canAccess = true;
    }
    catch (e) {
      this._canAccess = false;
    }
    return this._canAccess;
  },

  


  get priority() {
    return this._priority;
  },

  


  getItemLocation: function DirInstallLocation_getItemLocation(id) {
    var itemLocation = this.location;
    itemLocation.append(id);
    if (itemLocation.exists() && !itemLocation.isDirectory())
      return this._readDirectoryFromFile(itemLocation);
    if (!itemLocation.exists() && this.canAccess)
      itemLocation.create(Ci.nsILocalFile.DIRECTORY_TYPE,
                          FileUtils.PERMS_DIRECTORY);
    return itemLocation;
  },

  


  itemIsManagedIndependently: function DirInstallLocation_itemIsManagedIndependently(id) {
    if (this._independent)
      return true;
    var itemLocation = this.location;
    itemLocation.append(id);
    return itemLocation.exists() && !itemLocation.isDirectory();
  },

  


  getItemFile: function DirInstallLocation_getItemFile(id, filePath) {
    var itemLocation = this.getItemLocation(id);
    var parts = filePath.split("/");
    for (var i = 0; i < parts.length; ++i)
      itemLocation.append(parts[i]);
    return itemLocation;
  },

  






  stageFile: function DirInstallLocation_stageFile(file, id) {
    var stagedFile = this.location;
    stagedFile.append(DIR_STAGE);
    stagedFile.append(id);
    stagedFile.append(file.leafName);

    
    if (stagedFile.equals(file))
      return stagedFile;

    if (stagedFile.exists())
      stagedFile.remove(false);

    file.copyTo(stagedFile.parent, stagedFile.leafName);

    
    if (!stagedFile.isWritable())
      stagedFile.permissions = FileUtils.PERMS_FILE;

    return stagedFile;
  },

  






  getStageFile: function DirInstallLocation_getStageFile(id) {
    var stageFile = null;
    var stageDir = this.location;
    stageDir.append(DIR_STAGE);
    stageDir.append(id);
    if (!stageDir.exists() || !stageDir.isDirectory())
      return null;
    try {
      var entries = stageDir.directoryEntries.QueryInterface(Ci.nsIDirectoryEnumerator);
      while (entries.hasMoreElements()) {
        var file = entries.nextFile;
        if (!(file instanceof Ci.nsILocalFile))
          continue;
        if (file.isDirectory())
          removeDirRecursive(file);
        else if (fileIsItemPackage(file)) {
          if (stageFile)
            stageFile.remove(false);
          stageFile = file;
        }
        else
          file.remove(false);
      }
    }
    catch (e) {
    }
    if (entries instanceof Ci.nsIDirectoryEnumerator)
      entries.close();
    return stageFile;
  },

  





  removeFile: function DirInstallLocation_removeFile(file) {
    if (file.exists())
      file.remove(false);
    var parent = file.parent;
    var entries = parent.directoryEntries;
    try {
      
      
      while (parent && !parent.equals(this.location) &&
            !entries.hasMoreElements()) {
        parent.remove(false);
        parent = parent.parent;
        entries = parent.directoryEntries;
      }
      if (entries instanceof Ci.nsIDirectoryEnumerator)
        entries.close();
    }
    catch (e) {
      ERROR("DirectoryInstallLocation::removeFile: failed to remove staged " +
            " directory = " + parent.path + ", exception = " + e + "\n");
    }
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIInstallLocation])
};

#ifdef XP_WIN

const nsIWindowsRegKey = Ci.nsIWindowsRegKey;



















function WinRegInstallLocation(name, rootKey, restricted, priority) {
  this._name = name;
  this._rootKey = rootKey;
  this._restricted = restricted;
  this._priority = priority;
  this._IDToDirMap = {};
  this._DirToIDMap = {};

  
  
  try {
    var path = this._appKeyPath + "\\Extensions";
    var key = Cc["@mozilla.org/windows-registry-key;1"].
              createInstance(nsIWindowsRegKey);
    key.open(this._rootKey, path, nsIWindowsRegKey.ACCESS_READ);
    this._readAddons(key);
  } catch (e) {
    if (key)
      key.close();
  }
}
WinRegInstallLocation.prototype = {
  _name       : "",
  _rootKey    : null,
  _restricted : false,
  _priority   : 0,
  _IDToDirMap : null,  
  _DirToIDMap : null,  

  


  get _appKeyPath() {
    var appVendor = gApp.vendor;
    var appName = gApp.name;

#ifdef MOZ_THUNDERBIRD
    
    if (appVendor == "")
      appVendor = "Mozilla";
#endif

    
    if (appVendor != "")
      appVendor += "\\";

    return "SOFTWARE\\" + appVendor + appName;
  },

  





  _readAddons: function RegInstallLocation__readAddons(key) {
    var count = key.valueCount;
    for (var i = 0; i < count; ++i) {
      var id = key.getValueName(i);

      var dir = Cc["@mozilla.org/file/local;1"].
                createInstance(Ci.nsILocalFile);
      dir.initWithPath(key.readStringValue(id));

      if (dir.exists() && dir.isDirectory()) {
        this._IDToDirMap[id] = dir;
        this._DirToIDMap[dir.path] = id;
      }
    }
  },

  get name() {
    return this._name;
  },

  get itemLocations() {
    var locations = [];
    for (var id in this._IDToDirMap) {
      locations.push(this._IDToDirMap[id]);
    }
    return new FileEnumerator(locations);
  },

  get location() {
    return null;
  },

  get restricted() {
    return this._restricted;
  },

  
  get canAccess() {
    return false;
  },

  get priority() {
    return this._priority;
  },

  getItemLocation: function RegInstallLocation_getItemLocation(id) {
    if (!(id in this._IDToDirMap))
      return null;
    return this._IDToDirMap[id].clone();
  },

  getIDForLocation: function RegInstallLocation_getIDForLocation(dir) {
    return this._DirToIDMap[dir.path];
  },

  getItemFile: function RegInstallLocation_getItemFile(id, filePath) {
    var itemLocation = this.getItemLocation(id);
    if (!itemLocation)
      return null;
    var parts = filePath.split("/");
    for (var i = 0; i < parts.length; ++i)
      itemLocation.append(parts[i]);
    return itemLocation;
  },

  itemIsManagedIndependently: function RegInstallLocation_itemIsManagedIndependently(id) {
    return true;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIInstallLocation])
};

#endif














function safeInstallOperation(itemID, installLocation, file) {
  var movedFiles = [];

  



  function rollbackMove()
  {
    for (var i = 0; i < movedFiles.length; ++i) {
      var oldFile = movedFiles[i].oldFile;
      var newFile = movedFiles[i].newFile;
      try {
        newFile.moveTo(oldFile.parent, newFile.leafName);
      }
      catch (e) {
        ERROR("safeInstallOperation: failed to roll back files after an install " +
              "operation failed. Failed to roll back: " + newFile.path + " to: " +
              oldFile.path + " ... aborting installation.");
        throw e;
      }
    }
  }

  






  function moveFile(file, destination) {
    try {
      var oldFile = file.clone();
      file.moveTo(destination, file.leafName);
      movedFiles.push({ oldFile: oldFile, newFile: file });
    }
    catch (e) {
      ERROR("safeInstallOperation: failed to back up file: " + file.path + " to: " +
            destination.path + " ... rolling back file moves and aborting " +
            "installation.");
      rollbackMove();
      throw e;
    }
  }

  










  function moveDirectory(sourceDir, targetDir, currentDir) {
    var entries = currentDir.directoryEntries.QueryInterface(Ci.nsIDirectoryEnumerator);
    while (true) {
      var entry = entries.nextFile;
      if (!entry)
        break;
      if (entry.isDirectory())
        moveDirectory(sourceDir, targetDir, entry);
      else if (entry instanceof Ci.nsILocalFile) {
        var rd = entry.getRelativeDescriptor(sourceDir);
        var destination = targetDir.clone().QueryInterface(Ci.nsILocalFile);
        destination.setRelativeDescriptor(targetDir, rd);
        moveFile(entry, destination.parent);
      }
    }
    entries.close();
  }

  




  function cleanUpTrash(directory) {
    try {
      
      if (directory && directory.exists())
        removeDirRecursive(directory);
    }
    catch (e) {
      ERROR("safeInstallOperation: failed to clean up the temporary backup of the " +
            "older version: " + itemLocationTrash.path);
      
    }
  }

  if (!installLocation.itemIsManagedIndependently(itemID)) {
    var itemLocation = installLocation.getItemLocation(itemID);
    if (itemLocation.exists()) {
      var trashDirName = itemID + "-trash";
      var itemLocationTrash = itemLocation.parent.clone();
      itemLocationTrash.append(trashDirName);
      if (itemLocationTrash.exists()) {
        
        
        
        try {
          removeDirRecursive(itemLocationTrash);
        }
        catch (e) {
          ERROR("safeFileOperation: failed to remove existing trash directory " +
                itemLocationTrash.path + " ... aborting installation.");
          throw e;
        }
      }

      
      
      
      moveDirectory(itemLocation, itemLocationTrash, itemLocation);

      
      
      try {
        removeDirRecursive(itemLocation);
      }
      catch (e) {
        ERROR("safeInstallOperation: failed to clean up item location after its contents " +
              "were properly backed up. Failed to clean up: " + itemLocation.path +
              " ... rolling back file moves and aborting installation.");
        rollbackMove();
        cleanUpTrash(itemLocationTrash);
        throw e;
      }
    }
  }
  else if (installLocation.name == KEY_APP_PROFILE ||
           installLocation.name == KEY_APP_GLOBAL ||
           installLocation.name == KEY_APP_SYSTEM_USER) {
    
    var pointerFile = installLocation.location.clone();
    pointerFile.append(itemID);
    if (pointerFile.exists() && !pointerFile.isDirectory()) {
      var trashFileName = itemID + "-trash";
      var itemLocationTrash = installLocation.location.clone();
      itemLocationTrash.append(trashFileName);
      if (itemLocationTrash.exists()) {
        
        
        
        try {
          removeDirRecursive(itemLocationTrash);
        }
        catch (e) {
          ERROR("safeFileOperation: failed to remove existing trash directory " +
                itemLocationTrash.path + " ... aborting installation.");
          throw e;
        }
      }
      itemLocationTrash.create(Ci.nsILocalFile.DIRECTORY_TYPE,
                               FileUtils.PERMS_DIRECTORY);
      
      moveFile(pointerFile, itemLocationTrash);
    }
  }

  if (file) {
    
    try {
      var zipReader = getZipReaderForFile(file);

      
      var entries = zipReader.findEntries("*/");
      while (entries.hasMore()) {
        var entryName = entries.getNext();
        var target = installLocation.getItemFile(itemID, entryName);
        if (!target.exists()) {
          try {
            target.create(Ci.nsILocalFile.DIRECTORY_TYPE,
                          FileUtils.PERMS_DIRECTORY);
          }
          catch (e) {
            ERROR("extractFiles: failed to create target directory for extraction " +
                  "file = " + target.path + ", exception = " + e + "\n");
          }
        }
      }

      entries = zipReader.findEntries(null);
      while (entries.hasMore()) {
        var entryName = entries.getNext();
        target = installLocation.getItemFile(itemID, entryName);
        if (target.exists())
          continue;

        zipReader.extract(entryName, target);
        LOG("Extracted file " + entryName + " with permissions " + target.permissions.toString(8));
        target.permissions |= FileUtils.PERMS_FILE;
      }
    }
    catch (e) {
      
      ERROR("safeInstallOperation: file extraction failed, " +
            "rolling back file moves and aborting installation.");
      try {
        
        removeDirRecursive(itemLocation);
      }
      catch (e) {
        ERROR("safeInstallOperation: failed to remove the folder we failed to install " +
              "an item into: " + itemLocation.path + " -- There is not much to suggest " +
              "here... maybe restart and try again?");
        cleanUpTrash(itemLocationTrash);
        throw e;
      }
      rollbackMove();
      cleanUpTrash(itemLocationTrash);
      throw e;
    }
    finally {
      if (zipReader)
        zipReader.close();
    }
  }

  
  
  
  cleanUpTrash(itemLocationTrash);
}




var PendingOperations = {
  _ops: { },

  









  addItem: function PendingOperations_addItem(opType, entry) {
    if (opType == OP_NONE)
      this.clearOpsForItem(entry.id);
    else {
      if (!(opType in this._ops))
        this._ops[opType] = { };
      this._ops[opType][entry.id] = entry.locationKey;
    }
  },

  






  clearItem: function PendingOperations_clearItem(opType, id) {
    if (opType in this._ops && id in this._ops[opType])
      delete this._ops[opType][id];
  },

  




  clearOpsForItem: function PendingOperations_clearOpsForItem(id) {
    for (var opType in this._ops) {
      if (id in this._ops[opType])
        delete this._ops[opType][id];
    }
  },

  




  clearItems: function PendingOperations_clearItems(opType) {
    if (opType in this._ops)
      delete this._ops[opType];
  },

  




  getOperations: function PendingOperations_getOperations(opType) {
    if (!(opType in this._ops))
      return [];
    var ops = [];
    for (var id in this._ops[opType])
      ops.push( {id: id, locationKey: this._ops[opType][id] } );
    return ops;
  },

  


  get size() {
    var size = 0;
    for (var opType in this._ops) {
      for (var id in this._ops[opType])
        ++size;
    }
    return size;
  }
};




var InstallLocations = {
  _locations: { },

  


  get enumeration() {
    var installLocations = [];
    for (var key in this._locations)
      installLocations.push(InstallLocations.get(key));
    return new ArrayEnumerator(installLocations);
  },

  




  get: function InstallLocations_get(name) {
    return name in this._locations ? this._locations[name] : null;
  },

  




  put: function InstallLocations_put(installLocation) {
    this._locations[installLocation.name] = installLocation;
  }
};







var StartupCache = {
  







  entries: { },

  











  put: function StartupCache_put(installLocation, id, op, shouldCreate) {
    var itemLocation = installLocation.getItemLocation(id);

    var descriptor = null;
    var mtime = null;
    if (itemLocation) {
      itemLocation.QueryInterface(Ci.nsILocalFile);
      descriptor = getDescriptorFromFile(itemLocation, installLocation);
      if (itemLocation.exists() && itemLocation.isDirectory())
        mtime = Math.floor(itemLocation.lastModifiedTime / 1000);
    }

    this._putRaw(installLocation.name, id, descriptor, mtime, op, shouldCreate);
  },

  



















  _putRaw: function StartupCache__putRaw(key, id, descriptor, mtime, op, shouldCreate) {
    if (!(key in this.entries))
      this.entries[key] = { };
    if (!(id in this.entries[key]))
      this.entries[key][id] = { };
    if (shouldCreate) {
      if (!this.entries[key][id])
        this.entries[key][id] = { };

      var entry = this.entries[key][id];

      if (descriptor)
        entry.descriptor = descriptor;
      if (mtime)
        entry.mtime = mtime;
      entry.op = op;
      entry.location = key;
    }
    else
      this.entries[key][id] = null;
  },

  






  clearEntry: function StartupCache_clearEntry(installLocation, id) {
    var key = installLocation.name;
    if (key in this.entries && id in this.entries[key])
      this.entries[key][id] = null;
  },

  





  findEntries: function StartupCache_findEntries(id) {
    var entries = [];
    for (var key in this.entries) {
      if (id in this.entries[key])
        entries.push(this.entries[key][id]);
    }
    return entries;
  },

  




  read: function StartupCache_read() {
    var itemChangeManifest = FileUtils.getFile(KEY_PROFILEDIR,
                                               [FILE_EXTENSIONS_STARTUP_CACHE]);
    if (!itemChangeManifest.exists()) {
      
      
      
      gFirstRun = true;
      return;
    }
    var fis = Cc["@mozilla.org/network/file-input-stream;1"].
              createInstance(Ci.nsIFileInputStream);
    fis.init(itemChangeManifest, -1, -1, false);
    if (fis instanceof Ci.nsILineInputStream) {
      var line = { value: "" };
      var more = false;
      do {
        more = fis.readLine(line);
        if (line.value) {
          
          
          
          
          
          
          
          
          
          
          var parts = line.value.split("\t");
          
          if (!InstallLocations.get(parts[0]))
            continue;
          var op = parts[4];
          this._putRaw(parts[0], parts[1], parts[2], parts[3], op, true);
          if (op)
            PendingOperations.addItem(op, { locationKey: parts[0], id: parts[1] });
        }
      }
      while (more);
    }
    fis.close();
  },

  


  write: function StartupCache_write() {
    var extensionsCacheFile = FileUtils.getFile(KEY_PROFILEDIR,
                                                [FILE_EXTENSIONS_STARTUP_CACHE]);
    var fos = FileUtils.openSafeFileOutputStream(extensionsCacheFile);
    for (var locationKey in this.entries) {
      for (var id in this.entries[locationKey]) {
        var entry = this.entries[locationKey][id];
        if (entry) {
          try {
            var itemLocation = getFileFromDescriptor(entry.descriptor, InstallLocations.get(locationKey));

            
            
            var itemMTime = 0;
            if (itemLocation.exists() && itemLocation.isDirectory())
              itemMTime = Math.floor(itemLocation.lastModifiedTime / 1000);

            
            
            var line = locationKey + "\t" + id + "\t" + entry.descriptor + "\t" +
                       itemMTime + "\t" + entry.op + "\r\n";
            fos.write(line, line.length);
          }
          catch (e) {}
        }
      }
    }
    FileUtils.closeSafeFileOutputStream(fos);
  }
};





function ExtensionManager() {
  gApp = Cc["@mozilla.org/xre/app-info;1"].
         getService(Ci.nsIXULAppInfo).QueryInterface(Ci.nsIXULRuntime);
  gOSTarget = gApp.OS;
  try {
    gXPCOMABI = gApp.XPCOMABI;
  } catch (ex) {
    
    
    
    
    gXPCOMABI = UNKNOWN_XPCOM_ABI;
  }
  gPref = Cc["@mozilla.org/preferences-service;1"].
          getService(Ci.nsIPrefBranch2).
          QueryInterface(Ci.nsIPrefService);
  var defaults = gPref.getDefaultBranch("");
  try {
    gDefaultTheme = defaults.getCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN);
  } catch(e) {}

  gOS = Cc["@mozilla.org/observer-service;1"].
        getService(Ci.nsIObserverService);
  gOS.addObserver(this, "xpcom-shutdown", false);
  gOS.addObserver(this, "lightweight-theme-preview-requested", false);
  gOS.addObserver(this, "lightweight-theme-change-requested", false);

  gConsole = Cc["@mozilla.org/consoleservice;1"].
             getService(Ci.nsIConsoleService);

  gRDF = Cc["@mozilla.org/rdf/rdf-service;1"].
         getService(Ci.nsIRDFService);
  gInstallManifestRoot = gRDF.GetResource(RDFURI_INSTALL_MANIFEST_ROOT);

  
  var appGlobalExtensions = FileUtils.getDir(KEY_APPDIR, [DIR_EXTENSIONS],
                                             false);
  var priority = Ci.nsIInstallLocation.PRIORITY_APP_SYSTEM_GLOBAL;
  var globalLocation = new DirectoryInstallLocation(KEY_APP_GLOBAL,
                                                    appGlobalExtensions, true,
                                                    priority, false);
  InstallLocations.put(globalLocation);

  
  var appProfileExtensions = FileUtils.getDir(KEY_PROFILEDS, [DIR_EXTENSIONS],
                                              false);
  var priority = Ci.nsIInstallLocation.PRIORITY_APP_PROFILE;
  var profileLocation = new DirectoryInstallLocation(KEY_APP_PROFILE,
                                                     appProfileExtensions, false,
                                                     priority, false);
  InstallLocations.put(profileLocation);

  
  try {
    var appSystemUExtensions = FileUtils.getDir("XREUSysExt", [gApp.ID], false);
  }
  catch(e) { }

  if (appSystemUExtensions) {
    var priority = Ci.nsIInstallLocation.PRIORITY_APP_SYSTEM_USER;
    var systemLocation = new DirectoryInstallLocation(KEY_APP_SYSTEM_USER,
                                                      appSystemUExtensions, false,
                                                      priority, true);

    InstallLocations.put(systemLocation);
  }

  
  try {
    var appSystemSExtensions = FileUtils.getDir("XRESysSExtPD", [gApp.ID], false);
  }
  catch (e) { }

  if (appSystemSExtensions) {
    var priority = Ci.nsIInstallLocation.PRIORITY_APP_SYSTEM_GLOBAL + 10;
    var systemLocation = new DirectoryInstallLocation(KEY_APP_SYSTEM_SHARE,
                                                      appSystemSExtensions, true,
                                                      priority, true);
    InstallLocations.put(systemLocation);
  }

  
  try {
    var appSystemLExtensions = FileUtils.getDir("XRESysLExtPD", [gApp.ID], false);
  }
  catch (e) { }

  if (appSystemLExtensions) {
    var priority = Ci.nsIInstallLocation.PRIORITY_APP_SYSTEM_GLOBAL + 20;
    var systemLocation = new DirectoryInstallLocation(KEY_APP_SYSTEM_LOCAL,
                                                      appSystemLExtensions, true,
                                                      priority, true);
    InstallLocations.put(systemLocation);
  }

#ifdef XP_WIN
  
  InstallLocations.put(
      new WinRegInstallLocation("winreg-app-global",
                                nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE,
                                true,
                                Ci.nsIInstallLocation.PRIORITY_APP_SYSTEM_GLOBAL + 10));

  
  InstallLocations.put(
      new WinRegInstallLocation("winreg-app-user",
                                nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                                false,
                                Ci.nsIInstallLocation.PRIORITY_APP_SYSTEM_USER + 10));
#endif

  
  var categoryManager = Cc["@mozilla.org/categorymanager;1"].
                        getService(Ci.nsICategoryManager);
  var locations = categoryManager.enumerateCategory(CATEGORY_INSTALL_LOCATIONS);
  while (locations.hasMoreElements()) {
    var entry = locations.getNext().QueryInterface(Ci.nsISupportsCString).data;
    var contractID = categoryManager.getCategoryEntry(CATEGORY_INSTALL_LOCATIONS, entry);
    var location = Cc[contractID].getService(Ci.nsIInstallLocation);
    InstallLocations.put(location);
  }
}

ExtensionManager.prototype = {
  


  observe: function EM_observe(subject, topic, data) {
    switch (topic) {
    case "profile-after-change":
      this._profileSelected();
      break;
    case "quit-application-requested":
      this._confirmCancelDownloadsOnQuit(subject);
      break;
    case "offline-requested":
      this._confirmCancelDownloadsOnOffline(subject);
      break;
    case "lightweight-theme-preview-requested":
      if (gPref.prefHasUserValue(PREF_GENERAL_SKINS_SELECTEDSKIN)) {
        let cancel = subject.QueryInterface(Ci.nsISupportsPRBool);
        cancel.data = true;
      }
      break;
    case "lightweight-theme-change-requested":
      let theme = JSON.parse(data);
      if (!theme)
        return;

      if (gPref.prefHasUserValue(PREF_GENERAL_SKINS_SELECTEDSKIN)) {
        if (getPref("getBoolPref", PREF_EM_DSS_ENABLED, false)) {
          gPref.clearUserPref(PREF_GENERAL_SKINS_SELECTEDSKIN);
          return;
        }

        let cancel = subject.QueryInterface(Ci.nsISupportsPRBool);
        cancel.data = true;
        gPref.setBoolPref(PREF_DSS_SWITCHPENDING, true);
        gPref.setCharPref(PREF_DSS_SKIN_TO_SELECT, gDefaultTheme);
        gPref.setCharPref(PREF_LWTHEME_TO_SELECT, theme.id);

        
        var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
                 getService(Ci.nsIWindowMediator);
        var win = wm.getMostRecentWindow("Extension:Manager");

        if (win) {
          win.showView("themes");
          return;
        }

        var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
                 getService(Ci.nsIWindowWatcher);
        var param = Cc["@mozilla.org/supports-array;1"].
                    createInstance(Ci.nsISupportsArray);
        var arg = Cc["@mozilla.org/supports-string;1"].
                  createInstance(Ci.nsISupportsString);
        arg.data = "themes";
        param.AppendElement(arg);
        ww.openWindow(null, URI_EXTENSION_MANAGER, null, FEATURES_EXTENSION_MANAGER, param);
        return;
      }
      else {
        
        
        if (gPref.prefHasUserValue(PREF_DSS_SWITCHPENDING))
          gPref.clearUserPref(PREF_DSS_SWITCHPENDING);
        if (gPref.prefHasUserValue(PREF_DSS_SKIN_TO_SELECT))
          gPref.clearUserPref(PREF_DSS_SKIN_TO_SELECT);
      }
      break;
    case "xpcom-shutdown":
      this._shutdown();
      break;
    case "nsPref:changed":
      if (data == PREF_EM_LOGGING_ENABLED)
        this._loggingToggled();
      else if (data == gCheckCompatibilityPref ||
               data == PREF_EM_CHECK_UPDATE_SECURITY)
        this._updateAppDisabledState();
      else if ((data == PREF_MATCH_OS_LOCALE) || (data == PREF_SELECTED_LOCALE))
        this._updateLocale();
      break;
    }
  },

  



  _loggingToggled: function EM__loggingToggled() {
    gLoggingEnabled = getPref("getBoolPref", PREF_EM_LOGGING_ENABLED, false);
  },

  


  _updateLocale: function EM__updateLocale() {
    try {
      if (gPref.getBoolPref(PREF_MATCH_OS_LOCALE)) {
        var localeSvc = Cc["@mozilla.org/intl/nslocaleservice;1"].
                        getService(Ci.nsILocaleService);
        gLocale = localeSvc.getLocaleComponentForUserAgent();
        return;
      }
    }
    catch (ex) {
    }
    gLocale = gPref.getCharPref(PREF_SELECTED_LOCALE);
  },

  



  _updateAppDisabledState: function EM__updateAppDisabledState() {
    gCheckCompatibility = getPref("getBoolPref", gCheckCompatibilityPref, true);
    gCheckUpdateSecurity = getPref("getBoolPref", PREF_EM_CHECK_UPDATE_SECURITY, true);
    var ds = this.datasource;

    
    var ctr = getContainer(ds, ds._itemRoot);
    var elements = ctr.GetElements();
    while (elements.hasMoreElements()) {
      var itemResource = elements.getNext().QueryInterface(Ci.nsIRDFResource);

      
      
      
      var id = stripPrefix(itemResource.Value, PREFIX_ITEM_URI);
      if (this._isUsableItem(id))
        this._appEnableItem(id);
      else
        this._appDisableItem(id);
    }
  },

  


  _profileSelected: function EM__profileSelected() {
    
    try {
      if (gPref.getBoolPref(PREF_DSS_SWITCHPENDING)) {
        var toSelect = gPref.getCharPref(PREF_DSS_SKIN_TO_SELECT);
        gPref.setCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN, toSelect);
        gPref.clearUserPref(PREF_DSS_SWITCHPENDING);
        gPref.clearUserPref(PREF_DSS_SKIN_TO_SELECT);

        
        
        if (toSelect != gDefaultTheme) {
          if (gPref.prefHasUserValue(PREF_LWTHEME_TO_SELECT))
            gPref.clearUserPref(PREF_LWTHEME_TO_SELECT);
          LightweightThemeManager.currentTheme = null;
        }
      }

      if (gPref.prefHasUserValue(PREF_LWTHEME_TO_SELECT)) {
        var id = gPref.getCharPref(PREF_LWTHEME_TO_SELECT);
        if (id)
          LightweightThemeManager.currentTheme = LightweightThemeManager.getUsedTheme(id);
        else
          LightweightThemeManager.currentTheme = null;
        gPref.clearUserPref(PREF_LWTHEME_TO_SELECT);
      }
    }
    catch (e) {
    }

    var version = gApp.version.replace(gBranchVersion, "$1");
    gCheckCompatibilityPref = PREF_EM_CHECK_COMPATIBILITY + "." + version;

    gLoggingEnabled = getPref("getBoolPref", PREF_EM_LOGGING_ENABLED, false);
    gCheckCompatibility = getPref("getBoolPref", gCheckCompatibilityPref, true);
    gCheckUpdateSecurity = getPref("getBoolPref", PREF_EM_CHECK_UPDATE_SECURITY, true);

    if ("nsICrashReporter" in Ci && gApp instanceof Ci.nsICrashReporter) {
      
      try {
        gApp.annotateCrashReport("Add-ons", gPref.getCharPref(PREF_EM_ENABLED_ITEMS));
      } catch (e) { }
      try {
        gApp.annotateCrashReport("Theme", gPref.getCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN));
      } catch (e) { }
      try {
        gApp.annotateCrashReport("EMCheckCompatibility", gCheckCompatibility);
      } catch (e) { }
    }

    gPref.addObserver("extensions.", this, false);
    gPref.addObserver(PREF_MATCH_OS_LOCALE, this, false);
    gPref.addObserver(PREF_SELECTED_LOCALE, this, false);
    this._updateLocale();
  },

  


  _showUpdatesWindow: function EM__showUpdatesWindow() {
    if (!getPref("getBoolPref", PREF_UPDATE_NOTIFYUSER, false))
      return;

    var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
             getService(Ci.nsIWindowWatcher);
    var param = Cc["@mozilla.org/supports-array;1"].
                createInstance(Ci.nsISupportsArray);
    var arg = Cc["@mozilla.org/supports-string;1"].
              createInstance(Ci.nsISupportsString);
    arg.data = "updates-only";
    param.AppendElement(arg);
    ww.openWindow(null, URI_EXTENSION_MANAGER, null, FEATURES_EXTENSION_UPDATES, param);
  },

  


  _shutdown: function EM__shutdown() {
    if (!gAllowFlush) {
      
      ERROR("Reached _shutdown and without clearing any pending flushes");
      try {
        gAllowFlush = true;
        if (gManifestNeedsFlush) {
          gManifestNeedsFlush = false;
          this._updateManifests(false);
        }
        if (gDSNeedsFlush) {
          gDSNeedsFlush = false;
          this.datasource.Flush();
        }
      }
      catch (e) {
        ERROR("Error flushing caches: " + e);
      }
    }

    gOS.removeObserver(this, "xpcom-shutdown");
    gOS.removeObserver(this, "lightweight-theme-preview-requested");
    gOS.removeObserver(this, "lightweight-theme-change-requested");

    
    gOS = null;
    if (this._ds) {
      gRDF.UnregisterDataSource(this._ptr);
      this._ptr = null;
      this._ds.shutdown();
      this._ds = null;
    }
    gRDF = null;
    if (gPref) {
      gPref.removeObserver("extensions.", this);
      gPref.removeObserver(PREF_MATCH_OS_LOCALE, this);
      gPref.removeObserver(PREF_SELECTED_LOCALE, this);
    }
    gPref = null;
    gConsole = null;
    gVersionChecker = null;
    gInstallManifestRoot = null;
    gApp = null;
  },

  






  _ensureDatasetIntegrity: function EM__ensureDatasetIntegrity() {
    var profD = FileUtils.getDir(KEY_PROFILEDIR, [], false);
    var extensionsDS = profD.clone();
    extensionsDS.append(FILE_EXTENSIONS);
    var extensionsINI = profD.clone();
    extensionsINI.append(FILE_EXTENSION_MANIFEST);
    var extensionsCache = profD;
    extensionsCache.append(FILE_EXTENSIONS_STARTUP_CACHE);

    var dsExists = extensionsDS.exists();
    var iniExists = extensionsINI.exists();
    var cacheExists = extensionsCache.exists();

    if (dsExists && iniExists && cacheExists)
      return [false, !iniExists];

    
    if (iniExists)
      extensionsINI.remove(false);

    
    if (!dsExists && cacheExists)
      extensionsCache.remove(false);

    return [true, !iniExists];
  },

  


  start: function EM_start() {
    var isDirty, forceAutoReg;

    
    
    
    
    [isDirty, forceAutoReg] = this._ensureDatasetIntegrity();

    
    gAllowFlush = false;

    
    
    
    
    if (this._checkForFileChanges())
      isDirty = true;

    this._showUpdatesWindow();

    if (PendingOperations.size != 0)
      isDirty = true;

    var needsRestart = false;
    
    if (isDirty) {
      needsRestart = this._finishOperations();

      if (forceAutoReg) {
        this._extensionListChanged = true;
        needsRestart = true;
      }
    }

    
    try {
      gAllowFlush = true;
      if (gManifestNeedsFlush) {
        gManifestNeedsFlush = false;
        this._updateManifests(false);
      }
      if (gDSNeedsFlush) {
        gDSNeedsFlush = false;
        this.datasource.Flush();
      }
    }
    catch (e) {
      ERROR("Error flushing caches: " + e);
    }

    return needsRestart;
  },

  




  notify: function EM_notify(timer) {
    if (!getPref("getBoolPref", PREF_EM_UPDATE_ENABLED, true))
      return;

    var items = this.getItemList(Ci.nsIUpdateItem.TYPE_ANY);

    var updater = new ExtensionItemUpdater(this);
    updater.checkForUpdates(items, items.length,
                            Ci.nsIExtensionManager.UPDATE_CHECK_NEWVERSION,
                            new BackgroundUpdateCheckListener(this.datasource),
                            UPDATE_WHEN_PERIODIC_UPDATE);

    LightweightThemeManager.updateCurrentTheme();
  },

  










  _getItemForDroppedFile: function EM__getItemForDroppedFile(file, location) {
    if (fileIsItemPackage(file)) {
      
      
      
      LOG("A Item Package appeared at: " + file.path + " that we know " +
          "nothing about, assuming it was dropped in by the user and " +
          "configuring for installation now. Location Key: " + location.name);

      var installManifestFile = extractRDFFileToTempDir(file, FILE_INSTALL_MANIFEST, true);
      if (!installManifestFile.exists())
        return null;
      var installManifest = getInstallManifest(installManifestFile);
      installManifestFile.remove(false);
      var ds = this.datasource;
      var installData = this._getInstallData(installManifest);
      var targetAppInfo = ds.getTargetApplicationInfo(installData.id, installManifest);
      return makeItem(installData.id,
                      installData.version,
                      location.name,
                      targetAppInfo ? targetAppInfo.minVersion : "",
                      targetAppInfo ? targetAppInfo.maxVersion : "",
                      getManifestProperty(installManifest, "name"),
                      "", 
                      "", 
                      getManifestProperty(installManifest, "iconURL"),
                      getManifestProperty(installManifest, "updateURL"),
                      getManifestProperty(installManifest, "updateKey"),
                      installData.type,
                      targetAppInfo ? targetAppInfo.appID : gApp.ID);
    }
    return null;
  },

  




















   installItem: function EM_installItem(id, location, callback) {
    
    var installRDF = location.getItemFile(id, FILE_INSTALL_MANIFEST);
    if (installRDF.exists()) {
      LOG("Item Installed/Upgraded at Install Location: " + location.name +
          " Item ID: " + id + ", attempting to register...");
      var installManifest = getInstallManifest(installRDF);
      var installData = this._getInstallData(installManifest);
      if (installData.error == INSTALLERROR_SUCCESS) {
        LOG("... success, item is compatible");
        callback(installManifest, installData.id, location, installData.type);
      }
      else if (installData.error == INSTALLERROR_INCOMPATIBLE_VERSION) {
        LOG("... success, item installed but is not compatible");
        callback(installManifest, installData.id, location, installData.type);
        this._appDisableItem(id);
      }
      else if (installData.error == INSTALLERROR_INSECURE_UPDATE) {
        LOG("... success, item installed but does not provide updates securely");
        callback(installManifest, installData.id, location, installData.type);
        this._appDisableItem(id);
      }
      else if (installData.error == INSTALLERROR_BLOCKLISTED) {
        LOG("... success, item installed but is blocklisted");
        callback(installManifest, installData.id, location, installData.type);
        this._appDisableItem(id);
      }
      else if (installData.error == INSTALLERROR_SOFTBLOCKED) {
        LOG("... success, item installed but is soft blocked, item will be disabled");
        callback(installManifest, installData.id, location, installData.type);
        this.disableItem(id);
      }
      else {
        





        function translateErrorMessage(error) {
          switch (error) {
          case INSTALLERROR_INVALID_GUID:
            return "Invalid GUID";
          case INSTALLERROR_INVALID_VERSION:
            return "Invalid Version";
          case INSTALLERROR_INCOMPATIBLE_PLATFORM:
            return "Incompatible Platform";
          }
        }
        LOG("... failure, item is not compatible, error: " +
            translateErrorMessage(installData.error));

        
        
        StartupCache.put(location, id, OP_NONE, true);
      }
    }
  },

  




  _checkForFileChanges: function EM__checkForFileChanges() {
    var em = this;

    










    function canUse(id, location) {
      for (var locationKey in StartupCache.entries) {
        if (locationKey != location.name &&
            id in StartupCache.entries[locationKey]) {
          if (StartupCache.entries[locationKey][id]) {
            var oldInstallLocation = InstallLocations.get(locationKey);
            if (oldInstallLocation.priority <= location.priority)
              return false;
          }
        }
      }
      return true;
    }

    






    function getParamBlock(strings) {
      var dpb = Cc["@mozilla.org/embedcomp/dialogparam;1"].
                createInstance(Ci.nsIDialogParamBlock);
      
      dpb.SetInt(0, 2);
      
      dpb.SetInt(1, strings.length);
      dpb.SetNumberStrings(strings.length);
      
      for (var i = 0; i < strings.length; ++i)
        dpb.SetString(i, strings[i]);

      var supportsString = Cc["@mozilla.org/supports-string;1"].
                           createInstance(Ci.nsISupportsString);
      var bundle = BundleManager.getBundle(URI_EXTENSIONS_PROPERTIES);
      supportsString.data = bundle.GetStringFromName("droppedInWarning");
      var objs = Cc["@mozilla.org/array;1"].
                 createInstance(Ci.nsIMutableArray);
      objs.appendElement(supportsString, false);
      dpb.objects = objs;
      return dpb;
    }

    










    function installDroppedInFiles(droppedInFiles, xpinstallStrings) {
      if (droppedInFiles.length == 0)
        return;

      var dpb = getParamBlock(xpinstallStrings);
      var ifptr = Cc["@mozilla.org/supports-interface-pointer;1"].
                  createInstance(Ci.nsISupportsInterfacePointer);
      ifptr.data = dpb;
      ifptr.dataIID = Ci.nsIDialogParamBlock;
      var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
               getService(Ci.nsIWindowWatcher);
      ww.openWindow(null, URI_XPINSTALL_CONFIRM_DIALOG,
                    "", "chrome,centerscreen,modal,dialog,titlebar", ifptr);
      if (!dpb.GetInt(0)) {
        
        for (var i = 0; i < droppedInFiles.length; ++i) {
          em.installItemFromFile(droppedInFiles[i].file,
                                 droppedInFiles[i].location.name);
          
          droppedInFiles[i].file.remove(false);
        }
      }
      else {
        for (i = 0; i < droppedInFiles.length; ++i) {
          
          droppedInFiles[i].file.remove(false);
        }
      }
    }

    var isDirty = false;
    var ignoreMTimeChanges = getPref("getBoolPref", PREF_EM_IGNOREMTIMECHANGES,
                                     false);
    StartupCache.read();

    
    var newItems = [];

    var droppedInFiles = [];
    var xpinstallStrings = [];

    
    
    var installLocations = this.installLocations;
    while (installLocations.hasMoreElements()) {
      var location = installLocations.getNext().QueryInterface(Ci.nsIInstallLocation);

      
      var actualItems = { };
      var entries = location.itemLocations;
      while (true) {
        var entry = entries.nextFile;
        if (!entry)
          break;

        
        
        if (entry.isDirectory()) {
          var installRDF = entry.clone();
          installRDF.append(FILE_INSTALL_MANIFEST);

          var id = location.getIDForLocation(entry);
          if (!id || (!installRDF.exists() &&
                      !location.itemIsManagedIndependently(id)))
            continue;

          actualItems[id] = entry;
        }
        else {
          
          
          
          
          var item = this._getItemForDroppedFile(entry, location);
          if (item) {
            var prettyName = "";
            try {
              var zipReader = getZipReaderForFile(entry);
              zipReader.QueryInterface(Ci.nsIJAR);
              var principal = zipReader.getCertificatePrincipal(null);
              if (principal && principal.hasCertificate) {
                if (verifyZipSigning(zipReader, principal)) {
                  var x509 = principal.certificate;
                  if (x509 instanceof Ci.nsIX509Cert && x509.commonName.length > 0)
                    prettyName = x509.commonName;
                  else
                    prettyName = principal.prettyName;
                }
                else {
                  
                  LOG("Ignoring " + entry.path + " as it is not correctly signed.");
                  zipReader.close();
                  entry.remove(true);
                  continue;
                }
              }
            }
            catch (e) { }
            if (zipReader)
              zipReader.close();
            droppedInFiles.push({ file: entry, location: location });
            xpinstallStrings = xpinstallStrings.concat([item.name,
                                                        getURLSpecFromFile(entry),
                                                        item.iconURL,
                                                        prettyName]);
            isDirty = true;
          }
        }
      }

      if (location.name in StartupCache.entries) {
        
        for (var id in StartupCache.entries[location.name]) {
          if (!StartupCache.entries[location.name] ||
              !StartupCache.entries[location.name][id])
            continue;

          
          
          
          if (StartupCache.entries[location.name][id].op == OP_NEEDS_ENABLE ||
              StartupCache.entries[location.name][id].op == OP_NEEDS_DISABLE)
            isDirty = true;

          if (!(id in actualItems) &&
              StartupCache.entries[location.name][id].op != OP_NEEDS_UNINSTALL &&
              StartupCache.entries[location.name][id].op != OP_NEEDS_INSTALL &&
              StartupCache.entries[location.name][id].op != OP_NEEDS_UPGRADE) {
            
            
            
            
            if (canUse(id, location)) {
              LOG("Item Uninstalled via file removal from: " + StartupCache.entries[location.name][id].descriptor +
                  " Item ID: " + id + " Location Key: " + location.name + ", uninstalling item.");

              
              
              
              
              
              
              this.datasource.updateVisibleList(id, location.name, false);
              this.uninstallItem(id);
              isDirty = true;
            }
          }
          else if (!ignoreMTimeChanges) {
            
            
            var lf = { path: StartupCache.entries[location.name][id].descriptor };
            try {
               lf = getFileFromDescriptor(StartupCache.entries[location.name][id].descriptor, location);
            }
            catch (e) { }

            if (lf.exists && lf.exists()) {
              var actualMTime = Math.floor(lf.lastModifiedTime / 1000);
              if (actualMTime != StartupCache.entries[location.name][id].mtime) {
                LOG("Item Location path changed: " + lf.path + " Item ID: " +
                    id + " Location Key: " + location.name + ", attempting to upgrade item...");
                if (canUse(id, location)) {
                  this.installItem(id, location,
                                   function(installManifest, id, location, type) {
                                     em._upgradeItem(installManifest, id, location,
                                                     type);
                                   });
                  isDirty = true;
                }
              }
            }
            else {
              isDirty = true;
              LOG("Install Location returned a missing or malformed item path! " +
                  "Item Path: " + lf.path + ", Location Key: " + location.name +
                  " Item ID: " + id);
              if (canUse(id, location)) {
                
                
                
                
                
                
                this.datasource.updateVisibleList(id, location.name, false);
                this.uninstallItem(id);
              }
            }
          }
        }
      }

      
      for (var id in actualItems) {
        if (!(location.name in StartupCache.entries) ||
            !(id in StartupCache.entries[location.name]) ||
            !StartupCache.entries[location.name][id]) {
          
          StartupCache.put(location, id, OP_NONE, true);
          
          newItems.push({location: location, id: id});
        }
      }
    }

    
    
    
    for (var i = 0; i < newItems.length; ++i) {
      var id = newItems[i].id;
      var location = newItems[i].location;
      if (canUse(id, location)) {
        LOG("Item Installed via directory addition to Install Location: " +
            location.name + " Item ID: " + id + ", attempting to register...");
        this.installItem(id, location,
                         function(installManifest, id, location, type) {
                           em._configureForthcomingItem(installManifest, id, location,
                                                        type);
                         });
        
        
        var installDisabled = location.getItemFile(id, "InstallDisabled");
        if (installDisabled.exists())
          em.disableItem(id);
        isDirty = true;
      }
    }

    
    
    installDroppedInFiles(droppedInFiles, xpinstallStrings);

    return isDirty;
  },

  _checkForUncoveredItem: function EM__checkForUncoveredItem(id) {
    var ds = this.datasource;
    var oldLocation = this.getInstallLocation(id);
    var newLocations = [];
    for (var locationKey in StartupCache.entries) {
      var location = InstallLocations.get(locationKey);
      if (id in StartupCache.entries[locationKey] &&
          location.priority > oldLocation.priority)
        newLocations.push(location);
    }
    newLocations.sort(function(a, b) { return b.priority - a.priority; });
    if (newLocations.length > 0) {
      for (var i = 0; i < newLocations.length; ++i) {
        
        var installRDF = newLocations[i].getItemFile(id, FILE_INSTALL_MANIFEST);
        if (installRDF.exists()) {
          
          
          var name = newLocations[i].name;
          ds.updateVisibleList(id, name, true);
          PendingOperations.addItem(OP_NEEDS_UPGRADE,
                                    { locationKey: name, id: id });
          PendingOperations.addItem(OP_NEEDS_INSTALL,
                                    { locationKey: name, id: id });
          break;
        }
        else {
          
          
          StartupCache.clearEntry(newLocations[i], id);
          ds.updateVisibleList(id, null, true);
        }
      }
    }
    else
      ds.updateVisibleList(id, null, true);
  },

  





  _finishOperations: function EM__finishOperations() {
    try {
      
      
      var ds = this.datasource;
      var updatedTargetAppInfos = [];

      var needsRestart = false;
      var upgrades = [];
      var newAddons = [];
      var addons = getPref("getCharPref", PREF_EM_NEW_ADDONS_LIST, "");
      if (addons != "")
        newAddons = addons.split(",");
      do {
        
        
        
        var items = PendingOperations.getOperations(OP_NEEDS_ENABLE);
        for (var i = items.length - 1; i >= 0; --i) {
          var id = items[i].id;
          var installLocation = this.getInstallLocation(id);
          StartupCache.put(installLocation, id, OP_NONE, true);
          PendingOperations.clearItem(OP_NEEDS_ENABLE, id);
          needsRestart = true;
        }
        PendingOperations.clearItems(OP_NEEDS_ENABLE);

        
        items = PendingOperations.getOperations(OP_NEEDS_DISABLE);
        for (i = items.length - 1; i >= 0; --i) {
          id = items[i].id;
          installLocation = this.getInstallLocation(id);
          StartupCache.put(installLocation, id, OP_NONE, true);
          PendingOperations.clearItem(OP_NEEDS_DISABLE, id);
          needsRestart = true;
        }
        PendingOperations.clearItems(OP_NEEDS_DISABLE);

        
        
        
        items = PendingOperations.getOperations(OP_NEEDS_UPGRADE);
        for (i = items.length - 1; i >= 0; --i) {
          id = items[i].id;
          var newLocation = InstallLocations.get(items[i].locationKey);
          
          var newTargetAppInfo = ds.getUpdatedTargetAppInfo(id);
          if (newTargetAppInfo)
            updatedTargetAppInfos.push(newTargetAppInfo);
          this._finalizeUpgrade(id, newLocation);
          upgrades.push(id);
        }
        PendingOperations.clearItems(OP_NEEDS_UPGRADE);

        
        items = PendingOperations.getOperations(OP_NEEDS_INSTALL);
        for (i = items.length - 1; i >= 0; --i) {
          needsRestart = true;
          id = items[i].id;
          
          newTargetAppInfo = ds.getUpdatedTargetAppInfo(id);
          if (newTargetAppInfo)
            updatedTargetAppInfos.push(newTargetAppInfo);
          this._finalizeInstall(id, null);
          if (upgrades.indexOf(id) < 0 && newAddons.indexOf(id) < 0)
            newAddons.push(id);
        }
        PendingOperations.clearItems(OP_NEEDS_INSTALL);

        
        
        
        items = PendingOperations.getOperations(OP_NEEDS_UNINSTALL);
        for (i = items.length - 1; i >= 0; --i) {
          id = items[i].id;
          this._finalizeUninstall(id);
          this._checkForUncoveredItem(id);
          needsRestart = true;
          var pos = newAddons.indexOf(id);
          if (pos >= 0)
            newAddons.splice(pos, 1);
        }
        PendingOperations.clearItems(OP_NEEDS_UNINSTALL);

        
        if (PendingOperations.size == 0) {
          
          for (i = 0; i < updatedTargetAppInfos.length; ++i)
            ds.setTargetApplicationInfo(updatedTargetAppInfos[i].id,
                                        updatedTargetAppInfos[i].targetAppID,
                                        updatedTargetAppInfos[i].minVersion,
                                        updatedTargetAppInfos[i].maxVersion,
                                        null);

          
          var ctr = getContainer(ds, ds._itemRoot);
          var elements = ctr.GetElements();
          while (elements.hasMoreElements()) {
            var itemResource = elements.getNext().QueryInterface(Ci.nsIRDFResource);

            
            id = stripPrefix(itemResource.Value, PREFIX_ITEM_URI);
            if (this._isUsableItem(id))
              ds.setItemProperty(id, EM_R("appDisabled"), null);
            else
              ds.setItemProperty(id, EM_R("appDisabled"), EM_L("true"));

            
            
            
            var value = stringData(ds.GetTarget(itemResource, EM_R("userDisabled"), true));
            if (value == OP_NEEDS_ENABLE)
              ds.setItemProperty(id, EM_R("userDisabled"), null);
            else if (value == OP_NEEDS_DISABLE)
              ds.setItemProperty(id, EM_R("userDisabled"), EM_L("true"));
          }
        }
      }
      while (PendingOperations.size > 0);

      
      
      
      this._updateManifests(needsRestart);

      
      
      if (!gFirstRun && newAddons.length > 0)
        gPref.setCharPref(PREF_EM_NEW_ADDONS_LIST, newAddons.join(","));
    }
    catch (e) {
      ERROR("ExtensionManager:_finishOperations - failure, catching exception - lineno: " +
            e.lineNumber + " - file: " + e.fileName + " - " + e);
    }
    return needsRestart;
  },

  







  checkForMismatches: function EM_checkForMismatches() {
    
    
    var currAppVersion = gApp.version;
    var lastAppVersion = getPref("getCharPref", PREF_EM_LAST_APP_VERSION, "");
    if (currAppVersion == lastAppVersion)
      return false;
    
    if (!lastAppVersion) {
      gPref.setCharPref(PREF_EM_LAST_APP_VERSION, currAppVersion);
      return false;
    }

    
    gAllowFlush = false;

    
    var isDirty;
    [isDirty,] = this._ensureDatasetIntegrity();

    if (this._checkForFileChanges())
      isDirty = true;

    if (PendingOperations.size != 0)
      isDirty = true;

    var ds = this.datasource;
    var inactiveItemIDs = [];
    var ctr = getContainer(ds, ds._itemRoot);
    var elements = ctr.GetElements();
    while (elements.hasMoreElements()) {
      var itemResource = elements.getNext().QueryInterface(Ci.nsIRDFResource);
      var id = stripPrefix(itemResource.Value, PREFIX_ITEM_URI);
      var appDisabled = ds.getItemProperty(id, "appDisabled");
      var userDisabled = ds.getItemProperty(id, "userDisabled")
      if (appDisabled == "true" || appDisabled == OP_NEEDS_DISABLE ||
          userDisabled == "true" || userDisabled == OP_NEEDS_DISABLE)
        inactiveItemIDs.push(id);
    }

    if (isDirty)
      this._finishOperations();

    
    ds.beginUpdateBatch();
    var allResources = ds.GetAllResources();
    while (allResources.hasMoreElements()) {
      var res = allResources.getNext().QueryInterface(Ci.nsIRDFResource);
      if (ds.GetTarget(res, EM_R("downloadURL"), true) ||
          (!ds.GetTarget(res, EM_R("installLocation"), true) &&
          stringData(ds.GetTarget(res, EM_R("appDisabled"), true)) == "true"))
        ds.removeDownload(res.Value);
    }
    ds.endUpdateBatch();

    var badItems = [];
    var disabledAddons = [];
    var allAppManaged = true;
    elements = ctr.GetElements();
    while (elements.hasMoreElements()) {
      var itemResource = elements.getNext().QueryInterface(Ci.nsIRDFResource);
      var id = stripPrefix(itemResource.Value, PREFIX_ITEM_URI);
      var location = this.getInstallLocation(id);
      if (!location) {
        
        badItems.push(id);
        continue;
      }

      if (ds.getItemProperty(id, "appManaged") == "true") {
        
        
        
        if (location.name == KEY_APP_GLOBAL) {
          var installRDF = location.getItemFile(id, FILE_INSTALL_MANIFEST);
          if (installRDF.exists()) {
            var metadataDS = getInstallManifest(installRDF);
            ds.addItemMetadata(id, metadataDS, location);
            ds.updateProperty(id, "compatible");
          }
        }
      }
      else if (allAppManaged)
        allAppManaged = false;

      var properties = {
        availableUpdateURL: null,
        availableUpdateVersion: null
      };

      if (ds.getItemProperty(id, "providesUpdatesSecurely") == "false") {
        

        installRDF = location.getItemFile(id, FILE_INSTALL_MANIFEST);
        if (installRDF.exists()) {
          metadataDS = getInstallManifest(installRDF);
          var literal = metadataDS.GetTarget(gInstallManifestRoot, EM_R("updateKey"), true);
          if (literal && literal instanceof Ci.nsIRDFLiteral)
            ds.setItemProperty(id, EM_R("updateKey"), literal);
        }
      }

      
      
      if (this._isUsableItem(id)) {
        if (ds.getItemProperty(id, "appDisabled"))
          properties.appDisabled = null;
      }
      else if (!ds.getItemProperty(id, "appDisabled")) {
        properties.appDisabled = EM_L("true");
        disabledAddons.push(id);
      }

      ds.setItemProperties(id, properties);
    }

    
    
    for (var i = 0; i < badItems.length; i++) {
      id = badItems[i];
      LOG("Item " + id + " was installed in an unknown location, removing.");
      var disabled = ds.getItemProperty(id, "userDisabled") == "true";
      
      ds.removeCorruptItem(id);
      
      var entries = StartupCache.findEntries(id);
      if (entries.length > 0) {
        var newLocation = InstallLocations.get(entries[0].location);
        for (var j = 1; j < entries.length; j++) {
          location = InstallLocations.get(entries[j].location);
          if (newLocation.priority < location.priority)
            newLocation = location;
        }
        LOG("Activating item " + id + " in " + newLocation.name);
        var em = this;
        this.installItem(id, newLocation,
                         function(installManifest, id, location, type) {
                           em._configureForthcomingItem(installManifest, id, location,
                                                        type);
                         });
        if (disabled)
          em.disableItem(id);
      }
    }

    
    this._updateManifests(true);

    
    
    if (!allAppManaged && !gFirstRun && disabledAddons.length > 0) {
      
      if (getPref("getBoolPref", PREF_EM_SHOW_MISMATCH_UI, true)) {
        this._showMismatchWindow(inactiveItemIDs);
      }
      else {
        
        gPref.setCharPref(PREF_EM_DISABLED_ADDONS_LIST, disabledAddons.join(","));
      }
    } else if (gPref.prefHasUserValue(PREF_EM_DISABLED_ADDONS_LIST)) {
      
      gPref.clearUserPref(PREF_EM_DISABLED_ADDONS_LIST);
    }

    
    
    if (PendingOperations.size != 0)
      this._finishOperations();

    
    gPref.setCharPref(PREF_EM_LAST_APP_VERSION, currAppVersion);

    
    gPref.setBoolPref(PREF_UPDATE_NOTIFYUSER, false);

    
    try {
      gAllowFlush = true;
      if (gManifestNeedsFlush) {
        gManifestNeedsFlush = false;
        this._updateManifests(false);
      }
      if (gDSNeedsFlush) {
        gDSNeedsFlush = false;
        this.datasource.Flush();
      }
    }
    catch (e) {
      ERROR("Error flushing caches: " + e);
    }

    return true;
  },

  





  _showMismatchWindow: function EM__showMismatchWindow(items) {
    var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
             getService(Ci.nsIWindowMediator);
    var wizard = wm.getMostRecentWindow("Update:Wizard");
    if (wizard)
      wizard.focus();
    else {
      var variant = Cc["@mozilla.org/variant;1"].
                    createInstance(Ci.nsIWritableVariant);
      variant.setFromVariant(items);
      var features = "chrome,centerscreen,dialog,titlebar,modal";
      
      
      var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
               getService(Ci.nsIWindowWatcher);
      ww.openWindow(null, URI_EXTENSION_UPDATE_DIALOG, "", features, variant);
    }
  },

  




  _updateManifests: function EM__updateManifests(needsRestart) {
    
    
    if (gAllowFlush) {
      
      StartupCache.write();
      
      this._updateExtensionsManifest();
    }
    else {
      gManifestNeedsFlush = true;
    }

    
    this._extensionListChanged = needsRestart;
  },

  






  _getActiveItems: function EM__getActiveItems(type) {
    var allItems = this.getItemList(type);
    var activeItems = [];
    var ds = this.datasource;
    for (var i = 0; i < allItems.length; ++i) {
      var item = allItems[i];

      var installLocation = this.getInstallLocation(item.id);
      
      if (!installLocation)
        continue;
      
      
      if (installLocation.name in StartupCache.entries &&
          item.id in StartupCache.entries[installLocation.name] &&
          StartupCache.entries[installLocation.name][item.id]) {
        var op = StartupCache.entries[installLocation.name][item.id].op;
        if (op == OP_NEEDS_INSTALL || op == OP_NEEDS_UPGRADE ||
            op == OP_NEEDS_UNINSTALL || op == OP_NEEDS_DISABLE)
          continue;
      }
      
      if (ds.getItemProperty(item.id, "isDisabled") != "true")
        activeItems.push({ id: item.id, version: item.version,
                           location: installLocation });
    }

    return activeItems;
  },

  


  _updateExtensionsManifest: function EM__updateExtensionsManifest() {
    
    
    
    
    
    
    
    var validExtensions = this._getActiveItems(Ci.nsIUpdateItem.TYPE_ANY -
                                               Ci.nsIUpdateItem.TYPE_THEME);
    var validThemes     = this._getActiveItems(Ci.nsIUpdateItem.TYPE_THEME);

    var extensionsLocationsFile = FileUtils.getFile(KEY_PROFILEDIR,
                                                    [FILE_EXTENSION_MANIFEST]);
    var fos = FileUtils.openSafeFileOutputStream(extensionsLocationsFile);

    var enabledItems = [];
    var extensionSectionHeader = "[ExtensionDirs]\r\n";
    fos.write(extensionSectionHeader, extensionSectionHeader.length);
    for (var i = 0; i < validExtensions.length; ++i) {
      var e = validExtensions[i];
      var itemLocation = e.location.getItemLocation(e.id).QueryInterface(Ci.nsILocalFile);
      var descriptor = getAbsoluteDescriptor(itemLocation);
      var line = "Extension" + i + "=" + descriptor + "\r\n";
      fos.write(line, line.length);
      enabledItems.push(e.id + ":" + e.version);
    }

    var themeSectionHeader = "[ThemeDirs]\r\n";
    fos.write(themeSectionHeader, themeSectionHeader.length);
    for (i = 0; i < validThemes.length; ++i) {
      var e = validThemes[i];
      var itemLocation = e.location.getItemLocation(e.id).QueryInterface(Ci.nsILocalFile);
      var descriptor = getAbsoluteDescriptor(itemLocation);
      var line = "Extension" + i + "=" + descriptor + "\r\n";
      fos.write(line, line.length);
      enabledItems.push(e.id + ":" + e.version);
    }

    FileUtils.closeSafeFileOutputStream(fos);

    
    gPref.setCharPref(PREF_EM_ENABLED_ITEMS, enabledItems.join(","));
  },

  






  set _extensionListChanged(val) {
    
    
    
    
    
    if (val) {
      let XRE = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime);
      XRE.invalidateCachesOnRestart();
    }
    return val;
  },

  

































  _getInstallData: function EM__getInstallData(installManifest) {
    var installData = { id          : "",
                        version     : "",
                        name        : "",
                        type        : 0,
                        error       : INSTALLERROR_SUCCESS,
                        targetApps  : [],
                        updateURL   : "",
                        updateKey   : "",
                        currentApp  : null };

    
    installData.id       = getManifestProperty(installManifest, "id");
    installData.version  = getManifestProperty(installManifest, "version");
    installData.name     = getManifestProperty(installManifest, "name");
    installData.type     = getAddonTypeFromInstallManifest(installManifest);
    installData.updateURL= getManifestProperty(installManifest, "updateURL");
    installData.updateKey= getManifestProperty(installManifest, "updateKey");

    







    function readTAProperty(resource, property) {
      return stringData(installManifest.GetTarget(resource, EM_R(property), true));
    }

    var targetApps = installManifest.GetTargets(gInstallManifestRoot,
                                                EM_R("targetApplication"),
                                                true);
    while (targetApps.hasMoreElements()) {
      var targetApp = targetApps.getNext();
      if (targetApp instanceof Ci.nsIRDFResource) {
        try {
          var data = { id        : readTAProperty(targetApp, "id"),
                       minVersion: readTAProperty(targetApp, "minVersion"),
                       maxVersion: readTAProperty(targetApp, "maxVersion") };
          installData.targetApps.push(data);
          if ((data.id == gApp.ID) ||
              (data.id == TOOLKIT_ID) && !installData.currentApp)
            installData.currentApp = data;
        }
        catch (e) {
          continue;
        }
      }
    }

    
    
    var targetPlatforms = null;
    try {
      targetPlatforms = installManifest.GetTargets(gInstallManifestRoot,
                                                   EM_R("targetPlatform"),
                                                   true);
    } catch(e) {
      
    }
    if (targetPlatforms != null && targetPlatforms.hasMoreElements()) {
      var foundMatchingOS = false;
      var foundMatchingOSAndABI = false;
      var requireABICompatibility = false;
      while (targetPlatforms.hasMoreElements()) {
        var targetPlatform = stringData(targetPlatforms.getNext());
        var os = targetPlatform.split("_")[0];
        var index = targetPlatform.indexOf("_");
        var abi = index != -1 ? targetPlatform.substr(index + 1) : null;
        if (os == gOSTarget) {
          foundMatchingOS = true;
          
          if (abi != null) {
            requireABICompatibility = true;
            
            if (abi == gXPCOMABI && abi != UNKNOWN_XPCOM_ABI) {
              foundMatchingOSAndABI = true;
              break;
            }
          }
        }
      }
      if (!foundMatchingOS || (requireABICompatibility && !foundMatchingOSAndABI)) {
        installData.error = INSTALLERROR_INCOMPATIBLE_PLATFORM;
        return installData;
      }
    }

    
    if (!gIDTest.test(installData.id)) {
      installData.error = INSTALLERROR_INVALID_GUID;
      return installData;
    }
    
    
    if (gCheckUpdateSecurity &&
        installData.updateURL &&
        installData.updateURL.substring(0, 6) != "https:" &&
        !installData.updateKey) {
      installData.error = INSTALLERROR_INSECURE_UPDATE;
      return installData;
    }

    
    if (!this.datasource.isCompatible(installManifest, gInstallManifestRoot, false)) {
      installData.error = INSTALLERROR_INCOMPATIBLE_VERSION;
      return installData;
    }
    
    
    if (!gBlocklist)
      gBlocklist = Cc["@mozilla.org/extensions/blocklist;1"].
                   getService(Ci.nsIBlocklistService);
    var state = gBlocklist.getAddonBlocklistState(installData.id, installData.version);
    if (state == Ci.nsIBlocklistService.STATE_BLOCKED)
      installData.error = INSTALLERROR_BLOCKLISTED;
    else if (state == Ci.nsIBlocklistService.STATE_SOFTBLOCKED)
      installData.error = INSTALLERROR_SOFTBLOCKED;

    return installData;
  },

  









  installItemFromFile: function EM_installItemFromFile(xpiFile, installLocationKey) {
    this.installItemFromFileInternal(xpiFile, installLocationKey, null);

    
    
    if (this._compatibilityCheckCount == 0 && this._transactions.length == 0)
      this._callInstallListeners("onInstallsCompleted");
  },

  














  installItemFromFileInternal: function EM_installItemFromFileInternal(aXPIFile,
                                                                       aInstallLocationKey,
                                                                       aInstallManifest) {
    var em = this;
    















    function getInstallLocation(itemID) {
      
      var installLocation = em.getInstallLocation(itemID);
      if (!installLocation) {
        
        

        
        
        installLocation = InstallLocations.get(aInstallLocationKey);
        if (installLocation) {
          
          
          
          
          
          
          if (!installLocation.location) {
            LOG("Install Location \"" + installLocation.name + "\" does not support " +
                "installation of items from XPI/JAR files. You must manage " +
                "installation and update of these items yourself.");
            installLocation = null;
          }
        }
        else {
          
          
          installLocation = InstallLocations.get(KEY_APP_PROFILE);
        }
      }
      else {
        
        
        
        
        

        
        
        
        
        if (installLocation.name != aInstallLocationKey)
          installLocation = InstallLocations.get(aInstallLocationKey);
      }
      if (!installLocation.canAccess) {
        LOG("Install Location\"" + installLocation.name + "\" cannot be written " +
            "to with your access privileges. Installation will not proceed.");
        installLocation = null;
      }
      return installLocation;
    }

    




    function stageXPIForOtherApps(xpiFile, installData) {
      for (var i = 0; i < installData.targetApps.length; ++i) {
        var targetApp = installData.targetApps[i];
        if (targetApp.id != gApp.ID && targetApp.id != TOOLKIT_ID) {
        














        }
      }
    }

    



    function installMultiXPI(xpiFile, installData) {
      var fileURL = getURIFromFile(xpiFile).QueryInterface(Ci.nsIURL);
      if (fileURL.fileExtension.toLowerCase() != "xpi") {
        LOG("Invalid File Extension: Item: \"" + fileURL.fileName + "\" has an " +
            "invalid file extension. Only xpi file extensions are allowed for " +
            "multiple item packages.");
        var bundle = BundleManager.getBundle(URI_EXTENSIONS_PROPERTIES);
        showMessage("invalidFileExtTitle", [],
                    "invalidFileExtMessage", [installData.name,
                    fileURL.fileExtension,
                    bundle.GetStringFromName("type-" + installData.type)]);
        return;
      }

      try {
        var zipReader = getZipReaderForFile(xpiFile);
      }
      catch (e) {
        LOG("installMultiXPI: failed to open xpi file: " + xpiFile.path);
        throw e;
      }

      var searchForEntries = ["*.xpi", "*.jar"];
      var files = [];
      for (var i = 0; i < searchForEntries.length; ++i) {
        var entries = zipReader.findEntries(searchForEntries[i]);
        while (entries.hasMore()) {
          var entryName = entries.getNext();
          var target = FileUtils.getFile(KEY_TEMPDIR, [entryName]);
          try {
            target.createUnique(Ci.nsILocalFile.NORMAL_FILE_TYPE,
                                FileUtils.PERMS_FILE);
          }
          catch (e) {
            LOG("installMultiXPI: failed to create target file for extraction " +
                " file = " + target.path + ", exception = " + e + "\n");
          }
          zipReader.extract(entryName, target);
          files.push(target);
        }
      }
      zipReader.close();

      if (files.length == 0) {
        LOG("Multiple Item Package: Item: \"" + fileURL.fileName + "\" does " +
            "not contain a valid package to install.");
        var bundle = BundleManager.getBundle(URI_EXTENSIONS_PROPERTIES);
        showMessage("missingPackageFilesTitle",
                    [bundle.GetStringFromName("type-" + installData.type)],
                    "missingPackageFilesMessage", [installData.name,
                    bundle.GetStringFromName("type-" + installData.type)]);
        return;
      }

      for (i = 0; i < files.length; ++i) {
        em.installItemFromFileInternal(files[i], aInstallLocationKey, null);
        files[i].remove(false);
      }
    }

    



    function IncompatibleObserver() {}
    IncompatibleObserver.prototype = {
      _xpi: null,
      _installManifest: null,

      













      checkForUpdates: function IncObs_checkForUpdates(item, installManifest, xpiFile) {
        this._xpi             = xpiFile;
        this._installManifest = installManifest;

        em._callInstallListeners("onCompatibilityCheckStarted", item);
        em._compatibilityCheckCount++;
        var updater = new ExtensionItemUpdater(em);
        updater.checkForUpdates([item], 1,
                                Ci.nsIExtensionManager.UPDATE_CHECK_COMPATIBILITY,
                                this, UPDATE_WHEN_ADDON_INSTALLED);
      },

      


      onUpdateStarted: function IncObs_onUpdateStarted() {
        LOG("Phone Home Listener: Update Started");
      },

      


      onUpdateEnded: function IncObs_onUpdateEnded() {
        LOG("Phone Home Listener: Update Ended");
      },

      


      onAddonUpdateStarted: function IncObs_onAddonUpdateStarted(addon) {
        if (!addon)
          throw Cr.NS_ERROR_INVALID_ARG;

        LOG("Phone Home Listener: Update For " + addon.id + " started");
        em.datasource.addIncompatibleUpdateItem(addon.name, this._xpi.path,
                                                addon.type, addon.version);
      },

      


      onAddonUpdateEnded: function IncObs_onAddonUpdateEnded(addon, status) {
        if (!addon)
          throw Cr.NS_ERROR_INVALID_ARG;

        LOG("Phone Home Listener: Update For " + addon.id + " ended, status = " + status);
        em.datasource.removeDownload(this._xpi.path);
        LOG("Version Check Phone Home Completed");

        em._callInstallListeners("onCompatibilityCheckEnded", addon, status);

        
        
        if (status == Ci.nsIAddonUpdateCheckListener.STATUS_VERSIONINFO) {
          em.datasource.setTargetApplicationInfo(addon.id,
                                                 addon.targetAppID,
                                                 addon.minAppVersion,
                                                 addon.maxAppVersion,
                                                 this._installManifest);

          
          
          var status = em.installItemFromFileInternal(this._xpi,
                                                      aInstallLocationKey,
                                                      this._installManifest);

          
          if (status == INSTALLERROR_SUCCESS) {
            
            if (StartupCache.entries[aInstallLocationKey][addon.id].op == OP_NONE) {
              em.datasource.setTargetApplicationInfo(addon.id,
                                                     addon.targetAppID,
                                                     addon.minAppVersion,
                                                     addon.maxAppVersion,
                                                     null);
            }
            else { 
              
              
              em.datasource.setUpdatedTargetAppInfo(addon.id,
                                                    addon.targetAppID,
                                                    addon.minAppVersion,
                                                    addon.maxAppVersion,
                                                    null);
            }
          }
        }
        else {
          em.datasource.removeDownload(this._xpi.path);
          showIncompatibleError(installData);
          LOG("Add-on " + addon.id + " is incompatible with " +
              BundleManager.appName + " " + gApp.version + ", Toolkit " +
              gApp.platformVersion + ". Remote compatibility check did not " +
              "resolve this.");

          em._callInstallListeners("onInstallEnded", addon, INSTALLERROR_INCOMPATIBLE_VERSION);

          
          InstallLocations.get(aInstallLocationKey).removeFile(this._xpi);
        }

        em._compatibilityCheckCount--;
        
        
        if (em._compatibilityCheckCount == 0 && em._transactions.length == 0)
          em._callInstallListeners("onInstallsCompleted");
      },

      QueryInterface: XPCOMUtils.generateQI([Ci.nsIAddonUpdateCheckListener])
    }

    var shouldPhoneHomeIfNecessary = false;
    if (!aInstallManifest) {
      
      
      
      

      var addon = makeItem(getURIFromFile(aXPIFile).spec, "",
                           aInstallLocationKey, "", "", "",
                           getURIFromFile(aXPIFile).spec,
                           "", "", "", "", 0, gApp.ID);
      this._callInstallListeners("onInstallStarted", addon);

      shouldPhoneHomeIfNecessary = true;
      var installManifest = null;
      var installManifestFile = extractRDFFileToTempDir(aXPIFile,
                                                        FILE_INSTALL_MANIFEST,
                                                        true);
      if (installManifestFile.exists()) {
        installManifest = getInstallManifest(installManifestFile);
        installManifestFile.remove(false);
      }
      if (!installManifest) {
        LOG("The Install Manifest supplied by this item is not well-formed. " +
            "Installation will not proceed.");
        this._callInstallListeners("onInstallEnded", addon, INSTALLERROR_INVALID_MANIFEST);
        return INSTALLERROR_INVALID_MANIFEST;
      }
    }
    else
      installManifest = aInstallManifest;

    var installData = this._getInstallData(installManifest);
    
    addon = makeItem(installData.id, installData.version,
                     aInstallLocationKey,
                     installData.currentApp ? installData.currentApp.minVersion : "",
                     installData.currentApp ? installData.currentApp.maxVersion : "",
                     installData.name,
                     getURIFromFile(aXPIFile).spec,
                     "", 
                     "", 
                     installData.updateURL || "",
                     installData.updateKey || "",
                     installData.type,
                     installData.currentApp ? installData.currentApp.id : "");

    switch (installData.error) {
    case INSTALLERROR_INCOMPATIBLE_VERSION:
      
      
      
      
      if (shouldPhoneHomeIfNecessary && installData.currentApp) {
        var installLocation = getInstallLocation(installData.id, aInstallLocationKey);
        if (!installLocation)
          return INSTALLERROR_INCOMPATIBLE_VERSION;
        var stagedFile = installLocation.stageFile(aXPIFile, installData.id);
        (new IncompatibleObserver(this)).checkForUpdates(addon, installManifest,
                                                         stagedFile);
        
        return INSTALLERROR_PHONING_HOME;
      }
      else {
        
        
        
        showIncompatibleError(installData);
        LOG("Add-on " + installData.id + " is incompatible with " +
            BundleManager.appName + " " + gApp.version + ", Toolkit " +
            gApp.platformVersion + ". Remote compatibility check was not performed.");
      }
      break;
    case INSTALLERROR_SOFTBLOCKED:
      if (!showBlocklistMessage(installData, true))
        break;
      installData.error = INSTALLERROR_SUCCESS;
      
    case INSTALLERROR_SUCCESS:
      
      if (installData.type == Ci.nsIUpdateItem.TYPE_MULTI_XPI) {
        installMultiXPI(aXPIFile, installData);
        break;
      }

      
      var installLocation = getInstallLocation(installData.id, aInstallLocationKey);
      if (!installLocation) {
        
        
        
        
        
        this._callInstallListeners("onInstallEnded", addon, INSTALLERROR_RESTRICTED);
        return INSTALLERROR_RESTRICTED;
      }

      
      stagedFile = installLocation.stageFile(aXPIFile, installData.id);

      var restartRequired = this.installRequiresRestart(installData.id,
                                                        installData.type);
      
      
      
      var ds = this.datasource;
      if (installData.id in ds.visibleItems && ds.visibleItems[installData.id]) {
        
        
        
        
        
        var oldInstallLocation = this.getInstallLocation(installData.id);
        if (oldInstallLocation.priority >= installLocation.priority) {
          this._upgradeItem(installManifest, installData.id, installLocation,
                            installData.type);
          if (!restartRequired) {
            this._finalizeUpgrade(installData.id, installLocation);
            this._finalizeInstall(installData.id, stagedFile);
          }
        }
      }
      else {
        this._configureForthcomingItem(installManifest, installData.id,
                                        installLocation, installData.type);
        if (!restartRequired) {
          this._finalizeInstall(installData.id, stagedFile);
          if (installData.type == Ci.nsIUpdateItem.TYPE_THEME) {
            var internalName = this.datasource.getItemProperty(installData.id, "internalName");
            if (gPref.getBoolPref(PREF_EM_DSS_ENABLED)) {
              gPref.setCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN, internalName);
            }
            else {
              gPref.setBoolPref(PREF_DSS_SWITCHPENDING, true);
              gPref.setCharPref(PREF_DSS_SKIN_TO_SELECT, internalName);
            }
          }
        }
      }
      this._updateManifests(restartRequired);
      break;
    case INSTALLERROR_INVALID_GUID:
      LOG("Invalid GUID: Item has GUID: \"" + installData.id + "\"" +
          " which is not well-formed.");
      var bundle = BundleManager.getBundle(URI_EXTENSIONS_PROPERTIES);
      showMessage("incompatibleTitle",
                  [bundle.GetStringFromName("type-" + installData.type)],
                  "invalidGUIDMessage", [installData.name, installData.id]);
      break;
    case INSTALLERROR_INVALID_VERSION:
      LOG("Invalid Version: Item: \"" + installData.id + "\" has version " +
          installData.version + " which is not well-formed.");
      var bundle = BundleManager.getBundle(URI_EXTENSIONS_PROPERTIES);
      showMessage("incompatibleTitle",
                  [bundle.GetStringFromName("type-" + installData.type)],
                  "invalidVersionMessage", [installData.name, installData.version]);
      break;
    case INSTALLERROR_INCOMPATIBLE_PLATFORM:
      const osABI = gOSTarget + "_" + gXPCOMABI;
      LOG("Incompatible Platform: Item: \"" + installData.id + "\" is not " +
          "compatible with '" + osABI + "'.");
      var bundle = BundleManager.getBundle(URI_EXTENSIONS_PROPERTIES);
      showMessage("incompatibleTitle",
                  [bundle.GetStringFromName("type-" + installData.type)],
                  "incompatiblePlatformMessage",
                  [installData.name, BundleManager.appName, osABI]);
      break;
    case INSTALLERROR_BLOCKLISTED:
      LOG("Blocklisted Item: Item: \"" + installData.id + "\" version " +
          installData.version + " was not installed.");
      showBlocklistMessage(installData, false);
      break;
    case INSTALLERROR_INSECURE_UPDATE:
      LOG("No secure updates: Item: \"" + installData.id + "\" version " + 
          installData.version + " was not installed.");
      var bundle = BundleManager.getBundle(URI_EXTENSIONS_PROPERTIES);
      showMessage("incompatibleTitle", 
                  [bundle.GetStringFromName("type-" + installData.type)], 
                  "insecureUpdateMessage", [installData.name]);
      break;
    default:
      break;
    }

    
    
    stageXPIForOtherApps(aXPIFile, installData);

    
    this._callInstallListeners("onInstallEnded", addon, installData.error);
    return installData.error;
  },

  









  installRequiresRestart: function EM_installRequiresRestart(id, type) {
    switch (type) {
    case Ci.nsIUpdateItem.TYPE_THEME:
      var internalName = this.datasource.getItemProperty(id, "internalName");
      var needsRestart = false;
      if (gPref.prefHasUserValue(PREF_DSS_SKIN_TO_SELECT))
        needsRestart = internalName == gPref.getCharPref(PREF_DSS_SKIN_TO_SELECT);
      if (!needsRestart &&
          gPref.prefHasUserValue(PREF_GENERAL_SKINS_SELECTEDSKIN))
        needsRestart = internalName == gPref.getCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN);
      return needsRestart;
    }
    return ((type & Ci.nsIUpdateItem.TYPE_ADDON) > 0);
  },

  













  _configureForthcomingItem: function EM__configureForthcomingItem(installManifest,
                                                                   id,
                                                                   installLocation,
                                                                   type) {
    var ds = this.datasource;
    ds.updateVisibleList(id, installLocation.name, false);

    var name = null;
    var localized = findClosestLocalizedResource(installManifest, gInstallManifestRoot);
    if (localized)
      name = installManifest.GetTarget(localized, EM_R("name"), true);
    else
      name = EM_L(getManifestProperty(installManifest, "name"));

    var props = { name            : name,
                  version         : EM_L(getManifestProperty(installManifest, "version")),
                  newVersion      : EM_L(getManifestProperty(installManifest, "version")),
                  installLocation : EM_L(installLocation.name),
                  type            : EM_I(type),
                  availableUpdateURL    : null,
                  availableUpdateHash   : null,
                  availableUpdateVersion: null,
                  availableUpdateInfo   : null };
    ds.setItemProperties(id, props);
    ds.updateProperty(id, "availableUpdateURL");

    this._setOp(id, OP_NEEDS_INSTALL);

    
    
    
    
    
    
    
    ds.insertItemIntoContainer(id);

    this._notifyAction(id, EM_ITEM_INSTALLED);
  },

  










  _upgradeItem: function EM__upgradeItem(installManifest, id, installLocation, type) {
    
    
    var ds = this.datasource;
    ds.updateVisibleList(id, installLocation.name, false);
    var props = { installLocation : EM_L(installLocation.name),
                  type            : EM_I(type),
                  newVersion      : EM_L(getManifestProperty(installManifest, "version")),
                  availableUpdateURL      : null,
                  availableUpdateHash     : null,
                  availableUpdateVersion  : null,
                  availableUpdateInfo     : null };
    ds.setItemProperties(id, props);
    ds.updateProperty(id, "availableUpdateURL");

    this._setOp(id, OP_NEEDS_UPGRADE);
    this._notifyAction(id, EM_ITEM_UPGRADED);
  },

  







  _finalizeInstall: function EM__finalizeInstall(id, file) {
    var ds = this.datasource;
    var type = ds.getItemProperty(id, "type");
    if (id == 0 || id == -1) {
      ds.removeCorruptItem(id);
      return;
    }
    var installLocation = this.getInstallLocation(id);
    if (!installLocation) {
      
      
      
      
      var entries = StartupCache.findEntries(id);
      for (var i = 0; i < entries.length; ++i) {
        var location = InstallLocations.get(entries[i].location);
        StartupCache.clearEntry(location, id);
        PendingOperations.clearItem(OP_NEEDS_INSTALL, id);
      }
      return;
    }
    var itemLocation = installLocation.getItemLocation(id);

    if (!file && "stageFile" in installLocation)
      file = installLocation.getStageFile(id);

    
    
    if (file && file.exists())
      safeInstallOperation(id, installLocation, file);

    var metadataFile = installLocation.getItemFile(id, FILE_INSTALL_MANIFEST);
    if (metadataFile && metadataFile.exists()) {
      var metadataDS = getInstallManifest(metadataFile);
      if (metadataDS) {
        
        this.datasource.addItemMetadata(id, metadataDS, installLocation);
      }
    }
    else {
      LOG("_finalizeInstall: install manifest for extension " + id + " at " +
          metadataFile.path + " could not be loaded. Add-on is not usable.");
    }

    
    
    if (file)
      installLocation.removeFile(file);

    
    StartupCache.put(installLocation, id, OP_NONE, true);
    PendingOperations.clearItem(OP_NEEDS_INSTALL, id);
  },

  






  _finalizeUpgrade: function EM__finalizeUpgrade(id, installLocation) {
    
    var ds = this.datasource;

    var stagedFile = null;
    if ("getStageFile" in installLocation)
      stagedFile = installLocation.getStageFile(id);

    if (stagedFile)
      var installRDF = extractRDFFileToTempDir(stagedFile, FILE_INSTALL_MANIFEST, true);
    else
      installRDF = installLocation.getItemFile(id, FILE_INSTALL_MANIFEST);
    if (installRDF && installRDF.exists()) {
      var installManifest = getInstallManifest(installRDF);
      if (installManifest) {
        var type = getAddonTypeFromInstallManifest(installManifest);
        var userDisabled = ds.getItemProperty(id, "userDisabled") == "true";

        
        ds.removeItemMetadata(id);
        
        
        this._configureForthcomingItem(installManifest, id, installLocation,
                                       type);
        if (userDisabled)
          ds.setItemProperty(id, EM_R("userDisabled"), EM_L("true"));
      }
      if (stagedFile)
        installRDF.remove(false);
    }
    
    
    
    PendingOperations.clearItem(OP_NEEDS_UPGRADE, id);
  },

  




  _finalizeUninstall: function EM__finalizeUninstall(id) {
    var ds = this.datasource;

    var installLocation = this.getInstallLocation(id);
    if (!installLocation.itemIsManagedIndependently(id)) {
      try {
        
        
        safeInstallOperation(id, installLocation, null);
      }
      catch (e) {
        ERROR("_finalizeUninstall: failed to remove directory for item: " + id +
              " at Install Location: " + installLocation.name + ", rolling back uninstall");
        var manifest = installLocation.getItemFile(id, "FILE_INSTALL_MANIFEST");
        
        
        
        if (manifest && manifest.exists()) {
          
          
          
          StartupCache.put(installLocation, id, OP_NONE, true);
          var restartRequired = this.installRequiresRestart(id, ds.getItemProperty(id, "type"))
          this._updateManifests(restartRequired);
          return;
        }
      }
    }
    else if (installLocation.name == KEY_APP_PROFILE ||
             installLocation.name == KEY_APP_GLOBAL ||
             installLocation.name == KEY_APP_SYSTEM_USER) {
      
      var pointerFile = installLocation.location.clone();
      pointerFile.append(id);
      if (pointerFile.exists() && !pointerFile.isDirectory())
        pointerFile.remove(false);
    }

    
    ds.removeItemMetadata(id);

    
    
    ds.removeItemFromContainer(id);

    
    StartupCache.clearEntry(installLocation, id);
    PendingOperations.clearItem(OP_NEEDS_UNINSTALL, id);
  },

  





  uninstallItem: function EM_uninstallItem(id) {
    var ds = this.datasource;
    ds.updateDownloadState(PREFIX_ITEM_URI + id, null);
    if (!ds.isDownloadItem(id)) {
      var opType = ds.getItemProperty(id, "opType");
      var installLocation = this.getInstallLocation(id);
      
      if (opType == OP_NEEDS_UPGRADE || opType == OP_NEEDS_INSTALL) {
        var stageFile = installLocation.getStageFile(id);
        if (stageFile)
          installLocation.removeFile(stageFile);
      }
      
      
      if (opType == OP_NEEDS_INSTALL) {
        ds.removeItemMetadata(id);
        ds.removeItemFromContainer(id);
        ds.updateVisibleList(id, null, true);
        StartupCache.clearEntry(installLocation, id);
        this._updateManifests(false);
      }
      else {
        if (opType == OP_NEEDS_UPGRADE)
          ds.setItemProperty(id, EM_R("newVersion"), null);
        this._setOp(id, OP_NEEDS_UNINSTALL);
        var type = ds.getItemProperty(id, "type");
        var restartRequired = this.installRequiresRestart(id, type);
        if (!restartRequired) {
          this._finalizeUninstall(id);
          this._updateManifests(restartRequired);
        }
      }
    }
    else {
      
      
      ds.removeCorruptDLItem(id);
    }

    this._notifyAction(id, EM_ITEM_UNINSTALLED);
  },

  
  cancelInstallItem: function EM_cancelInstallItem(id) {
    var ds = this.datasource;
    var opType = ds.getItemProperty(id, "opType");
    if (opType != OP_NEEDS_UPGRADE && opType != OP_NEEDS_INSTALL)
      return;

    ds.updateDownloadState(PREFIX_ITEM_URI + id, null);
    var installLocation = this.getInstallLocation(id);
    
    var stageFile = installLocation.getStageFile(id);
    if (stageFile)
      installLocation.removeFile(stageFile);
    
    
    if (opType == OP_NEEDS_INSTALL) {
      ds.removeItemMetadata(id);
      ds.removeItemFromContainer(id);
      ds.updateVisibleList(id, null, true);
      StartupCache.clearEntry(installLocation, id);
      this._updateManifests(false);
      this._notifyAction(id, EM_ITEM_CANCEL);
    }
    else {
      
      ds.setItemProperty(id, EM_R("newVersion"), null);
      var appDisabled = ds.getItemProperty(id, "appDisabled");
      var userDisabled = ds.getItemProperty(id, "userDisabled");
      if (appDisabled == "true" || appDisabled == OP_NONE && userDisabled == OP_NONE) {
        this._setOp(id, OP_NONE);
        this._notifyAction(id, EM_ITEM_CANCEL);
      }
      else if (appDisabled == OP_NEEDS_DISABLE || userDisabled == OP_NEEDS_DISABLE) {
        this._setOp(id, OP_NEEDS_DISABLE);
        this._notifyAction(id, EM_ITEM_DISABLED);
      }
      else if (appDisabled == OP_NEEDS_ENABLE || userDisabled == OP_NEEDS_ENABLE) {
        this._setOp(id, OP_NEEDS_ENABLE);
        this._notifyAction(id, EM_ITEM_ENABLED);
      }
      else {
        this._setOp(id, OP_NONE);
        this._notifyAction(id, EM_ITEM_CANCEL);
      }
    }
  },

  




  cancelUninstallItem: function EM_cancelUninstallItem(id) {
    var ds = this.datasource;
    var appDisabled = ds.getItemProperty(id, "appDisabled");
    var userDisabled = ds.getItemProperty(id, "userDisabled");
    if (appDisabled == "true" || appDisabled == OP_NONE && userDisabled == OP_NONE) {
      this._setOp(id, OP_NONE);
      this._notifyAction(id, EM_ITEM_CANCEL);
    }
    else if (appDisabled == OP_NEEDS_DISABLE || userDisabled == OP_NEEDS_DISABLE) {
      this._setOp(id, OP_NEEDS_DISABLE);
      this._notifyAction(id, EM_ITEM_DISABLED);
    }
    else if (appDisabled == OP_NEEDS_ENABLE || userDisabled == OP_NEEDS_ENABLE) {
      this._setOp(id, OP_NEEDS_ENABLE);
      this._notifyAction(id, EM_ITEM_ENABLED);
    }
    else {
      this._setOp(id, OP_NONE);
      this._notifyAction(id, EM_ITEM_CANCEL);
    }
  },

  






  _setOp: function EM__setOp(id, op) {
    var location = this.getInstallLocation(id);
    StartupCache.put(location, id, op, true);
    PendingOperations.addItem(op, { locationKey: location.name, id: id });
    var ds = this.datasource;
    if (op == OP_NEEDS_INSTALL || op == OP_NEEDS_UPGRADE)
      ds.updateDownloadState(PREFIX_ITEM_URI + id, "success");

    ds.updateProperty(id, "opType");
    ds.updateProperty(id, "updateable");
    ds.updateProperty(id, "satisfiesDependencies");
    var restartRequired = this.installRequiresRestart(id, ds.getItemProperty(id, "type"))
    this._updateDependentItemsForID(id);
    this._updateManifests(restartRequired);
  },

  









  











  _appEnableItem: function EM__appEnableItem(id) {
    var ds = this.datasource;
    var appDisabled = ds.getItemProperty(id, "appDisabled");
    if (appDisabled == OP_NONE || appDisabled == OP_NEEDS_ENABLE)
      return;

    var opType = ds.getItemProperty(id, "opType");
    var userDisabled = ds.getItemProperty(id, "userDisabled");
    
    
    if (userDisabled == OP_NEEDS_DISABLE)
      ds.setItemProperty(id, EM_R("userDisabled"), null);
    else if (userDisabled == OP_NEEDS_ENABLE)
      ds.setItemProperty(id, EM_R("userDisabled"), EM_L("true"));

    if (appDisabled == OP_NEEDS_DISABLE)
      ds.setItemProperty(id, EM_R("appDisabled"), null);
    else if (appDisabled == "true")
      ds.setItemProperty(id, EM_R("appDisabled"), EM_L(OP_NEEDS_ENABLE));

    
    if (opType == OP_NEEDS_UNINSTALL) {
      this._updateDependentItemsForID(id);
      return;
    }

    var operation, action;
    
    
    
    if (appDisabled == OP_NEEDS_DISABLE || appDisabled == OP_NONE ||
        userDisabled == "true") {
      if (opType != OP_NONE) {
        operation = OP_NONE;
        action = EM_ITEM_CANCEL;
      }
    }
    else {
      if (opType != OP_NEEDS_ENABLE) {
        operation = OP_NEEDS_ENABLE;
        action = EM_ITEM_ENABLED;
      }
    }

    if (action) {
      this._setOp(id, operation);
      this._notifyAction(id, action);
    }
    else {
      ds.updateProperty(id, "satisfiesDependencies");
      this._updateDependentItemsForID(id);
    }
  },

  











  _appDisableItem: function EM__appDisableItem(id) {
    var ds = this.datasource;
    var appDisabled = ds.getItemProperty(id, "appDisabled");
    if (appDisabled == "true" || appDisabled == OP_NEEDS_DISABLE)
      return;

    var opType = ds.getItemProperty(id, "opType");
    var userDisabled = ds.getItemProperty(id, "userDisabled");

    
    
    if (userDisabled == OP_NEEDS_DISABLE)
      ds.setItemProperty(id, EM_R("userDisabled"), null);
    else if (userDisabled == OP_NEEDS_ENABLE)
      ds.setItemProperty(id, EM_R("userDisabled"), EM_L("true"));

    if (appDisabled == OP_NEEDS_ENABLE || userDisabled == OP_NEEDS_ENABLE ||
        ds.getItemProperty(id, "userDisabled") == "true")
      ds.setItemProperty(id, EM_R("appDisabled"), EM_L("true"));
    else if (appDisabled == OP_NONE)
      ds.setItemProperty(id, EM_R("appDisabled"), EM_L(OP_NEEDS_DISABLE));

    
    if (opType == OP_NEEDS_UNINSTALL) {
      this._updateDependentItemsForID(id);
      return;
    }

    var operation, action;
    
    
    if (appDisabled == OP_NEEDS_ENABLE || appDisabled == "true" ||
        userDisabled == OP_NEEDS_ENABLE || userDisabled == "true") {
      if (opType != OP_NONE) {
        operation = OP_NONE;
        action = EM_ITEM_CANCEL;
      }
    }
    else {
      if (opType != OP_NEEDS_DISABLE) {
        operation = OP_NEEDS_DISABLE;
        action = EM_ITEM_DISABLED;
      }
    }

    if (action) {
      this._setOp(id, operation);
      this._notifyAction(id, action);
    }
    else {
      ds.updateProperty(id, "satisfiesDependencies");
      this._updateDependentItemsForID(id);
    }
  },

  






  enableItem: function EM_enableItem(id) {
    var ds = this.datasource;
    var opType = ds.getItemProperty(id, "opType");
    var appDisabled = ds.getItemProperty(id, "appDisabled");
    var userDisabled = ds.getItemProperty(id, "userDisabled");

    var operation, action;
    
    
    if (appDisabled == OP_NONE &&
        userDisabled == OP_NEEDS_DISABLE || userDisabled == OP_NONE) {
      if (userDisabled == OP_NEEDS_DISABLE)
        ds.setItemProperty(id, EM_R("userDisabled"), null);
      if (opType != OP_NONE) {
        operation = OP_NONE;
        action = EM_ITEM_CANCEL;
      }
    }
    else {
      if (userDisabled == "true")
        ds.setItemProperty(id, EM_R("userDisabled"), EM_L(OP_NEEDS_ENABLE));
      if (opType != OP_NEEDS_ENABLE) {
        operation = OP_NEEDS_ENABLE;
        action = EM_ITEM_ENABLED;
      }
    }

    if (action) {
      this._setOp(id, operation);
      this._notifyAction(id, action);
    }
    else {
      ds.updateProperty(id, "satisfiesDependencies");
      this._updateDependentItemsForID(id);
    }
  },

  






  disableItem: function EM_disableItem(id) {
    var ds = this.datasource;
    var opType = ds.getItemProperty(id, "opType");
    var appDisabled = ds.getItemProperty(id, "appDisabled");
    var userDisabled = ds.getItemProperty(id, "userDisabled");

    var operation, action;
    
    
    if (userDisabled == OP_NEEDS_ENABLE || userDisabled == "true" ||
        appDisabled == OP_NEEDS_ENABLE) {
      if (userDisabled != "true")
        ds.setItemProperty(id, EM_R("userDisabled"), EM_L("true"));
      if (opType != OP_NONE) {
        operation = OP_NONE;
        action = EM_ITEM_CANCEL;
      }
    }
    else {
      if (userDisabled == OP_NONE)
        ds.setItemProperty(id, EM_R("userDisabled"), EM_L(OP_NEEDS_DISABLE));
      if (opType != OP_NEEDS_DISABLE) {
        operation = OP_NEEDS_DISABLE;
        action = EM_ITEM_DISABLED;
      }
    }

    if (action) {
      this._setOp(id, operation);
      this._notifyAction(id, action);
    }
    else {
      ds.updateProperty(id, "satisfiesDependencies");
      this._updateDependentItemsForID(id);
    }
  },

  




  _isUsableItem: function EM__isUsableItem(id) {
    var ds = this.datasource;
    

    if (ds.isCompatible(ds, getResourceForID(id), false) &&
        ds.getItemProperty(id, "blocklisted") == "false" &&
        ds.getItemProperty(id, "satisfiesDependencies") == "true") {

      
      if (ds.getItemProperty(id, "appManaged") == "true")
        return true;

      

      return (!gCheckUpdateSecurity ||
              ds.getItemProperty(id, "providesUpdatesSecurely") == "true");
    }
    return false;
  },

  





  _updateDependentItemsForID: function EM__updateDependentItemsForID(id) {
    var ds = this.datasource;
    var dependentItems = this.getDependentItemListForID(id, true);
    for (var i = 0; i < dependentItems.length; ++i) {
      var dependentID = dependentItems[i].id;
      ds.updateProperty(dependentID, "satisfiesDependencies");
      if (this._isUsableItem(dependentID))
        this._appEnableItem(dependentID);
      else
        this._appDisableItem(dependentID);
    }
  },

  



  _notifyAction: function EM__notifyAction(id, reason) {
    gOS.notifyObservers(this.datasource.getItemForID(id),
                        EM_ACTION_REQUESTED_TOPIC, reason);
  },

  


  update: function EM_update(items, itemCount, updateCheckType, listener,
                             updateType, appVersion, platformVersion) {

    
    
    if (updateType > MAX_PUBLIC_UPDATE_WHEN)
      throw Cr.NS_ERROR_ILLEGAL_VALUE;

    for (var i = 0; i < itemCount; ++i) {
      var currItem = items[i];
      if (!currItem)
        throw Cr.NS_ERROR_ILLEGAL_VALUE;
    }

    if (items.length == 0)
      items = this.getItemList(Ci.nsIUpdateItem.TYPE_ANY);

    var updater = new ExtensionItemUpdater(this);
    updater.checkForUpdates(items, items.length, updateCheckType, listener,
                            updateType, appVersion, platformVersion);
  },

  


  updateAndGetNewBlocklistedItems: function EM_updateAndGetNewBlocklistedItems(itemCount) {
    if (!gBlocklist)
      gBlocklist = Cc["@mozilla.org/extensions/blocklist;1"].
                   getService(Ci.nsIBlocklistService);

    var list = [];
    var ds = this.datasource;
    var items = this.getItemList(Ci.nsIUpdateItem.TYPE_ANY);
    for (var i = 0; i < items.length; ++i) {
      var id = items[i].id;

      
      var appDisabled = (ds.getItemProperty(id, "appDisabled") == "true" ||
                         ds.getItemProperty(id, "appDisabled") == OP_NEEDS_DISABLE);
      var userDisabled = (ds.getItemProperty(id, "userDisabled") == "true" ||
                          ds.getItemProperty(id, "userDisabled") == OP_NEEDS_DISABLE);
      var usable = this._isUsableItem(id);
      var state = gBlocklist.getAddonBlocklistState(items[i].id, items[i].version);

      
      
      if (!appDisabled && !userDisabled && state != Ci.nsIBlocklistService.STATE_NOT_BLOCKED)
        list.push(items[i]);

      
      if (usable)
        this._appEnableItem(id);
      else
        this._appDisableItem(id);

      
      
      
      if (appDisabled && usable && !userDisabled &&
          state == Ci.nsIBlocklistService.STATE_SOFTBLOCKED)
        this.disableItem(id);

      ds.updateProperty(id, "blocklisted");
      ds.updateProperty(id, "blocklistedsoft");
    }

    if (itemCount)
      itemCount.value = list.length;
    return list;
  },

  


  get installLocations () {
    return InstallLocations.enumeration;
  },

  





  getInstallLocation: function EM_getInstallLocation(id) {
    var key = this.datasource.visibleItems[id];
    return key ? InstallLocations.get(this.datasource.visibleItems[id]) : null;
  },

  





  getItemForID: function EM_getItemForID(id) {
    return this.datasource.getItemForID(id);
  },

  











  getDependentItemListForID: function EM_getDependentItemListForID(id,
                                                                   includeDisabled,
                                                                   countRef) {
    return this.datasource.getDependentItemListForID(id, includeDisabled, countRef);
  },

  
  getItemList: function EM_getItemList(type, countRef) {
    return this.datasource.getItemList(type, countRef);
  },

  
  getIncompatibleItemList: function EM_getIncompatibleItemList(appVersion,
                                                               platformVersion,
                                                               type,
                                                               includeDisabled,
                                                               countRef) {
    var items = this.datasource.getIncompatibleItemList(appVersion ? appVersion : undefined,
                                                        platformVersion ? platformVersion : undefined,
                                                        type, includeDisabled);
    if (countRef)
      countRef.value = items.length;
    return items;
  },

  
  
  _transactions: [],
  _downloadCount: 0,
  _compatibilityCheckCount: 0,

  







  _confirmCancelDownloadsOnQuit: function EM__confirmCancelDownloadsOnQuit(subject) {
    
    if ((subject instanceof Ci.nsISupportsPRBool) && subject.data)
      return;

    if (this._downloadCount > 0) {
      
      
      this._downloadCount = 0;
      var result;
#ifndef XP_MACOSX
      result = this._confirmCancelDownloads(this._downloadCount,
                                            "quitCancelDownloadsAlertTitle",
                                            "quitCancelDownloadsAlertMsgMultiple",
                                            "quitCancelDownloadsAlertMsg",
                                            "dontQuitButtonWin");
#else
      result = this._confirmCancelDownloads(this._downloadCount,
                                            "quitCancelDownloadsAlertTitle",
                                            "quitCancelDownloadsAlertMsgMacMultiple",
                                            "quitCancelDownloadsAlertMsgMac",
                                            "dontQuitButtonMac");
#endif
      if (subject instanceof Ci.nsISupportsPRBool)
        subject.data = result;
    }
  },

  







  _confirmCancelDownloadsOnOffline: function EM__confirmCancelDownloadsOnOffline(subject) {
    if (this._downloadCount > 0) {
      result = this._confirmCancelDownloads(this._downloadCount,
                                            "offlineCancelDownloadsAlertTitle",
                                            "offlineCancelDownloadsAlertMsgMultiple",
                                            "offlineCancelDownloadsAlertMsg",
                                            "dontGoOfflineButton");
      if (subject instanceof Ci.nsISupportsPRBool)
        subject.data = result;
    }
  },

  
















  _confirmCancelDownloads: function EM__confirmCancelDownloads(count, title,
                                                               cancelMessageMultiple,
                                                               cancelMessageSingle,
                                                               dontCancelButton) {
    var bundle = BundleManager.getBundle(URI_DOWNLOADS_PROPERTIES);
    var title = bundle.GetStringFromName(title);
    var message, quitButton;
    if (count > 1) {
      message = bundle.formatStringFromName(cancelMessageMultiple, [count], 1);
      quitButton = bundle.formatStringFromName("cancelDownloadsOKTextMultiple", [count], 1);
    }
    else {
      message = bundle.GetStringFromName(cancelMessageSingle);
      quitButton = bundle.GetStringFromName("cancelDownloadsOKText");
    }
    var dontQuitButton = bundle.GetStringFromName(dontCancelButton);

    var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
             getService(Ci.nsIWindowMediator);
    var win = wm.getMostRecentWindow("Extension:Manager");
    const nsIPromptService = Ci.nsIPromptService;
    var ps = Cc["@mozilla.org/embedcomp/prompt-service;1"].
             getService(nsIPromptService);
    var flags = (nsIPromptService.BUTTON_TITLE_IS_STRING * nsIPromptService.BUTTON_POS_0) +
                (nsIPromptService.BUTTON_TITLE_IS_STRING * nsIPromptService.BUTTON_POS_1);
    var rv = ps.confirmEx(win, title, message, flags, quitButton, dontQuitButton, null, null, { });
    return rv == 1;
  },

  
  addDownloads: function EM_addDownloads(items, itemCount, manager) {
    if (itemCount == 0)
      throw Cr.NS_ERROR_ILLEGAL_VALUE;

    for (i = 0; i < itemCount; ++i) {
      var currItem = items[i];
      if (!currItem)
        throw Cr.NS_ERROR_ILLEGAL_VALUE;
    }

    var ds = this.datasource;
    
    if (this._downloadCount == 0) {
      gOS.addObserver(this, "offline-requested", false);
      gOS.addObserver(this, "quit-application-requested", false);
    }
    this._downloadCount += itemCount;

    var urls = [];
    var hashes = [];
    var txnID = Math.round(Math.random() * 100);
    var txn = new ItemDownloadTransaction(this, txnID);
    for (var i = 0; i < itemCount; ++i) {
      var currItem = items[i];

      txn.addDownload(currItem);
      urls.push(currItem.xpiURL);
      hashes.push(currItem.xpiHash ? currItem.xpiHash : null);
      
      
      if (!manager) {
        var id = currItem.id
        var props = {
          availableUpdateURL: null,
          availableUpdateHash: null,
          availableUpdateVersion: null,
          availableUpdateInfo: null
        };
        var updateVersion = ds.getItemProperty(id, "availableUpdateVersion");
        var updateURL = ds.getItemProperty(id, "availableUpdateURL");
        if (updateVersion && (updateURL == currItem.xpiURL))
          props.newVersion = EM_L(updateVersion);
        ds.setItemProperties(id, props);
        ds.updateProperty(id, "availableUpdateURL");
        ds.updateProperty(id, "updateable");
      }
      var id = !manager ? PREFIX_ITEM_URI + currItem.id : currItem.xpiURL;
      ds.updateDownloadState(id, "waiting");
    }
    this._transactions.push(txn);

    if (manager) {
      
      manager.observe(txn, "xpinstall-progress", "open");
    }
    else {
      
      var xpimgr = Cc["@mozilla.org/xpinstall/install-manager;1"].
                   createInstance(Ci.nsIXPInstallManager);
      xpimgr.initManagerWithHashes(urls, hashes, urls.length, txn);
    }
  },

  




















  onStateChange: function EM_onStateChange(transaction, addon, state, value) {
    var ds = this.datasource;
    var id = addon.id != addon.xpiURL ? PREFIX_ITEM_URI + addon.id : addon.xpiURL;
    const nsIXPIProgressDialog = Ci.nsIXPIProgressDialog;
    switch (state) {
    case nsIXPIProgressDialog.DOWNLOAD_START:
      ds.updateDownloadState(id, "downloading");
      this._callInstallListeners("onDownloadStarted", addon);
      break;
    case nsIXPIProgressDialog.DOWNLOAD_DONE:
      this._callInstallListeners("onDownloadEnded", addon);
      break;
    case nsIXPIProgressDialog.INSTALL_START:
      ds.updateDownloadState(id, "finishing");
      ds.updateDownloadProgress(id, null);
      break;
    case nsIXPIProgressDialog.INSTALL_DONE:
      --this._downloadCount;
      
      
      
      if (value != 0 && value != -210 && id != addon.xpiURL) {
        ds.updateDownloadState(id, "failure");
        ds.updateDownloadProgress(id, null);
      }
      transaction.removeDownload(addon.xpiURL);
      
      if (value != 0)
        this._callInstallListeners("onInstallEnded", addon, value);
      break;
    case nsIXPIProgressDialog.DIALOG_CLOSE:
      for (var i = 0; i < this._transactions.length; ++i) {
        if (this._transactions[i].id == transaction.id) {
          this._transactions.splice(i, 1);
          
          if (this._transactions.length == 0) {
            gOS.removeObserver(this, "offline-requested");
            gOS.removeObserver(this, "quit-application-requested");

            
            
            if (this._compatibilityCheckCount == 0)
              this._callInstallListeners("onInstallsCompleted");
          }
          break;
        }
      }
      
      transaction.removeAllDownloads();
      break;
    }
  },

  onProgress: function EM_onProgress(addon, value, maxValue) {
    this._callInstallListeners("onDownloadProgress", addon, value, maxValue);

    var id = addon.id != addon.xpiURL ? PREFIX_ITEM_URI + addon.id : addon.xpiURL;
    var progress = Math.round((value / maxValue) * 100);
    this.datasource.updateDownloadProgress(id, progress);
  },

  _installListeners: [],
  addInstallListener: function EM_addInstallListener(listener) {
    for (var i = 0; i < this._installListeners.length; ++i) {
      if (this._installListeners[i] == listener)
        return i;
    }
    this._installListeners.push(listener);
    return this._installListeners.length - 1;
  },

  removeInstallListenerAt: function EM_removeInstallListenerAt(index) {
    if (index < 0 || index >= this._installListeners.length)
      throw Cr.NS_ERROR_INVALID_ARG;

    this._installListeners[index] = null;
    while (this._installListeners[this._installListeners.length - 1] === null)
      this._installListeners.splice(this._installListeners.length - 1, 1);
  },

  _callInstallListeners: function EM__callInstallListeners(method) {
    for (var i = 0; i < this._installListeners.length; ++i) {
      try {
        if (this._installListeners[i])
          this._installListeners[i][method].apply(this._installListeners[i],
                                                  Array.slice(arguments, 1));
      }
      catch (e) {
        LOG("Failure in install listener's " + method + ": " + e);
      }
    }
  },

  


  _ds: null,
  _ptr: null,

  





  _ensureDS: function EM__ensureDS() {
    if (!this._ds) {
      this._ds = new ExtensionsDataSource(this);
      if (this._ds) {
        this._ds.loadExtensions();
        this._ptr = getContainer(this._ds, this._ds._itemRoot).DataSource;
        gRDF.RegisterDataSource(this._ptr, true);
      }
    }
  },

  


  get datasource() {
    this._ensureDS();
    return this._ds;
  },

  
  flags: Ci.nsIClassInfo.SINGLETON,
  implementationLanguage: Ci.nsIProgrammingLanguage.JAVASCRIPT,
  getHelperForLanguage: function(language) null,
  getInterfaces: function(count) {
    var interfaces = [Ci.nsIExtensionManager, Ci.nsIObserver];
    count.value = interfaces.length;
    return interfaces;
  },

  classDescription: "Extension Manager",
  contractID: "@mozilla.org/extensions/manager;1",
  classID: Components.ID("{8A115FAA-7DCB-4e8f-979B-5F53472F51CF}"),
  _xpcom_categories: [{ category: "profile-after-change" },
                      { category: "update-timer",
                        value: "@mozilla.org/extensions/manager;1," +
                               "getService,addon-background-update-timer," +
                               PREF_EM_UPDATE_INTERVAL + ",86400" }],
  _xpcom_factory: {
    createInstance: function(outer, iid) {
      if (outer != null)
        throw Cr.NS_ERROR_NO_AGGREGATION;
  
      if (!gEmSingleton)
        gEmSingleton = new ExtensionManager();
      return gEmSingleton.QueryInterface(iid);
    }
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIExtensionManager,
                                         Ci.nsITimerCallback,
                                         Ci.nsIObserver,
                                         Ci.nsIClassInfo])
};


















function ItemDownloadTransaction(manager, id) {
  this._manager = manager;
  this._downloads = [];
  this.id = id;
}
ItemDownloadTransaction.prototype = {
  _manager    : null,
  _downloads  : [],
  id          : -1,

  




  addDownload: function ItemDownloadTransaction_addDownload(addon) {
    this._downloads.push({ addon: addon, waiting: true });
    this._manager.datasource.addDownload(addon);
  },

  




  removeDownload: function ItemDownloadTransaction_removeDownload(url) {
    this._manager.datasource.removeDownload(url);
  },

  


  removeAllDownloads: function ItemDownloadTransaction_removeAllDownloads() {
    for (var i = 0; i < this._downloads.length; ++i) {
      var addon = this._downloads[i].addon;
      this.removeDownload(addon.xpiURL);
    }
  },

  





  containsURL: function ItemDownloadTransaction_containsURL(url) {
    for (var i = 0; i < this._downloads.length; ++i) {
      if (this._downloads[i].addon.xpiURL == url)
        return true;
    }
    return false;
  },

  


  onStateChange: function ItemDownloadTransaction_onStateChange(index, state, value) {
    this._manager.onStateChange(this, this._downloads[index].addon,
                                state, value);
  },

  


  onProgress: function ItemDownloadTransaction_onProgress(index, value, maxValue) {
    this._manager.onProgress(this._downloads[index].addon, value, maxValue);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIXPIProgressDialog])
};





function BackgroundUpdateCheckListener(datasource) {
  this._emDS = datasource;
}
BackgroundUpdateCheckListener.prototype = {
  _updateCount: 0,
  _emDS: null,

  
  observe: function BackgroundUpdateListener_observe(aSubject, aTopic, aData) {
    if (aTopic != "alertclickcallback")
      return;

    var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
             getService(Ci.nsIWindowMediator);
    var win = wm.getMostRecentWindow("Extension:Manager");
    if (win) {
      win.focus();
      win.showView("updates");
      
      gPref.setBoolPref(PREF_UPDATE_NOTIFYUSER, false);
    }
    else {
      var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
               getService(Ci.nsIWindowWatcher);
      var param = Cc["@mozilla.org/supports-array;1"].
                  createInstance(Ci.nsISupportsArray);
      var arg = Cc["@mozilla.org/supports-string;1"].
                createInstance(Ci.nsISupportsString);
      arg.data = "updates";
      param.AppendElement(arg);
      ww.openWindow(null, URI_EXTENSION_MANAGER, null, FEATURES_EXTENSION_MANAGER, param);
    }
  },
  
  
  onUpdateStarted: function BackgroundUpdateListener_onUpdateStarted() {
  },

  onUpdateEnded: function BackgroundUpdateListener_onUpdateEnded() {
    if (this._updateCount > 0 && Cc["@mozilla.org/alerts-service;1"]) {
      var extensionStrings = BundleManager.getBundle(URI_EXTENSIONS_PROPERTIES);
      var title = extensionStrings.GetStringFromName("updateNotificationTitle");
      var text;
      if (this._updateCount > 1)
        text = extensionStrings.formatStringFromName("multipleUpdateNotificationText",
                                                     [BundleManager.appName, this._updateCount], 2);
      else
        text = extensionStrings.formatStringFromName("updateNotificationText",
                                                     [BundleManager.appName], 1);

      try {
        var notifier = Cc["@mozilla.org/alerts-service;1"].
                       getService(Ci.nsIAlertsService);
        notifier.showAlertNotification(URI_GENERIC_ICON_XPINSTALL,
                                       title, text, true, "", this);
      }
      catch (e) {
        LOG("Failed to retrieve alerts service, probably an unsupported " +
            "platform - " + e);
      }
    }
  },

  onAddonUpdateStarted: function BackgroundUpdateListener_onAddonUpdateStarted(item) {
  },

  onAddonUpdateEnded: function BackgroundUpdateListener_onAddonUpdateEnded(item, status) {
    if (status == Ci.nsIAddonUpdateCheckListener.STATUS_UPDATE) {
      var lastupdate = this._emDS.getItemProperty(item.id, "availableUpdateVersion");
      if (lastupdate != item.version) {
        gPref.setBoolPref(PREF_UPDATE_NOTIFYUSER, true);
        this._updateCount++;
      }
    }
  }
};






function AddonUpdateCheckListener(listener, datasource) {
  this._listener = listener;
  this._ds = datasource;
}
AddonUpdateCheckListener.prototype = {
  _listener: null,
  _ds: null,

  onUpdateStarted: function AddonUpdateListener_onUpdateStarted() {
    if (this._listener)
      this._listener.onUpdateStarted();
    this._ds.onUpdateStarted();
  },

  onUpdateEnded: function AddonUpdateListener_onUpdateEnded() {
    if (this._listener)
      this._listener.onUpdateEnded();
    this._ds.onUpdateEnded();
  },

  onAddonUpdateStarted: function AddonUpdateListener_onAddonUpdateStarted(addon) {
    if (this._listener)
      this._listener.onAddonUpdateStarted(addon);
    this._ds.onAddonUpdateStarted(addon);
  },

  onAddonUpdateEnded: function AddonUpdateListener_onAddonUpdateEnded(addon, status) {
    if (this._listener)
      this._listener.onAddonUpdateEnded(addon, status);
    this._ds.onAddonUpdateEnded(addon, status);
  }
};





function ExtensionItemUpdater(aEM)
{
  this._emDS = aEM._ds;
  this._em = aEM;

  getVersionChecker();
}

ExtensionItemUpdater.prototype = {
  _emDS               : null,
  _em                 : null,
  _updateCheckType    : 0,
  _updateType         : 0,
  _items              : [],
  _listener           : null,

  



























  checkForUpdates: function ExtensionItemUpdater_checkForUpdates(aItems,
                                                                 aItemCount,
                                                                 aUpdateCheckType,
                                                                 aListener,
                                                                 aUpdateType,
                                                                 aAppVersion,
                                                                 aPlatformVersion) {
    if (aUpdateCheckType == Ci.nsIExtensionManager.UPDATE_NOTIFY_NEWVERSION) {
      this._listener = aListener;
      this._appVersion = aAppVersion ? aAppVersion : gApp.version;
      this._platformVersion = aPlatformVersion ? aPlatformVersion
                                               : gApp.platformVersion;
    }
    else {
      this._listener = new AddonUpdateCheckListener(aListener, this._emDS);
      this._appVersion = gApp.version;
      this._platformVersion = gApp.platformVersion;
    }

    if (this._listener)
      this._listener.onUpdateStarted();
    this._updateCheckType = aUpdateCheckType;
    this._items = aItems;
    this._responseCount = aItemCount;

    this._updateType = aUpdateType;
    
    this._updateType |= UPDATE_TYPE_COMPATIBILITY;
    
    if (aUpdateCheckType == Ci.nsIExtensionManager.UPDATE_NOTIFY_NEWVERSION ||
        aUpdateCheckType == Ci.nsIExtensionManager.UPDATE_CHECK_NEWVERSION)
      this._updateType |= UPDATE_TYPE_NEWVERSION;

    
    this._updateCount = 0;

    for (var i = 0; i < aItemCount; ++i) {
      var e = this._items[i];
      if (this._listener)
        this._listener.onAddonUpdateStarted(e);
      (new RDFItemUpdater(this)).checkForUpdates(e, aUpdateCheckType);
    }

    if (this._listener && aItemCount == 0)
      this._listener.onUpdateEnded();
  },

  
  
  _applyVersionUpdates: function ExtensionItemUpdater__applyVersionUpdates(aLocalItem,
                                                                           aRemoteItem) {
    var targetAppInfo = this._emDS.getTargetApplicationInfo(aLocalItem.id, this._emDS);
    
    
    
    
    if (!targetAppInfo ||
        gVersionChecker.compare(aLocalItem.maxAppVersion, targetAppInfo.maxVersion) != 0) {
      if (gVersionChecker.compare(aLocalItem.maxAppVersion, aRemoteItem.maxAppVersion) < 0)
        return true;
      else
        return false;
    }

    if (gVersionChecker.compare(targetAppInfo.maxVersion, aRemoteItem.maxAppVersion) < 0) {
      
      
      this._emDS.setTargetApplicationInfo(aLocalItem.id,
                                          aRemoteItem.targetAppID,
                                          aRemoteItem.minAppVersion,
                                          aRemoteItem.maxAppVersion,
                                          null);

      
      
      var op = StartupCache.entries[aLocalItem.installLocationKey][aLocalItem.id].op;
      if (op == OP_NEEDS_DISABLE ||
          this._emDS.getItemProperty(aLocalItem.id, "appDisabled") == "true")
        this._em._appEnableItem(aLocalItem.id);
      return true;
    }
    else if (this._updateCheckType == Ci.nsIExtensionManager.UPDATE_SYNC_COMPATIBILITY)
      this._emDS.setTargetApplicationInfo(aLocalItem.id,
                                          aRemoteItem.targetAppID,
                                          aRemoteItem.minAppVersion,
                                          aRemoteItem.maxAppVersion,
                                          null);
    return false;
  },

  











  _isValidUpdate: function _isValidUpdate(aLocalItem, aRemoteItem, aUpdateCheckType) {
    var appExtensionsVersion = (aRemoteItem.targetAppID != TOOLKIT_ID) ?
                               this._appVersion :
                               this._platformVersion;

    var min = aRemoteItem.minAppVersion;
    var max = aRemoteItem.maxAppVersion;
    
    if (!min || gVersionChecker.compare(appExtensionsVersion, min) < 0)
      return false;

    
    if (!max || gVersionChecker.compare(appExtensionsVersion, max) > 0)
      return false;

    
    if (aUpdateCheckType != Ci.nsIExtensionManager.UPDATE_CHECK_COMPATIBILITY) {
      if (!gBlocklist)
        gBlocklist = Cc["@mozilla.org/extensions/blocklist;1"].
                     getService(Ci.nsIBlocklistService);
      
      
      if (gBlocklist.isAddonBlocklisted(aLocalItem.id, aRemoteItem.version,
                                        this._appVersion, this._platformVersion))
        return false;
    }

    return true;
  },

  checkForDone: function ExtensionItemUpdater_checkForDone(item, status) {
    if (this._listener) {
      try {
        this._listener.onAddonUpdateEnded(item, status);
      }
      catch (e) {
        LOG("ExtensionItemUpdater:checkForDone: Failure in listener's onAddonUpdateEnded: " + e);
      }
    }
    if (--this._responseCount == 0 && this._listener) {
      try {
        this._listener.onUpdateEnded();
      }
      catch (e) {
        LOG("ExtensionItemUpdater:checkForDone: Failure in listener's onUpdateEnded: " + e);
      }
    }
  },
};

















function escapeAddonURI(aItem, aAppVersion, aUpdateType, aURI, aDS)
{
  var itemStatus = "userEnabled";
  if (aDS.getItemProperty(aItem.id, "userDisabled") == "true" ||
      aDS.getItemProperty(aItem.id, "userDisabled") == OP_NEEDS_ENABLE)
    itemStatus = "userDisabled";
  else if (aDS.getItemProperty(aItem.id, "type") == Ci.nsIUpdateItem.TYPE_THEME) {
    var currentSkin = gPref.getCharPref(PREF_GENERAL_SKINS_SELECTEDSKIN);
    if (aDS.getItemProperty(aItem.id, "internalName") != currentSkin)
      itemStatus = "userDisabled";
  }

  if (aDS.getItemProperty(aItem.id, "compatible") == "false")
    itemStatus += ",incompatible";
  if (aDS.getItemProperty(aItem.id, "blocklisted") == "true")
    itemStatus += ",blocklisted";
  if (aDS.getItemProperty(aItem.id, "satisfiesDependencies") == "false")
    itemStatus += ",needsDependencies";

  aURI = aURI.replace(/%ITEM_ID%/g, aItem.id);
  aURI = aURI.replace(/%ITEM_VERSION%/g, aItem.version);
  aURI = aURI.replace(/%ITEM_MAXAPPVERSION%/g, aItem.maxAppVersion);
  aURI = aURI.replace(/%ITEM_STATUS%/g, itemStatus);
  aURI = aURI.replace(/%APP_ID%/g, gApp.ID);
  aURI = aURI.replace(/%APP_VERSION%/g, aAppVersion ? aAppVersion : gApp.version);
  aURI = aURI.replace(/%REQ_VERSION%/g, REQ_VERSION);
  aURI = aURI.replace(/%APP_OS%/g, gOSTarget);
  aURI = aURI.replace(/%APP_ABI%/g, gXPCOMABI);
  aURI = aURI.replace(/%APP_LOCALE%/g, gLocale);
  aURI = aURI.replace(/%CURRENT_APP_VERSION%/g, gApp.version);
  if (aUpdateType)
    aURI = aURI.replace(/%UPDATE_TYPE%/g, aUpdateType);

  
  
  var catMan = null;
  aURI = aURI.replace(/%(\w{3,})%/g, function(match, param) {
    if (!catMan) {
      catMan = Cc["@mozilla.org/categorymanager;1"].
               getService(Ci.nsICategoryManager);
    }

    try {
      var contractID = catMan.getCategoryEntry(CATEGORY_UPDATE_PARAMS, param);
      var paramHandler = Cc[contractID].
                         getService(Ci.nsIPropertyBag2);
      return paramHandler.getPropertyAsAString(param);
    }
    catch(e) {
      return match;
    }
  });

  
  return aURI.replace(/\+/g, "%2B");
}

function RDFItemUpdater(aUpdater) {
  this._updater = aUpdater;
}

RDFItemUpdater.prototype = {
  _updater            : null,
  _updateCheckType    : 0,
  _item               : null,

  checkForUpdates: function RDFItemUpdater_checkForUpdates(aItem, aUpdateCheckType) {
    
    try {
      if (!gPref.getBoolPref(PREF_EM_ITEM_UPDATE_ENABLED.replace(/%UUID%/, aItem.id))) {
        var status = Ci.nsIAddonUpdateCheckListener.STATUS_DISABLED;
        this._updater.checkForDone(aItem, status);
        return;
      }
    }
    catch (e) { }

    
    var emDS = this._updater._emDS;
    if (emDS.getItemProperty(aItem.id, "appManaged") == "true") {
      var status = Ci.nsIAddonUpdateCheckListener.STATUS_APP_MANAGED;
      this._updater.checkForDone(aItem, status);
      return;
    }

    
    
    var opType = emDS.getItemProperty(aItem.id, "opType");
    if (opType) {
      var status = Ci.nsIAddonUpdateCheckListener.STATUS_PENDING_OP;
      this._updater.checkForDone(aItem, status);
      return;
    }

    var installLocation = InstallLocations.get(emDS.getInstallLocationKey(aItem.id));
    
    if (installLocation && installLocation.itemIsManagedIndependently(aItem.id)) {
      var status = Ci.nsIAddonUpdateCheckListener.STATUS_NOT_MANAGED;
      this._updater.checkForDone(aItem, status);
      return;
    }

    
    
    if ((aUpdateCheckType == Ci.nsIExtensionManager.UPDATE_CHECK_NEWVERSION ||
         aUpdateCheckType == Ci.nsIExtensionManager.UPDATE_NOTIFY_NEWVERSION) &&
        (!installLocation || !installLocation.canAccess)) {
      var status = Ci.nsIAddonUpdateCheckListener.STATUS_READ_ONLY;
      this._updater.checkForDone(aItem, status);
      return;
    }

    this._updateCheckType = aUpdateCheckType;
    this._item = aItem;

    
    
    try {
      var dsURI = gPref.getComplexValue(PREF_EM_ITEM_UPDATE_URL.replace(/%UUID%/, aItem.id),
                                        Ci.nsIPrefLocalizedString).data;
    }
    catch (e) { }
    if (!dsURI)
      dsURI = aItem.updateRDF;
    if (!dsURI)
      dsURI = gPref.getCharPref(PREF_UPDATE_DEFAULT_URL);

    dsURI = escapeAddonURI(aItem, this._updater._appVersion,
                           this._updater._updateType, dsURI, emDS);

    
    try {
      var uri = newURI(dsURI);
    }
    catch (e) {
      WARN("RDFItemUpdater:checkForUpdates: There was an error loading the \r\n" +
           " update datasource for: " + dsURI + ", item = " + aItem.id + ", error: " + e);
      this._updater.checkForDone(aItem,
                                 Ci.nsIAddonUpdateCheckListener.STATUS_FAILURE);
      return;
    }

    LOG("RDFItemUpdater:checkForUpdates sending a request to server for: " +
        uri.spec + ", item = " + aItem.objectSource);

    var request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
                  createInstance(Ci.nsIXMLHttpRequest);
    request.open("GET", uri.spec, true);
    request.channel.notificationCallbacks = new gCertUtils.BadCertHandler();
    request.overrideMimeType("text/xml");
    request.channel.loadFlags |= Ci.nsIRequest.LOAD_BYPASS_CACHE;

    var self = this;
    request.onerror     = function(event) { self.onXMLError(event, aItem);    };
    request.onload      = function(event) { self.onXMLLoad(event, aItem);     };
    request.send(null);
  },

  onXMLLoad: function RDFItemUpdater_onXMLLoad(aEvent, aItem) {
    var request = aEvent.target;
    try {
      gCertUtils.checkCert(request.channel);
    }
    catch (e) {
      
      
      
      
      
      
      
      
      
      
      LOG("RDFItemUpdater::onXMLLoad: " + e);
      this._updater.checkForDone(aItem,
                                 Ci.nsIAddonUpdateCheckListener.STATUS_FAILURE);
      return;
    }
    var responseXML = request.responseXML;

    
    
    
    if (!responseXML || responseXML.documentElement.namespaceURI == XMLURI_PARSE_ERROR ||
        (request.status != 200 && request.status != 0)) {
      this._updater.checkForDone(aItem, (aItem.updateRDF ? Ci.nsIAddonUpdateCheckListener.STATUS_FAILURE :
                                                           Ci.nsIAddonUpdateCheckListener.STATUS_NONE));
      return;
    }

    var rdfParser = Cc["@mozilla.org/rdf/xml-parser;1"].
                    createInstance(Ci.nsIRDFXMLParser)
    var ds = Cc["@mozilla.org/rdf/datasource;1?name=in-memory-datasource"].
             createInstance(Ci.nsIRDFDataSource);
    rdfParser.parseString(ds, request.channel.URI, request.responseText);

    this.onDatasourceLoaded(ds, aItem);
  },

  onXMLError: function RDFItemUpdater_onXMLError(aEvent, aItem) {
    try {
      var request = aEvent.target;
      
      var status = request.status;
    }
    catch (e) {
      request = aEvent.target.channel.QueryInterface(Ci.nsIRequest);
      status = request.status;
    }
    
    try {
      var statusText = request.statusText;
    }
    catch (e) {
      status = 0;
    }
    
    if (status == 0)
      statusText = "nsIXMLHttpRequest channel unavailable";

    WARN("RDFItemUpdater:onError: There was an error loading the \r\n" +
         "the update datasource for item " + aItem.id + ", error: " + statusText);
    this._updater.checkForDone(aItem,
                               Ci.nsIAddonUpdateCheckListener.STATUS_FAILURE);
  },

  onDatasourceLoaded: function RDFItemUpdater_onDatasourceLoaded(aDatasource, aLocalItem) {
    










































    if (!aDatasource.GetAllResources().hasMoreElements()) {
      LOG("RDFItemUpdater:onDatasourceLoaded: Datasource empty.\r\n" +
          "If you are an Extension developer and were expecting there to be\r\n" +
          "updates, this could mean any number of things, since the RDF system\r\n" +
          "doesn't give up much in the way of information when the load fails.\r\n" +
          "\r\nTry checking that: \r\n" +
          " 1. Your remote RDF file exists at the location.\r\n" +
          " 2. Your RDF file is valid XML (starts with <?xml version=\"1.0\"?>\r\n" +
          "    and loads in Firefox displaying pretty printed like other XML documents\r\n" +
          " 3. Your server is sending the data in the correct MIME\r\n" +
          "    type (text/xml)");
    }      

    
    if (aLocalItem.updateKey) {
      var extensionRes = gRDF.GetResource(getItemPrefix(aLocalItem.type) + aLocalItem.id);
      LOG(extensionRes.Value);
      var signature = this._getPropertyFromResource(aDatasource, extensionRes, "signature", null);
      if (signature) {
        var serializer = new RDFSerializer();
        try {
          var updateString = serializer.serializeResource(aDatasource, extensionRes);
          var verifier = Cc["@mozilla.org/security/datasignatureverifier;1"].
                         getService(Ci.nsIDataSignatureVerifier);
          try {
            if (!verifier.verifyData(updateString, signature, aLocalItem.updateKey)) {
              WARN("RDFItemUpdater:onDatasourceLoaded: Update manifest for " +
                   aLocalItem.id + " failed signature check.");
              this._updater.checkForDone(aLocalItem, Ci.nsIAddonUpdateCheckListener.STATUS_FAILURE);
              return;
            }
          }
          catch (e) {
            WARN("RDFItemUpdater:onDatasourceLoaded: Failed to verify signature for " +
                 aLocalItem.id + ". This indicates a malformed update key or signature.");
            this._updater.checkForDone(aLocalItem, Ci.nsIAddonUpdateCheckListener.STATUS_FAILURE);
            return;
          }
        }
        catch (e) {
          WARN("RDFItemUpdater:onDatasourceLoaded: Failed to generate signature " +
               "string for " + aLocalItem.id + ". Serializer threw " + e);
          this._updater.checkForDone(aLocalItem, Ci.nsIAddonUpdateCheckListener.STATUS_FAILURE);
          return;
        }
      }
      else {
        WARN("RDFItemUpdater:onDatasourceLoaded: Update manifest for " +
             aLocalItem.id + " did not contain a signature.");
        this._updater.checkForDone(aLocalItem, Ci.nsIAddonUpdateCheckListener.STATUS_FAILURE);
        return;
      }
    }
    


    
    var newerItem, sameItem;

    
    if (this._updateCheckType == Ci.nsIExtensionManager.UPDATE_CHECK_NEWVERSION ||
        this._updateCheckType == Ci.nsIExtensionManager.UPDATE_NOTIFY_NEWVERSION) {
      
      
      
      
      newerItem = this._parseV20UpdateInfo(aDatasource, aLocalItem,
                                           this._updateCheckType);

      if (newerItem) {
        ++this._updater._updateCount;
        LOG("RDFItemUpdater:onDatasourceLoaded: Found a newer version of this item:\r\n" +
            newerItem.objectSource);
      }
    }

    
    
    sameItem = this._parseV20UpdateInfo(aDatasource, aLocalItem,
                                        Ci.nsIExtensionManager.UPDATE_CHECK_COMPATIBILITY);

    if (sameItem) {
      
      
      
      
      if (!this._updater._applyVersionUpdates(aLocalItem, sameItem))
        sameItem = null;
      else
        LOG("RDFItemUpdater:onDatasourceLoaded: Found info about the installed\r\n" +
            "version of this item: " + sameItem.objectSource);
    }
    var item = null, status = Ci.nsIAddonUpdateCheckListener.STATUS_NONE;
    if ((this._updateCheckType == Ci.nsIExtensionManager.UPDATE_CHECK_NEWVERSION ||
         this._updateCheckType == Ci.nsIExtensionManager.UPDATE_NOTIFY_NEWVERSION)
        && newerItem) {
      item = newerItem;
      status = Ci.nsIAddonUpdateCheckListener.STATUS_UPDATE;
    }
    else if (sameItem) {
      item = sameItem;
      status = Ci.nsIAddonUpdateCheckListener.STATUS_VERSIONINFO;
    }
    else {
      item = aLocalItem;
      status = Ci.nsIAddonUpdateCheckListener.STATUS_NO_UPDATE;
    }
    
    
    this._updater.checkForDone(item, status);
  },

  
  
  _getPropertyFromResource: function RDFItemUpdater__getPropertyFromResource(aDataSource,
                                                                             aSourceResource,
                                                                             aProperty,
                                                                             aLocalItem) {
    var rv;
    try {
      var property = gRDF.GetResource(EM_NS(aProperty));
      rv = stringData(aDataSource.GetTarget(aSourceResource, property, true));
      if (rv === undefined)
        throw Cr.NS_ERROR_FAILURE;
    }
    catch (e) {
      
      return null;
    }
    return rv;
  },

  












  _parseV20UpdateInfo: function RDFItemUpdater__parseV20UpdateInfo(aDataSource,
                                                                   aLocalItem,
                                                                   aUpdateCheckType) {
    var extensionRes  = gRDF.GetResource(getItemPrefix(aLocalItem.type) + aLocalItem.id);

    var updatesArc = gRDF.GetResource(EM_NS("updates"));
    var updates = aDataSource.GetTarget(extensionRes, updatesArc, true);

    try {
      updates = updates.QueryInterface(Ci.nsIRDFResource);
    }
    catch (e) {
      WARN("RDFItemUpdater:_parseV20UpdateInfo: No updates were found for:\r\n" +
           aLocalItem.id + "\r\n" +
           "If you are an Extension developer and were expecting there to be\r\n" +
           "updates, this could mean any number of things, since the RDF system\r\n" +
           "doesn't give up much in the way of information when the load fails.\r\n" +
           "\r\nTry checking that: \r\n" +
           " 1. Your RDF File is correct - e.g. check that there is a top level\r\n" +
           "    RDF Resource with a URI urn:mozilla:extension:{GUID}, and that\r\n" +
           "    the <em:updates> listed all have matching GUIDs.");
      return null;
    }

    
    var updatedItem = null;

    var cu = Cc["@mozilla.org/rdf/container-utils;1"].
             getService(Ci.nsIRDFContainerUtils);
    if (cu.IsContainer(aDataSource, updates)) {
      var ctr = getContainer(aDataSource, updates);

      var versions = ctr.GetElements();
      while (versions.hasMoreElements()) {
        
        
        
        var version = versions.getNext().QueryInterface(Ci.nsIRDFResource);
        var foundItem = this._parseV20Update(aDataSource, version, aLocalItem,
                                             updatedItem ? updatedItem.version : aLocalItem.version,
                                             aUpdateCheckType);
        if (foundItem) {
          
          
          if (aUpdateCheckType)
            return foundItem;
          updatedItem = foundItem;
        }
      }
    }
    return updatedItem;
  },

  

















  _parseV20Update: function RDFItemUpdater__parseV20Update(aDataSource,
                                                           aUpdateResource,
                                                           aLocalItem,
                                                           aNewestVersionFound,
                                                           aUpdateCheckType) {
    var version = this._getPropertyFromResource(aDataSource, aUpdateResource,
                                                "version", aLocalItem);
    


    var result = gVersionChecker.compare(version, aNewestVersionFound);
    if ((aUpdateCheckType == Ci.nsIExtensionManager.UPDATE_CHECK_NEWVERSION ||
         aUpdateCheckType == Ci.nsIExtensionManager.UPDATE_NOTIFY_NEWVERSION) ? result <= 0 : result != 0)
      return null;

    var taArc = gRDF.GetResource(EM_NS("targetApplication"));
    var targetApps = aDataSource.GetTargets(aUpdateResource, taArc, true);
    
    
    var newestUpdateItem = null;
    while (targetApps.hasMoreElements()) {
      var targetApp = targetApps.getNext().QueryInterface(Ci.nsIRDFResource);
      var appID = this._getPropertyFromResource(aDataSource, targetApp, "id", aLocalItem);
      if (appID != gApp.ID && appID != TOOLKIT_ID)
        continue;

      var updateLink = this._getPropertyFromResource(aDataSource, targetApp, "updateLink", aLocalItem);
      var updateHash = this._getPropertyFromResource(aDataSource, targetApp, "updateHash", aLocalItem);
      if (aUpdateCheckType == Ci.nsIExtensionManager.UPDATE_CHECK_NEWVERSION ||
          aUpdateCheckType == Ci.nsIExtensionManager.UPDATE_NOTIFY_NEWVERSION) {
        
        if (!updateLink)
          continue;

        



        if (gCheckUpdateSecurity && updateLink.substring(0, 6) != "https:" && 
            (!updateHash || updateHash.substring(0, 3) != "sha")) {
          WARN("RDFItemUpdater:_parseV20Update: Update for " + aLocalItem.id +
               " at " + updateLink + " ignored because it is insecure. updateLink " +
               " must be a https url or an updateHash must be specified.");
          continue;
        }
      }

      var updatedItem = makeItem(aLocalItem.id,
                                 version,
                                 aLocalItem.installLocationKey,
                                 this._getPropertyFromResource(aDataSource, targetApp, "minVersion", aLocalItem),
                                 this._getPropertyFromResource(aDataSource, targetApp, "maxVersion", aLocalItem),
                                 aLocalItem.name,
                                 updateLink,
                                 updateHash,
                                 "", 
                                 "", 
                                 "", 
                                 aLocalItem.type,
                                 appID);

      if (this._updater._isValidUpdate(aLocalItem, updatedItem, aUpdateCheckType)) {
        if (aUpdateCheckType == Ci.nsIExtensionManager.UPDATE_CHECK_NEWVERSION) {
          var infourl = this._getPropertyFromResource(aDataSource, targetApp,
                                                      "updateInfoURL");
          if (infourl)
            infourl = EM_L(infourl);
          this._updater._emDS.setItemProperty(aLocalItem.id,
                                              EM_R("availableUpdateInfo"),
                                              infourl);
        }
        if (appID == gApp.ID) {
          
          return updatedItem;
        }
        newestUpdateItem = updatedItem;
      }
    }
    return newestUpdateItem;
  }
};









function RDFSerializer()
{
  this.cUtils = Cc["@mozilla.org/rdf/container-utils;1"].
                getService(Ci.nsIRDFContainerUtils);
  this.resources = [];
}

RDFSerializer.prototype = {
  INDENT: "  ",      
  resources: null,   
  
  





  escapeEntities: function RDFSerializer_escapeEntities(string)
  {
    string = string.replace(/&/g, "&amp;");
    string = string.replace(/</g, "&lt;");
    string = string.replace(/>/g, "&gt;");
    string = string.replace(/"/g, "&quot;");
    return string;
  },
  
  






  serializeContainerItems: function RDFSerializer_serializeContainerItems(ds, container, indent)
  {
    var result = "";
    var items = container.GetElements();
    while (items.hasMoreElements()) {
      var item = items.getNext().QueryInterface(Ci.nsIRDFResource);
      result += indent + "<RDF:li>\n"
      result += this.serializeResource(ds, item, indent + this.INDENT);
      result += indent + "</RDF:li>\n"
    }
    return result;
  },
  
  








  serializeResourceProperties: function RDFSerializer_serializeResourceProperties(ds, resource, indent)
  {
    var result = "";
    var items = [];
    var arcs = ds.ArcLabelsOut(resource);
    while (arcs.hasMoreElements()) {
      var arc = arcs.getNext().QueryInterface(Ci.nsIRDFResource);
      if (arc.ValueUTF8.substring(0, PREFIX_NS_EM.length) != PREFIX_NS_EM)
        continue;
      var prop = arc.ValueUTF8.substring(PREFIX_NS_EM.length);
      if (prop == "signature")
        continue;
  
      var targets = ds.GetTargets(resource, arc, true);
      while (targets.hasMoreElements()) {
        var target = targets.getNext();
        if (target instanceof Ci.nsIRDFResource) {
          var item = indent + "<em:" + prop + ">\n";
          item += this.serializeResource(ds, target, indent + this.INDENT);
          item += indent + "</em:" + prop + ">\n";
          items.push(item);
        }
        else if (target instanceof Ci.nsIRDFLiteral) {
          items.push(indent + "<em:" + prop + ">" + this.escapeEntities(target.Value) + "</em:" + prop + ">\n");
        }
        else if (target instanceof Ci.nsIRDFInt) {
          items.push(indent + "<em:" + prop + " NC:parseType=\"Integer\">" + target.Value + "</em:" + prop + ">\n");
        }
        else {
          throw new Error("Cannot serialize unknown literal type");
        }
      }
    }
    items.sort();
    result += items.join("");
    return result;
  },
  
  










  serializeResource: function RDFSerializer_serializeResource(ds, resource, indent)
  {
    if (this.resources.indexOf(resource) != -1 ) {
      
      throw new Error("Cannot serialize multiple references to "+resource.Value);
    }
    if (indent === undefined)
      indent = "";
    
    this.resources.push(resource);
    var container = null;
    var type = "Description";
    if (this.cUtils.IsSeq(ds, resource)) {
      type = "Seq";
      container = this.cUtils.MakeSeq(ds, resource);
    }
    else if (this.cUtils.IsAlt(ds, resource)) {
      type = "Alt";
      container = this.cUtils.MakeAlt(ds, resource);
    }
    else if (this.cUtils.IsBag(ds, resource)) {
      type = "Bag";
      container = this.cUtils.MakeBag(ds, resource);
    }
  
    var result = indent + "<RDF:" + type;
    if (!gRDF.IsAnonymousResource(resource))
      result += " about=\"" + this.escapeEntities(resource.ValueUTF8) + "\"";
    result += ">\n";
  
    if (container)
      result += this.serializeContainerItems(ds, container, indent + this.INDENT);
      
    result += this.serializeResourceProperties(ds, resource, indent + this.INDENT);
  
    result += indent + "</RDF:" + type + ">\n";
    return result;
  }
}








function ExtensionsDataSource(em) {
  this._em = em;

  this._itemRoot = gRDF.GetResource(RDFURI_ITEM_ROOT);
  this._defaultTheme = gRDF.GetResource(RDFURI_DEFAULT_THEME);
}
ExtensionsDataSource.prototype = {
  _inner    : null,
  _em       : null,
  _itemRoot     : null,
  _defaultTheme : null,

  



  shutdown: function EMDS_shutdown() {
    this._inner = null;
    this._em = null;
    this._itemRoot = null;
    this._defaultTheme = null;
  },

  









  satisfiesDependencies: function EMDS_satisfiesDependencies(id) {
    var ds = this._inner;
    var itemResource = getResourceForID(id);
    var targets = ds.GetTargets(itemResource, EM_R("requires"), true);
    if (!targets.hasMoreElements())
      return true;

    getVersionChecker();
    var idRes = EM_R("id");
    var minVersionRes = EM_R("minVersion");
    var maxVersionRes = EM_R("maxVersion");
    while (targets.hasMoreElements()) {
      var target = targets.getNext().QueryInterface(Ci.nsIRDFResource);
      var dependencyID = stringData(ds.GetTarget(target, idRes, true));
      var version = null;
      version = this.getItemProperty(dependencyID, "version");
      if (version) {
        var opType = this.getItemProperty(dependencyID, "opType");
        if (opType ==  OP_NEEDS_DISABLE || opType == OP_NEEDS_UNINSTALL)
          return false;

        if (this.getItemProperty(dependencyID, "userDisabled") == "true" ||
            this.getItemProperty(dependencyID, "appDisabled") == "true" ||
            this.getItemProperty(dependencyID, "userDisabled") == OP_NEEDS_DISABLE ||
            this.getItemProperty(dependencyID, "appDisabled") == OP_NEEDS_DISABLE)
          return false;

        var minVersion = stringData(ds.GetTarget(target, minVersionRes, true));
        var maxVersion = stringData(ds.GetTarget(target, maxVersionRes, true));
        var compatible = (gVersionChecker.compare(version, minVersion) >= 0 &&
                          gVersionChecker.compare(version, maxVersion) <= 0);
        if (!compatible)
          return false;
      }
      else {
        return false;
      }
    }

    return true;
  },

  



















  isCompatible: function EMDS_isCompatible(datasource, source, alwaysCheckVersion,
                                           appVersion, platformVersion) {
    
    if (source.EqualsNode(this._defaultTheme))
      return true;

    
    if (datasource === this &&
        this._getItemProperty(source, "opType") == OP_NEEDS_INSTALL)
      return true;

    var appID = gApp.ID;
    if (appVersion === undefined)
      appVersion = gApp.version;
    if (platformVersion === undefined)
      var platformVersion = gApp.platformVersion;

    var targets = datasource.GetTargets(source, EM_R("targetApplication"), true);
    var idRes = EM_R("id");
    var minVersionRes = EM_R("minVersion");
    var maxVersionRes = EM_R("maxVersion");
    var versionChecker = getVersionChecker();
    var rv = false;
    while (targets.hasMoreElements()) {
      var targetApp = targets.getNext().QueryInterface(Ci.nsIRDFResource);
      var id          = stringData(datasource.GetTarget(targetApp, idRes, true));
      var minVersion  = stringData(datasource.GetTarget(targetApp, minVersionRes, true));
      var maxVersion  = stringData(datasource.GetTarget(targetApp, maxVersionRes, true));
      if (id == appID) {
        if (!alwaysCheckVersion && !gCheckCompatibility)
          return true;
        rv = (versionChecker.compare(appVersion, minVersion) >= 0) &&
             (versionChecker.compare(appVersion, maxVersion) <= 0);
        return rv; 
      }

      if (id == TOOLKIT_ID) {
        if (!alwaysCheckVersion && !gCheckCompatibility)
          return true;
        rv =  (versionChecker.compare(platformVersion, minVersion) >= 0) &&
              (versionChecker.compare(platformVersion, maxVersion) <= 0);
        
      }
    }
    return rv;
  },

  












  getIncompatibleItemList: function EMDS_getIncompatibleItemList(appVersion,
                                                                 platformVersion,
                                                                 desiredType,
                                                                 includeDisabled) {
    var items = [];
    var ctr = getContainer(this._inner, this._itemRoot);
    var elements = ctr.GetElements();
    while (elements.hasMoreElements()) {
      var item = elements.getNext().QueryInterface(Ci.nsIRDFResource);
      var id = stripPrefix(item.Value, PREFIX_ITEM_URI);
      var type = this.getItemProperty(id, "type");
      
      if (!includeDisabled && this.getItemProperty(id, "isDisabled") == "true")
        continue;

      
      
      
      
      
      
      
      var locationKey = this.getItemProperty(id, "installLocation");
      var appManaged = this.getItemProperty(id, "appManaged") == "true";
      if (appManaged && locationKey == KEY_APP_GLOBAL)
        continue;

      if (type != -1 && (type & desiredType) &&
          !this.isCompatible(this, item, true, appVersion, platformVersion))
        items.push(this.getItemForID(id));
    }
    return items;
  },

  









  getItemList: function EMDS_getItemList(desiredType, countRef) {
    var items = [];
    var ctr = getContainer(this, this._itemRoot);
    var elements = ctr.GetElements();
    while (elements.hasMoreElements()) {
      var e = elements.getNext().QueryInterface(Ci.nsIRDFResource);
      var eID = stripPrefix(e.Value, PREFIX_ITEM_URI);
      var type = this.getItemProperty(eID, "type");
      if (type != -1 && type & desiredType)
        items.push(this.getItemForID(eID));
    }
    if (countRef)
      countRef.value = items.length;
    return items;
  },

  











  getDependentItemListForID: function EMDS_getDependentItemListForID(id,
                                                                     includeDisabled,
                                                                     countRef) {
    var items = [];
    var ds = this._inner;
    var ctr = getContainer(this, this._itemRoot);
    var elements = ctr.GetElements();
    while (elements.hasMoreElements()) {
      var e = elements.getNext().QueryInterface(Ci.nsIRDFResource);
      var dependentID = stripPrefix(e.Value, PREFIX_ITEM_URI);
      var targets = ds.GetTargets(e, EM_R("requires"), true);
      var idRes = EM_R("id");
      while (targets.hasMoreElements()) {
        var target = targets.getNext().QueryInterface(Ci.nsIRDFResource);
        var dependencyID = stringData(ds.GetTarget(target, idRes, true));
        if (dependencyID == id) {
          if (!includeDisabled && this.getItemProperty(dependentID, "isDisabled") == "true")
            continue;
          items.push(this.getItemForID(dependentID));
          break;
        }
      }
    }
    if (countRef)
      countRef.value = items.length;
    return items;
  },

  





  getItemForID: function EMDS_getItemForID(id) {
    if (!this.visibleItems[id])
      return null;

    var r = getResourceForID(id);
    if (!r)
      return null;

    var targetAppInfo = this.getTargetApplicationInfo(id, this);
    var updateHash = this.getItemProperty(id, "availableUpdateHash");
    return makeItem(id,
                    this.getItemProperty(id, "version"),
                    this.getItemProperty(id, "installLocation"),
                    targetAppInfo ? targetAppInfo.minVersion : "",
                    targetAppInfo ? targetAppInfo.maxVersion : "",
                    this.getItemProperty(id, "name"),
                    this.getItemProperty(id, "availableUpdateURL"),
                    updateHash ? updateHash : "",
                    this.getItemProperty(id, "iconURL"),
                    this.getItemProperty(id, "updateURL"),
                    this.getItemProperty(id, "updateKey"),
                    this.getItemProperty(id, "type"),
                    targetAppInfo ? targetAppInfo.appID : gApp.ID);
  },

  






  getInstallLocationKey: function EMDS_getInstallLocationKey(id) {
    return this.getItemProperty(id, "installLocation");
  },

  











  _setProperty: function EMDS__setProperty(datasource, source, property, newValue) {
    var oldValue = datasource.GetTarget(source, property, true);
    if (oldValue) {
      if (newValue)
        datasource.Change(source, property, oldValue, newValue);
      else
        datasource.Unassert(source, property, oldValue);
    }
    else if (newValue)
      datasource.Assert(source, property, newValue, true);
  },

  











  getUpdatedTargetAppInfo: function EMDS_getUpdatedTargetAppInfo(id) {
    
    if (getResourceForID(id).EqualsNode(this._defaultTheme))
      return null;

    var appID = gApp.ID;
    var r = getResourceForID(id);
    var targetApps = this._inner.GetTargets(r, EM_R("targetApplication"), true);
    if (!targetApps.hasMoreElements())
      targetApps = this._inner.GetTargets(gInstallManifestRoot, EM_R("targetApplication"), true);
    var outData = null;
    while (targetApps.hasMoreElements()) {
      var targetApp = targetApps.getNext();
      if (targetApp instanceof Ci.nsIRDFResource) {
        try {
          var foundAppID = stringData(this._inner.GetTarget(targetApp, EM_R("id"), true));
          
          if (foundAppID != appID && foundAppID != TOOLKIT_ID)
            continue;
          var updatedMinVersion = this._inner.GetTarget(targetApp, EM_R("updatedMinVersion"), true);
          var updatedMaxVersion = this._inner.GetTarget(targetApp, EM_R("updatedMaxVersion"), true);
          if (updatedMinVersion && updatedMaxVersion)
            outData = { id          : id,
                        targetAppID : foundAppID,
                        minVersion  : stringData(updatedMinVersion),
                        maxVersion  : stringData(updatedMaxVersion) };
          if (foundAppID == appID)
            return outData;
        }
        catch (e) {
          continue;
        }
      }
    }
    return outData;
  },

  

















  setUpdatedTargetAppInfo: function EMDS_setUpdatedTargetAppInfo(id, targetAppID,
                                                                 updatedMinVersion,
                                                                 updatedMaxVersion) {
    
    if (getResourceForID(id).EqualsNode(this._defaultTheme))
      return;

    
    var updatedMinVersionRes = EM_R("updatedMinVersion");
    var updatedMaxVersionRes = EM_R("updatedMaxVersion");

    var appID = gApp.ID;
    var r = getResourceForID(id);
    var targetApps = this._inner.GetTargets(r, EM_R("targetApplication"), true);
    
    if (!targetApps.hasMoreElements()) {
      var idRes = EM_R("id");
      var targetRes = getResourceForID(id);
      var property = EM_R("targetApplication");
      var anon = gRDF.GetAnonymousResource();
      this._inner.Assert(anon, idRes, EM_L(appID), true);
      this._inner.Assert(anon, updatedMinVersionRes, EM_L(updatedMinVersion), true);
      this._inner.Assert(anon, updatedMaxVersionRes, EM_L(updatedMaxVersion), true);
      this._inner.Assert(targetRes, property, anon, true);
    }
    else {
      while (targetApps.hasMoreElements()) {
        var targetApp = targetApps.getNext();
        if (targetApp instanceof Ci.nsIRDFResource) {
          var foundAppID = stringData(this._inner.GetTarget(targetApp, EM_R("id"), true));
          
          if (foundAppID != targetAppID)
            continue;
          this._inner.Assert(targetApp, updatedMinVersionRes, EM_L(updatedMinVersion), true);
          this._inner.Assert(targetApp, updatedMaxVersionRes, EM_L(updatedMaxVersion), true);
          break;
        }
      }
    }
    this.Flush();
  },

  















  getTargetApplicationInfo: function EMDS_getTargetApplicationInfo(id, datasource) {
    var appID = gApp.ID;
    
    if (getResourceForID(id).EqualsNode(this._defaultTheme)) {
      var ver = gApp.version;
      return { appID: appID, minVersion: ver, maxVersion: ver };
    }

    var r = getResourceForID(id);
    var targetApps = datasource.GetTargets(r, EM_R("targetApplication"), true);
    if (!targetApps)
      return null;

    if (!targetApps.hasMoreElements())
      targetApps = datasource.GetTargets(gInstallManifestRoot, EM_R("targetApplication"), true);
    var outData = null;
    while (targetApps.hasMoreElements()) {
      var targetApp = targetApps.getNext();
      if (targetApp instanceof Ci.nsIRDFResource) {
        try {
          var foundAppID = stringData(datasource.GetTarget(targetApp, EM_R("id"), true));
          
          if (foundAppID != appID && foundAppID != TOOLKIT_ID)
            continue;

          outData = { appID: foundAppID,
                      minVersion: stringData(datasource.GetTarget(targetApp, EM_R("minVersion"), true)),
                      maxVersion: stringData(datasource.GetTarget(targetApp, EM_R("maxVersion"), true)) };
          if (foundAppID == appID)
            return outData;
        }
        catch (e) {
          continue;
        }
      }
    }
    return outData;
  },

  



















  setTargetApplicationInfo: function EMDS_setTargetApplicationInfo(id, targetAppID,
                                                                   minVersion,
                                                                   maxVersion,
                                                                   datasource) {
    var targetDataSource = datasource;
    if (!targetDataSource)
      targetDataSource = this._inner;

    var appID = gApp.ID;
    var r = getResourceForID(id);
    var targetApps = targetDataSource.GetTargets(r, EM_R("targetApplication"), true);
    if (!targetApps.hasMoreElements())
      targetApps = datasource.GetTargets(gInstallManifestRoot, EM_R("targetApplication"), true);
    while (targetApps.hasMoreElements()) {
      var targetApp = targetApps.getNext();
      if (targetApp instanceof Ci.nsIRDFResource) {
        var foundAppID = stringData(targetDataSource.GetTarget(targetApp, EM_R("id"), true));
        
        if (foundAppID != targetAppID)
          continue;

        this._setProperty(targetDataSource, targetApp, EM_R("minVersion"), EM_L(minVersion));
        this._setProperty(targetDataSource, targetApp, EM_R("maxVersion"), EM_L(maxVersion));

        
        
        
        if (!datasource)
          this.Flush();

        break;
      }
    }
  },

  








  getItemProperty: function EMDS_getItemProperty(id, property) {
    var item = getResourceForID(id);
    if (!item) {
      LOG("getItemProperty failing for lack of an item. This means getResourceForItem \
           failed to locate a resource for aItemID (item ID = " + id + ", property = " + property + ")");
    }
    else
      return this._getItemProperty(item, property);
    return undefined;
  },

  








  _getItemProperty: function EMDS__getItemProperty(itemResource, property) {
    var target = this.GetTarget(itemResource, EM_R(property), true);
    var value = stringData(target);
    if (value === undefined)
      value = intData(target);
    return value === undefined ? "" : value;
  },

  








  setItemProperty: function EMDS_setItemProperty(id, propertyArc, propertyValue) {
    var item = getResourceForID(id);
    this._setProperty(this._inner, item, propertyArc, propertyValue);
    this.Flush();
  },

  






  setItemProperties: function EMDS_setItemProperties(id, properties) {
    var item = getResourceForID(id);
    for (var key in properties)
      this._setProperty(this._inner, item, EM_R(key), properties[key]);
    this.Flush();
  },

  




  insertItemIntoContainer: function EMDS_insertItemIntoContainer(id) {
    
    var ctr = getContainer(this._inner, this._itemRoot);
    var itemResource = getResourceForID(id);
    
    
    var oldIndex = ctr.IndexOf(itemResource);
    if (oldIndex == -1)
      ctr.AppendElement(itemResource);
    this.Flush();
  },

  




  removeItemFromContainer: function EMDS_removeItemFromContainer(id) {
    var ctr = getContainer(this._inner, this._itemRoot);
    var itemResource = getResourceForID(id);
    ctr.RemoveElement(itemResource, true);
    this.Flush();
  },

  





  removeCorruptItem: function EMDS_removeCorruptItem(id) {
    this.removeItemMetadata(id);
    this.removeItemFromContainer(id);
    this.visibleItems[id] = null;
  },

  





  removeCorruptDLItem: function EMDS_removeCorruptDLItem(uri) {
    var itemResource = gRDF.GetResource(uri);
    var ctr = getContainer(this._inner, this._itemRoot);
    if (ctr.IndexOf(itemResource) != -1) {
      ctr.RemoveElement(itemResource, true);
      this._cleanResource(itemResource);
      this.Flush();
    }
    return itemResource;
  },

  









  _addLocalizedMetadata: function EMDS__addLocalizedMetadata(installManifest,
                                                             sourceRes, targetRes)
  {
    var singleProps = ["name", "description", "creator", "homepageURL"];

    for (var i = 0; i < singleProps.length; ++i) {
      var property = EM_R(singleProps[i]);
      var literal = installManifest.GetTarget(sourceRes, property, true);
      
      this._setProperty(this._inner, targetRes, property, literal);
    }

    
    var manyProps = ["developer", "translator", "contributor"];
    for (var i = 0; i < manyProps.length; ++i) {
      var property = EM_R(manyProps[i]);
      var literals = installManifest.GetTargets(sourceRes, property, true);

      var oldValues = this._inner.GetTargets(targetRes, property, true);
      while (oldValues.hasMoreElements()) {
        var oldValue = oldValues.getNext().QueryInterface(Ci.nsIRDFNode);
        this._inner.Unassert(targetRes, property, oldValue);
      }
      while (literals.hasMoreElements()) {
        var literal = literals.getNext().QueryInterface(Ci.nsIRDFNode);
        this._inner.Assert(targetRes, property, literal, true);
      }
    }

  },

  









  addItemMetadata: function EMDS_addItemMetadata(id, installManifest, installLocation) {
    var targetRes = getResourceForID(id);
    
    this._setProperty(this._inner, targetRes, EM_R("newVersion"), null);
    
    
    var singleProps = ["version", "updateURL", "updateService", "optionsURL",
                       "aboutURL", "iconURL", "internalName", "updateKey"];

    
    
    if (installLocation.restricted)
      singleProps = singleProps.concat(["locked"]);
    if (installLocation.name == KEY_APP_GLOBAL)
      singleProps = singleProps.concat(["appManaged"]);
    for (var i = 0; i < singleProps.length; ++i) {
      var property = EM_R(singleProps[i]);
      var literal = installManifest.GetTarget(gInstallManifestRoot, property, true);
      
      this._setProperty(this._inner, targetRes, property, literal);
    }

    var localizedProp = EM_R("localized");
    var localeProp = EM_R("locale");
    
    var oldValues = this._inner.GetTargets(targetRes, localizedProp, true);
    while (oldValues.hasMoreElements()) {
      var oldValue = oldValues.getNext().QueryInterface(Ci.nsIRDFNode);
      this._cleanResource(oldValue);
      this._inner.Unassert(targetRes, localizedProp, oldValue);
    }
    
    var localizations = installManifest.GetTargets(gInstallManifestRoot, localizedProp, true);
    while (localizations.hasMoreElements()) {
      var localization = localizations.getNext().QueryInterface(Ci.nsIRDFResource);
      var anon = gRDF.GetAnonymousResource();
      var literals = installManifest.GetTargets(localization, localeProp, true);
      while (literals.hasMoreElements()) {
        var literal = literals.getNext().QueryInterface(Ci.nsIRDFNode);
        this._inner.Assert(anon, localeProp, literal, true);
      }
      this._addLocalizedMetadata(installManifest, localization, anon);
      this._inner.Assert(targetRes, localizedProp, anon, true);
    }
    
    this._addLocalizedMetadata(installManifest, gInstallManifestRoot, targetRes);

    
    var versionProps = ["targetApplication", "requires"];
    var idRes = EM_R("id");
    var minVersionRes = EM_R("minVersion");
    var maxVersionRes = EM_R("maxVersion");
    for (var i = 0; i < versionProps.length; ++i) {
      var property = EM_R(versionProps[i]);
      var newVersionInfos = installManifest.GetTargets(gInstallManifestRoot, property, true);

      var oldVersionInfos = this._inner.GetTargets(targetRes, property, true);
      while (oldVersionInfos.hasMoreElements()) {
        var oldVersionInfo = oldVersionInfos.getNext().QueryInterface(Ci.nsIRDFResource);
        this._cleanResource(oldVersionInfo);
        this._inner.Unassert(targetRes, property, oldVersionInfo);
      }
      while (newVersionInfos.hasMoreElements()) {
        var newVersionInfo = newVersionInfos.getNext().QueryInterface(Ci.nsIRDFResource);
        var anon = gRDF.GetAnonymousResource();
        this._inner.Assert(anon, idRes, installManifest.GetTarget(newVersionInfo, idRes, true), true);
        this._inner.Assert(anon, minVersionRes, installManifest.GetTarget(newVersionInfo, minVersionRes, true), true);
        this._inner.Assert(anon, maxVersionRes, installManifest.GetTarget(newVersionInfo, maxVersionRes, true), true);
        this._inner.Assert(targetRes, property, anon, true);
      }
    }
    this.updateProperty(id, "opType");
    this.updateProperty(id, "updateable");
    this.Flush();
  },

  




  removeItemMetadata: function EMDS_removeItemMetadata(id) {
    var item = getResourceForID(id);
    var resources = ["targetApplication", "requires", "localized"];
    for (var i = 0; i < resources.length; ++i) {
      var targetApps = this._inner.GetTargets(item, EM_R(resources[i]), true);
      while (targetApps.hasMoreElements()) {
        var targetApp = targetApps.getNext().QueryInterface(Ci.nsIRDFResource);
        this._cleanResource(targetApp);
      }
    }

    this._cleanResource(item);
  },

  






  _cleanResource: function EMDS__cleanResource(resource) {
    
    var arcs = this._inner.ArcLabelsOut(resource);
    while (arcs.hasMoreElements()) {
      var arc = arcs.getNext().QueryInterface(Ci.nsIRDFResource);
      var targets = this._inner.GetTargets(resource, arc, true);
      while (targets.hasMoreElements()) {
        var value = targets.getNext().QueryInterface(Ci.nsIRDFNode);
        if (value)
          this._inner.Unassert(resource, arc, value);
      }
    }
  },

  








  updateProperty: function EMDS_updateProperty(id, property) {
    var item = getResourceForID(id);
    this._updateProperty(item, property);
  },

  










  _updateProperty: function EMDS__updateProperty(item, property) {
    if (item) {
      var propertyResource = EM_R(property);
      var value = this.GetTarget(item, propertyResource, true);
      for (var i = 0; i < this._observers.length; ++i) {
        if (value)
          this._observers[i].onChange(this, item, propertyResource,
                                      EM_L(""), value);
        else
          this._observers[i].onUnassert(this, item, propertyResource,
                                        EM_L(""));
      }
    }
  },

  






  isDownloadItem: function EMDS_isDownloadItem(id) {
    var downloadURL = stringData(this.GetTarget(gRDF.GetResource(id), EM_R("downloadURL"), true));
    return downloadURL && downloadURL != "";
  },

  





  addDownload: function EMDS_addDownload(addon) {
    
    
    if (addon.id != addon.xpiURL) {
      this.updateDownloadState(PREFIX_ITEM_URI + addon.id, "waiting");
      return;
    }
    var res = gRDF.GetResource(addon.xpiURL);
    this._setProperty(this._inner, res, EM_R("name"), EM_L(addon.name));
    this._setProperty(this._inner, res, EM_R("version"), EM_L(addon.version));
    this._setProperty(this._inner, res, EM_R("iconURL"), EM_L(addon.iconURL));
    this._setProperty(this._inner, res, EM_R("downloadURL"), EM_L(addon.xpiURL));
    this._setProperty(this._inner, res, EM_R("type"), EM_I(addon.type));

    var ctr = getContainer(this._inner, this._itemRoot);
    if (ctr.IndexOf(res) == -1)
      ctr.AppendElement(res);

    this.updateDownloadState(addon.xpiURL, "waiting");
    this.Flush();
  },

  











  addIncompatibleUpdateItem: function EMDS_addIncompatibleUpdateItem(name, url, type, version) {
    var iconURL = (type == Ci.nsIUpdateItem.TYPE_THEME) ? URI_GENERIC_ICON_THEME :
                                                          URI_GENERIC_ICON_XPINSTALL;
    var extensionsStrings = BundleManager.getBundle(URI_EXTENSIONS_PROPERTIES);
    var updateMsg = extensionsStrings.formatStringFromName("incompatibleUpdateMessage",
                                                           [BundleManager.appName, name], 2)

    var res = gRDF.GetResource(url);
    this._setProperty(this._inner, res, EM_R("name"), EM_L(name));
    this._setProperty(this._inner, res, EM_R("iconURL"), EM_L(iconURL));
    this._setProperty(this._inner, res, EM_R("downloadURL"), EM_L(url));
    this._setProperty(this._inner, res, EM_R("type"), EM_I(type));
    this._setProperty(this._inner, res, EM_R("version"), EM_L(version));
    this._setProperty(this._inner, res, EM_R("incompatibleUpdate"), EM_L("true"));
    this._setProperty(this._inner, res, EM_R("description"), EM_L(updateMsg));

    var ctr = getContainer(this._inner, this._itemRoot);
    if (ctr.IndexOf(res) == -1)
      ctr.AppendElement(res);

    this.updateDownloadState(url, "incompatibleUpdate");
    this.Flush();
  },

  




  removeDownload: function EMDS_removeDownload(url) {
    var res = gRDF.GetResource(url);
    var ctr = getContainer(this._inner, this._itemRoot);
    if (ctr.IndexOf(res) != -1)
      ctr.RemoveElement(res, true);
    this._cleanResource(res);
    this.updateDownloadState(url, null);
    this.Flush();
  },

  



  _progressData: { },

  








  updateDownloadState: function EMDS_updateDownloadState(id, state) {
    if (!state) {
      if (id in this._progressData)
        delete this._progressData[id];
      return;
    }
    else {
      if (!(id in this._progressData))
        this._progressData[id] = { };
      this._progressData[id].state = state;
    }
    var item = gRDF.GetResource(id);
    this._updateProperty(item, "state");
  },

  updateDownloadProgress: function EMDS_updateDownloadProgress(id, progress) {
    if (!progress) {
      if (!(id in this._progressData))
        return;
      this._progressData[id].progress = null;
    }
    else {
      if (!(id in this._progressData))
        this.updateDownloadState(id, "downloading");

      if (this._progressData[id].progress == progress)
        return;

      this._progressData[id].progress = progress;
    }
    var item = gRDF.GetResource(id);
    this._updateProperty(item, "progress");
  },

  






  visibleItems: { },

  



  _buildVisibleItemList: function EMDS__buildVisibleItemList() {
    var ctr = getContainer(this, this._itemRoot);
    var items = ctr.GetElements();
    while (items.hasMoreElements()) {
      var item = items.getNext().QueryInterface(Ci.nsIRDFResource);
      
      var id = stripPrefix(item.Value, PREFIX_ITEM_URI);
      this.visibleItems[id] = this.getItemProperty(id, "installLocation");
    }
  },

  











  updateVisibleList: function EMDS_updateVisibleList(id, locationKey, forceReplace) {
    if (id in this.visibleItems && this.visibleItems[id]) {
      var oldLocation = InstallLocations.get(this.visibleItems[id]);
      var newLocation = InstallLocations.get(locationKey);
      if (forceReplace || !oldLocation || newLocation.priority < oldLocation.priority)
        this.visibleItems[id] = locationKey;
    }
    else
      this.visibleItems[id] = locationKey;
  },

  


  loadExtensions: function EMDS_loadExtensions() {
    var extensionsFile = FileUtils.getFile(KEY_PROFILEDIR, [FILE_EXTENSIONS]);
    try {
      this._inner = gRDF.GetDataSourceBlocking(getURLSpecFromFile(extensionsFile));
    }
    catch (e) {
      ERROR("Datasource::loadExtensions: removing corrupted extensions datasource " +
            " file = " + extensionsFile.path + ", exception = " + e + "\n");
      extensionsFile.remove(false);
      return;
    }

    var cu = Cc["@mozilla.org/rdf/container-utils;1"].
             getService(Ci.nsIRDFContainerUtils);
    cu.MakeSeq(this._inner, this._itemRoot);

    this._buildVisibleItemList();
  },

  


  onUpdateStarted: function EMDS_onUpdateStarted() {
    LOG("Datasource: Update Started");
  },

  


  onUpdateEnded: function EMDS_onUpdateEnded() {
    LOG("Datasource: Update Ended");
  },

  


  onAddonUpdateStarted: function EMDS_onAddonUpdateStarted(addon) {
    if (!addon)
      throw Cr.NS_ERROR_INVALID_ARG;

    LOG("Datasource: Addon Update Started: " + addon.id);
    this.updateProperty(addon.id, "availableUpdateURL");
  },

  


  onAddonUpdateEnded: function EMDS_onAddonUpdateEnded(addon, status) {
    if (!addon)
      throw Cr.NS_ERROR_INVALID_ARG;

    LOG("Datasource: Addon Update Ended: " + addon.id + ", status: " + status);
    var url = null, hash = null, version = null;
    var updateAvailable = status == Ci.nsIAddonUpdateCheckListener.STATUS_UPDATE;
    if (updateAvailable) {
      url = EM_L(addon.xpiURL);
      if (addon.xpiHash)
        hash = EM_L(addon.xpiHash);
      version = EM_L(addon.version);
    }
    this.setItemProperties(addon.id, {
      availableUpdateURL: url,
      availableUpdateHash: hash,
      availableUpdateVersion: version
    });
    this.updateProperty(addon.id, "availableUpdateURL");
  },

  
  
  get URI() {
    return "rdf:extensions";
  },

  GetSource: function EMDS_GetSource(property, target, truthValue) {
    return this._inner.GetSource(property, target, truthValue);
  },

  GetSources: function EMDS_GetSources(property, target, truthValue) {
    return this._inner.GetSources(property, target, truthValue);
  },

  








  _getImageURL: function EMDS__getImageURL(item, fileName) {
    var id = stripPrefix(item.Value, PREFIX_ITEM_URI);
    var installLocation = this._em.getInstallLocation(id);
    if (!installLocation)
      return null;
    var file = installLocation.getItemFile(id, fileName)
    if (file && file.exists())
      return gRDF.GetResource(getURLSpecFromFile(file));

    return null;
  },

  


  _rdfGet_iconURL: function EMDS__rdfGet_iconURL(item, property) {
    var id = stripPrefix(item.Value, PREFIX_ITEM_URI);

    var installLocation = this._em.getInstallLocation(id);
    if (!this.isDownloadItem(id) && !installLocation)
      return null;

    
    iconURL = this._getImageURL(item, "icon.png");
    if (iconURL)
      return iconURL;

    var type = this.getItemProperty(id, "type");
    if (type == Ci.nsIUpdateItem.TYPE_THEME)
      return gRDF.GetResource(URI_GENERIC_ICON_THEME);

    
    
    if (!inSafeMode() && this.getItemProperty(id, "isDisabled") != "true") {
      var iconURL = stringData(this._inner.GetTarget(item, property, true));
      if (iconURL) {
        try {
          var uri = newURI(iconURL);
          var scheme = uri.scheme;
          
          
          if (scheme == "chrome" || (scheme == "http" || scheme == "https") &&
              this._inner.hasArcOut(item, EM_R("downloadURL")))
            return null;
        }
        catch (e) {
        }
      }
    }

    return gRDF.GetResource(URI_GENERIC_ICON_XPINSTALL);
  },

  


  _rdfGet_previewImage: function EMDS__rdfGet_previewImage(item, property) {
    var type = this.getItemProperty(stripPrefix(item.Value, PREFIX_ITEM_URI), "type");
    if (type == Ci.nsIUpdateItem.TYPE_THEME)
      return this._getImageURL(item, "preview.png");
    return null;
  },

  



  _rdfGet_optionsURL: function EMDS__rdfGet_optionsURL(item, property) {
    var id = stripPrefix(item.Value, PREFIX_ITEM_URI);
    if (inSafeMode() || this.getItemProperty(id, "isDisabled") == "true" ||
        this.getItemProperty(id, "type") != Ci.nsIUpdateItem.TYPE_EXTENSION)
      return EM_L("");

    return null;
  },

  




  _rdfGet_aboutURL: function EMDS__rdfGet_aboutURL(item, property) {
    var id = stripPrefix(item.Value, PREFIX_ITEM_URI);
    if (inSafeMode() || this.getItemProperty(id, "isDisabled") == "true" ||
        this.getItemProperty(id, "type") != Ci.nsIUpdateItem.TYPE_EXTENSION ||
        this.getItemProperty(id, "opType") == OP_NEEDS_UPGRADE)
      return EM_L("");

    return null;
  },

  _rdfGet_installDate: function EMDS__rdfGet_installDate(item, property) {
    var id = stripPrefix(item.Value, PREFIX_ITEM_URI);
    var key = this.getItemProperty(id, "installLocation");
    if (key && key in StartupCache.entries && id in StartupCache.entries[key] &&
        StartupCache.entries[key][id] && StartupCache.entries[key][id].mtime)
      return EM_D(StartupCache.entries[key][id].mtime * 1000000);
    return null;
  },

  


  _rdfGet_compatible: function EMDS__rdfGet_compatible(item, property) {
    var compatible = this.isCompatible(this, item, true);
    return compatible ? EM_L("true") : EM_L("false");
  },

  



  _rdfGet_providesUpdatesSecurely: function EMDS__rdfGet_providesUpdatesSecurely(item, property) {
    var id = stripPrefix(item.Value, PREFIX_ITEM_URI);
    if (this.getItemProperty(id, "updateKey") ||
        !this.getItemProperty(id, "updateURL") ||
        this.getItemProperty(id, "updateURL").substring(0, 6) == "https:")
      return EM_L("true");
    return EM_L("false");
  },

  


  _rdfGet_blocklisted: function EMDS__rdfGet_blocklisted(item, property) {
    var id = stripPrefix(item.Value, PREFIX_ITEM_URI);
    var version = this.getItemProperty(id, "version");
    if (!gBlocklist)
      gBlocklist = Cc["@mozilla.org/extensions/blocklist;1"].
                   getService(Ci.nsIBlocklistService);
    if (gBlocklist.getAddonBlocklistState(id, version) == Ci.nsIBlocklistService.STATE_BLOCKED)
      return EM_L("true");

    return EM_L("false");
  },

  



  _rdfGet_blocklistedsoft: function EMDS__rdfGet_blocklistedsoft(item, property) {
    var id = stripPrefix(item.Value, PREFIX_ITEM_URI);
    var version = this.getItemProperty(id, "version");
    if (!gBlocklist)
      gBlocklist = Cc["@mozilla.org/extensions/blocklist;1"].
                   getService(Ci.nsIBlocklistService);
    if (gBlocklist.getAddonBlocklistState(id, version) == Ci.nsIBlocklistService.STATE_SOFTBLOCKED)
      return EM_L("true");

    return EM_L("false");
  },

  


  _rdfGet_state: function EMDS__rdfGet_state(item, property) {
    var id = item.Value;
    if (id in this._progressData)
      return EM_L(this._progressData[id].state);
    return null;
  },

  




  _rdfGet_progress: function EMDS__rdfGet_progress(item, property) {
    var id = item.Value;
    if (id in this._progressData)
      return EM_I(this._progressData[id].progress);
    return null;
  },

  



  _rdfGet_appManaged: function EMDS__rdfGet_appManaged(item, property) {
    var id = stripPrefix(item.Value, PREFIX_ITEM_URI);
    var locationKey = this.getItemProperty(id, "installLocation");
    if (locationKey != KEY_APP_GLOBAL)
      return EM_L("false");
    return null;
  },

  



  _rdfGet_locked: function EMDS__rdfGet_locked(item, property) {
    var id = stripPrefix(item.Value, PREFIX_ITEM_URI);
    var installLocation = InstallLocations.get(this.getInstallLocationKey(id));
    if (!installLocation || !installLocation.restricted)
      return EM_L("false");
    return null;
  },

  




  _rdfGet_satisfiesDependencies: function EMDS__rdfGet_satisfiesDependencies(item, property) {
    var id = stripPrefix(item.Value, PREFIX_ITEM_URI);
    if (this.satisfiesDependencies(id))
      return EM_L("true");
    return EM_L("false");
  },

  



  _rdfGet_opType: function EMDS__rdfGet_opType(item, property) {
    var id = stripPrefix(item.Value, PREFIX_ITEM_URI);
    var key = this.getItemProperty(id, "installLocation");
    if (key in StartupCache.entries && id in StartupCache.entries[key] &&
        StartupCache.entries[key][id] && StartupCache.entries[key][id].op != OP_NONE)
      return EM_L(StartupCache.entries[key][id].op);
    return null;
  },

  








  _getLocalizablePropertyValue: function EMDS__getLocalizablePropertyValue(item, property) {
    
    
    var prefName = PREF_EM_EXTENSION_FORMAT.replace(/%UUID%/,
                    stripPrefix(item.Value, PREFIX_ITEM_URI)) +
                    stripPrefix(property.Value, PREFIX_NS_EM);
    try {
      var value = gPref.getComplexValue(prefName,
                                        Ci.nsIPrefLocalizedString);
      if (value.data)
        return EM_L(value.data);
    }
    catch (e) {
    }

    var localized = findClosestLocalizedResource(this._inner, item);
    if (localized) {
      var value = this._inner.GetTarget(localized, property, true);
      return value ? value : EM_L("");
    }
    return null;
  },

  


  _rdfGet_name: function EMDS__rdfGet_name(item, property) {
    return this._getLocalizablePropertyValue(item, property);
  },

  


  _rdfGet_description: function EMDS__rdfGet_description(item, property) {
    return this._getLocalizablePropertyValue(item, property);
  },

  


  _rdfGet_creator: function EMDS__rdfGet_creator(item, property) {
    return this._getLocalizablePropertyValue(item, property);
  },

  


  _rdfGet_homepageURL: function EMDS__rdfGet_homepageURL(item, property) {
    return this._getLocalizablePropertyValue(item, property);
  },
  
  _rdfGet_availableUpdateInfo: function EMDS__rdfGet_availableUpdateInfo(item, property) {
    var id = stripPrefix(item.Value, PREFIX_ITEM_URI);
    var uri = stringData(this._inner.GetTarget(item, EM_R("availableUpdateInfo"), true));
    if (uri) {
      uri = escapeAddonURI(this.getItemForID(id), null, null, uri, this);
      return EM_L(uri);
    }
    return null;
  },

  



  _rdfGet_isDisabled: function EMDS__rdfGet_isDisabled(item, property) {
    var id = stripPrefix(item.Value, PREFIX_ITEM_URI);
    if (this.getItemProperty(id, "userDisabled") == "true" ||
        this.getItemProperty(id, "appDisabled") == "true" ||
        this.getItemProperty(id, "userDisabled") == OP_NEEDS_ENABLE ||
        this.getItemProperty(id, "appDisabled") == OP_NEEDS_ENABLE)
      return EM_L("true");
    return EM_L("false");
  },

  _rdfGet_addonID: function EMDS__rdfGet_addonID(item, property) {
    var id = this._inner.GetTarget(item, EM_R("downloadURL"), true) ? item.Value :
                                                                      stripPrefix(item.Value, PREFIX_ITEM_URI);
    return EM_L(id);
  },

  



  _rdfGet_updateable: function EMDS__rdfGet_updateable(item, property) {
    var id = stripPrefix(item.Value, PREFIX_ITEM_URI);
    var opType = this.getItemProperty(id, "opType");
    if (opType != OP_NONE || this.getItemProperty(id, "appManaged") == "true")
      return EM_L("false");

    if (getPref("getBoolPref", (PREF_EM_ITEM_UPDATE_ENABLED.replace(/%UUID%/, id), false)) == true)
      return EM_L("false");

    var installLocation = InstallLocations.get(this.getInstallLocationKey(id));
    if (!installLocation || !installLocation.canAccess)
      return EM_L("false");

    return EM_L("true");
  },

  


  GetTarget: function EMDS_GetTarget(source, property, truthValue) {
    if (!source)
      return null;

    var target = null;
    var getter = "_rdfGet_" + stripPrefix(property.Value, PREFIX_NS_EM);
    if (getter in this)
      target = this[getter](source, property);

    return target || this._inner.GetTarget(source, property, truthValue);
  },

  











  _getLocalizablePropertyValues: function EMDS__getLocalizablePropertyValues(item, property) {
    
    
    var values = [];
    var prefName = PREF_EM_EXTENSION_FORMAT.replace(/%UUID%/,
                    stripPrefix(item.Value, PREFIX_ITEM_URI)) +
                    stripPrefix(property.Value, PREFIX_NS_EM);
    var i = 0;
    while (true) {
      try {
        var value = gPref.getComplexValue(prefName + "." + ++i,
                                          Ci.nsIPrefLocalizedString);
        if (value.data)
          values.push(EM_L(value.data));
      }
      catch (e) {
        try {
          var value = gPref.getComplexValue(prefName,
                                            Ci.nsIPrefLocalizedString);
          if (value.data)
            values.push(EM_L(value.data));
        }
        catch (e) {
        }
        break;
      }
    }
    if (values.length > 0)
      return values;

    var localized = findClosestLocalizedResource(this._inner, item);
    if (localized) {
      var targets = this._inner.GetTargets(localized, property, true);
      while (targets.hasMoreElements())
        values.push(targets.getNext());
      return values;
    }
    return null;
  },

  


  _rdfGets_developer: function EMDS__rdfGets_developer(item, property) {
    return this._getLocalizablePropertyValues(item, property);
  },

  


  _rdfGets_translator: function EMDS__rdfGets_translator(item, property) {
    return this._getLocalizablePropertyValues(item, property);
  },

  


  _rdfGets_contributor: function EMDS__rdfGets_contributor(item, property) {
    return this._getLocalizablePropertyValues(item, property);
  },

  


  GetTargets: function EMDS_GetTargets(source, property, truthValue) {
    if (!source)
      return null;

    var ary = null;
    var propertyName = stripPrefix(property.Value, PREFIX_NS_EM);
    var getter = "_rdfGets_" + propertyName;
    if (getter in this)
      ary = this[getter](source, property);
    else {
      
      
      getter = "_rdfGet_" + propertyName;
      if (getter in this)
        ary = [ this[getter](source, property) ];
    }

    return ary ? new ArrayEnumerator(ary)
               : this._inner.GetTargets(source, property, truthValue);
  },

  Assert: function EMDS_Assert(source, property, target, truthValue) {
    this._inner.Assert(source, property, target, truthValue);
  },

  Unassert: function EMDS_Unassert(source, property, target) {
    this._inner.Unassert(source, property, target);
  },

  Change: function EMDS_Change(source, property, oldTarget, newTarget) {
    this._inner.Change(source, property, oldTarget, newTarget);
  },

  Move: function EMDS_Move(oldSource, newSource, property, target) {
    this._inner.Move(oldSource, newSource, property, target);
  },

  HasAssertion: function EMDS_HasAssertion(source, property, target, truthValue) {
    if (!source || !property || !target)
      return false;

    var getter = "_rdfGet_" + stripPrefix(property.Value, PREFIX_NS_EM);
    if (getter in this)
      return this[getter](source, property) == target;
    return this._inner.HasAssertion(source, property, target, truthValue);
  },

  _observers: [],
  AddObserver: function EMDS_AddObserver(observer) {
    for (var i = 0; i < this._observers.length; ++i) {
      if (this._observers[i] == observer)
        return;
    }
    this._observers.push(observer);
    this._inner.AddObserver(observer);
  },

  RemoveObserver: function EMDS_RemoveObserver(observer) {
    for (var i = 0; i < this._observers.length; ++i) {
      if (this._observers[i] == observer)
        this._observers.splice(i, 1);
    }
    this._inner.RemoveObserver(observer);
  },

  ArcLabelsIn: function EMDS_ArcLabelsIn(node) {
    return this._inner.ArcLabelsIn(node);
  },

  ArcLabelsOut: function EMDS_ArcLabelsOut(source) {
    return this._inner.ArcLabelsOut(source);
  },

  GetAllResources: function EMDS_GetAllResources() {
    return this._inner.GetAllResources();
  },

  IsCommandEnabled: function EMDS_IsCommandEnabled(sources, command, arguments) {
    return this._inner.IsCommandEnabled(sources, command, arguments);
  },

  DoCommand: function EMDS_DoCommand(sources, command, arguments) {
    this._inner.DoCommand(sources, command, arguments);
  },

  GetAllCmds: function EMDS_GetAllCmds(source) {
    return this._inner.GetAllCmds(source);
  },

  hasArcIn: function EMDS_hasArcIn(node, arc) {
    return this._inner.hasArcIn(node, arc);
  },

  hasArcOut: function EMDS_hasArcOut(source, arc) {
    return this._inner.hasArcOut(source, arc);
  },

  beginUpdateBatch: function EMDS_beginUpdateBatch() {
    return this._inner.beginUpdateBatch();
  },

  endUpdateBatch: function EMDS_endUpdateBatch() {
    return this._inner.endUpdateBatch();
  },

  


  get loaded() {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },

  Init: function EMDS_Init(uri) {
  },

  Refresh: function EMDS_Refresh(blocking) {
  },

  Flush: function EMDS_Flush() {
    
    
    if (!gAllowFlush) {
      gDSNeedsFlush = true;
      return;
    }
    if (this._inner instanceof Ci.nsIRDFRemoteDataSource)
      this._inner.Flush();
  },

  FlushTo: function EMDS_FlushTo(uri) {
  },

  classDescription: "Extension Manager Data Source",
  contractID: "@mozilla.org/rdf/datasource;1?name=extensions",
  classID: Components.ID("{69BB8313-2D4F-45EC-97E0-D39DA58ECCE9}"),
  _xpcom_factory: {
    createInstance: function() Cc[ExtensionManager.prototype.contractID].
                               getService(Ci.nsIExtensionManager).datasource
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRDFDataSource,
                                         Ci.nsIRDFRemoteDataSource])
};

function UpdateItem () {}
UpdateItem.prototype = {
  


  init: function(id, version, installLocationKey, minAppVersion, maxAppVersion,
                 name, downloadURL, xpiHash, iconURL, updateURL, updateKey, type,
                 targetAppID) {
    this._id                  = id;
    this._version             = version;
    this._installLocationKey  = installLocationKey;
    this._minAppVersion       = minAppVersion;
    this._maxAppVersion       = maxAppVersion;
    this._name                = name;
    this._downloadURL         = downloadURL;
    this._xpiHash             = xpiHash;
    this._iconURL             = iconURL;
    this._updateURL           = updateURL;
    this._updateKey           = updateKey;
    this._type                = type;
    this._targetAppID         = targetAppID;
  },

  


  get id()                { return this._id;                },
  get version()           { return this._version;           },
  get installLocationKey(){ return this._installLocationKey;},
  get minAppVersion()     { return this._minAppVersion;     },
  get maxAppVersion()     { return this._maxAppVersion;     },
  get name()              { return this._name;              },
  get xpiURL()            { return this._downloadURL;       },
  get xpiHash()           { return this._xpiHash;           },
  get iconURL()           { return this._iconURL            },
  get updateRDF()         { return this._updateURL;         },
  get updateKey()         { return this._updateKey;         },
  get type()              { return this._type;              },
  get targetAppID()       { return this._targetAppID;       },

  


  get objectSource() {
    return { id                 : this._id,
             version            : this._version,
             installLocationKey : this._installLocationKey,
             minAppVersion      : this._minAppVersion,
             maxAppVersion      : this._maxAppVersion,
             name               : this._name,
             xpiURL             : this._downloadURL,
             xpiHash            : this._xpiHash,
             iconURL            : this._iconURL,
             updateRDF          : this._updateURL,
             updateKey          : this._updateKey,
             type               : this._type,
             targetAppID        : this._targetAppID
           }.toSource();
  },

  classDescription: "Update Item",
  contractID: "@mozilla.org/updates/item;1",
  classID: Components.ID("{F3294B1C-89F4-46F8-98A0-44E1EAE92518}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUpdateItem])
};

function NSGetModule(compMgr, fileSpec)
  XPCOMUtils.generateModule([ExtensionManager, ExtensionsDataSource, UpdateItem]);

#if 0





function STACK(string) {
  dump("*** " + string + "\n");
  stackTrace(arguments.callee.caller.arguments, -1);
}

function stackTraceFunctionFormat(aFunctionName) {
  var classDelimiter = aFunctionName.indexOf("_");
  var className = aFunctionName.substr(0, classDelimiter);
  if (!className)
    className = "<global>";
  var functionName = aFunctionName.substr(classDelimiter + 1, aFunctionName.length);
  if (!functionName)
    functionName = "<anonymous>";
  return className + "::" + functionName;
}

function stackTraceArgumentsFormat(aArguments) {
  arglist = "";
  for (var i = 0; i < aArguments.length; i++) {
    arglist += aArguments[i];
    if (i < aArguments.length - 1)
      arglist += ", ";
  }
  return arglist;
}

function stackTrace(aArguments, aMaxCount) {
  dump("=[STACKTRACE]=====================================================\n");
  dump("*** at: " + stackTraceFunctionFormat(aArguments.callee.name) + "(" +
       stackTraceArgumentsFormat(aArguments) + ")\n");
  var temp = aArguments.callee.caller;
  var count = 0;
  while (temp) {
    dump("***     " + stackTraceFunctionFormat(temp.name) + "(" +
         stackTraceArgumentsFormat(temp.arguments) + ")\n");

    temp = temp.arguments.callee.caller;
    if (aMaxCount > 0 && ++count == aMaxCount)
      break;
  }
  dump("==================================================================\n");
}

function dumpFile(file) {
  dump("*** file = " + file.path + ", exists = " + file.exists() + "\n");
}
#endif
