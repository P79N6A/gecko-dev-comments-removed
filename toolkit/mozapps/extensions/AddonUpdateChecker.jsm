











































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

Components.utils.import("resource://gre/modules/Services.jsm");

Components.utils.import("resource://gre/modules/CertUtils.jsm");

var gRDF = Cc["@mozilla.org/rdf/rdf-service;1"].
           getService(Ci.nsIRDFService);







function LOG(str) {
  dump("*** addons.updates: " + str + "\n");
}







function WARN(str) {
  LOG(str);
}







function ERROR(str) {
  LOG(str);
}









function RDFSerializer() {
  this.cUtils = Cc["@mozilla.org/rdf/container-utils;1"].
                getService(Ci.nsIRDFContainerUtils);
  this.resources = [];
}

RDFSerializer.prototype = {
  INDENT: "  ",      
  resources: null,   

  







  escapeEntities: function RDFS_escapeEntities(string) {
    string = string.replace(/&/g, "&amp;");
    string = string.replace(/</g, "&lt;");
    string = string.replace(/>/g, "&gt;");
    return string.replace(/"/g, "&quot;");
  },

  










  serializeContainerItems: function RDFS_serializeContainerItems(ds, container, indent) {
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

  













  serializeResourceProperties: function RDFS_serializeResourceProperties(ds, resource, indent) {
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

  














  serializeResource: function RDFS_serializeResource(ds, resource, indent) {
    if (this.resources.indexOf(resource) != -1 ) {
      
      throw new Error("Cannot serialize multiple references to " + resource.Value);
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














function parseRDFManifest(id, type, updateKey, request) {
  function EM_R(prop) {
    return gRDF.GetResource(PREFIX_NS_EM + prop);
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

  function getRequiredProperty(ds, source, property) {
    let value = getProperty(ds, source, property);
    if (!value)
      throw new Error("Missing required property " + property);
    return value;
  }

  let rdfParser = Cc["@mozilla.org/rdf/xml-parser;1"].
                  createInstance(Ci.nsIRDFXMLParser);
  let ds = Cc["@mozilla.org/rdf/datasource;1?name=in-memory-datasource"].
           createInstance(Ci.nsIRDFDataSource);
  rdfParser.parseString(ds, request.channel.URI, request.responseText);

  switch (type) {
  case "extension":
    var item = PREFIX_EXTENSION + id;
    break;
  case "theme":
    item = PREFIX_THEME + id;
    break;
  default:
    item = PREFIX_ITEM + id;
    break;
  }

  let extensionRes  = gRDF.GetResource(item);

  
  if (updateKey) {
    let signature = getProperty(ds, extensionRes, "signature");
    if (!signature)
      throw new Error("Update manifest for " + id + " does not contain a required signature");
    let serializer = new RDFSerializer();
    let updateString = null;

    try {
      updateString = serializer.serializeResource(ds, extensionRes);
    }
    catch (e) {
      throw new Error("Failed to generate signed string for " + id + ". Serializer threw " + e);
    }

    let result = false;

    try {
      let verifier = Cc["@mozilla.org/security/datasignatureverifier;1"].
                     getService(Ci.nsIDataSignatureVerifier);
      result = verifier.verifyData(updateString, signature, updateKey);
    }
    catch (e) {
      throw new Error("The signature or updateKey for " + id + " is malformed");
    }

    if (!result)
      throw new Error("The signature for " + id + " was not created by the add-on's updateKey");
  }

  let updates = ds.GetTarget(extensionRes, EM_R("updates"), true);

  if (!updates || !(updates instanceof Ci.nsIRDFResource))
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
    let version = getRequiredProperty(ds, item, "version");
    LOG("Found an update entry for " + id + " version " + version);

    let targetApps = ds.GetTargets(item, EM_R("targetApplication"), true);
    while (targetApps.hasMoreElements()) {
      let targetApp = targetApps.getNext().QueryInterface(Ci.nsIRDFResource);

      let result = {
        version: version,
        updateURL: getProperty(ds, targetApp, "updateLink"),
        updateHash: getProperty(ds, targetApp, "updateHash"),
        updateInfoURL: getProperty(ds, targetApp, "updateInfoURL"),
        targetApplications: [{
          id: getRequiredProperty(ds, targetApp, "id"),
          minVersion: getRequiredProperty(ds, targetApp, "minVersion"),
          maxVersion: getRequiredProperty(ds, targetApp, "maxVersion"),
        }]
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
















function UpdateParser(id, type, updateKey, url, observer) {
  this.id = id;
  this.type = type;
  this.updateKey = updateKey;
  this.observer = observer;

  this.timer = Cc["@mozilla.org/timer;1"].
               createInstance(Ci.nsITimer);
  this.timer.initWithCallback(this, TIMEOUT, Ci.nsITimer.TYPE_ONE_SHOT);

  LOG("Requesting " + url);
  this.request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
                 createInstance(Ci.nsIXMLHttpRequest);
  this.request.open("GET", url, true);
  this.request.channel.notificationCallbacks = new BadCertHandler();
  this.request.channel.loadFlags |= Ci.nsIRequest.LOAD_BYPASS_CACHE;
  this.request.overrideMimeType("text/xml");
  var self = this;
  this.request.onload = function(event) { self.onLoad() };
  this.request.onerror = function(event) { self.onError() };
  this.request.send(null);
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

    try {
      checkCert(request.channel);
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

    WARN("Request failed: " + this.request.status);
    this.request = null;

    this.notifyError(AddonUpdateChecker.ERROR_DOWNLOAD_ERROR);
  },

  


  notifyError: function UP_notifyError(status) {
    if ("onUpdateCheckError" in this.observer)
      this.observer.onUpdateCheckError(status);
  },

  


  notify: function UP_notify(timer) {
    this.timer = null;
    this.request.abort();
    this.request = null;

    WARN("Request timed out");

    this.notifyError(AddonUpdateChecker.ERROR_TIMEOUT);
  }
};












function matchesVersions(update, appVersion, platformVersion) {
  let result = false;
  for (let i = 0; i < update.targetApplications.length; i++) {
    let app = update.targetApplications[i];
    if (app.id == Services.appinfo.ID) {
      return (Services.vc.compare(appVersion, app.minVersion) >= 0) &&
             (Services.vc.compare(appVersion, app.maxVersion) <= 0);
    }
    if (app.id == TOOLKIT_ID) {
      result = (Services.vc.compare(platformVersion, app.minVersion) >= 0) &&
               (Services.vc.compare(platformVersion, app.maxVersion) <= 0);
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

  













  getCompatibilityUpdate: function AUC_getCompatibilityUpdate(updates, version,
                                                              appVersion,
                                                              platformVersion) {
    if (!appVersion)
      appVersion = Services.appinfo.version;
    if (!platformVersion)
      platformVersion = Services.appinfo.platformVersion;

    for (let i = 0; i < updates.length; i++) {
      if ((Services.vc.compare(updates[i].version, version) == 0) &&
          matchesVersions(updates[i], appVersion, platformVersion))
        return updates[i];
    }
    return null;
  },

  










  getNewestCompatibleUpdate: function AUC_getNewestCompatibleUpdate(updates,
                                                                    appVersion,
                                                                    platformVersion) {
    if (!appVersion)
      appVersion = Services.appinfo.version;
    if (!platformVersion)
      platformVersion = Services.appinfo.platformVersion;

    let newest = null;
    for (let i = 0; i < updates.length; i++) {
      if (!updates[i].updateURL)
        continue;
      if ((newest == null || (Services.vc.compare(newest.version, updates[i].version) < 0)) &&
          matchesVersions(updates[i], appVersion, platformVersion))
        newest = updates[i];
    }
    return newest;
  },

  













  checkForUpdates: function AUC_checkForUpdates(id, type, updateKey, url,
                                                observer) {
    new UpdateParser(id, type, updateKey, url, observer);
  }
};
