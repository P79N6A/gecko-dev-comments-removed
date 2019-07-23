









































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const TOOLKIT_ID                      = "toolkit@mozilla.org"
const KEY_PROFILEDIR                  = "ProfD";
const KEY_APPDIR                      = "XCurProcD";
const FILE_BLOCKLIST                  = "blocklist.xml";
const PREF_BLOCKLIST_URL              = "extensions.blocklist.url";
const PREF_BLOCKLIST_ENABLED          = "extensions.blocklist.enabled";
const PREF_BLOCKLIST_INTERVAL         = "extensions.blocklist.interval";
const PREF_BLOCKLIST_LEVEL            = "extensions.blocklist.level";
const PREF_GENERAL_USERAGENT_LOCALE   = "general.useragent.locale";
const PREF_PARTNER_BRANCH             = "app.partner.";
const PREF_APP_DISTRIBUTION           = "distribution.id";
const PREF_APP_DISTRIBUTION_VERSION   = "distribution.version";
const PREF_APP_UPDATE_CHANNEL         = "app.update.channel";
const PREF_EM_LOGGING_ENABLED         = "extensions.logging.enabled";
const XMLURI_BLOCKLIST                = "http://www.mozilla.org/2006/addons-blocklist";
const XMLURI_PARSE_ERROR              = "http://www.mozilla.org/newlayout/xml/parsererror.xml"
const UNKNOWN_XPCOM_ABI               = "unknownABI";
const URI_BLOCKLIST_DIALOG            = "chrome://mozapps/content/extensions/blocklist.xul"
const DEFAULT_SEVERITY                = 3;
const DEFAULT_LEVEL                   = 2;
const MAX_BLOCK_LEVEL                 = 3;

const MODE_RDONLY   = 0x01;
const MODE_WRONLY   = 0x02;
const MODE_CREATE   = 0x08;
const MODE_APPEND   = 0x10;
const MODE_TRUNCATE = 0x20;

const PERMS_FILE      = 0644;
const PERMS_DIRECTORY = 0755;

var gApp = null;
var gPref = null;
var gOS = null;
var gConsole = null;
var gVersionChecker = null;
var gLoggingEnabled = null;
var gABI = null;
var gOSVersion = null;
var gBlocklistEnabled = true;
var gBlocklistLevel = DEFAULT_LEVEL;


#include ../../shared/src/badCertHandler.js






function LOG(string) {
  if (gLoggingEnabled) {
    dump("*** " + string + "\n");
    if (gConsole)
      gConsole.logStringMessage(string);
  }
}













function getPref(func, preference, defaultValue) {
  try {
    return gPref[func](preference);
  }
  catch (e) {
  }
  return defaultValue;
}













function getFile(key, pathArray) {
  var fileLocator = Cc["@mozilla.org/file/directory_service;1"].
                    getService(Ci.nsIProperties);
  var file = fileLocator.get(key, Ci.nsILocalFile);
  for (var i = 0; i < pathArray.length - 1; ++i) {
    file.append(pathArray[i]);
    if (!file.exists())
      file.create(Ci.nsILocalFile.DIRECTORY_TYPE, PERMS_DIRECTORY);
  }
  file.followLinks = false;
  file.append(pathArray[pathArray.length - 1]);
  return file;
}









function openSafeFileOutputStream(file, modeFlags) {
  var fos = Cc["@mozilla.org/network/safe-file-output-stream;1"].
            createInstance(Ci.nsIFileOutputStream);
  if (modeFlags === undefined)
    modeFlags = MODE_WRONLY | MODE_CREATE | MODE_TRUNCATE;
  if (!file.exists())
    file.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, PERMS_FILE);
  fos.init(file, modeFlags, PERMS_FILE, 0);
  return fos;
}






function closeSafeFileOutputStream(stream) {
  if (stream instanceof Ci.nsISafeOutputStream)
    stream.finish();
  else
    stream.close();
}







function newURI(spec) {
  var ioServ = Cc["@mozilla.org/network/io-service;1"].
               getService(Ci.nsIIOService);
  return ioServ.newURI(spec, null, null);
}


