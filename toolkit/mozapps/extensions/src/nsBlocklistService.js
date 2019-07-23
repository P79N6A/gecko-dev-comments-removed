








































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const kELEMENT_NODE                   = Ci.nsIDOMNode.ELEMENT_NODE;
const TOOLKIT_ID                      = "toolkit@mozilla.org"
const KEY_PROFILEDIR                  = "ProfD";
const FILE_BLOCKLIST                  = "blocklist.xml";
const PREF_BLOCKLIST_URL              = "extensions.blocklist.url";
const PREF_BLOCKLIST_ENABLED          = "extensions.blocklist.enabled";
const PREF_BLOCKLIST_INTERVAL         = "extensions.blocklist.interval";
const PREF_EM_LOGGING_ENABLED         = "extensions.logging.enabled";
const XMLURI_BLOCKLIST                = "http://www.mozilla.org/2006/addons-blocklist";
const XMLURI_PARSE_ERROR              = "http://www.mozilla.org/newlayout/xml/parsererror.xml"

const MODE_RDONLY   = 0x01;
const MODE_WRONLY   = 0x02;
const MODE_CREATE   = 0x08;
const MODE_APPEND   = 0x10;
const MODE_TRUNCATE = 0x20;

const PERMS_FILE      = 0644;
const PERMS_DIRECTORY = 0755;

const CID = Components.ID("{66354bc9-7ed1-4692-ae1d-8da97d6b205e}");
const CONTRACT_ID = "@mozilla.org/extensions/blocklist;1"
const CLASS_NAME = "Blocklist Service";

var gApp = null;
var gPref = null;
var gOS = null;
var gConsole = null;
var gVersionChecker = null;
var gLoggingEnabled = null;


#include ../../shared/src/badCertHandler.js






