








"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

this.EXPORTED_SYMBOLS = [ "AddonUpdateChecker" ];

const TIMEOUT               = 60 * 1000;
const PREFIX_NS_RDF         = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
const PREFIX_NS_EM          = "http://www.mozilla.org/2004/em-rdf#";
const PREFIX_ITEM           = "urn:mozilla:item:";
const PREFIX_EXTENSION      = "urn:mozilla:extension:";
const PREFIX_THEME          = "urn:mozilla:theme:";
const TOOLKIT_ID            = "toolkit@mozilla.org"
const XMLURI_PARSE_ERROR    = "http://www.mozilla.org/newlayout/xml/parsererror.xml"

const PREF_UPDATE_REQUIREBUILTINCERTS = "extensions.update.requireBuiltInCerts";

Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AddonManager",
                                  "resource://gre/modules/AddonManager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AddonRepository",
                                  "resource://gre/modules/addons/AddonRepository.jsm");


XPCOMUtils.defineLazyGetter(this, "CertUtils", function certUtilsLazyGetter() {
  let certUtils = {};
  Components.utils.import("resource://gre/modules/CertUtils.jsm", certUtils);
  return certUtils;
});

var gRDF = Cc["@mozilla.org/rdf/rdf-service;1"].
           getService(Ci.nsIRDFService);

Cu.import("resource://gre/modules/Log.jsm");
const LOGGER_ID = "addons.update-checker";



let logger = Log.repository.getLogger(LOGGER_ID);









function RDFSerializer() {
  this.cUtils = Cc["@mozilla.org/rdf/container-utils;1"].
                getService(Ci.nsIRDFContainerUtils);
  this.resources = [];
}