function restartApp() {
  
  var os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  var cancelQuit = Cc["@mozilla.org/supports-PRBool;1"].
                   createInstance(Ci.nsISupportsPRBool);
  os.notifyObservers(cancelQuit, "quit-application-requested", null);

  
  if (cancelQuit.data)
    return;

  var as = Cc["@mozilla.org/toolkit/app-startup;1"].
           getService(Ci.nsIAppStartup);
  as.quit(Ci.nsIAppStartup.eRestart | Ci.nsIAppStartup.eAttemptQuit);
}







function matchesOSABI(blocklistElement) {
  if (blocklistElement.hasAttribute("os")) {
    var choices = blocklistElement.getAttribute("os").split(",");
    if (choices.length > 0 && choices.indexOf(gApp.OS) < 0)
      return false;
  }
  
  if (blocklistElement.hasAttribute("xpcomabi")) {
    choices = blocklistElement.getAttribute("xpcomabi").split(",");
    if (choices.length > 0 && choices.indexOf(gApp.XPCOMABI) < 0)
      return false;
  }
  
  return true;
}






function getLocale() {
  try {
      
      var defaultPrefs = gPref.getDefaultBranch(null);
      return defaultPrefs.getCharPref(PREF_GENERAL_USERAGENT_LOCALE);
  } catch (e) {}

  return gPref.getCharPref(PREF_GENERAL_USERAGENT_LOCALE);
}






function getUpdateChannel() {
  var channel = "default";
  var prefName;
  var prefValue;

  var defaults = gPref.getDefaultBranch(null);
  try {
    channel = defaults.getCharPref(PREF_APP_UPDATE_CHANNEL);
  } catch (e) {
    
  }

  try {
    var partners = gPref.getChildList(PREF_PARTNER_BRANCH, { });
    if (partners.length) {
      channel += "-cck";
      partners.sort();

      for each (prefName in partners) {
        prefValue = gPref.getCharPref(prefName);
        channel += "-" + prefValue;
      }
    }
  }
  catch (e) {
    Components.utils.reportError(e);
  }

  return channel;
}


function getDistributionPrefValue(aPrefName) {
  var prefValue = "default";

  var defaults = gPref.getDefaultBranch(null);
  try {
    prefValue = defaults.getCharPref(aPrefName);
  } catch (e) {
    
  }

  return prefValue;
}








function Blocklist() {
  gApp = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULAppInfo);
  gApp.QueryInterface(Ci.nsIXULRuntime);
  gPref = Cc["@mozilla.org/preferences-service;1"].
          getService(Ci.nsIPrefService).
          QueryInterface(Ci.nsIPrefBranch2);
  gVersionChecker = Cc["@mozilla.org/xpcom/version-comparator;1"].
                    getService(Ci.nsIVersionComparator);
  gConsole = Cc["@mozilla.org/consoleservice;1"].
             getService(Ci.nsIConsoleService);

  gOS = Cc["@mozilla.org/observer-service;1"].
        getService(Ci.nsIObserverService);
  gOS.addObserver(this, "xpcom-shutdown", false);

  
  try {
    gABI = gApp.XPCOMABI;
  }
  catch (e) {
    LOG("Blocklist: XPCOM ABI unknown.");
    gABI = UNKNOWN_XPCOM_ABI;
  }

  var osVersion;
  var sysInfo = Components.classes["@mozilla.org/system-info;1"]
                          .getService(Components.interfaces.nsIPropertyBag2);
  try {
    osVersion = sysInfo.getProperty("name") + " " + sysInfo.getProperty("version");
  }
  catch (e) {
    LOG("Blocklist: OS Version unknown.");
  }

  if (osVersion) {
    try {
      osVersion += " (" + sysInfo.getProperty("secondaryLibrary") + ")";
    }
    catch (e) {
      
    }
    gOSVersion = encodeURIComponent(osVersion);
  }

#ifdef XP_MACOSX
  
  
  var macutils = Components.classes["@mozilla.org/xpcom/mac-utils;1"]
                           .getService(Components.interfaces.nsIMacUtils);

  if (macutils.isUniversalBinary)
    gABI = "Universal-gcc3";
