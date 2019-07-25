












































"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;

var EXPORTED_SYMBOLS = [ "AddonUpdateChecker" ];

const TIMEOUT               = 2 * 60 * 1000;
const PREFIX_NS_RDF         = "http://www.w3.org/1999/02/22-rdf-syntax-ns#";
const PREFIX_NS_EM          = "http://www.mozilla.org/2004/em-rdf#";
const PREFIX_ITEM           = "urn:mozilla:item:";
const PREFIX_EXTENSION      = "urn:mozilla:extension:";
const PREFIX_THEME          = "urn:mozilla:theme:";
const TOOLKIT_ID            = "toolkit@mozilla.org"
const XMLURI_PARSE_ERROR    = "http://www.mozilla.org/newlayout/xml/parsererror.xml"

const PREF_UPDATE_REQUIREBUILTINCERTS = "extensions.update.requireBuiltInCerts";

Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/AddonRepository.jsm");

Components.utils.import("resource://gre/modules/CertUtils.jsm");

var gRDF = Cc["@mozilla.org/rdf/rdf-service;1"].
           getService(Ci.nsIRDFService);

["LOG", "WARN", "ERROR"].forEach(function(aName) {
  this.__defineGetter__(aName, function() {
    Components.utils.import("resource://gre/modules/AddonLogging.jsm");

    LogManager.getLogger("addons.updates", this);
    return this[aName];
  });
}, this);









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
          throw new Error("Cannot serialize unknown literal type");
        }
      }
    }
    items.sort();
    result += items.join("");
    return result;
  },

  














  serializeResource: function RDFS_serializeResource(aDs, aResource, aIndent) {
    if (this.resources.indexOf(aResource) != -1 ) {
      
      throw new Error("Cannot serialize multiple references to " + aResource.Value);
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















function parseRDFManifest(aId, aType, aUpdateKey, aRequest) {
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

  function getRequiredProperty(aDs, aSource, aProperty) {
    let value = getProperty(aDs, aSource, aProperty);
    if (!value)
      throw new Error("Update manifest is missing a required " + aProperty + " property.");
    return value;
  }

  let rdfParser = Cc["@mozilla.org/rdf/xml-parser;1"].
                  createInstance(Ci.nsIRDFXMLParser);
  let ds = Cc["@mozilla.org/rdf/datasource;1?name=in-memory-datasource"].
           createInstance(Ci.nsIRDFDataSource);
  rdfParser.parseString(ds, aRequest.channel.URI, aRequest.responseText);

  switch (aType) {
  case "extension":
    var item = PREFIX_EXTENSION + aId;
    break;
  case "theme":
    item = PREFIX_THEME + aId;
    break;
  default:
    item = PREFIX_ITEM + aId;
    break;
  }

  let extensionRes  = gRDF.GetResource(item);

  
  if (aUpdateKey) {
    let signature = getProperty(ds, extensionRes, "signature");
    if (!signature)
      throw new Error("Update manifest for " + aId + " does not contain a required signature");
    let serializer = new RDFSerializer();
    let updateString = null;

    try {
      updateString = serializer.serializeResource(ds, extensionRes);
    }
    catch (e) {
      throw new Error("Failed to generate signed string for " + aId + ". Serializer threw " + e);
    }

    let result = false;

    try {
      let verifier = Cc["@mozilla.org/security/datasignatureverifier;1"].
                     getService(Ci.nsIDataSignatureVerifier);
      result = verifier.verifyData(updateString, signature, aUpdateKey);
    }
    catch (e) {
      throw new Error("The signature or updateKey for " + aId + " is malformed");
    }

    if (!result)
      throw new Error("The signature for " + aId + " was not created by the add-on's updateKey");
  }

  let updates = ds.GetTarget(extensionRes, EM_R("updates"), true);

  
  
  if (!updates) {
    WARN("Update manifest for " + aId + " did not contain an updates property");
    return [];
  }

  if (!(updates instanceof Ci.nsIRDFResource))
    throw new Error("Missing updates property for " + extensionRes.Value);

  let cu = Cc["@mozilla.org/rdf/container-utils;1"].
           getService(Ci.nsIRDFContainerUtils);
  if (!cu.IsContainer(ds, updates))
    throw new Error("Updates property was not an RDF container");

  let checkSecurity = true;

  try {
    checkSecurity = Services.prefs.getBoolPref("extensions.checkUpdateSecurity");
  }
  catch (e) {
  }

  let results = [];
  let ctr = Cc["@mozilla.org/rdf/container;1"].
            createInstance(Ci.nsIRDFContainer);
  ctr.Init(ds, updates);
  let items = ctr.GetElements();
  while (items.hasMoreElements()) {
    let item = items.getNext().QueryInterface(Ci.nsIRDFResource);
    let version = getProperty(ds, item, "version");
    if (!version) {
      WARN("Update manifest is missing a required version property.");
      continue;
    }

    LOG("Found an update entry for " + aId + " version " + version);

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
        WARN(e);
        continue;
      }

      let result = {
        id: aId,
        version: version,
        updateURL: getProperty(ds, targetApp, "updateLink"),
        updateHash: getProperty(ds, targetApp, "updateHash"),
        updateInfoURL: getProperty(ds, targetApp, "updateInfoURL"),
        targetApplications: [appEntry]
      };

      if (result.updateURL && checkSecurity &&
          result.updateURL.substring(0, 6) != "https:" &&
          (!result.updateHash || result.updateHash.substring(0, 3) != "sha")) {
        WARN("updateLink " + result.updateURL + " is not secure and is not verified" +
             " by a strong enough hash (needs to be sha1 or stronger).");
        delete result.updateURL;
        delete result.updateHash;
      }
      results.push(result);
    }
  }
  return results;
}
