RDFSerializer.prototype = {
  INDENT: "  ",      
  resources: null,   

  







  escapeEntities: function RDFS_escapeEntities(aString) {
    aString = aString.replace(/&/g, "&amp;");
    aString = aString.replace(/</g, "&lt;");
    aString = aString.replace(/>/g, "&gt;");
    return aString.replace(/"/g, "&quot;");
  },

  










  serializeContainerItems: function RDFS_serializeContainerItems(aDs, aContainer,
                                                                 aIndent) {
    var result = "";
    var items = aContainer.GetElements();
    while (items.hasMoreElements()) {
      var item = items.getNext().QueryInterface(Ci.nsIRDFResource);
      result += aIndent + "<RDF:li>\n"
      result += this.serializeResource(aDs, item, aIndent + this.INDENT);
      result += aIndent + "</RDF:li>\n"
    }
    return result;
  },

  













  serializeResourceProperties: function RDFS_serializeResourceProperties(aDs,
                                                                         aResource,
                                                                         aIndent) {
    var result = "";
    var items = [];
    var arcs = aDs.ArcLabelsOut(aResource);
    while (arcs.hasMoreElements()) {
      var arc = arcs.getNext().QueryInterface(Ci.nsIRDFResource);
      if (arc.ValueUTF8.substring(0, PREFIX_NS_EM.length) != PREFIX_NS_EM)
        continue;
      var prop = arc.ValueUTF8.substring(PREFIX_NS_EM.length);
      if (prop == "signature")
        continue;

      var targets = aDs.GetTargets(aResource, arc, true);
      while (targets.hasMoreElements()) {
        var target = targets.getNext();
        if (target instanceof Ci.nsIRDFResource) {
          var item = aIndent + "<em:" + prop + ">\n";
          item += this.serializeResource(aDs, target, aIndent + this.INDENT);
          item += aIndent + "</em:" + prop + ">\n";
          items.push(item);
        }
        else if (target instanceof Ci.nsIRDFLiteral) {
          items.push(aIndent + "<em:" + prop + ">" +
                     this.escapeEntities(target.Value) + "</em:" + prop + ">\n");
        }
        else if (target instanceof Ci.nsIRDFInt) {
          items.push(aIndent + "<em:" + prop + " NC:parseType=\"Integer\">" +
                     target.Value + "</em:" + prop + ">\n");
        }
        else {
          throw Components.Exception("Cannot serialize unknown literal type");
        }
      }
    }
    items.sort();
    result += items.join("");
    return result;
  },

  














  serializeResource: function RDFS_serializeResource(aDs, aResource, aIndent) {
    if (this.resources.indexOf(aResource) != -1 ) {
      
      throw Components.Exception("Cannot serialize multiple references to " + aResource.Value);
    }
    if (aIndent === undefined)
      aIndent = "";

    this.resources.push(aResource);
    var container = null;
    var type = "Description";
    if (this.cUtils.IsSeq(aDs, aResource)) {
      type = "Seq";
      container = this.cUtils.MakeSeq(aDs, aResource);
    }
    else if (this.cUtils.IsAlt(aDs, aResource)) {
      type = "Alt";
      container = this.cUtils.MakeAlt(aDs, aResource);
    }
    else if (this.cUtils.IsBag(aDs, aResource)) {
      type = "Bag";
      container = this.cUtils.MakeBag(aDs, aResource);
    }

    var result = aIndent + "<RDF:" + type;
    if (!gRDF.IsAnonymousResource(aResource))
      result += " about=\"" + this.escapeEntities(aResource.ValueUTF8) + "\"";
    result += ">\n";

    if (container)
      result += this.serializeContainerItems(aDs, container, aIndent + this.INDENT);

    result += this.serializeResourceProperties(aDs, aResource, aIndent + this.INDENT);

    result += aIndent + "</RDF:" + type + ">\n";
    return result;
  }
}













function parseRDFManifest(aId, aUpdateKey, aRequest) {
  function EM_R(aProp) {
    return gRDF.GetResource(PREFIX_NS_EM + aProp);
  }

  function getValue(aLiteral) {
    if (aLiteral instanceof Ci.nsIRDFLiteral)
      return aLiteral.Value;
    if (aLiteral instanceof Ci.nsIRDFResource)
      return aLiteral.Value;
    if (aLiteral instanceof Ci.nsIRDFInt)
      return aLiteral.Value;
    return null;
  }

  function getProperty(aDs, aSource, aProperty) {
    return getValue(aDs.GetTarget(aSource, EM_R(aProperty), true));
  }

  function getBooleanProperty(aDs, aSource, aProperty) {
    let propValue = aDs.GetTarget(aSource, EM_R(aProperty), true);
    if (!propValue)
      return undefined;
    return getValue(propValue) == "true";
  }

  function getRequiredProperty(aDs, aSource, aProperty) {
    let value = getProperty(aDs, aSource, aProperty);
    if (!value)
      throw Components.Exception("Update manifest is missing a required " + aProperty + " property.");
    return value;
  }

  let rdfParser = Cc["@mozilla.org/rdf/xml-parser;1"].
                  createInstance(Ci.nsIRDFXMLParser);
  let ds = Cc["@mozilla.org/rdf/datasource;1?name=in-memory-datasource"].
           createInstance(Ci.nsIRDFDataSource);
  rdfParser.parseString(ds, aRequest.channel.URI, aRequest.responseText);

  
  let extensionRes = gRDF.GetResource(PREFIX_EXTENSION + aId);
  let themeRes = gRDF.GetResource(PREFIX_THEME + aId);
  let itemRes = gRDF.GetResource(PREFIX_ITEM + aId);
  let addonRes = ds.ArcLabelsOut(extensionRes).hasMoreElements() ? extensionRes
               : ds.ArcLabelsOut(themeRes).hasMoreElements() ? themeRes
               : itemRes;

  
  if (aUpdateKey) {
    let signature = getProperty(ds, addonRes, "signature");
    if (!signature)
      throw Components.Exception("Update manifest for " + aId + " does not contain a required signature");
    let serializer = new RDFSerializer();
    let updateString = null;

    try {
      updateString = serializer.serializeResource(ds, addonRes);
    }
    catch (e) {
      throw Components.Exception("Failed to generate signed string for " + aId + ". Serializer threw " + e,
                                 e.result);
    }

    let result = false;

    try {
      let verifier = Cc["@mozilla.org/security/datasignatureverifier;1"].
                     getService(Ci.nsIDataSignatureVerifier);
      result = verifier.verifyData(updateString, signature, aUpdateKey);
    }
    catch (e) {
      throw Components.Exception("The signature or updateKey for " + aId + " is malformed." +
                                 "Verifier threw " + e, e.result);
    }

    if (!result)
      throw Components.Exception("The signature for " + aId + " was not created by the add-on's updateKey");
  }

  let updates = ds.GetTarget(addonRes, EM_R("updates"), true);

  
  
  if (!updates) {
    logger.warn("Update manifest for " + aId + " did not contain an updates property");
    return [];
  }

  if (!(updates instanceof Ci.nsIRDFResource))
    throw Components.Exception("Missing updates property for " + addonRes.Value);

  let cu = Cc["@mozilla.org/rdf/container-utils;1"].
           getService(Ci.nsIRDFContainerUtils);
  if (!cu.IsContainer(ds, updates))
    throw Components.Exception("Updates property was not an RDF container");

  let results = [];
  let ctr = Cc["@mozilla.org/rdf/container;1"].
            createInstance(Ci.nsIRDFContainer);
  ctr.Init(ds, updates);
  let items = ctr.GetElements();
  while (items.hasMoreElements()) {
    let item = items.getNext().QueryInterface(Ci.nsIRDFResource);
    let version = getProperty(ds, item, "version");
    if (!version) {
      logger.warn("Update manifest is missing a required version property.");
      continue;
    }

    logger.debug("Found an update entry for " + aId + " version " + version);

    let targetApps = ds.GetTargets(item, EM_R("targetApplication"), true);
    while (targetApps.hasMoreElements()) {
      let targetApp = targetApps.getNext().QueryInterface(Ci.nsIRDFResource);

      let appEntry = {};
      try {
        appEntry.id = getRequiredProperty(ds, targetApp, "id");
        appEntry.minVersion = getRequiredProperty(ds, targetApp, "minVersion");
        appEntry.maxVersion = getRequiredProperty(ds, targetApp, "maxVersion");
      }
      catch (e) {
        logger.warn(e);
        continue;
      }

      let result = {
        id: aId,
        version: version,
        multiprocessCompatible: getBooleanProperty(ds, item, "multiprocessCompatible"),
        updateURL: getProperty(ds, targetApp, "updateLink"),
        updateHash: getProperty(ds, targetApp, "updateHash"),
        updateInfoURL: getProperty(ds, targetApp, "updateInfoURL"),
        strictCompatibility: !!getBooleanProperty(ds, targetApp, "strictCompatibility"),
        targetApplications: [appEntry]
      };

      if (result.updateURL && AddonManager.checkUpdateSecurity &&
          result.updateURL.substring(0, 6) != "https:" &&
          (!result.updateHash || result.updateHash.substring(0, 3) != "sha")) {
        logger.warn("updateLink " + result.updateURL + " is not secure and is not verified" +
             " by a strong enough hash (needs to be sha1 or stronger).");
        delete result.updateURL;
        delete result.updateHash;
      }
      results.push(result);
    }
  }
  return results;
}














function UpdateParser(aId, aUpdateKey, aUrl, aObserver) {
  this.id = aId;
  this.updateKey = aUpdateKey;
  this.observer = aObserver;
  this.url = aUrl;

  let requireBuiltIn = true;
  try {
    requireBuiltIn = Services.prefs.getBoolPref(PREF_UPDATE_REQUIREBUILTINCERTS);
  }
  catch (e) {
  }

  logger.debug("Requesting " + aUrl);
  try {
    this.request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
                   createInstance(Ci.nsIXMLHttpRequest);
    this.request.open("GET", this.url, true);
    this.request.channel.notificationCallbacks = new CertUtils.BadCertHandler(!requireBuiltIn);
    this.request.channel.loadFlags |= Ci.nsIRequest.LOAD_BYPASS_CACHE;
    
    this.request.channel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;
    this.request.overrideMimeType("text/xml");
    this.request.setRequestHeader("Moz-XPI-Update", "1", true);
    this.request.timeout = TIMEOUT;
    var self = this;
    this.request.addEventListener("load", function loadEventListener(event) { self.onLoad() }, false);
    this.request.addEventListener("error", function errorEventListener(event) { self.onError() }, false);
    this.request.addEventListener("timeout", function timeoutEventListener(event) { self.onTimeout() }, false);
    this.request.send(null);
  }
  catch (e) {
    logger.error("Failed to request update manifest", e);
  }
}

UpdateParser.prototype = {
  id: null,
  updateKey: null,
  observer: null,
  request: null,
  url: null,

  


  onLoad: function UP_onLoad() {
    let request = this.request;
    this.request = null;
    this._doneAt = new Error("place holder");

    let requireBuiltIn = true;
    try {
      requireBuiltIn = Services.prefs.getBoolPref(PREF_UPDATE_REQUIREBUILTINCERTS);
    }
    catch (e) {
    }

    try {
      CertUtils.checkCert(request.channel, !requireBuiltIn);
    }
    catch (e) {
      logger.warn("Request failed: " + this.url + " - " + e);
      this.notifyError(AddonUpdateChecker.ERROR_DOWNLOAD_ERROR);
      return;
    }

    if (!Components.isSuccessCode(request.status)) {
      logger.warn("Request failed: " + this.url + " - " + request.status);
      this.notifyError(AddonUpdateChecker.ERROR_DOWNLOAD_ERROR);
      return;
    }

    let channel = request.channel;
    if (channel instanceof Ci.nsIHttpChannel && !channel.requestSucceeded) {
      logger.warn("Request failed: " + this.url + " - " + channel.responseStatus +
           ": " + channel.responseStatusText);
      this.notifyError(AddonUpdateChecker.ERROR_DOWNLOAD_ERROR);
      return;
    }

    let xml = request.responseXML;
    if (!xml || xml.documentElement.namespaceURI == XMLURI_PARSE_ERROR) {
      logger.warn("Update manifest was not valid XML");
      this.notifyError(AddonUpdateChecker.ERROR_PARSE_ERROR);
      return;
    }

    
    if (xml.documentElement.namespaceURI == PREFIX_NS_RDF) {
      let results = null;

      try {
        results = parseRDFManifest(this.id, this.updateKey, request);
      }
      catch (e) {
        logger.warn("onUpdateCheckComplete failed to parse RDF manifest", e);
        this.notifyError(AddonUpdateChecker.ERROR_PARSE_ERROR);
        return;
      }
      if ("onUpdateCheckComplete" in this.observer) {
        try {
          this.observer.onUpdateCheckComplete(results);
        }
        catch (e) {
          logger.warn("onUpdateCheckComplete notification failed", e);
        }
      }
      else {
        logger.warn("onUpdateCheckComplete may not properly cancel", new Error("stack marker"));
      }
      return;
    }

    logger.warn("Update manifest had an unrecognised namespace: " + xml.documentElement.namespaceURI);
    this.notifyError(AddonUpdateChecker.ERROR_UNKNOWN_FORMAT);
  },

  


  onTimeout: function() {
    this.request = null;
    this._doneAt = new Error("Timed out");
    logger.warn("Request for " + this.url + " timed out");
    this.notifyError(AddonUpdateChecker.ERROR_TIMEOUT);
  },

  


  onError: function UP_onError() {
    if (!Components.isSuccessCode(this.request.status)) {
      logger.warn("Request failed: " + this.url + " - " + this.request.status);
    }
    else if (this.request.channel instanceof Ci.nsIHttpChannel) {
      try {
        if (this.request.channel.requestSucceeded) {
          logger.warn("Request failed: " + this.url + " - " +
               this.request.channel.responseStatus + ": " +
               this.request.channel.responseStatusText);
        }
      }
      catch (e) {
        logger.warn("HTTP Request failed for an unknown reason");
      }
    }
    else {
      logger.warn("Request failed for an unknown reason");
    }

    this.request = null;
    this._doneAt = new Error("UP_onError");

    this.notifyError(AddonUpdateChecker.ERROR_DOWNLOAD_ERROR);
  },

  


  notifyError: function UP_notifyError(aStatus) {
    if ("onUpdateCheckError" in this.observer) {
      try {
        this.observer.onUpdateCheckError(aStatus);
      }
      catch (e) {
        logger.warn("onUpdateCheckError notification failed", e);
      }
    }
  },

  


  cancel: function UP_cancel() {
    if (!this.request) {
      logger.error("Trying to cancel already-complete request", this._doneAt);
      return;
    }
    this.request.abort();
    this.request = null;
    this._doneAt = new Error("UP_cancel");
    this.notifyError(AddonUpdateChecker.ERROR_CANCELLED);
  }
};


















function matchesVersions(aUpdate, aAppVersion, aPlatformVersion,
                         aIgnoreMaxVersion, aIgnoreStrictCompat,
                         aCompatOverrides) {
  if (aCompatOverrides) {
    let override = AddonRepository.findMatchingCompatOverride(aUpdate.version,
                                                              aCompatOverrides,
                                                              aAppVersion,
                                                              aPlatformVersion);
    if (override && override.type == "incompatible")
      return false;
  }

  if (aUpdate.strictCompatibility && !aIgnoreStrictCompat)
    aIgnoreMaxVersion = false;

  let result = false;
  for (let app of aUpdate.targetApplications) {
    if (app.id == Services.appinfo.ID) {
      return (Services.vc.compare(aAppVersion, app.minVersion) >= 0) &&
             (aIgnoreMaxVersion || (Services.vc.compare(aAppVersion, app.maxVersion) <= 0));
    }
    if (app.id == TOOLKIT_ID) {
      result = (Services.vc.compare(aPlatformVersion, app.minVersion) >= 0) &&
               (aIgnoreMaxVersion || (Services.vc.compare(aPlatformVersion, app.maxVersion) <= 0));
    }
  }
  return result;
}

this.AddonUpdateChecker = {
  
  
  ERROR_TIMEOUT: -1,
  
  ERROR_DOWNLOAD_ERROR: -2,
  
  ERROR_PARSE_ERROR: -3,
  
  ERROR_UNKNOWN_FORMAT: -4,
  
  ERROR_SECURITY_ERROR: -5,
  
  ERROR_CANCELLED: -6,

  




















  getCompatibilityUpdate: function AUC_getCompatibilityUpdate(aUpdates, aVersion,
                                                              aIgnoreCompatibility,
                                                              aAppVersion,
                                                              aPlatformVersion,
                                                              aIgnoreMaxVersion,
                                                              aIgnoreStrictCompat) {
    if (!aAppVersion)
      aAppVersion = Services.appinfo.version;
    if (!aPlatformVersion)
      aPlatformVersion = Services.appinfo.platformVersion;

    for (let update of aUpdates) {
      if (Services.vc.compare(update.version, aVersion) == 0) {
        if (aIgnoreCompatibility) {
          for (let targetApp of update.targetApplications) {
            let id = targetApp.id;
            if (id == Services.appinfo.ID || id == TOOLKIT_ID)
              return update;
          }
        }
        else if (matchesVersions(update, aAppVersion, aPlatformVersion,
                                 aIgnoreMaxVersion, aIgnoreStrictCompat)) {
          return update;
        }
      }
    }
    return null;
  },

  
















  getNewestCompatibleUpdate: function AUC_getNewestCompatibleUpdate(aUpdates,
                                                                    aAppVersion,
                                                                    aPlatformVersion,
                                                                    aIgnoreMaxVersion,
                                                                    aIgnoreStrictCompat,
                                                                    aCompatOverrides) {
    if (!aAppVersion)
      aAppVersion = Services.appinfo.version;
    if (!aPlatformVersion)
      aPlatformVersion = Services.appinfo.platformVersion;

    let blocklist = Cc["@mozilla.org/extensions/blocklist;1"].
                    getService(Ci.nsIBlocklistService);

    let newest = null;
    for (let update of aUpdates) {
      if (!update.updateURL)
        continue;
      let state = blocklist.getAddonBlocklistState(update, aAppVersion, aPlatformVersion);
      if (state != Ci.nsIBlocklistService.STATE_NOT_BLOCKED)
        continue;
      if ((newest == null || (Services.vc.compare(newest.version, update.version) < 0)) &&
          matchesVersions(update, aAppVersion, aPlatformVersion,
                          aIgnoreMaxVersion, aIgnoreStrictCompat,
                          aCompatOverrides)) {
        newest = update;
      }
    }
    return newest;
  },

  













  checkForUpdates: function AUC_checkForUpdates(aId, aUpdateKey, aUrl,
                                                aObserver) {
    return new UpdateParser(aId, aUpdateKey, aUrl, aObserver);
  }
};