#endif
}

Blocklist.prototype = {
  














  _addonEntries: null,
  _pluginEntries: null,

  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
    case "profile-after-change":
      gLoggingEnabled = getPref("getBoolPref", PREF_EM_LOGGING_ENABLED, false);
      gBlocklistEnabled = getPref("getBoolPref", PREF_BLOCKLIST_ENABLED, true);
      gBlocklistLevel = Math.min(getPref("getIntPref", PREF_BLOCKLIST_LEVEL, DEFAULT_LEVEL),
                                 MAX_BLOCK_LEVEL);
      gPref.addObserver("extensions.blocklist.", this, false);
      var tm = Cc["@mozilla.org/updates/timer-manager;1"].
               getService(Ci.nsIUpdateTimerManager);
      var interval = getPref("getIntPref", PREF_BLOCKLIST_INTERVAL, 86400);
      tm.registerTimer("blocklist-background-update-timer", this, interval);
      break;
    case "xpcom-shutdown":
      gOS.removeObserver(this, "xpcom-shutdown");
      gOS = null;
      gPref.removeObserver("extensions.blocklist.", this);
      gPref = null;
      gConsole = null;
      gVersionChecker = null;
      gApp = null;
      break;
    case "nsPref:changed":
      switch (aData) {
        case PREF_BLOCKLIST_ENABLED:
          gBlocklistEnabled = getPref("getBoolPref", PREF_BLOCKLIST_ENABLED, true);
          this._loadBlocklist();
          this._blocklistUpdated(null, null);
          break;
        case PREF_BLOCKLIST_LEVEL:
          gBlocklistLevel = Math.min(getPref("getIntPref", PREF_BLOCKLIST_LEVEL, DEFAULT_LEVEL),
                                     MAX_BLOCK_LEVEL);
          this._blocklistUpdated(null, null);
          break;
      }
      break;
    }
  },

  
  isAddonBlocklisted: function(id, version, appVersion, toolkitVersion) {
    return this.getAddonBlocklistState(id, version, appVersion, toolkitVersion) ==
                   Ci.nsIBlocklistService.STATE_BLOCKED;
  },

  
  getAddonBlocklistState: function(id, version, appVersion, toolkitVersion) {
    if (!this._addonEntries)
      this._loadBlocklist();
    return this._getAddonBlocklistState(id, version, this._addonEntries,
                                        appVersion, toolkitVersion);
  },

  


















  _getAddonBlocklistState: function(id, version, addonEntries, appVersion, toolkitVersion) {
    if (!gBlocklistEnabled)
      return Ci.nsIBlocklistService.STATE_NOT_BLOCKED;

    if (!appVersion)
      appVersion = gApp.version;
    if (!toolkitVersion)
      toolkitVersion = gApp.platformVersion;

    var blItem = addonEntries[id];
    if (!blItem)
      return Ci.nsIBlocklistService.STATE_NOT_BLOCKED;

    for (var i = 0; i < blItem.length; ++i) {
      if (blItem[i].includesItem(version, appVersion, toolkitVersion))
        return blItem[i].severity >= gBlocklistLevel ? Ci.nsIBlocklistService.STATE_BLOCKED :
                                                       Ci.nsIBlocklistService.STATE_SOFTBLOCKED;
    }
    return Ci.nsIBlocklistService.STATE_NOT_BLOCKED;
  },

  notify: function(aTimer) {
    if (!gBlocklistEnabled)
      return;

    try {
      var dsURI = gPref.getCharPref(PREF_BLOCKLIST_URL);
    }
    catch (e) {
      LOG("Blocklist::notify: The " + PREF_BLOCKLIST_URL + " preference" +
          " is missing!");
      return;
    }

    dsURI = dsURI.replace(/%APP_ID%/g, gApp.ID);
    dsURI = dsURI.replace(/%APP_VERSION%/g, gApp.version);
    dsURI = dsURI.replace(/%PRODUCT%/g, gApp.name);
    dsURI = dsURI.replace(/%VERSION%/g, gApp.version);
    dsURI = dsURI.replace(/%BUILD_ID%/g, gApp.appBuildID);
    dsURI = dsURI.replace(/%BUILD_TARGET%/g, gApp.OS + "_" + gABI);
    dsURI = dsURI.replace(/%OS_VERSION%/g, gOSVersion);
    dsURI = dsURI.replace(/%LOCALE%/g, getLocale());
    dsURI = dsURI.replace(/%CHANNEL%/g, getUpdateChannel());
    dsURI = dsURI.replace(/%PLATFORM_VERSION%/g, gApp.platformVersion);
    dsURI = dsURI.replace(/%DISTRIBUTION%/g,
                      getDistributionPrefValue(PREF_APP_DISTRIBUTION));
    dsURI = dsURI.replace(/%DISTRIBUTION_VERSION%/g,
                      getDistributionPrefValue(PREF_APP_DISTRIBUTION_VERSION));
    dsURI = dsURI.replace(/\+/g, "%2B");

    
    try {
      var uri = newURI(dsURI);
    }
    catch (e) {
      LOG("Blocklist::notify: There was an error creating the blocklist URI\r\n" +
          "for: " + dsURI + ", error: " + e);
      return;
    }

    var request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
                  createInstance(Ci.nsIXMLHttpRequest);
    request.open("GET", uri.spec, true);
    request.channel.notificationCallbacks = new BadCertHandler();
    request.overrideMimeType("text/xml");
    request.setRequestHeader("Cache-Control", "no-cache");
    request.QueryInterface(Components.interfaces.nsIJSXMLHttpRequest);

    var self = this;
    request.onerror = function(event) { self.onXMLError(event); };
    request.onload  = function(event) { self.onXMLLoad(event);  };
    request.send(null);

    
    
    if (!this._addonEntries)
      this._loadBlocklist();
  },

  onXMLLoad: function(aEvent) {
    var request = aEvent.target;
    try {
      checkCert(request.channel);
    }
    catch (e) {
      LOG("Blocklist::onXMLLoad: " + e);
      return;
    }
    var responseXML = request.responseXML;
    if (!responseXML || responseXML.documentElement.namespaceURI == XMLURI_PARSE_ERROR ||
        (request.status != 200 && request.status != 0)) {
      LOG("Blocklist::onXMLLoad: there was an error during load");
      return;
    }
    var blocklistFile = getFile(KEY_PROFILEDIR, [FILE_BLOCKLIST]);
    if (blocklistFile.exists())
      blocklistFile.remove(false);
    var fos = openSafeFileOutputStream(blocklistFile);
    fos.write(request.responseText, request.responseText.length);
    closeSafeFileOutputStream(fos);

    var oldAddonEntries = this._addonEntries;
    var oldPluginEntries = this._pluginEntries;
    this._addonEntries = { };
    this._pluginEntries = { };
    this._loadBlocklistFromFile(getFile(KEY_PROFILEDIR, [FILE_BLOCKLIST]));

    this._blocklistUpdated(oldAddonEntries, oldPluginEntries);
  },

  onXMLError: function(aEvent) {
    try {
      var request = aEvent.target;
      
      var status = request.status;
    }
    catch (e) {
      request = aEvent.target.channel.QueryInterface(Ci.nsIRequest);
      status = request.status;
    }
    var statusText = request.statusText;
    
    if (status == 0)
      statusText = "nsIXMLHttpRequest channel unavailable";
    LOG("Blocklist:onError: There was an error loading the blocklist file\r\n" +
        statusText);
  },

  



  _loadBlocklist: function() {
    this._addonEntries = { };
    this._pluginEntries = { };
    var profFile = getFile(KEY_PROFILEDIR, [FILE_BLOCKLIST]);
    if (profFile.exists()) {
      this._loadBlocklistFromFile(profFile);
      return;
    }
    var appFile = getFile(KEY_APPDIR, [FILE_BLOCKLIST]);
    if (appFile.exists()) {
      this._loadBlocklistFromFile(appFile);
      return;
    }
    LOG("Blocklist::_loadBlocklist: no XML File found");
  },

  



















































  _loadBlocklistFromFile: function(file) {
    if (!gBlocklistEnabled) {
      LOG("Blocklist::_loadBlocklistFromFile: blocklist is disabled");
      return;
    }

    if (!file.exists()) {
      LOG("Blocklist::_loadBlocklistFromFile: XML File does not exist");
      return;
    }

    var fileStream = Components.classes["@mozilla.org/network/file-input-stream;1"]
                               .createInstance(Components.interfaces.nsIFileInputStream);
    fileStream.init(file, MODE_RDONLY, PERMS_FILE, 0);
    try {
      var parser = Cc["@mozilla.org/xmlextras/domparser;1"].
                   createInstance(Ci.nsIDOMParser);
      var doc = parser.parseFromStream(fileStream, "UTF-8", file.fileSize, "text/xml");
      if (doc.documentElement.namespaceURI != XMLURI_BLOCKLIST) {
        LOG("Blocklist::_loadBlocklistFromFile: aborting due to incorrect " +
            "XML Namespace.\r\nExpected: " + XMLURI_BLOCKLIST + "\r\n" +
            "Received: " + doc.documentElement.namespaceURI);
        return;
      }

      var childNodes = doc.documentElement.childNodes;
      this._addonEntries = this._processItemNodes(childNodes, "em",
                                                  this._handleEmItemNode);
      this._pluginEntries = this._processItemNodes(childNodes, "plugin",
                                                   this._handlePluginItemNode);
    }
    catch (e) {
      LOG("Blocklist::_loadBlocklistFromFile: Error constructing blocklist " + e);
      return;
    }
    fileStream.close();
  },

  _processItemNodes: function(deChildNodes, prefix, handler) {
    var result = [];
    var itemNodes;
    var containerName = prefix + "Items";
    for (var i = 0; i < deChildNodes.length; ++i) {
      var emItemsElement = deChildNodes.item(i);
      if (emItemsElement instanceof Ci.nsIDOMElement &&
          emItemsElement.localName == containerName) {
        itemNodes = emItemsElement.childNodes;
        break;
      }
    }
    if (!itemNodes)
      return result;

    var itemName = prefix + "Item";
    for (var i = 0; i < itemNodes.length; ++i) {
      var blocklistElement = itemNodes.item(i);
      if (!(blocklistElement instanceof Ci.nsIDOMElement) ||
          blocklistElement.localName != itemName)
        continue;

      handler(blocklistElement, result);
    }
    return result;
  },

  _handleEmItemNode: function(blocklistElement, result) {
    if (!matchesOSABI(blocklistElement))
      return;

    var versionNodes = blocklistElement.childNodes;
    var id = blocklistElement.getAttribute("id");
    result[id] = [];
    for (var x = 0; x < versionNodes.length; ++x) {
      var versionRangeElement = versionNodes.item(x);
      if (!(versionRangeElement instanceof Ci.nsIDOMElement) ||
          versionRangeElement.localName != "versionRange")
        continue;

      result[id].push(new BlocklistItemData(versionRangeElement));
    }
    
    
    if (result[id].length == 0)
      result[id].push(new BlocklistItemData(null));
  },

  _handlePluginItemNode: function(blocklistElement, result) {
    if (!matchesOSABI(blocklistElement))
      return;

    var matchNodes = blocklistElement.childNodes;
    var blockEntry = {
      matches: {},
      versions: []
    };
    var hasMatch = false;
    for (var x = 0; x < matchNodes.length; ++x) {
      var matchElement = matchNodes.item(x);
      if (!(matchElement instanceof Ci.nsIDOMElement))
        continue;
      if (matchElement.localName == "match") {
        var name = matchElement.getAttribute("name");
        var exp = matchElement.getAttribute("exp");
        try {
          blockEntry.matches[name] = new RegExp(exp, "m");
          hasMatch = true;
        } catch (e) {
          
        }
      }
      if (matchElement.localName == "versionRange")
        blockEntry.versions.push(new BlocklistItemData(matchElement));
    }
    
    if (!hasMatch)
      return;
    
    if (blockEntry.versions.length == 0)
      blockEntry.versions.push(new BlocklistItemData(null));
    result.push(blockEntry);
  },

  
  getPluginBlocklistState: function(plugin, appVersion, toolkitVersion) {
    if (!this._pluginEntries)
      this._loadBlocklist();
    return this._getPluginBlocklistState(plugin, this._pluginEntries,
                                         appVersion, toolkitVersion);
  },

  
















  _getPluginBlocklistState: function(plugin, pluginEntries, appVersion, toolkitVersion) {
    if (!gBlocklistEnabled)
      return Ci.nsIBlocklistService.STATE_NOT_BLOCKED;

    if (!appVersion)
      appVersion = gApp.version;
    if (!toolkitVersion)
      toolkitVersion = gApp.platformVersion;

    for each (var blockEntry in pluginEntries) {
      var matchFailed = false;
      for (var name in blockEntry.matches) {
        if (!(name in plugin) ||
            typeof(plugin[name]) != "string" ||
            !blockEntry.matches[name].test(plugin[name])) {
          matchFailed = true;
          break;
        }
      }

      if (matchFailed)
        continue;

      for (var i = 0; i < blockEntry.versions.length; i++) {
        if (blockEntry.versions[i].includesItem(plugin.version, appVersion,
                                                toolkitVersion))
          return blockEntry.versions[i].severity >= gBlocklistLevel ?
                                                    Ci.nsIBlocklistService.STATE_BLOCKED :
                                                    Ci.nsIBlocklistService.STATE_SOFTBLOCKED;
      }
    }

    return Ci.nsIBlocklistService.STATE_NOT_BLOCKED;
  },

  _blocklistUpdated: function(oldAddonEntries, oldPluginEntries) {
    var addonList = [];

    var em = Cc["@mozilla.org/extensions/manager;1"].
             getService(Ci.nsIExtensionManager);
    var addons = em.updateAndGetNewBlocklistedItems({});

    for (let i = 0; i < addons.length; i++) {
      let oldState = -1;
      if (oldAddonEntries)
        oldState = this._getAddonBlocklistState(addons[i].id, addons[i].version,
                                                oldAddonEntries);
      let state = this.getAddonBlocklistState(addons[i].id, addons[i].version);
      
      if (state == oldState)
        continue;

      addonList.push({
        name: addons[i].name,
        version: addons[i].version,
        icon: addons[i].iconURL,
        disable: false,
        blocked: state == Ci.nsIBlocklistService.STATE_BLOCKED,
        item: addons[i]
      });
    }

    var phs = Cc["@mozilla.org/plugin/host;1"].
              getService(Ci.nsIPluginHost);
    var plugins = phs.getPluginTags({});

    for (let i = 0; i < plugins.length; i++) {
      let oldState = -1;
      if (oldPluginEntries)
        oldState = this._getPluginBlocklistState(plugins[i], oldPluginEntries);
      let state = this.getPluginBlocklistState(plugins[i]);
      
      if (state == oldState)
        continue;

      if (plugins[i].blocklisted) {
        if (state == Ci.nsIBlocklistService.STATE_SOFTBLOCKED)
          plugins[i].disabled = true;
      }
      else if (!plugins[i].disabled && state != Ci.nsIBlocklistService.STATE_NOT_BLOCKED) {
        addonList.push({
          name: plugins[i].name,
          version: plugins[i].version,
          icon: "chrome://mozapps/skin/plugins/pluginGeneric.png",
          disable: false,
          blocked: state == Ci.nsIBlocklistService.STATE_BLOCKED,
          item: plugins[i]
        });
      }
      plugins[i].blocklisted = state == Ci.nsIBlocklistService.STATE_BLOCKED;
    }

    if (addonList.length == 0)
      return;

    var args = {
      restart: false,
      list: addonList
    };
    
    args.wrappedJSObject = args;

    var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
             getService(Ci.nsIWindowWatcher);
    ww.openWindow(null, URI_BLOCKLIST_DIALOG, "",
                  "chrome,centerscreen,dialog,modal,titlebar", args);

    for (let i = 0; i < addonList.length; i++) {
      if (!addonList[i].disable)
        continue;

      if (addonList[i].item instanceof Ci.nsIUpdateItem)
        em.disableItem(addonList[i].item.id);
      else if (addonList[i].item instanceof Ci.nsIPluginTag)
        addonList[i].item.disabled = true;
      else
        LOG("Unknown add-on type: " + addonList[i].item);
    }

    if (args.restart)
      restartApp();
  },

  classDescription: "Blocklist Service",
  contractID: "@mozilla.org/extensions/blocklist;1",
  classID: Components.ID("{66354bc9-7ed1-4692-ae1d-8da97d6b205e}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsIBlocklistService,
                                         Ci.nsITimerCallback]),
  _xpcom_categories: [{ category: "profile-after-change" }]
};