function UpdateParser(aId, aType, aUpdateKey, aUrl, aObserver) {
  this.id = aId;
  this.type = aType;
  this.updateKey = aUpdateKey;
  this.observer = aObserver;

  this.timer = Cc["@mozilla.org/timer;1"].
               createInstance(Ci.nsITimer);
  this.timer.initWithCallback(this, TIMEOUT, Ci.nsITimer.TYPE_ONE_SHOT);

  let requireBuiltIn = true;
  try {
    requireBuiltIn = Services.prefs.getBoolPref(PREF_UPDATE_REQUIREBUILTINCERTS);
  }
  catch (e) {
  }

  LOG("Requesting " + aUrl);
  try {
    this.request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
                   createInstance(Ci.nsIXMLHttpRequest);
    this.request.open("GET", aUrl, true);
    this.request.channel.notificationCallbacks = new BadCertHandler(!requireBuiltIn);
    this.request.channel.loadFlags |= Ci.nsIRequest.LOAD_BYPASS_CACHE;
    this.request.overrideMimeType("text/xml");
    var self = this;
    this.request.addEventListener("load", function(event) { self.onLoad() }, false);
    this.request.addEventListener("error", function(event) { self.onError() }, false);
    this.request.send(null);
  }
  catch (e) {
    ERROR("Failed to request update manifest", e);
  }
}

UpdateParser.prototype = {
  id: null,
  type: null,
  updateKey: null,
  observer: null,
  request: null,
  timer: null,

  


  onLoad: function UP_onLoad() {
    this.timer.cancel();
    this.timer = null;
    let request = this.request;
    this.request = null;

    let requireBuiltIn = true;
    try {
      requireBuiltIn = Services.prefs.getBoolPref(PREF_UPDATE_REQUIREBUILTINCERTS);
    }
    catch (e) {
    }

    try {
      checkCert(request.channel, !requireBuiltIn);
    }
    catch (e) {
      this.notifyError(AddonUpdateChecker.ERROR_DOWNLOAD_ERROR);
      return;
    }

    if (!Components.isSuccessCode(request.status)) {
      WARN("Request failed: " + request.status);
      this.notifyError(AddonUpdateChecker.ERROR_DOWNLOAD_ERROR);
      return;
    }

    let channel = request.channel;
    if (channel instanceof Ci.nsIHttpChannel && !channel.requestSucceeded) {
      WARN("Request failed: " + channel.responseStatus + ": " + channel.responseStatusText);
      this.notifyError(AddonUpdateChecker.ERROR_DOWNLOAD_ERROR);
      return;
    }

    let xml = request.responseXML;
    if (!xml || xml.documentElement.namespaceURI == XMLURI_PARSE_ERROR) {
      WARN("Update manifest was not valid XML");
      this.notifyError(AddonUpdateChecker.ERROR_PARSE_ERROR);
      return;
    }

    
    if (xml.documentElement.namespaceURI == PREFIX_NS_RDF) {
      let results = null;

      try {
        results = parseRDFManifest(this.id, this.type, this.updateKey, request);
      }
      catch (e) {
        WARN(e);
        this.notifyError(AddonUpdateChecker.ERROR_PARSE_ERROR);
        return;
      }
      if ("onUpdateCheckComplete" in this.observer)
        this.observer.onUpdateCheckComplete(results);
      return;
    }

    WARN("Update manifest had an unrecognised namespace: " + xml.documentElement.namespaceURI);
    this.notifyError(AddonUpdateChecker.ERROR_UNKNOWN_FORMAT);
  },

  


  onError: function UP_onError() {
    this.timer.cancel();
    this.timer = null;

    if (!Components.isSuccessCode(this.request.status)) {
      WARN("Request failed: " + this.request.status);
    }
    else if (this.request.channel instanceof Ci.nsIHttpChannel) {
      try {
        if (this.request.channel.requestSucceeded) {
          WARN("Request failed: " + this.request.channel.responseStatus + ": " +
               this.request.channel.responseStatusText);
        }
      }
      catch (e) {
        WARN("HTTP Request failed for an unknown reason");
      }
    }
    else {
      WARN("Request failed for an unknown reason");
    }

    this.request = null;

    this.notifyError(AddonUpdateChecker.ERROR_DOWNLOAD_ERROR);
  },

  


  notifyError: function UP_notifyError(aStatus) {
    if ("onUpdateCheckError" in this.observer)
      this.observer.onUpdateCheckError(aStatus);
  },

  


  notify: function UP_notify(aTimer) {
    this.timer = null;
    this.request.abort();
    this.request = null;

    WARN("Request timed out");

    this.notifyError(AddonUpdateChecker.ERROR_TIMEOUT);
  }
};
