function LOG(string) {
  if (gLoggingEnabled) {
    dump("*** " + string + "\n");
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








function Blocklist() {
  gApp = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULAppInfo);
  gApp.QueryInterface(Ci.nsIXULRuntime);
  gPref = Cc["@mozilla.org/preferences-service;1"].
          getService(Ci.nsIPrefBranch2);
  gVersionChecker = Cc["@mozilla.org/xpcom/version-comparator;1"].
                    getService(Ci.nsIVersionComparator);
  gConsole = Cc["@mozilla.org/consoleservice;1"].
             getService(Ci.nsIConsoleService);  
  
  gOS = Cc["@mozilla.org/observer-service;1"].
        getService(Ci.nsIObserverService);
  gOS.addObserver(this, "xpcom-shutdown", false);
}

Blocklist.prototype = {
  














  _addonEntries: null,
  _pluginEntries: null,

  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
    case "app-startup":
      gOS.addObserver(this, "plugins-list-updated", false);
      gOS.addObserver(this, "profile-after-change", false);
      break;
    case "profile-after-change":
      gLoggingEnabled = getPref("getBoolPref", PREF_EM_LOGGING_ENABLED, false);
      var tm = Cc["@mozilla.org/updates/timer-manager;1"].
               getService(Ci.nsIUpdateTimerManager);
      var interval = getPref("getIntPref", PREF_BLOCKLIST_INTERVAL, 86400);
      tm.registerTimer("blocklist-background-update-timer", this, interval);
      break;
    case "plugins-list-updated":
      this._checkPluginsList();
      break;
    case "xpcom-shutdown":
      gOS.removeObserver(this, "xpcom-shutdown");
      gOS.removeObserver(this, "profile-after-change");
      gOS.removeObserver(this, "plugins-list-updated");
      gOS = null;
      gPref = null;
      gConsole = null;
      gVersionChecker = null;
      gApp = null;
      break;
    }
  },

  isAddonBlocklisted: function(id, version, appVersion, toolkitVersion) {
    if (!this._addonEntries)
      this._loadBlocklistFromFile(getFile(KEY_PROFILEDIR, [FILE_BLOCKLIST]));
    if (appVersion === undefined)
      appVersion = gApp.version;
    if (toolkitVersion === undefined)
      toolkitVersion = gApp.platformVersion;

    var blItem = this._addonEntries[id];
    if (!blItem)
      return false;

    for (var i = 0; i < blItem.length; ++i) {
      if (gVersionChecker.compare(version, blItem[i].minVersion) < 0  ||
          gVersionChecker.compare(version, blItem[i].maxVersion) > 0)
        continue;

      var blTargetApp = blItem[i].targetApps[gApp.ID];
      if (blTargetApp) {
        for (var x = 0; x < blTargetApp.length; ++x) {
          if (gVersionChecker.compare(appVersion, blTargetApp[x].minVersion) < 0 ||
              gVersionChecker.compare(appVersion, blTargetApp[x].maxVersion) > 0)
            continue;
          return true;
        }
      }

      blTargetApp = blItem[i].targetApps[TOOLKIT_ID];
      if (!blTargetApp)
        return false;
      for (x = 0; x < blTargetApp.length; ++x) {
        if (gVersionChecker.compare(toolkitVersion, blTargetApp[x].minVersion) < 0 ||
            gVersionChecker.compare(toolkitVersion, blTargetApp[x].maxVersion) > 0)
          continue;
        return true;
      }
    }
    return false;
  },

  notify: function(aTimer) {
    if (getPref("getBoolPref", PREF_BLOCKLIST_ENABLED, true) == false)
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
    this._loadBlocklistFromFile(getFile(KEY_PROFILEDIR, [FILE_BLOCKLIST]));
    var em = Cc["@mozilla.org/extensions/manager;1"].
             getService(Ci.nsIExtensionManager);
    em.checkForBlocklistChanges();
    this._checkPluginsList();
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

  



















































  _loadBlocklistFromFile: function(file) {
    this._addonEntries = { };
    this._pluginEntries = { };
    if (getPref("getBoolPref", PREF_BLOCKLIST_ENABLED, true) == false) {
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
      var emItemsElement = deChildNodes[i];
      if (emItemsElement.nodeType == kELEMENT_NODE &&
          emItemsElement.localName == containerName) {
        itemNodes = emItemsElement.childNodes;
        break;
      }
    }
    if (!itemNodes)
      return result;

    var itemName = prefix + "Item";
    for (var i = 0; i < itemNodes.length; ++i) {
      var blocklistElement = itemNodes[i];
      if (blocklistElement.nodeType != kELEMENT_NODE ||
          blocklistElement.localName != itemName)
        continue;

      blocklistElement.QueryInterface(Ci.nsIDOMElement);
      handler(blocklistElement, result);
    }
    return result;
  },

  _handleEmItemNode: function(blocklistElement, result) {
    var versionNodes = blocklistElement.childNodes;
    var id = blocklistElement.getAttribute("id");
    result[id] = [];
    for (var x = 0; x < versionNodes.length; ++x) {
      var versionRangeElement = versionNodes[x];
      if (versionRangeElement.nodeType != kELEMENT_NODE ||
          versionRangeElement.localName != "versionRange")
        continue;

      result[id].push(new BlocklistItemData(versionRangeElement));
    }
    
    
    if (result[id].length == 0)
      result[id].push(new BlocklistItemData(null));
  },

  _handlePluginItemNode: function(blocklistElement, result) {
    var matchNodes = blocklistElement.childNodes;
    var matchList;
    for (var x = 0; x < matchNodes.length; ++x) {
      var matchElement = matchNodes[x];
      if (matchElement.nodeType != kELEMENT_NODE ||
          matchElement.localName != "match")
        continue;

      var name = matchElement.getAttribute("name");
      var exp = matchElement.getAttribute("exp");
      if (!matchList)
        matchList = { };
      matchList[name] = new RegExp(exp, "m");
    }
    if (matchList)
      result.push(matchList);
  },

  _checkPlugin: function(plugin) {
    for each (var matchList in this._pluginEntries) {
      var matchFailed = false;
      for (var name in matchList) {
        if (typeof(plugin[name]) != "string" ||
            !matchList[name].test(plugin[name])) {
          matchFailed = true;
          break;
        }
      }

      if (!matchFailed) {
        plugin.blocklisted = true;
        return;
      }
    }
    plugin.blocklisted = false;
  },

  _checkPluginsList: function() {
    if (!this._addonEntries)
      this._loadBlocklistFromFile(getFile(KEY_PROFILEDIR, [FILE_BLOCKLIST]));
    var phs = Cc["@mozilla.org/plugin/host;1"].
              getService(Ci.nsIPluginHost);
    phs.getPluginTags({ }).forEach(this._checkPlugin, this);
  },

  QueryInterface: function(aIID) {
    if (!aIID.equals(Ci.nsIObserver) &&
        !aIID.equals(Ci.nsIBlocklistService) &&
        !aIID.equals(Ci.nsITimerCallback) &&
        !aIID.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};




function BlocklistItemData(versionRangeElement) {
  var versionRange = this.getBlocklistVersionRange(versionRangeElement);
  this.minVersion = versionRange.minVersion;
  this.maxVersion = versionRange.maxVersion;
  this.targetApps = { };
  var found = false;

  if (versionRangeElement) {
    for (var i = 0; i < versionRangeElement.childNodes.length; ++i) {
      var targetAppElement = versionRangeElement.childNodes[i];
      if (targetAppElement.nodeType != Ci.nsIDOMNode.ELEMENT_NODE ||
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









  getBlocklistAppVersions: function(targetAppElement) {
    var appVersions = [ ];
    var found = false;

    if (targetAppElement) {
      for (var i = 0; i < targetAppElement.childNodes.length; ++i) {
        var versionRangeElement = targetAppElement.childNodes[i];
        if (versionRangeElement.nodeType != Ci.nsIDOMNode.ELEMENT_NODE ||
            versionRangeElement.localName != "versionRange")
          continue;
        found = true;
        appVersions.push(this.getBlocklistVersionRange(versionRangeElement));
      }
    }
    
    if (!found)
      return [ this.getBlocklistVersionRange(null) ];
    return appVersions;
  },










  getBlocklistVersionRange: function(versionRangeElement) {
    var minVersion = "0";
    var maxVersion = "*";
    if (!versionRangeElement)
      return { minVersion: minVersion, maxVersion: maxVersion };

    if (versionRangeElement.hasAttribute("minVersion"))
      minVersion = versionRangeElement.getAttribute("minVersion");
    if (versionRangeElement.hasAttribute("maxVersion"))
      maxVersion = versionRangeElement.getAttribute("maxVersion");

    return { minVersion: minVersion, maxVersion: maxVersion };
  }
};

const BlocklistFactory = {
  createInstance: function(aOuter, aIID) {
    if (aOuter != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;

    return (new Blocklist()).QueryInterface(aIID);
  }
};

const gModule = {
  registerSelf: function(aCompMgr, aFileSpec, aLocation, aType) {
    aCompMgr.QueryInterface(Ci.nsIComponentRegistrar);
    aCompMgr.registerFactoryLocation(CID, CLASS_NAME, CONTRACT_ID,
                                     aFileSpec, aLocation, aType);

    var catMan = Cc["@mozilla.org/categorymanager;1"].
                 getService(Ci.nsICategoryManager);
    catMan.addCategoryEntry("app-startup", CLASS_NAME, "service," + CONTRACT_ID, true, true);
  },

  unregisterSelf: function(aCompMgr, aLocation, aType) {
    aCompMgr.QueryInterface(Ci.nsIComponentRegistrar);
    aCompMgr.unregisterFactoryLocation(CID, aLocation);

    var catMan = Cc["@mozilla.org/categorymanager;1"].
                 getService(Ci.nsICategoryManager);
    catMan.deleteCategoryEntry("app-startup", "service," + CONTRACT_ID, true);
  },

  getClassObject: function(aCompMgr, aCID, aIID) {
    if (aCID.equals(CID))
      return BlocklistFactory;

    throw Cr.NS_ERROR_NOT_REGISTERED;
  },

  canUnload: function(aCompMgr) {
    return true;
  }
};

function NSGetModule(aCompMgr, aFileSpec) {
  return gModule;
}