function BlocklistItemData(versionRangeElement) {
  var versionRange = this.getBlocklistVersionRange(versionRangeElement);
  this.minVersion = versionRange.minVersion;
  this.maxVersion = versionRange.maxVersion;
  if (versionRangeElement && versionRangeElement.hasAttribute("severity"))
    this.severity = versionRangeElement.getAttribute("severity");
  else
    this.severity = DEFAULT_SEVERITY;
  this.targetApps = { };
  var found = false;

  if (versionRangeElement) {
    for (var i = 0; i < versionRangeElement.childNodes.length; ++i) {
      var targetAppElement = versionRangeElement.childNodes.item(i);
      if (!(targetAppElement instanceof Ci.nsIDOMElement) ||
          targetAppElement.localName != "targetApplication")
        continue;
      found = true;
      
      var appID = targetAppElement.hasAttribute("id") ? targetAppElement.getAttribute("id") : gApp.ID;
      this.targetApps[appID] = this.getBlocklistAppVersions(targetAppElement);
    }
  }
  
  
  if (!found)
    this.targetApps[gApp.ID] = this.getBlocklistAppVersions(null);
}

BlocklistItemData.prototype = {
  












  includesItem: function(version, appVersion, toolkitVersion) {
    
    
    if (!version && (this.minVersion || this.maxVersion))
      return false;

    
    if (!this.matchesRange(version, this.minVersion, this.maxVersion))
      return false;

    
    if (this.matchesTargetRange(gApp.ID, appVersion))
      return true;

    
    return this.matchesTargetRange(TOOLKIT_ID, toolkitVersion);
  },

  











  matchesRange: function(version, minVersion, maxVersion) {
    if (minVersion && gVersionChecker.compare(version, minVersion) < 0)
      return false;
    if (maxVersion && gVersionChecker.compare(version, maxVersion) > 0)
      return false;
    return true;
  },

  








  matchesTargetRange: function(appID, appVersion) {
    var blTargetApp = this.targetApps[appID];
    if (!blTargetApp)
      return false;

    for (var x = 0; x < blTargetApp.length; ++x) {
      if (this.matchesRange(appVersion, blTargetApp[x].minVersion, blTargetApp[x].maxVersion))
        return true;
    }

    return false;
  },

  








  getBlocklistAppVersions: function(targetAppElement) {
    var appVersions = [ ];

    if (targetAppElement) {
      for (var i = 0; i < targetAppElement.childNodes.length; ++i) {
        var versionRangeElement = targetAppElement.childNodes.item(i);
        if (!(versionRangeElement instanceof Ci.nsIDOMElement) ||
            versionRangeElement.localName != "versionRange")
          continue;
        appVersions.push(this.getBlocklistVersionRange(versionRangeElement));
      }
    }
    
    
    if (appVersions.length == 0)
      appVersions.push(this.getBlocklistVersionRange(null));
    return appVersions;
  },

  








  getBlocklistVersionRange: function(versionRangeElement) {
    var minVersion = null;
    var maxVersion = null;
    if (!versionRangeElement)
      return { minVersion: minVersion, maxVersion: maxVersion };

    if (versionRangeElement.hasAttribute("minVersion"))
      minVersion = versionRangeElement.getAttribute("minVersion");
    if (versionRangeElement.hasAttribute("maxVersion"))
      maxVersion = versionRangeElement.getAttribute("maxVersion");

    return { minVersion: minVersion, maxVersion: maxVersion };
  }
};

function NSGetModule(aCompMgr, aFileSpec) {
  return XPCOMUtils.generateModule([Blocklist]);
}