function matchesVersions(aUpdate, aAppVersion, aPlatformVersion,
                         aIgnoreMaxVersion, aCompatOverrides) {
  if (aCompatOverrides) {
    let override = AddonRepository.findMatchingCompatOverride(aUpdate.version,
                                                              aCompatOverrides,
                                                              aAppVersion,
                                                              aPlatformVersion);
    if (override && override.type == "incompatible")
      return false;
  }

  let result = false;
  for (let i = 0; i < aUpdate.targetApplications.length; i++) {
    let app = aUpdate.targetApplications[i];
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

var AddonUpdateChecker = {
  
  
  ERROR_TIMEOUT: -1,
  
  ERROR_DOWNLOAD_ERROR: -2,
  
  ERROR_PARSE_ERROR: -3,
  
  ERROR_UNKNOWN_FORMAT: -4,
  
  ERROR_SECURITY_ERROR: -5,

  


















  getCompatibilityUpdate: function AUC_getCompatibilityUpdate(aUpdates, aVersion,
                                                              aIgnoreCompatibility,
                                                              aAppVersion,
                                                              aPlatformVersion,
                                                              aIgnoreMaxVersion) {
    if (!aAppVersion)
      aAppVersion = Services.appinfo.version;
    if (!aPlatformVersion)
      aPlatformVersion = Services.appinfo.platformVersion;

    for (let i = 0; i < aUpdates.length; i++) {
      if (Services.vc.compare(aUpdates[i].version, aVersion) == 0) {
        if (aIgnoreCompatibility) {
          for (let j = 0; j < aUpdates[i].targetApplications.length; j++) {
            let id = aUpdates[i].targetApplications[j].id;
            if (id == Services.appinfo.ID || id == TOOLKIT_ID)
              return aUpdates[i];
          }
        }
        else if (matchesVersions(aUpdates[i], aAppVersion, aPlatformVersion,
                                 aIgnoreMaxVersion)) {
          return aUpdates[i];
        }
      }
    }
    return null;
  },

  














  getNewestCompatibleUpdate: function AUC_getNewestCompatibleUpdate(aUpdates,
                                                                    aAppVersion,
                                                                    aPlatformVersion,
                                                                    aIgnoreMaxVersion,
                                                                    aCompatOverrides) {
    if (!aAppVersion)
      aAppVersion = Services.appinfo.version;
    if (!aPlatformVersion)
      aPlatformVersion = Services.appinfo.platformVersion;

    let blocklist = Cc["@mozilla.org/extensions/blocklist;1"].
                    getService(Ci.nsIBlocklistService);

    let newest = null;
    for (let i = 0; i < aUpdates.length; i++) {
      if (!aUpdates[i].updateURL)
        continue;
      let state = blocklist.getAddonBlocklistState(aUpdates[i].id, aUpdates[i].version,
                                                   aAppVersion, aPlatformVersion);
      if (state != Ci.nsIBlocklistService.STATE_NOT_BLOCKED)
        continue;
      if ((newest == null || (Services.vc.compare(newest.version, aUpdates[i].version) < 0)) &&
          matchesVersions(aUpdates[i], aAppVersion, aPlatformVersion,
                          aIgnoreMaxVersion, aCompatOverrides)) {
        newest = aUpdates[i];
      }
    }
    return newest;
  },

  













  checkForUpdates: function AUC_checkForUpdates(aId, aType, aUpdateKey, aUrl,
                                                aObserver) {
    new UpdateParser(aId, aType, aUpdateKey, aUrl, aObserver);
  }
};
