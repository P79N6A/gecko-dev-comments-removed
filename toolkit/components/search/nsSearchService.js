# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/Promise.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AsyncShutdown",
  "resource://gre/modules/AsyncShutdown.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DeferredTask",
  "resource://gre/modules/DeferredTask.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "TelemetryStopwatch",
  "resource://gre/modules/TelemetryStopwatch.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Deprecated",
  "resource://gre/modules/Deprecated.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SearchStaticData",
  "resource://gre/modules/SearchStaticData.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gTextToSubURI",
                                   "@mozilla.org/intl/texttosuburi;1",
                                   "nsITextToSubURI");



XPCOMUtils.defineLazyGetter(this, "gEncoder",
                            function() {
                              return new TextEncoder();
                            });

const MODE_RDONLY   = 0x01;
const MODE_WRONLY   = 0x02;
const MODE_CREATE   = 0x08;
const MODE_APPEND   = 0x10;
const MODE_TRUNCATE = 0x20;


const NS_APP_SEARCH_DIR_LIST  = "SrchPluginsDL";
const NS_APP_USER_SEARCH_DIR  = "UsrSrchPlugns";
const NS_APP_SEARCH_DIR       = "SrchPlugns";
const NS_APP_USER_PROFILE_50_DIR = "ProfD";



const SEARCH_APP_DIR = 1;
const SEARCH_PROFILE_DIR = 2;
const SEARCH_IN_EXTENSION = 3;
const SEARCH_JAR = 4;


const SEARCH_ENGINE_TOPIC        = "browser-search-engine-modified";
const QUIT_APPLICATION_TOPIC     = "quit-application";

const SEARCH_ENGINE_REMOVED      = "engine-removed";
const SEARCH_ENGINE_ADDED        = "engine-added";
const SEARCH_ENGINE_CHANGED      = "engine-changed";
const SEARCH_ENGINE_LOADED       = "engine-loaded";
const SEARCH_ENGINE_CURRENT      = "engine-current";
const SEARCH_ENGINE_DEFAULT      = "engine-default";







const SEARCH_SERVICE_TOPIC       = "browser-search-service";




const SEARCH_SERVICE_METADATA_WRITTEN  = "write-metadata-to-disk-complete";




const SEARCH_SERVICE_CACHE_WRITTEN  = "write-cache-to-disk-complete";

const SEARCH_TYPE_MOZSEARCH      = Ci.nsISearchEngine.TYPE_MOZSEARCH;
const SEARCH_TYPE_OPENSEARCH     = Ci.nsISearchEngine.TYPE_OPENSEARCH;
const SEARCH_TYPE_SHERLOCK       = Ci.nsISearchEngine.TYPE_SHERLOCK;

const SEARCH_DATA_XML            = Ci.nsISearchEngine.DATA_XML;
const SEARCH_DATA_TEXT           = Ci.nsISearchEngine.DATA_TEXT;


const LAZY_SERIALIZE_DELAY = 100;


const CACHE_INVALIDATION_DELAY = 1000;



const CACHE_VERSION = 7;

const ICON_DATAURL_PREFIX = "data:image/x-icon;base64,";

const NEW_LINES = /(\r\n|\r|\n)/;



const MAX_ICON_SIZE   = 10000;



const DEFAULT_QUERY_CHARSET = "ISO-8859-1";

const SEARCH_BUNDLE = "chrome://global/locale/search/search.properties";
const BRAND_BUNDLE = "chrome://branding/locale/brand.properties";

const OPENSEARCH_NS_10  = "http://a9.com/-/spec/opensearch/1.0/";
const OPENSEARCH_NS_11  = "http://a9.com/-/spec/opensearch/1.1/";




const OPENSEARCH_NAMESPACES = [
  OPENSEARCH_NS_11, OPENSEARCH_NS_10,
  "http://a9.com/-/spec/opensearchdescription/1.1/",
  "http://a9.com/-/spec/opensearchdescription/1.0/"
];

const OPENSEARCH_LOCALNAME = "OpenSearchDescription";

const MOZSEARCH_NS_10     = "http://www.mozilla.org/2006/browser/search/";
const MOZSEARCH_LOCALNAME = "SearchPlugin";

const URLTYPE_SUGGEST_JSON = "application/x-suggestions+json";
const URLTYPE_SEARCH_HTML  = "text/html";
const URLTYPE_OPENSEARCH   = "application/opensearchdescription+xml";


const EMPTY_DOC = "<?xml version=\"1.0\"?>\n" +
                  "<" + MOZSEARCH_LOCALNAME +
                  " xmlns=\"" + MOZSEARCH_NS_10 + "\"" +
                  " xmlns:os=\"" + OPENSEARCH_NS_11 + "\"" +
                  "/>";

const BROWSER_SEARCH_PREF = "browser.search.";
const LOCALE_PREF = "general.useragent.locale";

const USER_DEFINED = "{searchTerms}";


#ifdef MOZ_OFFICIAL_BRANDING
const MOZ_OFFICIAL = "official";
#else
const MOZ_OFFICIAL = "unofficial";
#endif
#expand const MOZ_DISTRIBUTION_ID = __MOZ_DISTRIBUTION_ID__;

const MOZ_PARAM_LOCALE         = /\{moz:locale\}/g;
const MOZ_PARAM_DIST_ID        = /\{moz:distributionID\}/g;
const MOZ_PARAM_OFFICIAL       = /\{moz:official\}/g;



const OS_PARAM_USER_DEFINED    = /\{searchTerms\??\}/g;
const OS_PARAM_INPUT_ENCODING  = /\{inputEncoding\??\}/g;
const OS_PARAM_LANGUAGE        = /\{language\??\}/g;
const OS_PARAM_OUTPUT_ENCODING = /\{outputEncoding\??\}/g;


const OS_PARAM_LANGUAGE_DEF         = "*";
const OS_PARAM_OUTPUT_ENCODING_DEF  = "UTF-8";
const OS_PARAM_INPUT_ENCODING_DEF   = "UTF-8";




const OS_PARAM_COUNT        = /\{count\??\}/g;
const OS_PARAM_START_INDEX  = /\{startIndex\??\}/g;
const OS_PARAM_START_PAGE   = /\{startPage\??\}/g;


const OS_PARAM_COUNT_DEF        = "20"; 
const OS_PARAM_START_INDEX_DEF  = "1";  
const OS_PARAM_START_PAGE_DEF   = "1";  


const OS_PARAM_OPTIONAL     = /\{(?:\w+:)?\w+\?\}/g;





var OS_UNSUPPORTED_PARAMS = [
  [OS_PARAM_COUNT, OS_PARAM_COUNT_DEF],
  [OS_PARAM_START_INDEX, OS_PARAM_START_INDEX_DEF],
  [OS_PARAM_START_PAGE, OS_PARAM_START_PAGE_DEF],
];



const SEARCH_DEFAULT_UPDATE_INTERVAL = 7;



function isUsefulLine(aLine) {
  return !(/^\s*($|#)/i.test(aLine));
}

this.__defineGetter__("FileUtils", function() {
  delete this.FileUtils;
  Components.utils.import("resource://gre/modules/FileUtils.jsm");
  return FileUtils;
});

this.__defineGetter__("NetUtil", function() {
  delete this.NetUtil;
  Components.utils.import("resource://gre/modules/NetUtil.jsm");
  return NetUtil;
});

this.__defineGetter__("gChromeReg", function() {
  delete this.gChromeReg;
  return this.gChromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"].
                           getService(Ci.nsIChromeRegistry);
});




const SEARCH_LOG_PREFIX = "*** Search: ";




function DO_LOG(aText) {
  dump(SEARCH_LOG_PREFIX + aText + "\n");
  Services.console.logStringMessage(aText);
}

#ifdef DEBUG




function PREF_LOG(aText) {
  if (getBoolPref(BROWSER_SEARCH_PREF + "log", false))
    DO_LOG(aText);
}
var LOG = PREF_LOG;

#else





var LOG = function(){};

#endif









function ERROR(message, resultCode) {
  NS_ASSERT(false, SEARCH_LOG_PREFIX + message);
  throw Components.Exception(message, resultCode);
}









function FAIL(message, resultCode) {
  LOG(message);
  throw Components.Exception(message, resultCode || Cr.NS_ERROR_INVALID_ARG);
}








function limitURILength(str, len) {
  len = len || 140;
  if (str.length > len)
    return str.slice(0, len) + "...";
  return str;
}












function ENSURE_WARN(assertion, message, resultCode) {
  NS_ASSERT(assertion, SEARCH_LOG_PREFIX + message);
  if (!assertion)
    throw Components.Exception(message, resultCode);
}

function loadListener(aChannel, aEngine, aCallback) {
  this._channel = aChannel;
  this._bytes = [];
  this._engine = aEngine;
  this._callback = aCallback;
}
loadListener.prototype = {
  _callback: null,
  _channel: null,
  _countRead: 0,
  _engine: null,
  _stream: null,

  QueryInterface: function SRCH_loadQI(aIID) {
    if (aIID.equals(Ci.nsISupports)           ||
        aIID.equals(Ci.nsIRequestObserver)    ||
        aIID.equals(Ci.nsIStreamListener)     ||
        aIID.equals(Ci.nsIChannelEventSink)   ||
        aIID.equals(Ci.nsIInterfaceRequestor) ||
        
        aIID.equals(Ci.nsIHttpEventSink)      ||
        aIID.equals(Ci.nsIProgressEventSink)  ||
        false)
      return this;

    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  
  onStartRequest: function SRCH_loadStartR(aRequest, aContext) {
    LOG("loadListener: Starting request: " + aRequest.name);
    this._stream = Cc["@mozilla.org/binaryinputstream;1"].
                   createInstance(Ci.nsIBinaryInputStream);
  },

  onStopRequest: function SRCH_loadStopR(aRequest, aContext, aStatusCode) {
    LOG("loadListener: Stopping request: " + aRequest.name);

    var requestFailed = !Components.isSuccessCode(aStatusCode);
    if (!requestFailed && (aRequest instanceof Ci.nsIHttpChannel))
      requestFailed = !aRequest.requestSucceeded;

    if (requestFailed || this._countRead == 0) {
      LOG("loadListener: request failed!");
      
      this._callback(null, this._engine);
    } else
      this._callback(this._bytes, this._engine);
    this._channel = null;
    this._engine  = null;
  },

  
  onDataAvailable: function SRCH_loadDAvailable(aRequest, aContext,
                                                aInputStream, aOffset,
                                                aCount) {
    this._stream.setInputStream(aInputStream);

    
    this._bytes = this._bytes.concat(this._stream.readByteArray(aCount));
    this._countRead += aCount;
  },

  
  asyncOnChannelRedirect: function SRCH_loadCRedirect(aOldChannel, aNewChannel,
                                                      aFlags, callback) {
    this._channel = aNewChannel;
    callback.onRedirectVerifyCallback(Components.results.NS_OK);
  },

  
  getInterface: function SRCH_load_GI(aIID) {
    return this.QueryInterface(aIID);
  },

  
  
  onRedirect: function (aChannel, aNewChannel) {},
  
  onProgress: function (aRequest, aContext, aProgress, aProgressMax) {},
  onStatus: function (aRequest, aContext, aStatus, aStatusArg) {}
}
















function checkNameSpace(aElement, aLocalNameArray, aNameSpaceArray) {
  if (!aLocalNameArray || !aNameSpaceArray)
    FAIL("missing aLocalNameArray or aNameSpaceArray for checkNameSpace");
  return (aElement                                                &&
          (aLocalNameArray.indexOf(aElement.localName)    != -1)  &&
          (aNameSpaceArray.indexOf(aElement.namespaceURI) != -1));
}






function closeSafeOutputStream(aFOS) {
  if (aFOS instanceof Ci.nsISafeOutputStream) {
    try {
      aFOS.finish();
      return;
    } catch (e) { }
  }
  aFOS.close();
}







function makeURI(aURLSpec, aCharset) {
  try {
    return NetUtil.newURI(aURLSpec, aCharset);
  } catch (ex) { }

  return null;
}






function getDir(aKey, aIFace) {
  if (!aKey)
    FAIL("getDir requires a directory key!");

  return Services.dirsvc.get(aKey, aIFace || Ci.nsIFile);
}





function queryCharsetFromCode(aCode) {
  const codes = [];
  codes[0] = "macintosh";
  codes[6] = "x-mac-greek";
  codes[35] = "x-mac-turkish";
  codes[513] = "ISO-8859-1";
  codes[514] = "ISO-8859-2";
  codes[517] = "ISO-8859-5";
  codes[518] = "ISO-8859-6";
  codes[519] = "ISO-8859-7";
  codes[520] = "ISO-8859-8";
  codes[521] = "ISO-8859-9";
  codes[1280] = "windows-1252";
  codes[1281] = "windows-1250";
  codes[1282] = "windows-1251";
  codes[1283] = "windows-1253";
  codes[1284] = "windows-1254";
  codes[1285] = "windows-1255";
  codes[1286] = "windows-1256";
  codes[1536] = "us-ascii";
  codes[1584] = "GB2312";
  codes[1585] = "gbk";
  codes[1600] = "EUC-KR";
  codes[2080] = "ISO-2022-JP";
  codes[2096] = "ISO-2022-CN";
  codes[2112] = "ISO-2022-KR";
  codes[2336] = "EUC-JP";
  codes[2352] = "GB2312";
  codes[2353] = "x-euc-tw";
  codes[2368] = "EUC-KR";
  codes[2561] = "Shift_JIS";
  codes[2562] = "KOI8-R";
  codes[2563] = "Big5";
  codes[2565] = "HZ-GB-2312";

  if (codes[aCode])
    return codes[aCode];

  
  return "windows-1252";
}
function fileCharsetFromCode(aCode) {
  const codes = [
    "macintosh",             
    "Shift_JIS",             
    "Big5",                  
    "EUC-KR",                
    "X-MAC-ARABIC",          
    "X-MAC-HEBREW",          
    "X-MAC-GREEK",           
    "X-MAC-CYRILLIC",        
    "X-MAC-DEVANAGARI" ,     
    "X-MAC-GURMUKHI",        
    "X-MAC-GUJARATI",        
    "X-MAC-ORIYA",           
    "X-MAC-BENGALI",         
    "X-MAC-TAMIL",           
    "X-MAC-TELUGU",          
    "X-MAC-KANNADA",         
    "X-MAC-MALAYALAM",       
    "X-MAC-SINHALESE",       
    "X-MAC-BURMESE",         
    "X-MAC-KHMER",           
    "X-MAC-THAI",            
    "X-MAC-LAOTIAN",         
    "X-MAC-GEORGIAN",        
    "X-MAC-ARMENIAN",        
    "GB2312",                
    "X-MAC-TIBETAN",         
    "X-MAC-MONGOLIAN",       
    "X-MAC-ETHIOPIC",        
    "X-MAC-CENTRALEURROMAN", 
    "X-MAC-VIETNAMESE",      
    "X-MAC-EXTARABIC"        
  ];
  
  return codes[aCode] || codes[0];
}





function bytesToString(aBytes, aCharset) {
  var converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                  createInstance(Ci.nsIScriptableUnicodeConverter);
  LOG("bytesToString: converting using charset: " + aCharset);

  try {
    converter.charset = aCharset;
    return converter.convertFromByteArray(aBytes, aBytes.length);
  } catch (ex) {}

  return null;
}











function sherlockBytesToLines(aBytes, aCharsetCode) {
  
  var charset = fileCharsetFromCode(aCharsetCode);

  var dataString = bytesToString(aBytes, charset);
  if (!dataString)
    FAIL("sherlockBytesToLines: Couldn't convert byte array!", Cr.NS_ERROR_FAILURE);

  
  
  return dataString.split(NEW_LINES).filter(isUsefulLine);
}






function getLocale() {
  let locale = getLocalizedPref(LOCALE_PREF);
  if (locale)
    return locale;

  
  return Services.prefs.getCharPref(LOCALE_PREF);
}







function getLocalizedPref(aPrefName, aDefault) {
  const nsIPLS = Ci.nsIPrefLocalizedString;
  try {
    return Services.prefs.getComplexValue(aPrefName, nsIPLS).data;
  } catch (ex) {}

  return aDefault;
}






function setLocalizedPref(aPrefName, aValue) {
  const nsIPLS = Ci.nsIPrefLocalizedString;
  try {
    var pls = Components.classes["@mozilla.org/pref-localizedstring;1"]
                        .createInstance(Ci.nsIPrefLocalizedString);
    pls.data = aValue;
    Services.prefs.setComplexValue(aPrefName, nsIPLS, pls);
  } catch (ex) {}
}







function getBoolPref(aName, aDefault) {
  try {
    return Services.prefs.getBoolPref(aName);
  } catch (ex) {
    return aDefault;
  }
}









function getSanitizedFile(aName) {
  var fileName = sanitizeName(aName) + ".xml";
  var file = getDir(NS_APP_USER_SEARCH_DIR);
  file.append(fileName);
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
  return file;
}






function sanitizeName(aName) {
  const maxLength = 60;
  const minLength = 1;
  var name = aName.toLowerCase();
  name = name.replace(/\s+/g, "-");
  name = name.replace(/[^-a-z0-9]/g, "");

  
  if (name.length < minLength)
    name = Math.random().toString(36).replace(/^.*\./, '');

  
  return name.substring(0, maxLength);
}







function getMozParamPref(prefName)
  Services.prefs.getCharPref(BROWSER_SEARCH_PREF + "param." + prefName);












let gInitialized = false;
function notifyAction(aEngine, aVerb) {
  if (gInitialized) {
    LOG("NOTIFY: Engine: \"" + aEngine.name + "\"; Verb: \"" + aVerb + "\"");
    Services.obs.notifyObservers(aEngine, SEARCH_ENGINE_TOPIC, aVerb);
  }
}

function  parseJsonFromStream(aInputStream) {
  const json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
  const data = json.decodeFromStream(aInputStream, aInputStream.available());
  return data;
}




function QueryParameter(aName, aValue, aPurpose) {
  if (!aName || (aValue == null))
    FAIL("missing name or value for QueryParameter!");

  this.name = aName;
  this.value = aValue;
  this.purpose = aPurpose;
}
















function ParamSubstitution(aParamValue, aSearchTerms, aEngine) {
  var value = aParamValue;

  var distributionID = MOZ_DISTRIBUTION_ID;
  try {
    distributionID = Services.prefs.getCharPref(BROWSER_SEARCH_PREF + "distributionID");
  }
  catch (ex) { }
  var official = MOZ_OFFICIAL;
  try {
    if (Services.prefs.getBoolPref(BROWSER_SEARCH_PREF + "official"))
      official = "official";
    else
      official = "unofficial";
  }
  catch (ex) { }

  
  
  if (aEngine._isDefault) {
    value = value.replace(MOZ_PARAM_LOCALE, getLocale());
    value = value.replace(MOZ_PARAM_DIST_ID, distributionID);
    value = value.replace(MOZ_PARAM_OFFICIAL, official);
  }

  
  value = value.replace(OS_PARAM_USER_DEFINED, aSearchTerms);
  value = value.replace(OS_PARAM_INPUT_ENCODING, aEngine.queryCharset);
  value = value.replace(OS_PARAM_LANGUAGE,
                        getLocale() || OS_PARAM_LANGUAGE_DEF);
  value = value.replace(OS_PARAM_OUTPUT_ENCODING,
                        OS_PARAM_OUTPUT_ENCODING_DEF);

  
  value = value.replace(OS_PARAM_OPTIONAL, "");

  
  for (var i = 0; i < OS_UNSUPPORTED_PARAMS.length; ++i) {
    value = value.replace(OS_UNSUPPORTED_PARAMS[i][0],
                          OS_UNSUPPORTED_PARAMS[i][1]);
  }

  return value;
}





















function EngineURL(aType, aMethod, aTemplate, aResultDomain) {
  if (!aType || !aMethod || !aTemplate)
    FAIL("missing type, method or template for EngineURL!");

  var method = aMethod.toUpperCase();
  var type   = aType.toLowerCase();

  if (method != "GET" && method != "POST")
    FAIL("method passed to EngineURL must be \"GET\" or \"POST\"");

  this.type     = type;
  this.method   = method;
  this.params   = [];
  this.rels     = [];
  
  this.mozparams = {};

  var templateURI = makeURI(aTemplate);
  if (!templateURI)
    FAIL("new EngineURL: template is not a valid URI!", Cr.NS_ERROR_FAILURE);

  switch (templateURI.scheme) {
    case "http":
    case "https":
    
    
    
      this.template = aTemplate;
      break;
    default:
      FAIL("new EngineURL: template uses invalid scheme!", Cr.NS_ERROR_FAILURE);
  }

  
  
  this.resultDomain = aResultDomain || templateURI.host;
  
  if (this.resultDomain.startsWith("www.")) {
    this.resultDomain = this.resultDomain.substr(4);
  }
}
EngineURL.prototype = {

  addParam: function SRCH_EURL_addParam(aName, aValue, aPurpose) {
    this.params.push(new QueryParameter(aName, aValue, aPurpose));
  },

  
  
  _addMozParam: function SRCH_EURL__addMozParam(aObj) {
    aObj.mozparam = true;
    this.mozparams[aObj.name] = aObj;
  },

  getSubmission: function SRCH_EURL_getSubmission(aSearchTerms, aEngine, aPurpose) {
    var url = ParamSubstitution(this.template, aSearchTerms, aEngine);
    
    
    var purpose = aPurpose || "";

    
    
    var dataString = "";
    for (var i = 0; i < this.params.length; ++i) {
      var param = this.params[i];

      
      if (param.purpose !== undefined && param.purpose != purpose)
        continue;

      var value = ParamSubstitution(param.value, aSearchTerms, aEngine);

      dataString += (i > 0 ? "&" : "") + param.name + "=" + value;
    }

    var postData = null;
    if (this.method == "GET") {
      
      
      if (url.indexOf("?") == -1 && dataString)
        url += "?";
      url += dataString;
    } else if (this.method == "POST") {
      
      
      var stringStream = Cc["@mozilla.org/io/string-input-stream;1"].
                         createInstance(Ci.nsIStringInputStream);
      stringStream.data = dataString;

      postData = Cc["@mozilla.org/network/mime-input-stream;1"].
                 createInstance(Ci.nsIMIMEInputStream);
      postData.addHeader("Content-Type", "application/x-www-form-urlencoded");
      postData.addContentLength = true;
      postData.setData(stringStream);
    }

    return new Submission(makeURI(url), postData);
  },

  _getTermsParameterName: function SRCH_EURL__getTermsParameterName() {
    let queryParam = this.params.find(p => p.value == USER_DEFINED);
    return queryParam ? queryParam.name : "";
  },

  _hasRelation: function SRC_EURL__hasRelation(aRel)
    this.rels.some(function(e) e == aRel.toLowerCase()),

  _initWithJSON: function SRC_EURL__initWithJSON(aJson, aEngine) {
    if (!aJson.params)
      return;

    this.rels = aJson.rels;

    for (let i = 0; i < aJson.params.length; ++i) {
      let param = aJson.params[i];
      if (param.mozparam) {
        if (param.condition == "defaultEngine") {
          if (aEngine._isDefaultEngine())
            this.addParam(param.name, param.trueValue);
          else
            this.addParam(param.name, param.falseValue);
        } else if (param.condition == "pref") {
          let value = getMozParamPref(param.pref);
          this.addParam(param.name, value);
        }
        this._addMozParam(param);
      }
      else
        this.addParam(param.name, param.value, param.purpose);
    }
  },

  



  _serializeToJSON: function SRCH_EURL__serializeToJSON() {
    var json = {
      template: this.template,
      rels: this.rels,
      resultDomain: this.resultDomain
    };

    if (this.type != URLTYPE_SEARCH_HTML)
      json.type = this.type;
    if (this.method != "GET")
      json.method = this.method;

    function collapseMozParams(aParam)
      this.mozparams[aParam.name] || aParam;
    json.params = this.params.map(collapseMozParams, this);

    return json;
  },

  








  _serializeToElement: function SRCH_EURL_serializeToEl(aDoc, aElement) {
    var url = aDoc.createElementNS(OPENSEARCH_NS_11, "Url");
    url.setAttribute("type", this.type);
    url.setAttribute("method", this.method);
    url.setAttribute("template", this.template);
    if (this.rels.length)
      url.setAttribute("rel", this.rels.join(" "));
    if (this.resultDomain)
      url.setAttribute("resultDomain", this.resultDomain);

    for (var i = 0; i < this.params.length; ++i) {
      var param = aDoc.createElementNS(OPENSEARCH_NS_11, "Param");
      param.setAttribute("name", this.params[i].name);
      param.setAttribute("value", this.params[i].value);
      url.appendChild(aDoc.createTextNode("\n  "));
      url.appendChild(param);
    }
    url.appendChild(aDoc.createTextNode("\n"));
    aElement.appendChild(url);
  }
};













function Engine(aLocation, aSourceDataType, aIsReadOnly) {
  this._dataType = aSourceDataType;
  this._readOnly = aIsReadOnly;
  this._urls = [];

  if (aLocation.type) {
    if (aLocation.type == "filePath")
      this._file = aLocation.value;
    else if (aLocation.type == "uri")
      this._uri = aLocation.value;
  } else if (aLocation instanceof Ci.nsILocalFile) {
    
    this._file = aLocation;
  } else if (aLocation instanceof Ci.nsIURI) {
    switch (aLocation.scheme) {
      case "https":
      case "http":
      case "ftp":
      case "data":
      case "file":
      case "resource":
      case "chrome":
        this._uri = aLocation;
        break;
      default:
        ERROR("Invalid URI passed to the nsISearchEngine constructor",
              Cr.NS_ERROR_INVALID_ARG);
    }
  } else
    ERROR("Engine location is neither a File nor a URI object",
          Cr.NS_ERROR_INVALID_ARG);
}

Engine.prototype = {
  
  
  _alias: undefined,
  
  
  _identifier: undefined,
  
  
  _data: null,
  
  _dataType: null,
  
  _readOnly: true,
  
  _description: "",
  
  
  _engineToUpdate: null,
  
  __file: null,
  get _file() {
    if (this.__file && !(this.__file instanceof Ci.nsILocalFile)) {
      let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      file.persistentDescriptor = this.__file;
      return this.__file = file;
    }
    return this.__file;
  },
  set _file(aValue) {
    this.__file = aValue;
  },
  
  
  _hasPreferredIcon: null,
  
  _hidden: null,
  
  _name: null,
  
  _type: null,
  
  _queryCharset: null,
  
  __searchForm: null,
  get _searchForm() {
    return this.__searchForm;
  },
  set _searchForm(aValue) {
    if (/^https?:/i.test(aValue))
      this.__searchForm = aValue;
    else
      LOG("_searchForm: Invalid URL dropped for " + this._name ||
          "the current engine");
  },
  
  
  
  __uri: null,
  get _uri() {
    if (this.__uri && !(this.__uri instanceof Ci.nsIURI))
      this.__uri = makeURI(this.__uri);

    return this.__uri;
  },
  set _uri(aValue) {
    this.__uri = aValue;
  },
  
  
  _confirm: false,
  
  
  _useNow: false,
  
  
  _installCallback: null,
  
  
  __installLocation: null,
  
  _updateInterval: null,
  
  _updateURL: null,
  
  _iconUpdateURL: null,
  
  _lazySerializeTask: null,
  
  _extensionID: null,

  





  _initFromFile: function SRCH_ENG_initFromFile() {
    if (!this._file || !this._file.exists())
      FAIL("File must exist before calling initFromFile!", Cr.NS_ERROR_UNEXPECTED);

    var fileInStream = Cc["@mozilla.org/network/file-input-stream;1"].
                       createInstance(Ci.nsIFileInputStream);

    fileInStream.init(this._file, MODE_RDONLY, FileUtils.PERMS_FILE, false);

    if (this._dataType == SEARCH_DATA_XML) {
      var domParser = Cc["@mozilla.org/xmlextras/domparser;1"].
                      createInstance(Ci.nsIDOMParser);
      var doc = domParser.parseFromStream(fileInStream, "UTF-8",
                                          this._file.fileSize,
                                          "text/xml");

      this._data = doc.documentElement;
    } else {
      ERROR("Unsuppored engine _dataType in _initFromFile: \"" +
            this._dataType + "\"",
            Cr.NS_ERROR_UNEXPECTED);
    }
    fileInStream.close();

    
    this._initFromData();
  },

  






  _asyncInitFromFile: function SRCH_ENG__asyncInitFromFile() {
    return Task.spawn(function() {
      if (!this._file || !(yield OS.File.exists(this._file.path)))
        FAIL("File must exist before calling initFromFile!", Cr.NS_ERROR_UNEXPECTED);

      if (this._dataType == SEARCH_DATA_XML) {
        let fileURI = NetUtil.ioService.newFileURI(this._file);
        yield this._retrieveSearchXMLData(fileURI.spec);
      } else {
        ERROR("Unsuppored engine _dataType in _initFromFile: \"" +
              this._dataType + "\"",
              Cr.NS_ERROR_UNEXPECTED);
      }

      
      this._initFromData();
    }.bind(this));
  },

  



  _initFromURIAndLoad: function SRCH_ENG_initFromURIAndLoad() {
    ENSURE_WARN(this._uri instanceof Ci.nsIURI,
                "Must have URI when calling _initFromURIAndLoad!",
                Cr.NS_ERROR_UNEXPECTED);

    LOG("_initFromURIAndLoad: Downloading engine from: \"" + this._uri.spec + "\".");

    var chan = NetUtil.ioService.newChannelFromURI(this._uri);

    if (this._engineToUpdate && (chan instanceof Ci.nsIHttpChannel)) {
      var lastModified = engineMetadataService.getAttr(this._engineToUpdate,
                                                       "updatelastmodified");
      if (lastModified)
        chan.setRequestHeader("If-Modified-Since", lastModified, false);
    }
    var listener = new loadListener(chan, this, this._onLoad);
    chan.notificationCallbacks = listener;
    chan.asyncOpen(listener, null);
  },

  





  _asyncInitFromURI: function SRCH_ENG__asyncInitFromURI() {
    return Task.spawn(function() {
      LOG("_asyncInitFromURI: Loading engine from: \"" + this._uri.spec + "\".");
      yield this._retrieveSearchXMLData(this._uri.spec);
      
      this._initFromData();
    }.bind(this));
  },

  





  _retrieveSearchXMLData: function SRCH_ENG__retrieveSearchXMLData(aURL) {
    let deferred = Promise.defer();
    let request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
                    createInstance(Ci.nsIXMLHttpRequest);
    request.overrideMimeType("text/xml");
    request.onload = (aEvent) => {
      let responseXML = aEvent.target.responseXML;
      this._data = responseXML.documentElement;
      deferred.resolve();
    };
    request.onerror = function(aEvent) {
      deferred.resolve();
    };
    request.open("GET", aURL, true);
    request.send();

    return deferred.promise;
  },

  _initFromURISync: function SRCH_ENG_initFromURISync() {
    ENSURE_WARN(this._uri instanceof Ci.nsIURI,
                "Must have URI when calling _initFromURISync!",
                Cr.NS_ERROR_UNEXPECTED);

    ENSURE_WARN(this._uri.schemeIs("chrome"), "_initFromURISync called for non-chrome URI",
                Cr.NS_ERROR_FAILURE);

    LOG("_initFromURISync: Loading engine from: \"" + this._uri.spec + "\".");

    var chan = NetUtil.ioService.newChannelFromURI(this._uri);

    var stream = chan.open();
    var parser = Cc["@mozilla.org/xmlextras/domparser;1"].
                 createInstance(Ci.nsIDOMParser);
    var doc = parser.parseFromStream(stream, "UTF-8", stream.available(), "text/xml");

    this._data = doc.documentElement;

    
    this._initFromData();
  },

  









  _getURLOfType: function SRCH_ENG__getURLOfType(aType, aRel) {
    for (var i = 0; i < this._urls.length; ++i) {
      if (this._urls[i].type == aType && (!aRel || this._urls[i]._hasRelation(aRel)))
        return this._urls[i];
    }

    return null;
  },

  _confirmAddEngine: function SRCH_SVC_confirmAddEngine() {
    var stringBundle = Services.strings.createBundle(SEARCH_BUNDLE);
    var titleMessage = stringBundle.GetStringFromName("addEngineConfirmTitle");

    
    var dialogMessage =
        stringBundle.formatStringFromName("addEngineConfirmation",
                                          [this._name, this._uri.host], 2);
    var checkboxMessage = null;
    if (!getBoolPref(BROWSER_SEARCH_PREF + "noCurrentEngine", false))
      checkboxMessage = stringBundle.GetStringFromName("addEngineAsCurrentText");

    var addButtonLabel =
        stringBundle.GetStringFromName("addEngineAddButtonLabel");

    var ps = Services.prompt;
    var buttonFlags = (ps.BUTTON_TITLE_IS_STRING * ps.BUTTON_POS_0) +
                      (ps.BUTTON_TITLE_CANCEL    * ps.BUTTON_POS_1) +
                       ps.BUTTON_POS_0_DEFAULT;

    var checked = {value: false};
    
    
    var confirm = !ps.confirmEx(null,
                                titleMessage,
                                dialogMessage,
                                buttonFlags,
                                addButtonLabel,
                                null, null, 
                                checkboxMessage,
                                checked);

    return {confirmed: confirm, useNow: checked.value};
  },

  




  _onLoad: function SRCH_ENG_onLoad(aBytes, aEngine) {
    



    function onError(errorCode = Ci.nsISearchInstallCallback.ERROR_UNKNOWN_FAILURE) {
      
      if (aEngine._installCallback) {
        aEngine._installCallback(errorCode);
      }
    }

    function promptError(strings = {}, error = undefined) {
      onError(error);

      if (aEngine._engineToUpdate) {
        
        LOG("updating " + aEngine._engineToUpdate.name + " failed");
        return;
      }
      var brandBundle = Services.strings.createBundle(BRAND_BUNDLE);
      var brandName = brandBundle.GetStringFromName("brandShortName");

      var searchBundle = Services.strings.createBundle(SEARCH_BUNDLE);
      var msgStringName = strings.error || "error_loading_engine_msg2";
      var titleStringName = strings.title || "error_loading_engine_title";
      var title = searchBundle.GetStringFromName(titleStringName);
      var text = searchBundle.formatStringFromName(msgStringName,
                                                   [brandName, aEngine._location],
                                                   2);

      Services.ww.getNewPrompter(null).alert(title, text);
    }

    if (!aBytes) {
      promptError();
      return;
    }

    var engineToUpdate = null;
    if (aEngine._engineToUpdate) {
      engineToUpdate = aEngine._engineToUpdate.wrappedJSObject;

      
      aEngine._file = engineToUpdate._file;
    }

    switch (aEngine._dataType) {
      case SEARCH_DATA_XML:
        var parser = Cc["@mozilla.org/xmlextras/domparser;1"].
                     createInstance(Ci.nsIDOMParser);
        var doc = parser.parseFromBuffer(aBytes, aBytes.length, "text/xml");
        aEngine._data = doc.documentElement;
        break;
      case SEARCH_DATA_TEXT:
        aEngine._data = aBytes;
        break;
      default:
        promptError();
        LOG("_onLoad: Bogus engine _dataType: \"" + this._dataType + "\"");
        return;
    }

    try {
      
      aEngine._initFromData();
    } catch (ex) {
      LOG("_onLoad: Failed to init engine!\n" + ex);
      
      promptError();
      return;
    }

    
    
    if (!engineToUpdate) {
      if (Services.search.getEngineByName(aEngine.name)) {
        
        
        if (aEngine._confirm) {
          promptError({ error: "error_duplicate_engine_msg",
                        title: "error_invalid_engine_title"
                      }, Ci.nsISearchInstallCallback.ERROR_DUPLICATE_ENGINE);
        } else {
          onError(Ci.nsISearchInstallCallback.ERROR_DUPLICATE_ENGINE);
        }
        LOG("_onLoad: duplicate engine found, bailing");
        return;
      }
    }

    
    
    
    if (aEngine._confirm) {
      var confirmation = aEngine._confirmAddEngine();
      LOG("_onLoad: confirm is " + confirmation.confirmed +
          "; useNow is " + confirmation.useNow);
      if (!confirmation.confirmed) {
        onError();
        return;
      }
      aEngine._useNow = confirmation.useNow;
    }

    
    
    if (!aEngine._file)
      aEngine._file = getSanitizedFile(aEngine.name);

    if (engineToUpdate) {
      
      
      engineMetadataService.setAttr(aEngine, "updatelastmodified",
                                    (new Date()).toUTCString());

      
      
      if (engineToUpdate._isInAppDir) {
        let oldUpdateURL = engineToUpdate._updateURL;
        let newUpdateURL = aEngine._updateURL;
        let oldSelfURL = engineToUpdate._getURLOfType(URLTYPE_OPENSEARCH, "self");
        if (oldSelfURL) {
          oldUpdateURL = oldSelfURL.template;
          let newSelfURL = aEngine._getURLOfType(URLTYPE_OPENSEARCH, "self");
          if (!newSelfURL) {
            LOG("_onLoad: updateURL missing in updated engine for " +
                aEngine.name + " aborted");
            onError();
            return;
          }
          newUpdateURL = newSelfURL.template;
        }

        if (oldUpdateURL != newUpdateURL) {
          LOG("_onLoad: updateURLs do not match! Update of " + aEngine.name + " aborted");
          onError();
          return;
        }
      }

      
      if (!aEngine._iconURI && engineToUpdate._iconURI)
        aEngine._iconURI = engineToUpdate._iconURI;
    }

    
    
    if (!aEngine._readOnly)
      aEngine._serializeToFile();

    
    
    notifyAction(aEngine, SEARCH_ENGINE_LOADED);

    
    if (aEngine._installCallback) {
      aEngine._installCallback();
    }
  },

  









  _getIconKey: function SRCH_ENG_getIconKey(aWidth, aHeight) {
    let keyObj = {
     width: aWidth,
     height: aHeight
    };

    return JSON.stringify(keyObj);
  },

  









  _addIconToMap: function SRCH_ENG_addIconToMap(aWidth, aHeight, aURISpec) {
    
    this._iconMapObj = this._iconMapObj || {};
    let key = this._getIconKey(aWidth, aHeight);
    this._iconMapObj[key] = aURISpec;
  },

  

















  _setIcon: function SRCH_ENG_setIcon(aIconURL, aIsPreferred, aWidth, aHeight) {
    var uri = makeURI(aIconURL);

    
    if (!uri)
      return;

    LOG("_setIcon: Setting icon url \"" + limitURILength(uri.spec) + "\" for engine \""
        + this.name + "\".");
    
    switch (uri.scheme) {
      case "data":
        if (!this._hasPreferredIcon || aIsPreferred) {
          this._iconURI = uri;
          notifyAction(this, SEARCH_ENGINE_CHANGED);
          this._hasPreferredIcon = aIsPreferred;
        }

        if (aWidth && aHeight) {
          this._addIconToMap(aWidth, aHeight, aIconURL)
        }
        break;
      case "http":
      case "https":
      case "ftp":
        
        if (!this._readOnly ||
            getBoolPref(BROWSER_SEARCH_PREF + "cache.enabled", true)) {
          LOG("_setIcon: Downloading icon: \"" + uri.spec +
              "\" for engine: \"" + this.name + "\"");
          var chan = NetUtil.ioService.newChannelFromURI(uri);

          let iconLoadCallback = function (aByteArray, aEngine) {
            
            
            if (aEngine._hasPreferredIcon && !aIsPreferred)
              return;

            if (!aByteArray || aByteArray.length > MAX_ICON_SIZE) {
              LOG("iconLoadCallback: load failed, or the icon was too large!");
              return;
            }

            var str = btoa(String.fromCharCode.apply(null, aByteArray));
            let dataURL = ICON_DATAURL_PREFIX + str;
            aEngine._iconURI = makeURI(dataURL);

            if (aWidth && aHeight) {
              aEngine._addIconToMap(aWidth, aHeight, dataURL)
            }

            
            
            
            
            
            if (aEngine._file && !aEngine._readOnly)
              aEngine._serializeToFile();

            notifyAction(aEngine, SEARCH_ENGINE_CHANGED);
            aEngine._hasPreferredIcon = aIsPreferred;
          }

          
          
          
          var engineToSet = this._engineToUpdate || this;

          var listener = new loadListener(chan, engineToSet, iconLoadCallback);
          chan.notificationCallbacks = listener;
          chan.asyncOpen(listener, null);
        }
        break;
    }
  },

  


  _initFromData: function SRCH_ENG_initFromData() {
    ENSURE_WARN(this._data, "Can't init an engine with no data!",
                Cr.NS_ERROR_UNEXPECTED);

    
    switch (this._dataType) {
      case SEARCH_DATA_XML:
        if (checkNameSpace(this._data, [MOZSEARCH_LOCALNAME],
            [MOZSEARCH_NS_10])) {

          LOG("_init: Initing MozSearch plugin from " + this._location);

          this._type = SEARCH_TYPE_MOZSEARCH;
          this._parseAsMozSearch();

        } else if (checkNameSpace(this._data, [OPENSEARCH_LOCALNAME],
                                  OPENSEARCH_NAMESPACES)) {

          LOG("_init: Initing OpenSearch plugin from " + this._location);

          this._type = SEARCH_TYPE_OPENSEARCH;
          this._parseAsOpenSearch();

        } else
          FAIL(this._location + " is not a valid search plugin.", Cr.NS_ERROR_FAILURE);

        break;
      case SEARCH_DATA_TEXT:
        LOG("_init: Initing Sherlock plugin from " + this._location);

        
        this._type = SEARCH_TYPE_SHERLOCK;
        this._parseAsSherlock();
    }

    
    
    this._data = null;
  },

  


  _initFromMetadata: function SRCH_ENG_initMetaData(aName, aIconURL, aAlias,
                                                    aDescription, aMethod,
                                                    aTemplate, aExtensionID) {
    ENSURE_WARN(!this._readOnly,
                "Can't call _initFromMetaData on a readonly engine!",
                Cr.NS_ERROR_FAILURE);

    this._urls.push(new EngineURL(URLTYPE_SEARCH_HTML, aMethod, aTemplate));

    this._name = aName;
    this.alias = aAlias;
    this._description = aDescription;
    this._setIcon(aIconURL, true);
    this._extensionID = aExtensionID;

    this._serializeToFile();
  },

  








  _parseURL: function SRCH_ENG_parseURL(aElement) {
    var type     = aElement.getAttribute("type");
    
    
    var method   = aElement.getAttribute("method") || "GET";
    var template = aElement.getAttribute("template");
    var resultDomain = aElement.getAttribute("resultdomain");

    try {
      var url = new EngineURL(type, method, template, resultDomain);
    } catch (ex) {
      FAIL("_parseURL: failed to add " + template + " as a URL",
           Cr.NS_ERROR_FAILURE);
    }

    if (aElement.hasAttribute("rel"))
      url.rels = aElement.getAttribute("rel").toLowerCase().split(/\s+/);

    for (var i = 0; i < aElement.childNodes.length; ++i) {
      var param = aElement.childNodes[i];
      if (param.localName == "Param") {
        try {
          url.addParam(param.getAttribute("name"), param.getAttribute("value"));
        } catch (ex) {
          
          LOG("_parseURL: Url element has an invalid param");
        }
      } else if (param.localName == "MozParam" &&
                 
                 this._isDefault) {
        var value;
        let condition = param.getAttribute("condition");

        
        if (!condition) {
          let engineLoc = this._location;
          let paramName = param.getAttribute("name");
          LOG("_parseURL: MozParam (" + paramName + ") without a condition attribute found parsing engine: " + engineLoc);
          continue;
        }

        switch (condition) {
          case "purpose":
            url.addParam(param.getAttribute("name"),
                         param.getAttribute("value"),
                         param.getAttribute("purpose"));
            
            
            break;
          case "defaultEngine":
            
            if (this._isDefaultEngine())
              value = param.getAttribute("trueValue");
            else
              value = param.getAttribute("falseValue");
            url.addParam(param.getAttribute("name"), value);
            url._addMozParam({"name": param.getAttribute("name"),
                              "falseValue": param.getAttribute("falseValue"),
                              "trueValue": param.getAttribute("trueValue"),
                              "condition": "defaultEngine"});
            break;

          case "pref":
            try {
              value = getMozParamPref(param.getAttribute("pref"), value);
              url.addParam(param.getAttribute("name"), value);
              url._addMozParam({"pref": param.getAttribute("pref"),
                                "name": param.getAttribute("name"),
                                "condition": "pref"});
            } catch (e) { }
            break;
          default:
            let engineLoc = this._location;
            let paramName = param.getAttribute("name");
            LOG("_parseURL: MozParam (" + paramName + ") has an unknown condition: " + condition + ". Found parsing engine: " + engineLoc);
          break;
        }
      }
    }

    this._urls.push(url);
  },

  _isDefaultEngine: function SRCH_ENG__isDefaultEngine() {
    let defaultPrefB = Services.prefs.getDefaultBranch(BROWSER_SEARCH_PREF);
    let nsIPLS = Ci.nsIPrefLocalizedString;
    let defaultEngine;
    try {
      defaultEngine = defaultPrefB.getComplexValue("defaultenginename", nsIPLS).data;
    } catch (ex) {}
    return this.name == defaultEngine;
  },

  



  _parseImage: function SRCH_ENG_parseImage(aElement) {
    LOG("_parseImage: Image textContent: \"" + limitURILength(aElement.textContent) + "\"");

    let width = parseInt(aElement.getAttribute("width"), 10);
    let height = parseInt(aElement.getAttribute("height"), 10);
    let isPrefered = width == 16 && height == 16;

    if (isNaN(width) || isNaN(height) || width <= 0 || height <=0) {
      LOG("OpenSearch image element must have positive width and height.");
      return;
    }

    this._setIcon(aElement.textContent, isPrefered, width, height);
  },

  _parseAsMozSearch: function SRCH_ENG_parseAsMoz() {
    
    this._parseAsOpenSearch();
  },

  



  _parseAsOpenSearch: function SRCH_ENG_parseAsOS() {
    var doc = this._data;

    
    this._queryCharset = OS_PARAM_INPUT_ENCODING_DEF;

    for (var i = 0; i < doc.childNodes.length; ++i) {
      var child = doc.childNodes[i];
      switch (child.localName) {
        case "ShortName":
          this._name = child.textContent;
          break;
        case "Description":
          this._description = child.textContent;
          break;
        case "Url":
          try {
            this._parseURL(child);
          } catch (ex) {
            
            LOG("_parseAsOpenSearch: failed to parse URL child: " + ex);
          }
          break;
        case "Image":
          this._parseImage(child);
          break;
        case "InputEncoding":
          this._queryCharset = child.textContent.toUpperCase();
          break;

        
        case "SearchForm":
          this._searchForm = child.textContent;
          break;
        case "UpdateUrl":
          this._updateURL = child.textContent;
          break;
        case "UpdateInterval":
          this._updateInterval = parseInt(child.textContent);
          break;
        case "IconUpdateUrl":
          this._iconUpdateURL = child.textContent;
          break;
        case "ExtensionID":
          this._extensionID = child.textContent;
          breakk;
      }
    }
    if (!this.name || (this._urls.length == 0))
      FAIL("_parseAsOpenSearch: No name, or missing URL!", Cr.NS_ERROR_FAILURE);
    if (!this.supportsResponseType(URLTYPE_SEARCH_HTML))
      FAIL("_parseAsOpenSearch: No text/html result type!", Cr.NS_ERROR_FAILURE);
  },

  



  _parseAsSherlock: function SRCH_ENG_parseAsSherlock() {
    












    function getSection(aLines, aSection) {
      LOG("_parseAsSherlock::getSection: Sherlock lines:\n" +
          aLines.join("\n"));
      var lines = aLines;
      var startMark = new RegExp("^\\s*<" + aSection.toLowerCase() + "\\s*",
                                 "gi");
      var endMark   = /\s*>\s*$/gi;

      var foundStart = false;
      var startLine, numberOfLines;
      
      for (var i = 0; i < lines.length; i++) {
        if (foundStart) {
          if (endMark.test(lines[i])) {
            numberOfLines = i - startLine;
            
            lines[i] = lines[i].replace(endMark, "");
            
            
            if (lines[i])
              numberOfLines++;
            break;
          }
        } else {
          if (startMark.test(lines[i])) {
            foundStart = true;
            
            lines[i] = lines[i].replace(startMark, "");
            startLine = i;
            
            if (!lines[i])
              startLine++;
          }
        }
      }
      LOG("_parseAsSherlock::getSection: Start index: " + startLine +
          "\nNumber of lines: " + numberOfLines);
      lines = lines.splice(startLine, numberOfLines);
      LOG("_parseAsSherlock::getSection: Section lines:\n" +
          lines.join("\n"));

      var section = {};
      for (var i = 0; i < lines.length; i++) {
        var line = lines[i].trim();

        var els = line.split("=");
        var name = els.shift().trim().toLowerCase();
        var value = els.join("=").trim();

        if (!name || !value)
          continue;

        
        
        value = value.replace(/^["']/, "")
                     .replace(/["']\s*[\\\/]?>?\s*$/, "") || "";
        value = value.trim();

        
        if (!(name in section))
          section[name] = value;
      }
      return section;
    }

    














    function getInputs(aLines) {

      


















      function getAttr(aAttr, aLine) {
        
        
        const userInput = /(\s|["'=])user(\s|[>="'\/\\+]|$)/i;

        LOG("_parseAsSherlock::getAttr: Getting attr: \"" +
            aAttr + "\" for line: \"" + aLine + "\"");
        
        
        var lLine = aLine.toLowerCase();
        var attr = aAttr.toLowerCase();

        var attrStart = lLine.search(new RegExp("\\s" + attr, "i"));
        if (attrStart == -1) {

          
          
          if (userInput.test(lLine) && attr == "value") {
            LOG("_parseAsSherlock::getAttr: Found user input!\nLine:\"" + lLine
                + "\"");
            return USER_DEFINED;
          }
          
          LOG("_parseAsSherlock::getAttr: Failed to find attribute:\nLine:\""
              + lLine + "\"\nAttr:\"" + attr + "\"");
          return "";
        }

        var valueStart = lLine.indexOf("=", attrStart) + "=".length;
        if (valueStart == -1)
          return "";

        var quoteStart = lLine.indexOf("\"", valueStart);
        if (quoteStart == -1) {

          
          
          
          return lLine.substr(valueStart).replace(/\s.*$/, "");

        } else {
          
          
          
          
          
          var betweenEqualAndQuote = lLine.substring(valueStart, quoteStart);
          if (/\S/.test(betweenEqualAndQuote))
            return lLine.substr(valueStart).replace(/\s.*$/, "");

          
          valueStart = quoteStart + "\"".length;
          
          var valueEnd = lLine.indexOf("\"", valueStart);
          
          if (valueEnd == -1)
            valueEnd = aLine.length;
        }
        return aLine.substring(valueStart, valueEnd);
      }

      var inputs = [];

      LOG("_parseAsSherlock::getInputs: Lines:\n" + aLines);
      
      let lines = aLines.filter(function (line) {
        return /^\s*<input/i.test(line);
      });
      LOG("_parseAsSherlock::getInputs: Filtered lines:\n" + lines);

      lines.forEach(function (line) {
        
        
        line = line.trim().replace(/^<input/i, "").replace(/>$/, "");

        
        const directionalInput = /^(prev|next)/i;
        if (directionalInput.test(line)) {

          
          line = line.replace(directionalInput, "");

          
          
          if (/name\s*=/i.test(line)) {
            line += " value=\"0\"";
          } else
            return; 
        }

        var attrName = getAttr("name", line);
        var attrValue = getAttr("value", line);
        LOG("_parseAsSherlock::getInputs: Got input:\nName:\"" + attrName +
            "\"\nValue:\"" + attrValue + "\"");
        if (attrValue)
          inputs.push([attrName, attrValue]);
      });
      return inputs;
    }

    function err(aErr) {
      FAIL("_parseAsSherlock::err: Sherlock param error:\n" + aErr,
           Cr.NS_ERROR_FAILURE);
    }

    
    
    
    var sherlockLines, searchSection, sourceTextEncoding, browserSection;
    try {
      sherlockLines = sherlockBytesToLines(this._data);
      searchSection = getSection(sherlockLines, "search");
      browserSection = getSection(sherlockLines, "browser");
      sourceTextEncoding = parseInt(searchSection["sourcetextencoding"]);
      if (sourceTextEncoding) {
        
        sherlockLines = sherlockBytesToLines(this._data, sourceTextEncoding);
        searchSection = getSection(sherlockLines, "search");
        browserSection = getSection(sherlockLines, "browser");
      }
    } catch (ex) {
      
      
      var asciiBytes = this._data.filter(function (n) {return !(0x80 & n);});
      var asciiString = String.fromCharCode.apply(null, asciiBytes);
      sherlockLines = asciiString.split(NEW_LINES).filter(isUsefulLine);
      searchSection = getSection(sherlockLines, "search");
      sourceTextEncoding = parseInt(searchSection["sourcetextencoding"]);
      if (sourceTextEncoding) {
        sherlockLines = sherlockBytesToLines(this._data, sourceTextEncoding);
        searchSection = getSection(sherlockLines, "search");
        browserSection = getSection(sherlockLines, "browser");
      } else
        ERROR("Couldn't find a working charset", Cr.NS_ERROR_FAILURE);
    }

    LOG("_parseAsSherlock: Search section:\n" + searchSection.toSource());

    this._name = searchSection["name"] || err("Missing name!");
    this._description = searchSection["description"] || "";
    this._queryCharset = searchSection["querycharset"] ||
                         queryCharsetFromCode(searchSection["queryencoding"]);
    this._searchForm = searchSection["searchform"];

    this._updateInterval = parseInt(browserSection["updatecheckdays"]);

    this._updateURL = browserSection["update"];
    this._iconUpdateURL = browserSection["updateicon"];

    var method = (searchSection["method"] || "GET").toUpperCase();
    var template = searchSection["action"] || err("Missing action!");

    var inputs = getInputs(sherlockLines);
    LOG("_parseAsSherlock: Inputs:\n" + inputs.toSource());

    var url = null;

    if (method == "GET") {
      
      
      
      
      
      
      for (var i = 0; i < inputs.length; i++) {
        var name  = inputs[i][0];
        var value = inputs[i][1];
        if (i==0) {
          if (name == "")
            template += USER_DEFINED;
          else
            template += "?" + name + "=" + value;
        } else if (name != "")
          template += "&" + name + "=" + value;
      }
      url = new EngineURL(URLTYPE_SEARCH_HTML, method, template);

    } else if (method == "POST") {
      
      url = new EngineURL(URLTYPE_SEARCH_HTML, method, template);
      for (var i = 0; i < inputs.length; i++) {
        var name  = inputs[i][0];
        var value = inputs[i][1];
        if (name)
          url.addParam(name, value);
      }
    } else
      err("Invalid method!");

    this._urls.push(url);
  },

  


  _initWithJSON: function SRCH_ENG__initWithJSON(aJson) {
    this.__id = aJson._id;
    this._name = aJson._name;
    this._description = aJson.description;
    if (aJson._hasPreferredIcon == undefined)
      this._hasPreferredIcon = true;
    else
      this._hasPreferredIcon = false;
    this._hidden = aJson._hidden;
    this._type = aJson.type || SEARCH_TYPE_MOZSEARCH;
    this._queryCharset = aJson.queryCharset || DEFAULT_QUERY_CHARSET;
    this.__searchForm = aJson.__searchForm;
    this.__installLocation = aJson._installLocation || SEARCH_APP_DIR;
    this._updateInterval = aJson._updateInterval || null;
    this._updateURL = aJson._updateURL || null;
    this._iconUpdateURL = aJson._iconUpdateURL || null;
    if (aJson._readOnly == undefined)
      this._readOnly = true;
    else
      this._readOnly = false;
    this._iconURI = makeURI(aJson._iconURL);
    this._iconMapObj = aJson._iconMapObj;
    if (aJson.extensionID) {
      this._extensionID = aJson.extensionID;
    }
    for (let i = 0; i < aJson._urls.length; ++i) {
      let url = aJson._urls[i];
      let engineURL = new EngineURL(url.type || URLTYPE_SEARCH_HTML,
                                    url.method || "GET", url.template,
                                    url.resultDomain);
      engineURL._initWithJSON(url, this);
      this._urls.push(engineURL);
    }
  },

  






  _serializeToJSON: function SRCH_ENG__serializeToJSON(aFilter) {
    var json = {
      _id: this._id,
      _name: this._name,
      _hidden: this.hidden,
      description: this.description,
      __searchForm: this.__searchForm,
      _iconURL: this._iconURL,
      _iconMapObj: this._iconMapObj,
      _urls: [url._serializeToJSON() for each(url in this._urls)]
    };

    if (this._file instanceof Ci.nsILocalFile)
      json.filePath = this._file.persistentDescriptor;
    if (this._uri)
      json._url = this._uri.spec;
    if (this._installLocation != SEARCH_APP_DIR || !aFilter)
      json._installLocation = this._installLocation;
    if (this._updateInterval || !aFilter)
      json._updateInterval = this._updateInterval;
    if (this._updateURL || !aFilter)
      json._updateURL = this._updateURL;
    if (this._iconUpdateURL || !aFilter)
      json._iconUpdateURL = this._iconUpdateURL;
    if (!this._hasPreferredIcon || !aFilter)
      json._hasPreferredIcon = this._hasPreferredIcon;
    if (this.type != SEARCH_TYPE_MOZSEARCH || !aFilter)
      json.type = this.type;
    if (this.queryCharset != DEFAULT_QUERY_CHARSET || !aFilter)
      json.queryCharset = this.queryCharset;
    if (this._dataType != SEARCH_DATA_XML || !aFilter)
      json._dataType = this._dataType;
    if (!this._readOnly || !aFilter)
      json._readOnly = this._readOnly;
    if (this._extensionID) {
      json.extensionID = this._extensionID;
    }

    return json;
  },

  



  _serializeToElement: function SRCH_ENG_serializeToEl() {
    function appendTextNode(aNameSpace, aLocalName, aValue) {
      if (!aValue)
        return null;
      var node = doc.createElementNS(aNameSpace, aLocalName);
      node.appendChild(doc.createTextNode(aValue));
      docElem.appendChild(node);
      docElem.appendChild(doc.createTextNode("\n"));
      return node;
    }

    var parser = Cc["@mozilla.org/xmlextras/domparser;1"].
                 createInstance(Ci.nsIDOMParser);

    var doc = parser.parseFromString(EMPTY_DOC, "text/xml");
    var docElem = doc.documentElement;

    docElem.appendChild(doc.createTextNode("\n"));

    appendTextNode(OPENSEARCH_NS_11, "ShortName", this.name);
    appendTextNode(OPENSEARCH_NS_11, "Description", this._description);
    appendTextNode(OPENSEARCH_NS_11, "InputEncoding", this._queryCharset);

    if (this._iconURI) {
      var imageNode = appendTextNode(OPENSEARCH_NS_11, "Image",
                                     this._iconURI.spec);
      if (imageNode) {
        imageNode.setAttribute("width", "16");
        imageNode.setAttribute("height", "16");
      }
    }

    appendTextNode(MOZSEARCH_NS_10, "UpdateInterval", this._updateInterval);
    appendTextNode(MOZSEARCH_NS_10, "UpdateUrl", this._updateURL);
    appendTextNode(MOZSEARCH_NS_10, "IconUpdateUrl", this._iconUpdateURL);
    appendTextNode(MOZSEARCH_NS_10, "SearchForm", this._searchForm);

    if (this._extensionID) {
      appendTextNode(MOZSEARCH_NS_10, "ExtensionID", this._extensionID);
    }

    for (var i = 0; i < this._urls.length; ++i)
      this._urls[i]._serializeToElement(doc, docElem);
    docElem.appendChild(doc.createTextNode("\n"));

    return doc;
  },

  get lazySerializeTask() {
    if (!this._lazySerializeTask) {
      let task = function taskCallback() {
        this._serializeToFile();
      }.bind(this);
      this._lazySerializeTask = new DeferredTask(task, LAZY_SERIALIZE_DELAY);
    }

    return this._lazySerializeTask;
  },

  


  _serializeToFile: function SRCH_ENG_serializeToFile() {
    var file = this._file;
    ENSURE_WARN(!this._readOnly, "Can't serialize a read only engine!",
                Cr.NS_ERROR_FAILURE);
    ENSURE_WARN(file && file.exists(), "Can't serialize: file doesn't exist!",
                Cr.NS_ERROR_UNEXPECTED);

    var fos = Cc["@mozilla.org/network/safe-file-output-stream;1"].
              createInstance(Ci.nsIFileOutputStream);

    
    
    var doc = this._serializeToElement();

    fos.init(file, (MODE_WRONLY | MODE_TRUNCATE), FileUtils.PERMS_FILE, 0);

    try {
      var serializer = Cc["@mozilla.org/xmlextras/xmlserializer;1"].
                       createInstance(Ci.nsIDOMSerializer);
      serializer.serializeToStream(doc.documentElement, fos, null);
    } catch (e) {
      LOG("_serializeToFile: Error serializing engine:\n" + e);
    }

    closeSafeOutputStream(fos);

    Services.obs.notifyObservers(file.clone(), SEARCH_SERVICE_TOPIC,
                                 "write-engine-to-disk-complete");
  },

  




  _remove: function SRCH_ENG_remove() {
    if (this._readOnly)
      FAIL("Can't remove read only engine!", Cr.NS_ERROR_FAILURE);
    if (!this._file || !this._file.exists())
      FAIL("Can't remove engine: file doesn't exist!", Cr.NS_ERROR_FILE_NOT_FOUND);

    this._file.remove(false);
  },

  
  get alias() {
    if (this._alias === undefined)
      this._alias = engineMetadataService.getAttr(this, "alias");

    return this._alias;
  },
  set alias(val) {
    this._alias = val;
    engineMetadataService.setAttr(this, "alias", val);
    notifyAction(this, SEARCH_ENGINE_CHANGED);
  },

  










  get identifier() {
    if (this._identifier !== undefined) {
      return this._identifier;
    }

    
    if (!this._isInAppDir && !this._isInJAR) {
      return this._identifier = null;
    }

    let leaf = this._getLeafName();
    ENSURE_WARN(leaf, "identifier: app-provided engine has no leafName");

    
    let ext = leaf.lastIndexOf(".");
    if (ext == -1) {
      return this._identifier = leaf;
    }
    return this._identifier = leaf.substring(0, ext);
  },

  get description() {
    return this._description;
  },

  get hidden() {
    if (this._hidden === null)
      this._hidden = engineMetadataService.getAttr(this, "hidden") || false;
    return this._hidden;
  },
  set hidden(val) {
    var value = !!val;
    if (value != this._hidden) {
      this._hidden = value;
      engineMetadataService.setAttr(this, "hidden", value);
      notifyAction(this, SEARCH_ENGINE_CHANGED);
    }
  },

  get iconURI() {
    if (this._iconURI)
      return this._iconURI;
    return null;
  },

  get _iconURL() {
    if (!this._iconURI)
      return "";
    return this._iconURI.spec;
  },

  
  
  
  get _location() {
    if (this._file)
      return this._file.path;

    if (this._uri)
      return this._uri.spec;

    return "";
  },

  



  _getLeafName: function () {
    if (this._file) {
      return this._file.leafName;
    }
    if (this._uri && this._uri instanceof Ci.nsIURL) {
      return this._uri.fileName;
    }
    return null;
  },

  
  
  __id: null,
  get _id() {
    if (this.__id) {
      return this.__id;
    }

    let leafName = this._getLeafName();

    
    
    
    
    
    
    
    
    
    if (this._isInAppDir || this._isInJAR) {
      
      ENSURE_WARN(leafName, "_id: no leafName for appDir or JAR engine",
                  Cr.NS_ERROR_UNEXPECTED);
      return this.__id = "[app]/" + leafName;
    }

    if (this._isInProfile) {
      ENSURE_WARN(leafName, "_id: no leafName for profile engine",
                  Cr.NS_ERROR_UNEXPECTED);
      return this.__id = "[profile]/" + leafName;
    }

    
    ENSURE_WARN(this._file, "_id: no _file for non-JAR engine",
                Cr.NS_ERROR_UNEXPECTED);

    
    
    return this.__id = this._file.path;
  },

  get _installLocation() {
    if (this.__installLocation === null) {
      if (!this._file) {
        ENSURE_WARN(this._uri, "Engines without files must have URIs",
                    Cr.NS_ERROR_UNEXPECTED);
        this.__installLocation = SEARCH_JAR;
      }
      else if (this._file.parent.equals(getDir(NS_APP_SEARCH_DIR)))
        this.__installLocation = SEARCH_APP_DIR;
      else if (this._file.parent.equals(getDir(NS_APP_USER_SEARCH_DIR)))
        this.__installLocation = SEARCH_PROFILE_DIR;
      else
        this.__installLocation = SEARCH_IN_EXTENSION;
    }

    return this.__installLocation;
  },

  get _isInJAR() {
    return this._installLocation == SEARCH_JAR;
  },
  get _isInAppDir() {
    return this._installLocation == SEARCH_APP_DIR;
  },
  get _isInProfile() {
    return this._installLocation == SEARCH_PROFILE_DIR;
  },

  get _isDefault() {
    
    
    
    return !this._isInProfile;
  },

  get _hasUpdates() {
    
    let selfURL = this._getURLOfType(URLTYPE_OPENSEARCH, "self");
    return !!(this._updateURL || this._iconUpdateURL || selfURL);
  },

  get name() {
    return this._name;
  },

  get type() {
    return this._type;
  },

  get searchForm() {
    
    var searchFormURL = this._getURLOfType(URLTYPE_SEARCH_HTML, "searchform");
    if (searchFormURL) {
      let submission = searchFormURL.getSubmission("", this);

      
      
      if (!submission.postData)
        return submission.uri.spec;
    }

    if (!this._searchForm) {
      
      
      var htmlUrl = this._getURLOfType(URLTYPE_SEARCH_HTML);
      ENSURE_WARN(htmlUrl, "Engine has no HTML URL!", Cr.NS_ERROR_UNEXPECTED);
      this._searchForm = makeURI(htmlUrl.template).prePath;
    }

    return ParamSubstitution(this._searchForm, "", this);
  },

  get queryCharset() {
    if (this._queryCharset)
      return this._queryCharset;
    return this._queryCharset = queryCharsetFromCode();
  },

  
  addParam: function SRCH_ENG_addParam(aName, aValue, aResponseType) {
    if (!aName || (aValue == null))
      FAIL("missing name or value for nsISearchEngine::addParam!");
    ENSURE_WARN(!this._readOnly,
                "called nsISearchEngine::addParam on a read-only engine!",
                Cr.NS_ERROR_FAILURE);
    if (!aResponseType)
      aResponseType = URLTYPE_SEARCH_HTML;

    var url = this._getURLOfType(aResponseType);
    if (!url)
      FAIL("Engine object has no URL for response type " + aResponseType,
           Cr.NS_ERROR_FAILURE);

    url.addParam(aName, aValue);

    
    this.lazySerializeTask.arm();
  },

#ifdef ANDROID
  get _defaultMobileResponseType() {
    let type = URLTYPE_SEARCH_HTML;

    let sysInfo = Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2);
    let isTablet = sysInfo.get("tablet");
    if (isTablet && this.supportsResponseType("application/x-moz-tabletsearch")) {
      
      type = "application/x-moz-tabletsearch";
    } else if (!isTablet && this.supportsResponseType("application/x-moz-phonesearch")) {
      
      type = "application/x-moz-phonesearch";
    }

    delete this._defaultMobileResponseType;
    return this._defaultMobileResponseType = type;
  },
#endif

  
  getSubmission: function SRCH_ENG_getSubmission(aData, aResponseType, aPurpose) {
#ifdef ANDROID
    if (!aResponseType) {
      aResponseType = this._defaultMobileResponseType;
    }
#endif
    if (!aResponseType) {
      aResponseType = URLTYPE_SEARCH_HTML;
    }

    var url = this._getURLOfType(aResponseType);

    if (!url)
      return null;

    if (!aData) {
      
      return new Submission(makeURI(this.searchForm), null);
    }

    LOG("getSubmission: In data: \"" + aData + "\"; Purpose: \"" + aPurpose + "\"");
    var data = "";
    try {
      data = gTextToSubURI.ConvertAndEscape(this.queryCharset, aData);
    } catch (ex) {
      LOG("getSubmission: Falling back to default queryCharset!");
      data = gTextToSubURI.ConvertAndEscape(DEFAULT_QUERY_CHARSET, aData);
    }
    LOG("getSubmission: Out data: \"" + data + "\"");
    return url.getSubmission(data, this, aPurpose);
  },

  
  supportsResponseType: function SRCH_ENG_supportsResponseType(type) {
    return (this._getURLOfType(type) != null);
  },

  
  getResultDomain: function SRCH_ENG_getResultDomain(aResponseType) {
#ifdef ANDROID
    if (!aResponseType) {
      aResponseType = this._defaultMobileResponseType;
    }
#endif
    if (!aResponseType) {
      aResponseType = URLTYPE_SEARCH_HTML;
    }

    LOG("getResultDomain: responseType: \"" + aResponseType + "\"");

    let url = this._getURLOfType(aResponseType);
    if (url)
      return url.resultDomain;
    return "";
  },

  


  getURLParsingInfo: function () {
#ifdef ANDROID
    let responseType = this._defaultMobileResponseType;
#else
    let responseType = URLTYPE_SEARCH_HTML;
#endif

    LOG("getURLParsingInfo: responseType: \"" + responseType + "\"");

    let url = this._getURLOfType(responseType);
    if (!url || url.method != "GET") {
      return null;
    }

    let termsParameterName = url._getTermsParameterName();
    if (!termsParameterName) {
      return null;
    }

    let templateUrl = NetUtil.newURI(url.template).QueryInterface(Ci.nsIURL);
    return {
      mainDomain: templateUrl.host,
      path: templateUrl.filePath.toLowerCase(),
      termsParameterName: termsParameterName,
    };
  },

  
  QueryInterface: function SRCH_ENG_QI(aIID) {
    if (aIID.equals(Ci.nsISearchEngine) ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  get wrappedJSObject() {
    return this;
  },

  








  getIconURLBySize: function SRCH_ENG_getIconURLBySize(aWidth, aHeight) {
    if (!this._iconMapObj)
      return null;

    let key = this._getIconKey(aWidth, aHeight);
    if (key in this._iconMapObj) {
      return this._iconMapObj[key];
    }
    return null;
  },

  





  getIcons: function SRCH_ENG_getIcons() {
    let result = [];

    if (!this._iconMapObj)
      return result;

    for (let key of Object.keys(this._iconMapObj)) {
      let iconSize = JSON.parse(key);
      result.push({
        width: iconSize.width,
        height: iconSize.height,
        url: this._iconMapObj[key]
      });
    }

    return result;
  },

  










  speculativeConnect: function SRCH_ENG_speculativeConnect(options) {
    if (!options || !options.window) {
      Cu.reportError("invalid options arg passed to nsISearchEngine.speculativeConnect");
      throw Cr.NS_ERROR_INVALID_ARG;
    }
    let connector =
        Services.io.QueryInterface(Components.interfaces.nsISpeculativeConnect);

    let searchURI = this.getSubmission("dummy").uri;

    let callbacks = options.window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                           .getInterface(Components.interfaces.nsIWebNavigation)
                           .QueryInterface(Components.interfaces.nsILoadContext);

    connector.speculativeConnect(searchURI, callbacks);

    if (this.supportsResponseType(URLTYPE_SUGGEST_JSON)) {
      let suggestURI = this.getSubmission("dummy", URLTYPE_SUGGEST_JSON).uri;
      if (suggestURI.prePath != searchURI.prePath)
        connector.speculativeConnect(suggestURI, callbacks);
    }
  },
};


function Submission(aURI, aPostData = null) {
  this._uri = aURI;
  this._postData = aPostData;
}
Submission.prototype = {
  get uri() {
    return this._uri;
  },
  get postData() {
    return this._postData;
  },
  QueryInterface: function SRCH_SUBM_QI(aIID) {
    if (aIID.equals(Ci.nsISearchSubmission) ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
}


function ParseSubmissionResult(aEngine, aTerms, aTermsOffset, aTermsLength) {
  this._engine = aEngine;
  this._terms = aTerms;
  this._termsOffset = aTermsOffset;
  this._termsLength = aTermsLength;
}
ParseSubmissionResult.prototype = {
  get engine() {
    return this._engine;
  },
  get terms() {
    return this._terms;
  },
  get termsOffset() {
    return this._termsOffset;
  },
  get termsLength() {
    return this._termsLength;
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISearchParseSubmissionResult]),
}

const gEmptyParseSubmissionResult =
      Object.freeze(new ParseSubmissionResult(null, "", -1, 0));

function executeSoon(func) {
  Services.tm.mainThread.dispatch(func, Ci.nsIThread.DISPATCH_NORMAL);
}









function checkForSyncCompletion(aPromise) {
  return aPromise.then(function(aValue) {
    if (gInitialized) {
      throw Components.Exception("Synchronous fallback was called and has " +
                                 "finished so no need to pursue asynchronous " +
                                 "initialization",
                                 Cr.NS_ERROR_ALREADY_INITIALIZED);
    }
    return aValue;
  });
}


function SearchService() {
  
  if (getBoolPref(BROWSER_SEARCH_PREF + "log", false))
    LOG = DO_LOG;

  this._initObservers = Promise.defer();
}

SearchService.prototype = {
  classID: Components.ID("{7319788a-fe93-4db3-9f39-818cf08f4256}"),

  
  
  _initRV: Cr.NS_OK,

  
  _initStarted: null,

  
  
  
  _ensureInitialized: function  SRCH_SVC__ensureInitialized() {
    if (gInitialized) {
      if (!Components.isSuccessCode(this._initRV)) {
        LOG("_ensureInitialized: failure");
        throw this._initRV;
      }
      return;
    }

    let warning =
      "Search service falling back to synchronous initialization. " +
      "This is generally the consequence of an add-on using a deprecated " +
      "search service API.";
    Deprecated.warning(warning, "https://developer.mozilla.org/en-US/docs/XPCOM_Interface_Reference/nsIBrowserSearchService#async_warning");
    LOG(warning);

    engineMetadataService.syncInit();
    this._syncInit();
    if (!Components.isSuccessCode(this._initRV)) {
      throw this._initRV;
    }
  },

  
  
  
  _syncInit: function SRCH_SVC__syncInit() {
    LOG("_syncInit start");
    this._initStarted = true;
    try {
      this._syncLoadEngines();
    } catch (ex) {
      this._initRV = Cr.NS_ERROR_FAILURE;
      LOG("_syncInit: failure loading engines: " + ex);
    }
    this._addObservers();

    gInitialized = true;

    this._initObservers.resolve(this._initRV);

    Services.obs.notifyObservers(null, SEARCH_SERVICE_TOPIC, "init-complete");
    Services.telemetry.getHistogramById("SEARCH_SERVICE_INIT_SYNC").add(true);

    LOG("_syncInit end");
  },

  





  _asyncInit: function SRCH_SVC__asyncInit() {
    return Task.spawn(function() {
      LOG("_asyncInit start");
      try {
        yield checkForSyncCompletion(this._asyncLoadEngines());
      } catch (ex if ex.result != Cr.NS_ERROR_ALREADY_INITIALIZED) {
        this._initRV = Cr.NS_ERROR_FAILURE;
        LOG("_asyncInit: failure loading engines: " + ex);
      }
      this._addObservers();
      gInitialized = true;
      this._initObservers.resolve(this._initRV);
      Services.obs.notifyObservers(null, SEARCH_SERVICE_TOPIC, "init-complete");
      Services.telemetry.getHistogramById("SEARCH_SERVICE_INIT_SYNC").add(false);

      LOG("_asyncInit: Completed _asyncInit");
    }.bind(this));
  },


  _engines: { },
  __sortedEngines: null,
  get _sortedEngines() {
    if (!this.__sortedEngines)
      return this._buildSortedEngineList();
    return this.__sortedEngines;
  },

  
  
  get _originalDefaultEngine() {
    let defaultPrefB = Services.prefs.getDefaultBranch(BROWSER_SEARCH_PREF);
    let nsIPLS = Ci.nsIPrefLocalizedString;
    let defaultEngine;
    try {
      defaultEngine = defaultPrefB.getComplexValue("defaultenginename", nsIPLS).data;
    } catch (ex) {
      
      
    }
    return this.getEngineByName(defaultEngine);
  },

  _buildCache: function SRCH_SVC__buildCache() {
    if (!getBoolPref(BROWSER_SEARCH_PREF + "cache.enabled", true))
      return;

    TelemetryStopwatch.start("SEARCH_SERVICE_BUILD_CACHE_MS");
    let cache = {};
    let locale = getLocale();
    let buildID = Services.appinfo.platformBuildID;

    
    cache.version = CACHE_VERSION;
    
    
    
    
    
    
    cache.buildID = buildID;
    cache.locale = locale;

    cache.directories = {};

    function getParent(engine) {
      if (engine._file)
        return engine._file.parent;

      let uri = engine._uri;
      if (!uri.schemeIs("chrome")) {
        LOG("getParent: engine URI must be a chrome URI if it has no file");
        return null;
      }

      
      try {
        uri = gChromeReg.convertChromeURL(uri);
        if (uri instanceof Ci.nsINestedURI)
          uri = uri.innermostURI;
        uri.QueryInterface(Ci.nsIFileURL)

        return uri.file;
      } catch (ex) {
        LOG("getParent: couldn't map chrome:// URI to a file: " + ex)
      }

      return null;
    }

    for each (let engine in this._engines) {
      let parent = getParent(engine);
      if (!parent) {
        LOG("Error: no parent for engine " + engine._location + ", failing to cache it");

        continue;
      }

      let cacheKey = parent.path;
      if (!cache.directories[cacheKey]) {
        let cacheEntry = {};
        cacheEntry.lastModifiedTime = parent.lastModifiedTime;
        cacheEntry.engines = [];
        cache.directories[cacheKey] = cacheEntry;
      }
      cache.directories[cacheKey].engines.push(engine._serializeToJSON(true));
    }

    try {
      LOG("_buildCache: Writing to cache file.");
      let path = OS.Path.join(OS.Constants.Path.profileDir, "search.json");
      let data = gEncoder.encode(JSON.stringify(cache));
      let promise = OS.File.writeAtomic(path, data, { tmpPath: path + ".tmp"});

      promise.then(
        function onSuccess() {
          Services.obs.notifyObservers(null, SEARCH_SERVICE_TOPIC, SEARCH_SERVICE_CACHE_WRITTEN);
        },
        function onError(e) {
          LOG("_buildCache: failure during writeAtomic: " + e);
        }
      );
    } catch (ex) {
      LOG("_buildCache: Could not write to cache file: " + ex);
    }
    TelemetryStopwatch.finish("SEARCH_SERVICE_BUILD_CACHE_MS");
  },

  _syncLoadEngines: function SRCH_SVC__syncLoadEngines() {
    LOG("_syncLoadEngines: start");
    
    let cache = {};
    let cacheEnabled = getBoolPref(BROWSER_SEARCH_PREF + "cache.enabled", true);
    if (cacheEnabled) {
      let cacheFile = getDir(NS_APP_USER_PROFILE_50_DIR);
      cacheFile.append("search.json");
      if (cacheFile.exists())
        cache = this._readCacheFile(cacheFile);
    }

    let loadDirs = [];
    let locations = getDir(NS_APP_SEARCH_DIR_LIST, Ci.nsISimpleEnumerator);
    while (locations.hasMoreElements()) {
      let dir = locations.getNext().QueryInterface(Ci.nsIFile);
      if (dir.directoryEntries.hasMoreElements())
        loadDirs.push(dir);
    }

    let loadFromJARs = getBoolPref(BROWSER_SEARCH_PREF + "loadFromJars", false);
    let chromeURIs = [];
    let chromeFiles = [];
    if (loadFromJARs)
      [chromeFiles, chromeURIs] = this._findJAREngines();

    let toLoad = chromeFiles.concat(loadDirs);

    function modifiedDir(aDir) {
      return (!cache.directories || !cache.directories[aDir.path] ||
              cache.directories[aDir.path].lastModifiedTime != aDir.lastModifiedTime);
    }

    function notInCachePath(aPathToLoad)
      cachePaths.indexOf(aPathToLoad.path) == -1;

    let buildID = Services.appinfo.platformBuildID;
    let cachePaths = [path for (path in cache.directories)];

    let rebuildCache = !cache.directories ||
                       cache.version != CACHE_VERSION ||
                       cache.locale != getLocale() ||
                       cache.buildID != buildID ||
                       cachePaths.length != toLoad.length ||
                       toLoad.some(notInCachePath) ||
                       toLoad.some(modifiedDir);

    if (!cacheEnabled || rebuildCache) {
      LOG("_loadEngines: Absent or outdated cache. Loading engines from disk.");
      loadDirs.forEach(this._loadEnginesFromDir, this);

      this._loadFromChromeURLs(chromeURIs);

      if (cacheEnabled)
        this._buildCache();
      return;
    }

    LOG("_loadEngines: loading from cache directories");
    for each (let dir in cache.directories)
      this._loadEnginesFromCache(dir);

    LOG("_loadEngines: done");
  },

  





  _asyncLoadEngines: function SRCH_SVC__asyncLoadEngines() {
    return Task.spawn(function() {
      LOG("_asyncLoadEngines: start");
      
      let cache = {};
      let cacheEnabled = getBoolPref(BROWSER_SEARCH_PREF + "cache.enabled", true);
      if (cacheEnabled) {
        let cacheFilePath = OS.Path.join(OS.Constants.Path.profileDir, "search.json");
        cache = yield checkForSyncCompletion(this._asyncReadCacheFile(cacheFilePath));
      }

      
      
      let loadDirs = [];
      let locations = getDir(NS_APP_SEARCH_DIR_LIST, Ci.nsISimpleEnumerator);
      while (locations.hasMoreElements()) {
        let dir = locations.getNext().QueryInterface(Ci.nsIFile);
        let iterator = new OS.File.DirectoryIterator(dir.path,
                                                     { winPattern: "*.xml" });
        try {
          
          yield checkForSyncCompletion(iterator.next());
          loadDirs.push(dir);
        } catch (ex if ex.result != Cr.NS_ERROR_ALREADY_INITIALIZED) {
          
        } finally {
          iterator.close();
        }
      }

      let loadFromJARs = getBoolPref(BROWSER_SEARCH_PREF + "loadFromJars", false);
      let chromeURIs = [];
      let chromeFiles = [];
      if (loadFromJARs) {
        Services.obs.notifyObservers(null, SEARCH_SERVICE_TOPIC, "find-jar-engines");
        [chromeFiles, chromeURIs] =
          yield checkForSyncCompletion(this._asyncFindJAREngines());
      }

      let toLoad = chromeFiles.concat(loadDirs);
      function hasModifiedDir(aList) {
        return Task.spawn(function() {
          let modifiedDir = false;

          for (let dir of aList) {
            if (!cache.directories || !cache.directories[dir.path]) {
              modifiedDir = true;
              break;
            }

            let info = yield OS.File.stat(dir.path);
            if (cache.directories[dir.path].lastModifiedTime !=
                info.lastModificationDate.getTime()) {
              modifiedDir = true;
              break;
            }
          }
          throw new Task.Result(modifiedDir);
        });
      }

      function notInCachePath(aPathToLoad)
        cachePaths.indexOf(aPathToLoad.path) == -1;

      let buildID = Services.appinfo.platformBuildID;
      let cachePaths = [path for (path in cache.directories)];

      let rebuildCache = !cache.directories ||
                         cache.version != CACHE_VERSION ||
                         cache.locale != getLocale() ||
                         cache.buildID != buildID ||
                         cachePaths.length != toLoad.length ||
                         toLoad.some(notInCachePath) ||
                         (yield checkForSyncCompletion(hasModifiedDir(toLoad)));

      if (!cacheEnabled || rebuildCache) {
        LOG("_asyncLoadEngines: Absent or outdated cache. Loading engines from disk.");
        let engines = [];
        for (let loadDir of loadDirs) {
          let enginesFromDir =
            yield checkForSyncCompletion(this._asyncLoadEnginesFromDir(loadDir));
          engines = engines.concat(enginesFromDir);
        }
        let enginesFromURLs =
           yield checkForSyncCompletion(this._asyncLoadFromChromeURLs(chromeURIs));
        engines = engines.concat(enginesFromURLs);

        for (let engine of engines) {
          this._addEngineToStore(engine);
        }
        if (cacheEnabled)
          this._buildCache();
        return;
      }

      LOG("_asyncLoadEngines: loading from cache directories");
      for each (let dir in cache.directories)
        this._loadEnginesFromCache(dir);

      LOG("_asyncLoadEngines: done");
    }.bind(this));
  },

  _asyncReInit: function () {
    
    gInitialized = false;

    
    this._engines = {};
    this.__sortedEngines = null;
    this._currentEngine = null;
    this._defaultEngine = null;

    
    engineMetadataService._initState = engineMetadataService._InitStates.NOT_STARTED;
    engineMetadataService._initializer = null;

    Task.spawn(function* () {
      try {
        yield engineMetadataService.init();
        yield this._asyncLoadEngines();

        
        
        Services.obs.notifyObservers(null, SEARCH_SERVICE_TOPIC, "reinit-complete");
        gInitialized = true;
      } catch (err) {
        LOG("Reinit failed: " + err);
        Services.obs.notifyObservers(null, SEARCH_SERVICE_TOPIC, "reinit-failed");
      }
    }.bind(this));
  },

  _readCacheFile: function SRCH_SVC__readCacheFile(aFile) {
    let stream = Cc["@mozilla.org/network/file-input-stream;1"].
                 createInstance(Ci.nsIFileInputStream);
    let json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);

    try {
      stream.init(aFile, MODE_RDONLY, FileUtils.PERMS_FILE, 0);
      return json.decodeFromStream(stream, stream.available());
    } catch (ex) {
      LOG("_readCacheFile: Error reading cache file: " + ex);
    } finally {
      stream.close();
    }
    return false;
  },

  







  _asyncReadCacheFile: function SRCH_SVC__asyncReadCacheFile(aPath) {
    return Task.spawn(function() {
      let json;
      try {
        let bytes = yield OS.File.read(aPath);
        json = JSON.parse(new TextDecoder().decode(bytes));
      } catch (ex) {
        LOG("_asyncReadCacheFile: Error reading cache file: " + ex);
        json = {};
      }
      throw new Task.Result(json);
    });
  },

  _batchTask: null,
  get batchTask() {
    if (!this._batchTask) {
      let task = function taskCallback() {
        LOG("batchTask: Invalidating engine cache");
        this._buildCache();
      }.bind(this);
      this._batchTask = new DeferredTask(task, CACHE_INVALIDATION_DELAY);
    }
    return this._batchTask;
  },

  _addEngineToStore: function SRCH_SVC_addEngineToStore(aEngine) {
    LOG("_addEngineToStore: Adding engine: \"" + aEngine.name + "\"");

    
    
    var hasSameNameAsUpdate = (aEngine._engineToUpdate &&
                               aEngine.name == aEngine._engineToUpdate.name);
    if (aEngine.name in this._engines && !hasSameNameAsUpdate) {
      LOG("_addEngineToStore: Duplicate engine found, aborting!");
      return;
    }

    if (aEngine._engineToUpdate) {
      
      var oldEngine = aEngine._engineToUpdate;

      
      
      delete this._engines[oldEngine.name];

      
      
      
      
      for (var p in aEngine) {
        if (!(aEngine.__lookupGetter__(p) || aEngine.__lookupSetter__(p)))
          oldEngine[p] = aEngine[p];
      }
      aEngine = oldEngine;
      aEngine._engineToUpdate = null;

      
      this._engines[aEngine.name] = aEngine;
      notifyAction(aEngine, SEARCH_ENGINE_CHANGED);
    } else {
      
      this._engines[aEngine.name] = aEngine;
      
      
      
      
      if (this.__sortedEngines) {
        this.__sortedEngines.push(aEngine);
        this._saveSortedEngineList();
      }
      notifyAction(aEngine, SEARCH_ENGINE_ADDED);
    }

    if (aEngine._hasUpdates) {
      
      if (!engineMetadataService.getAttr(aEngine, "updateexpir"))
        engineUpdateService.scheduleNextUpdate(aEngine);

      
      
      
      
      
      if (!engineMetadataService.getAttr(aEngine, "updatedatatype"))
        engineMetadataService.setAttr(aEngine, "updatedatatype",
                                      aEngine._dataType);
    }
  },

  _loadEnginesFromCache: function SRCH_SVC__loadEnginesFromCache(aDir) {
    let engines = aDir.engines;
    LOG("_loadEnginesFromCache: Loading from cache. " + engines.length + " engines to load.");
    for (let i = 0; i < engines.length; i++) {
      let json = engines[i];

      try {
        let engine;
        if (json.filePath)
          engine = new Engine({type: "filePath", value: json.filePath}, json._dataType,
                               json._readOnly);
        else if (json._url)
          engine = new Engine({type: "uri", value: json._url}, json._dataType, json._readOnly);

        engine._initWithJSON(json);
        this._addEngineToStore(engine);
      } catch (ex) {
        LOG("Failed to load " + engines[i]._name + " from cache: " + ex);
        LOG("Engine JSON: " + engines[i].toSource());
      }
    }
  },

  _loadEnginesFromDir: function SRCH_SVC__loadEnginesFromDir(aDir) {
    LOG("_loadEnginesFromDir: Searching in " + aDir.path + " for search engines.");

    
    var isInProfile = aDir.equals(getDir(NS_APP_USER_SEARCH_DIR));

    var files = aDir.directoryEntries
                    .QueryInterface(Ci.nsIDirectoryEnumerator);

    while (files.hasMoreElements()) {
      var file = files.nextFile;

      
      if (!file.isFile() || file.fileSize == 0 || file.isHidden())
        continue;

      var fileURL = NetUtil.ioService.newFileURI(file).QueryInterface(Ci.nsIURL);
      var fileExtension = fileURL.fileExtension.toLowerCase();
      var isWritable = isInProfile && file.isWritable();

      if (fileExtension != "xml") {
        
        continue;
      }

      var addedEngine = null;
      try {
        addedEngine = new Engine(file, SEARCH_DATA_XML, !isWritable);
        addedEngine._initFromFile();
      } catch (ex) {
        LOG("_loadEnginesFromDir: Failed to load " + file.path + "!\n" + ex);
        continue;
      }

      this._addEngineToStore(addedEngine);
    }
  },

  







  _asyncLoadEnginesFromDir: function SRCH_SVC__asyncLoadEnginesFromDir(aDir) {
    LOG("_asyncLoadEnginesFromDir: Searching in " + aDir.path + " for search engines.");

    
    let isInProfile = aDir.equals(getDir(NS_APP_USER_SEARCH_DIR));
    let iterator = new OS.File.DirectoryIterator(aDir.path);
    return Task.spawn(function() {
      let osfiles = yield iterator.nextBatch();
      iterator.close();

      let engines = [];
      for (let osfile of osfiles) {
        if (osfile.isDir || osfile.isSymLink)
          continue;

        let fileInfo = yield OS.File.stat(osfile.path);
        if (fileInfo.size == 0)
          continue;

        let parts = osfile.path.split(".");
        if (parts.length <= 1 || (parts.pop()).toLowerCase() != "xml") {
          
          continue;
        }

        let addedEngine = null;
        try {
          let file = new FileUtils.File(osfile.path);
          let isWritable = isInProfile;
          addedEngine = new Engine(file, SEARCH_DATA_XML, !isWritable);
          yield checkForSyncCompletion(addedEngine._asyncInitFromFile());
        } catch (ex if ex.result != Cr.NS_ERROR_ALREADY_INITIALIZED) {
          LOG("_asyncLoadEnginesFromDir: Failed to load " + osfile.path + "!\n" + ex);
          continue;
        }
        engines.push(addedEngine);
      }
      throw new Task.Result(engines);
    }.bind(this));
  },

  _loadFromChromeURLs: function SRCH_SVC_loadFromChromeURLs(aURLs) {
    aURLs.forEach(function (url) {
      try {
        LOG("_loadFromChromeURLs: loading engine from chrome url: " + url);

        let engine = new Engine(makeURI(url), SEARCH_DATA_XML, true);

        engine._initFromURISync();

        this._addEngineToStore(engine);
      } catch (ex) {
        LOG("_loadFromChromeURLs: failed to load engine: " + ex);
      }
    }, this);
  },

  







  _asyncLoadFromChromeURLs: function SRCH_SVC__asyncLoadFromChromeURLs(aURLs) {
    return Task.spawn(function() {
      let engines = [];
      for (let url of aURLs) {
        try {
          LOG("_asyncLoadFromChromeURLs: loading engine from chrome url: " + url);
          let engine = new Engine(NetUtil.newURI(url), SEARCH_DATA_XML, true);
          yield checkForSyncCompletion(engine._asyncInitFromURI());
          engines.push(engine);
        } catch (ex if ex.result != Cr.NS_ERROR_ALREADY_INITIALIZED) {
          LOG("_asyncLoadFromChromeURLs: failed to load engine: " + ex);
        }
      }
      throw new Task.Result(engines);
    }.bind(this));
  },

  _findJAREngines: function SRCH_SVC_findJAREngines() {
    LOG("_findJAREngines: looking for engines in JARs")

    let rootURIPref = ""
    try {
      rootURIPref = Services.prefs.getCharPref(BROWSER_SEARCH_PREF + "jarURIs");
    } catch (ex) {}

    if (!rootURIPref) {
      LOG("_findJAREngines: no JAR URIs were specified");

      return [[], []];
    }

    let rootURIs = rootURIPref.split(",");
    let uris = [];
    let chromeFiles = [];

    rootURIs.forEach(function (root) {
      
      
      let chromeFile;
      try {
        let chromeURI = gChromeReg.convertChromeURL(makeURI(root));
        let fileURI = chromeURI; 
        while (fileURI instanceof Ci.nsIJARURI)
          fileURI = fileURI.JARFile; 
        fileURI.QueryInterface(Ci.nsIFileURL);
        chromeFile = fileURI.file;
      } catch (ex) {
        LOG("_findJAREngines: failed to get chromeFile for " + root + ": " + ex);
      }

      if (!chromeFile)
        return;

      chromeFiles.push(chromeFile);

      
      
      let listURL = root + "list.txt";
      let names = [];
      try {
        let chan = NetUtil.ioService.newChannelFromURI(makeURI(listURL));
        let sis = Cc["@mozilla.org/scriptableinputstream;1"].
                  createInstance(Ci.nsIScriptableInputStream);
        sis.init(chan.open());
        let list = sis.read(sis.available());
        names = list.split("\n").filter(function (n) !!n);
      } catch (ex) {
        LOG("_findJAREngines: failed to retrieve list.txt from " + listURL + ": " + ex);

        return;
      }

      names.forEach(function (n) uris.push(root + n + ".xml"));
    });

    return [chromeFiles, uris];
  },

  





  _asyncFindJAREngines: function SRCH_SVC__asyncFindJAREngines() {
    return Task.spawn(function() {
      LOG("_asyncFindJAREngines: looking for engines in JARs")

      let rootURIPref = "";
      try {
        rootURIPref = Services.prefs.getCharPref(BROWSER_SEARCH_PREF + "jarURIs");
      } catch (ex) {}

      if (!rootURIPref) {
        LOG("_asyncFindJAREngines: no JAR URIs were specified");
        throw new Task.Result([[], []]);
      }

      let rootURIs = rootURIPref.split(",");
      let uris = [];
      let chromeFiles = [];

      for (let root of rootURIs) {
        
        
        let chromeFile;
        try {
          let chromeURI = gChromeReg.convertChromeURL(makeURI(root));
          let fileURI = chromeURI; 
          while (fileURI instanceof Ci.nsIJARURI)
            fileURI = fileURI.JARFile; 
          fileURI.QueryInterface(Ci.nsIFileURL);
          chromeFile = fileURI.file;
        } catch (ex) {
          LOG("_asyncFindJAREngines: failed to get chromeFile for " + root + ": " + ex);
        }

        if (!chromeFile) {
          return;
        }

        chromeFiles.push(chromeFile);

        
        
        let listURL = root + "list.txt";
        let deferred = Promise.defer();
        let request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
                        createInstance(Ci.nsIXMLHttpRequest);
        request.onload = function(aEvent) {
          deferred.resolve(aEvent.target.responseText);
        };
        request.onerror = function(aEvent) {
          LOG("_asyncFindJAREngines: failed to retrieve list.txt from " + listURL);
          deferred.resolve("");
        };
        request.open("GET", NetUtil.newURI(listURL).spec, true);
        request.send();
        let list = yield deferred.promise;

        let names = [];
        names = list.split("\n").filter(function (n) !!n);
        names.forEach(function (n) uris.push(root + n + ".xml"));
      }
      throw new Task.Result([chromeFiles, uris]);
    });
  },


  _saveSortedEngineList: function SRCH_SVC_saveSortedEngineList() {
    LOG("SRCH_SVC_saveSortedEngineList: starting");

    
    
    Services.prefs.setBoolPref(BROWSER_SEARCH_PREF + "useDBForOrder", true);

    var engines = this._getSortedEngines(true);

    let instructions = [];
    for (var i = 0; i < engines.length; ++i) {
      instructions.push(
        {key: "order",
         value: i+1,
         engine: engines[i]
        });
    }

    engineMetadataService.setAttrs(instructions);
    LOG("SRCH_SVC_saveSortedEngineList: done");
  },

  _buildSortedEngineList: function SRCH_SVC_buildSortedEngineList() {
    LOG("_buildSortedEngineList: building list");
    var addedEngines = { };
    this.__sortedEngines = [];
    var engine;

    
    
    
    if (getBoolPref(BROWSER_SEARCH_PREF + "useDBForOrder", false)) {
      LOG("_buildSortedEngineList: using db for order");

      
      let needToSaveEngineList = false;

      for each (engine in this._engines) {
        var orderNumber = engineMetadataService.getAttr(engine, "order");

        
        
        
        
        if (orderNumber && !this.__sortedEngines[orderNumber-1]) {
          this.__sortedEngines[orderNumber-1] = engine;
          addedEngines[engine.name] = engine;
        } else {
          
          needToSaveEngineList = true;
        }
      }

      
      var filteredEngines = this.__sortedEngines.filter(function(a) { return !!a; });
      if (this.__sortedEngines.length != filteredEngines.length)
        needToSaveEngineList = true;
      this.__sortedEngines = filteredEngines;

      if (needToSaveEngineList)
        this._saveSortedEngineList();
    } else {
      
      var i = 0;
      var engineName;
      var prefName;

      try {
        var extras =
          Services.prefs.getChildList(BROWSER_SEARCH_PREF + "order.extra.");

        for each (prefName in extras) {
          engineName = Services.prefs.getCharPref(prefName);

          engine = this._engines[engineName];
          if (!engine || engine.name in addedEngines)
            continue;

          this.__sortedEngines.push(engine);
          addedEngines[engine.name] = engine;
        }
      }
      catch (e) { }

      while (true) {
        engineName = getLocalizedPref(BROWSER_SEARCH_PREF + "order." + (++i));
        if (!engineName)
          break;

        engine = this._engines[engineName];
        if (!engine || engine.name in addedEngines)
          continue;

        this.__sortedEngines.push(engine);
        addedEngines[engine.name] = engine;
      }
    }

    
    var alphaEngines = [];

    for each (engine in this._engines) {
      if (!(engine.name in addedEngines))
        alphaEngines.push(this._engines[engine.name]);
    }
    alphaEngines = alphaEngines.sort(function (a, b) {
                                       return a.name.localeCompare(b.name);
                                     });
    return this.__sortedEngines = this.__sortedEngines.concat(alphaEngines);
  },

  




  _getSortedEngines: function SRCH_SVC_getSorted(aWithHidden) {
    if (aWithHidden)
      return this._sortedEngines;

    return this._sortedEngines.filter(function (engine) {
                                        return !engine.hidden;
                                      });
  },

  _getVerificationHash: function SRCH_SVC__getVerificationHash(aName) {
    let disclaimer = "By modifying this file, I agree that I am doing so " +
      "only within $appName itself, using official, user-driven search " +
      "engine selection processes, and in a way which does not circumvent " +
      "user consent. I acknowledge that any attempt to change this file " +
      "from outside of $appName is a malicious act, and will be responded " +
      "to accordingly."

    let salt = OS.Path.basename(OS.Constants.Path.profileDir) + aName +
               disclaimer.replace(/\$appName/g, Services.appinfo.name);

    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                      .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    
    let data = converter.convertToByteArray(salt, {});
    let hasher = Cc["@mozilla.org/security/hash;1"]
                   .createInstance(Ci.nsICryptoHash);
    hasher.init(hasher.SHA256);
    hasher.update(data, data.length);

    return hasher.finish(true);
  },

  
  init: function SRCH_SVC_init(observer) {
    LOG("SearchService.init");
    let self = this;
    if (!this._initStarted) {
      TelemetryStopwatch.start("SEARCH_SERVICE_INIT_MS");
      this._initStarted = true;
      Task.spawn(function task() {
        try {
          yield checkForSyncCompletion(engineMetadataService.init());
          
          yield self._asyncInit();
          TelemetryStopwatch.finish("SEARCH_SERVICE_INIT_MS");
        } catch (ex if ex.result == Cr.NS_ERROR_ALREADY_INITIALIZED) {
          
          
          TelemetryStopwatch.finish("SEARCH_SERVICE_INIT_MS");
        } catch (ex) {
          self._initObservers.reject(ex);
          TelemetryStopwatch.cancel("SEARCH_SERVICE_INIT_MS");
        }
      });
    }
    if (observer) {
      this._initObservers.promise.then(
        function onSuccess() {
          observer.onInitComplete(self._initRV);
        },
        function onError(aReason) {
          Components.utils.reportError("Internal error while initializing SearchService: " + aReason);
          observer.onInitComplete(Components.results.NS_ERROR_UNEXPECTED);
        }
      );
    }
  },

  get isInitialized() {
    return gInitialized;
  },

  getEngines: function SRCH_SVC_getEngines(aCount) {
    this._ensureInitialized();
    LOG("getEngines: getting all engines");
    var engines = this._getSortedEngines(true);
    aCount.value = engines.length;
    return engines;
  },

  getVisibleEngines: function SRCH_SVC_getVisible(aCount) {
    this._ensureInitialized();
    LOG("getVisibleEngines: getting all visible engines");
    var engines = this._getSortedEngines(false);
    aCount.value = engines.length;
    return engines;
  },

  getDefaultEngines: function SRCH_SVC_getDefault(aCount) {
    this._ensureInitialized();
    function isDefault(engine) {
      return engine._isDefault;
    };
    var engines = this._sortedEngines.filter(isDefault);
    var engineOrder = {};
    var engineName;
    var i = 1;

    
    
    

    
    try {
      var extras = Services.prefs.getChildList(BROWSER_SEARCH_PREF + "order.extra.");

      for each (var prefName in extras) {
        engineName = Services.prefs.getCharPref(prefName);

        if (!(engineName in engineOrder))
          engineOrder[engineName] = i++;
      }
    } catch (e) {
      LOG("Getting extra order prefs failed: " + e);
    }

    
    for (var j = 1; ; j++) {
      engineName = getLocalizedPref(BROWSER_SEARCH_PREF + "order." + j);
      if (!engineName)
        break;

      if (!(engineName in engineOrder))
        engineOrder[engineName] = i++;
    }

    LOG("getDefaultEngines: engineOrder: " + engineOrder.toSource());

    function compareEngines (a, b) {
      var aIdx = engineOrder[a.name];
      var bIdx = engineOrder[b.name];

      if (aIdx && bIdx)
        return aIdx - bIdx;
      if (aIdx)
        return -1;
      if (bIdx)
        return 1;

      return a.name.localeCompare(b.name);
    }
    engines.sort(compareEngines);

    aCount.value = engines.length;
    return engines;
  },

  getEngineByName: function SRCH_SVC_getEngineByName(aEngineName) {
    this._ensureInitialized();
    return this._engines[aEngineName] || null;
  },

  getEngineByAlias: function SRCH_SVC_getEngineByAlias(aAlias) {
    this._ensureInitialized();
    for (var engineName in this._engines) {
      var engine = this._engines[engineName];
      if (engine && engine.alias == aAlias)
        return engine;
    }
    return null;
  },

  addEngineWithDetails: function SRCH_SVC_addEWD(aName, aIconURL, aAlias,
                                                 aDescription, aMethod,
                                                 aTemplate, aExtensionID) {
    this._ensureInitialized();
    if (!aName)
      FAIL("Invalid name passed to addEngineWithDetails!");
    if (!aMethod)
      FAIL("Invalid method passed to addEngineWithDetails!");
    if (!aTemplate)
      FAIL("Invalid template passed to addEngineWithDetails!");
    if (this._engines[aName])
      FAIL("An engine with that name already exists!", Cr.NS_ERROR_FILE_ALREADY_EXISTS);

    var engine = new Engine(getSanitizedFile(aName), SEARCH_DATA_XML, false);
    engine._initFromMetadata(aName, aIconURL, aAlias, aDescription,
                             aMethod, aTemplate, aExtensionID);
    this._addEngineToStore(engine);
  },

  addEngine: function SRCH_SVC_addEngine(aEngineURL, aDataType, aIconURL,
                                         aConfirm, aCallback) {
    LOG("addEngine: Adding \"" + aEngineURL + "\".");
    this._ensureInitialized();
    try {
      var uri = makeURI(aEngineURL);
      var engine = new Engine(uri, aDataType, false);
      if (aCallback) {
        engine._installCallback = function (errorCode) {
          try {
            if (errorCode == null)
              aCallback.onSuccess(engine);
            else
              aCallback.onError(errorCode);
          } catch (ex) {
            Cu.reportError("Error invoking addEngine install callback: " + ex);
          }
          
          engine._installCallback = null;
        };
      }
      engine._initFromURIAndLoad();
    } catch (ex) {
      
      if (engine)
        engine._installCallback = null;
      FAIL("addEngine: Error adding engine:\n" + ex, Cr.NS_ERROR_FAILURE);
    }
    engine._setIcon(aIconURL, false);
    engine._confirm = aConfirm;
  },

  removeEngine: function SRCH_SVC_removeEngine(aEngine) {
    this._ensureInitialized();
    if (!aEngine)
      FAIL("no engine passed to removeEngine!");

    var engineToRemove = null;
    for (var e in this._engines) {
      if (aEngine.wrappedJSObject == this._engines[e])
        engineToRemove = this._engines[e];
    }

    if (!engineToRemove)
      FAIL("removeEngine: Can't find engine to remove!", Cr.NS_ERROR_FILE_NOT_FOUND);

    if (engineToRemove == this.currentEngine) {
      this._currentEngine = null;
    }

    if (engineToRemove == this.defaultEngine) {
      this._defaultEngine = null;
    }

    if (engineToRemove._readOnly) {
      
      
      engineToRemove.hidden = true;
      engineToRemove.alias = null;
    } else {
      
      
      if (engineToRemove._lazySerializeTask) {
        engineToRemove._lazySerializeTask.disarm();
        engineToRemove._lazySerializeTask = null;
      }

      
      engineToRemove._remove();
      engineToRemove._file = null;

      
      var index = this._sortedEngines.indexOf(engineToRemove);
      if (index == -1)
        FAIL("Can't find engine to remove in _sortedEngines!", Cr.NS_ERROR_FAILURE);
      this.__sortedEngines.splice(index, 1);

      
      delete this._engines[engineToRemove.name];

      notifyAction(engineToRemove, SEARCH_ENGINE_REMOVED);

      
      this._saveSortedEngineList();
    }
  },

  moveEngine: function SRCH_SVC_moveEngine(aEngine, aNewIndex) {
    this._ensureInitialized();
    if ((aNewIndex > this._sortedEngines.length) || (aNewIndex < 0))
      FAIL("SRCH_SVC_moveEngine: Index out of bounds!");
    if (!(aEngine instanceof Ci.nsISearchEngine))
      FAIL("SRCH_SVC_moveEngine: Invalid engine passed to moveEngine!");
    if (aEngine.hidden)
      FAIL("moveEngine: Can't move a hidden engine!", Cr.NS_ERROR_FAILURE);

    var engine = aEngine.wrappedJSObject;

    var currentIndex = this._sortedEngines.indexOf(engine);
    if (currentIndex == -1)
      FAIL("moveEngine: Can't find engine to move!", Cr.NS_ERROR_UNEXPECTED);

    
    
    
    
    
    
    
    
    
    
    
    var newIndexEngine = this._getSortedEngines(false)[aNewIndex];
    if (!newIndexEngine)
      FAIL("moveEngine: Can't find engine to replace!", Cr.NS_ERROR_UNEXPECTED);

    for (var i = 0; i < this._sortedEngines.length; ++i) {
      if (newIndexEngine == this._sortedEngines[i])
        break;
      if (this._sortedEngines[i].hidden)
        aNewIndex++;
    }

    if (currentIndex == aNewIndex)
      return; 

    
    var movedEngine = this.__sortedEngines.splice(currentIndex, 1)[0];
    this.__sortedEngines.splice(aNewIndex, 0, movedEngine);

    notifyAction(engine, SEARCH_ENGINE_CHANGED);

    
    this._saveSortedEngineList();
  },

  restoreDefaultEngines: function SRCH_SVC_resetDefaultEngines() {
    this._ensureInitialized();
    for each (var e in this._engines) {
      
      if (e.hidden && e._isDefault)
        e.hidden = false;
    }
  },

  get defaultEngine() {
    this._ensureInitialized();
    if (!this._defaultEngine) {
      let defPref = BROWSER_SEARCH_PREF + "defaultenginename";
      let defaultEngine = this.getEngineByName(getLocalizedPref(defPref, ""))
      if (!defaultEngine)
        defaultEngine = this._getSortedEngines(false)[0] || null;
      this._defaultEngine = defaultEngine;
    }
    if (this._defaultEngine.hidden)
      return this._getSortedEngines(false)[0];
    return this._defaultEngine;
  },

  set defaultEngine(val) {
    this._ensureInitialized();
    
    
    
    if (!(val instanceof Ci.nsISearchEngine) && !(val instanceof Engine))
      FAIL("Invalid argument passed to defaultEngine setter");

    let newDefaultEngine = this.getEngineByName(val.name);
    if (!newDefaultEngine)
      FAIL("Can't find engine in store!", Cr.NS_ERROR_UNEXPECTED);

    if (newDefaultEngine == this._defaultEngine)
      return;

    this._defaultEngine = newDefaultEngine;

    let defPref = BROWSER_SEARCH_PREF + "defaultenginename";
    
    
    
    
    
    if (this._defaultEngine == this._originalDefaultEngine) {
      Services.prefs.clearUserPref(defPref);
    }
    else {
      setLocalizedPref(defPref, this._defaultEngine.name);
    }

    notifyAction(this._defaultEngine, SEARCH_ENGINE_DEFAULT);
  },

  get currentEngine() {
    this._ensureInitialized();
    if (!this._currentEngine) {
      let name = engineMetadataService.getGlobalAttr("current");
      if (engineMetadataService.getGlobalAttr("hash") == this._getVerificationHash(name)) {
        this._currentEngine = this.getEngineByName(name);
      }
    }

    if (!this._currentEngine || this._currentEngine.hidden)
      this._currentEngine = this._originalDefaultEngine;
    if (!this._currentEngine || this._currentEngine.hidden)
      this._currentEngine = this._getSortedEngines(false)[0];
    return this._currentEngine;
  },

  set currentEngine(val) {
    this._ensureInitialized();
    
    
    
    if (!(val instanceof Ci.nsISearchEngine) && !(val instanceof Engine))
      FAIL("Invalid argument passed to currentEngine setter");

    var newCurrentEngine = this.getEngineByName(val.name);
    if (!newCurrentEngine)
      FAIL("Can't find engine in store!", Cr.NS_ERROR_UNEXPECTED);

    if (newCurrentEngine == this._currentEngine)
      return;

    this._currentEngine = newCurrentEngine;

    
    
    
    
    
    let newName = this._currentEngine.name;
    if (this._currentEngine == this._originalDefaultEngine) {
      newName = "";
    }

    engineMetadataService.setGlobalAttr("current", newName);
    engineMetadataService.setGlobalAttr("hash", this._getVerificationHash(newName));

    notifyAction(this._currentEngine, SEARCH_ENGINE_CURRENT);
  },

  














  _parseSubmissionMap: null,

  _buildParseSubmissionMap: function SRCH_SVC__buildParseSubmissionMap() {
    LOG("_buildParseSubmissionMap");
    this._parseSubmissionMap = new Map();

    
    
    
    let keysOfAlternates = new Set();

    for (let engine of this._sortedEngines) {
      LOG("Processing engine: " + engine.name);

      if (engine.hidden) {
        LOG("Engine is hidden.");
        continue;
      }

      let urlParsingInfo = engine.getURLParsingInfo();
      if (!urlParsingInfo) {
        LOG("Engine does not support URL parsing.");
        continue;
      }

      
      let mapValueForEngine = {
        engine: engine,
        termsParameterName: urlParsingInfo.termsParameterName,
      };

      let processDomain = (domain, isAlternate) => {
        let key = domain + urlParsingInfo.path;

        
        
        let existingEntry = this._parseSubmissionMap.get(key);
        if (!existingEntry) {
          LOG("Adding new entry: " + key);
          if (isAlternate) {
            keysOfAlternates.add(key);
          }
        } else if (!isAlternate && keysOfAlternates.has(key)) {
          LOG("Overriding alternate entry: " + key +
              " (" + existingEntry.engine.name + ")");
          keysOfAlternates.delete(key);
        } else {
          LOG("Keeping existing entry: " + key +
              " (" + existingEntry.engine.name + ")");
          return;
        }

        this._parseSubmissionMap.set(key, mapValueForEngine);
      };

      processDomain(urlParsingInfo.mainDomain, false);
      SearchStaticData.getAlternateDomains(urlParsingInfo.mainDomain)
                      .forEach(d => processDomain(d, true));
    }
  },

  parseSubmissionURL: function SRCH_SVC_parseSubmissionURL(aURL) {
    this._ensureInitialized();
    LOG("parseSubmissionURL: Parsing \"" + aURL + "\".");

    if (!this._parseSubmissionMap) {
      this._buildParseSubmissionMap();
    }

    
    let soughtKey, soughtQuery;
    try {
      let soughtUrl = NetUtil.newURI(aURL).QueryInterface(Ci.nsIURL);

      
      if (soughtUrl.scheme != "http" && soughtUrl.scheme != "https") {
        LOG("The URL scheme is not HTTP or HTTPS.");
        return gEmptyParseSubmissionResult;
      }

      
      soughtKey = soughtUrl.host + soughtUrl.filePath.toLowerCase();
      soughtQuery = soughtUrl.query;
    } catch (ex) {
      
      LOG("The value does not look like a structured URL.");
      return gEmptyParseSubmissionResult;
    }

    
    let mapEntry = this._parseSubmissionMap.get(soughtKey);
    if (!mapEntry) {
      LOG("No engine associated with domain and path: " + soughtKey);
      return gEmptyParseSubmissionResult;
    }

    
    
    let encodedTerms = null;
    for (let param of soughtQuery.split("&")) {
      let equalPos = param.indexOf("=");
      if (equalPos != -1 &&
          param.substr(0, equalPos) == mapEntry.termsParameterName) {
        
        encodedTerms = param.substr(equalPos + 1);
        break;
      }
    }
    if (encodedTerms === null) {
      LOG("Missing terms parameter: " + mapEntry.termsParameterName);
      return gEmptyParseSubmissionResult;
    }

    let length = 0;
    let offset = aURL.indexOf("?") + 1;
    let query = aURL.slice(offset);
    
    
    for (let param of query.split("&")) {
      let equalPos = param.indexOf("=");
      if (equalPos != -1 &&
          param.substr(0, equalPos) == mapEntry.termsParameterName) {
        
        offset += equalPos + 1;
        length = param.length - equalPos - 1;
        break;
      }
      offset += param.length + 1;
    }

    
    let terms;
    try {
      terms = gTextToSubURI.UnEscapeAndConvert(
                                       mapEntry.engine.queryCharset,
                                       encodedTerms.replace("+", " ", "g"));
    } catch (ex) {
      
      LOG("Parameter decoding failed. Charset: " +
          mapEntry.engine.queryCharset);
      return gEmptyParseSubmissionResult;
    }

    LOG("Match found. Terms: " + terms);
    return new ParseSubmissionResult(mapEntry.engine, terms, offset, length);
  },

  
  observe: function SRCH_SVC_observe(aEngine, aTopic, aVerb) {
    switch (aTopic) {
      case SEARCH_ENGINE_TOPIC:
        switch (aVerb) {
          case SEARCH_ENGINE_LOADED:
            var engine = aEngine.QueryInterface(Ci.nsISearchEngine);
            LOG("nsSearchService::observe: Done installation of " + engine.name
                + ".");
            this._addEngineToStore(engine.wrappedJSObject);
            if (engine.wrappedJSObject._useNow) {
              LOG("nsSearchService::observe: setting current");
              this.currentEngine = aEngine;
            }
            
            
            break;
          case SEARCH_ENGINE_ADDED:
          case SEARCH_ENGINE_CHANGED:
          case SEARCH_ENGINE_REMOVED:
            this.batchTask.disarm();
            this.batchTask.arm();
            
            this._parseSubmissionMap = null;
            break;
        }
        break;

      case QUIT_APPLICATION_TOPIC:
        this._removeObservers();
        break;

      case "nsPref:changed":
        if (aVerb == LOCALE_PREF) {
          
          
          this._asyncReInit();
          break;
        }
    }
  },

  
  notify: function SRCH_SVC_notify(aTimer) {
    LOG("_notify: checking for updates");

    if (!getBoolPref(BROWSER_SEARCH_PREF + "update", true))
      return;

    
    
    var currentTime = Date.now();
    LOG("currentTime: " + currentTime);
    for each (let engine in this._engines) {
      engine = engine.wrappedJSObject;
      if (!engine._hasUpdates)
        continue;

      LOG("checking " + engine.name);

      var expirTime = engineMetadataService.getAttr(engine, "updateexpir");
      LOG("expirTime: " + expirTime + "\nupdateURL: " + engine._updateURL +
          "\niconUpdateURL: " + engine._iconUpdateURL);

      var engineExpired = expirTime <= currentTime;

      if (!expirTime || !engineExpired) {
        LOG("skipping engine");
        continue;
      }

      LOG(engine.name + " has expired");

      engineUpdateService.update(engine);

      
      engineUpdateService.scheduleNextUpdate(engine);

    } 
  },

  _addObservers: function SRCH_SVC_addObservers() {
    Services.obs.addObserver(this, SEARCH_ENGINE_TOPIC, false);
    Services.obs.addObserver(this, QUIT_APPLICATION_TOPIC, false);

#ifdef MOZ_FENNEC
    Services.prefs.addObserver(LOCALE_PREF, this, false);
#endif

    
    
    let shutdownState = {
      step: "Not started",
      latestError: {
        message: undefined,
        stack: undefined
      }
    };
    OS.File.profileBeforeChange.addBlocker(
      "Search service: shutting down",
      () => Task.spawn(function* () {
        if (this._batchTask) {
          shutdownState.step = "Finalizing batched task";
          try {
            yield this._batchTask.finalize();
            shutdownState.step = "Batched task finalized";
          } catch (ex) {
            shutdownState.step = "Batched task failed to finalize";

            shutdownState.latestError.message = "" + ex;
            if (ex && typeof ex == "object") {
              shutdownState.latestError.stack = ex.stack || undefined;
            }

            
            
            Promise.reject(ex);
          }
        }

        shutdownState.step = "Finalizing engine metadata service";
        yield engineMetadataService.finalize();
        shutdownState.step = "Engine metadata service finalized";

      }.bind(this)),

      () => shutdownState
    );
  },

  _removeObservers: function SRCH_SVC_removeObservers() {
    Services.obs.removeObserver(this, SEARCH_ENGINE_TOPIC);
    Services.obs.removeObserver(this, QUIT_APPLICATION_TOPIC);

#ifdef MOZ_FENNEC
    Services.prefs.removeObserver(LOCALE_PREF, this);
#endif
  },

  QueryInterface: function SRCH_SVC_QI(aIID) {
    if (aIID.equals(Ci.nsIBrowserSearchService) ||
        aIID.equals(Ci.nsIObserver)             ||
        aIID.equals(Ci.nsITimerCallback)        ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};

var engineMetadataService = {
  _jsonFile: OS.Path.join(OS.Constants.Path.profileDir, "search-metadata.json"),

  









  _InitStates: {
    NOT_STARTED: "NOT_STARTED"
      ,
    FINISHED_SUCCESS: "FINISHED_SUCCESS"
      
  },

  




  _initState: null,

  
  _initializer: null,

  




  init: function epsInit() {
    if (!this._initializer) {
      
      let initializer = this._initializer = Promise.defer();
      Task.spawn((function task_init() {
        LOG("metadata init: starting");
        switch (this._initState) {
          case engineMetadataService._InitStates.NOT_STARTED:
            
            try {
              let contents = yield OS.File.read(this._jsonFile);
              if (this._initState == engineMetadataService._InitStates.FINISHED_SUCCESS) {
                
                
                return;
              }
              this._store = JSON.parse(new TextDecoder().decode(contents));
            } catch (ex) {
              if (this._initState == engineMetadataService._InitStates.FINISHED_SUCCESS) {
                
                
                return;
              }
              
              LOG("metadata init: could not load JSON file " + ex);
              this._store = {};
            }
            break;

          default:
            throw new Error("metadata init: invalid state " + this._initState);
        }

        this._initState = this._InitStates.FINISHED_SUCCESS;
        LOG("metadata init: complete");
      }).bind(this)).then(
        
        function onSuccess() {
          initializer.resolve();
        },
        function onError() {
          initializer.reject();
        }
      );
    }
    return this._initializer.promise;
  },

  







  syncInit: function epsSyncInit() {
    LOG("metadata syncInit start");
    if (this._initState == engineMetadataService._InitStates.FINISHED_SUCCESS) {
      return;
    }
    switch (this._initState) {
      case engineMetadataService._InitStates.NOT_STARTED:
        let jsonFile = new FileUtils.File(this._jsonFile);
        
        if (jsonFile.exists()) {
          try {
            let uri = Services.io.newFileURI(jsonFile);
            let stream = Services.io.newChannelFromURI(uri).open();
            this._store = parseJsonFromStream(stream);
          } catch (x) {
            LOG("metadata syncInit: could not load JSON file " + x);
            this._store = {};
          }
        } else {
          LOG("metadata syncInit: using an empty store");
          this._store = {};
        }

        this._initState = this._InitStates.FINISHED_SUCCESS;
        break;

      default:
        throw new Error("metadata syncInit: invalid state " + this._initState);
    }

    
    if (this._initializer) {
      this._initializer.resolve();
    } else {
      this._initializer = Promise.resolve();
    }
    LOG("metadata syncInit end");
  },

  getAttr: function epsGetAttr(engine, name) {
    let record = this._store[engine._id];
    if (!record) {
      return null;
    }

    
    let aName = name.toLowerCase();
    if (!record[aName])
      return null;
    return record[aName];
  },

  _globalFakeEngine: {_id: "[global]"},
  getGlobalAttr: function epsGetGlobalAttr(name) {
    return this.getAttr(this._globalFakeEngine, name);
  },

  _setAttr: function epsSetAttr(engine, name, value) {
    
    name = name.toLowerCase();
    let db = this._store;
    let record = db[engine._id];
    if (!record) {
      record = db[engine._id] = {};
    }
    if (!record[name] || (record[name] != value)) {
      record[name] = value;
      return true;
    }
    return false;
  },

  











  setAttr: function epsSetAttr(engine, key, value) {
    if (this._setAttr(engine, key, value)) {
      this._commit();
    }
  },

  setGlobalAttr: function epsGetGlobalAttr(key, value) {
    this.setAttr(this._globalFakeEngine, key, value);
  },

  









  setAttrs: function epsSetAttrs(changes) {
    let self = this;
    let changed = false;
    changes.forEach(function(change) {
      changed |= self._setAttr(change.engine, change.key, change.value);
    });
    if (changed) {
      this._commit();
    }
  },

  


  finalize: function () this._lazyWriter ? this._lazyWriter.finalize()
                                         : Promise.resolve(),

  







  _commit: function epsCommit() {
    LOG("metadata _commit: start");
    if (!this._store) {
      LOG("metadata _commit: nothing to do");
      return;
    }

    if (!this._lazyWriter) {
      LOG("metadata _commit: initializing lazy writer");
      let writeCommit = function () {
        LOG("metadata writeCommit: start");
        let data = gEncoder.encode(JSON.stringify(engineMetadataService._store));
        let path = engineMetadataService._jsonFile;
        LOG("metadata writeCommit: path " + path);
        let promise = OS.File.writeAtomic(path, data, { tmpPath: path + ".tmp" });
        promise = promise.then(
          function onSuccess() {
            Services.obs.notifyObservers(null,
              SEARCH_SERVICE_TOPIC,
              SEARCH_SERVICE_METADATA_WRITTEN);
            LOG("metadata writeCommit: done");
          }
        );
        return promise;
      }
      this._lazyWriter = new DeferredTask(writeCommit, LAZY_SERIALIZE_DELAY);
    }
    LOG("metadata _commit: (re)setting timer");
    this._lazyWriter.disarm();
    this._lazyWriter.arm();
  },
  _lazyWriter: null
};

engineMetadataService._initState = engineMetadataService._InitStates.NOT_STARTED;

const SEARCH_UPDATE_LOG_PREFIX = "*** Search update: ";





function ULOG(aText) {
  if (getBoolPref(BROWSER_SEARCH_PREF + "update.log", false)) {
    dump(SEARCH_UPDATE_LOG_PREFIX + aText + "\n");
    Services.console.logStringMessage(aText);
  }
}

var engineUpdateService = {
  scheduleNextUpdate: function eus_scheduleNextUpdate(aEngine) {
    var interval = aEngine._updateInterval || SEARCH_DEFAULT_UPDATE_INTERVAL;
    var milliseconds = interval * 86400000; 
    engineMetadataService.setAttr(aEngine, "updateexpir",
                                  Date.now() + milliseconds);
  },

  update: function eus_Update(aEngine) {
    let engine = aEngine.wrappedJSObject;
    ULOG("update called for " + aEngine._name);
    if (!getBoolPref(BROWSER_SEARCH_PREF + "update", true) || !engine._hasUpdates)
      return;

    
    
    if (engine._readOnly &&
        !getBoolPref(BROWSER_SEARCH_PREF + "cache.enabled", true))
      return;

    let testEngine = null;
    let updateURL = engine._getURLOfType(URLTYPE_OPENSEARCH);
    let updateURI = (updateURL && updateURL._hasRelation("self")) ?
                     updateURL.getSubmission("", engine).uri :
                     makeURI(engine._updateURL);
    if (updateURI) {
      if (engine._isDefault && !updateURI.schemeIs("https")) {
        ULOG("Invalid scheme for default engine update");
        return;
      }

      let dataType = engineMetadataService.getAttr(engine, "updatedatatype");
      if (!dataType) {
        ULOG("No loadtype to update engine!");
        return;
      }

      ULOG("updating " + engine.name + " from " + updateURI.spec);
      testEngine = new Engine(updateURI, dataType, false);
      testEngine._engineToUpdate = engine;
      testEngine._initFromURIAndLoad();
    } else
      ULOG("invalid updateURI");

    if (engine._iconUpdateURL) {
      
      
      (testEngine || engine)._setIcon(engine._iconUpdateURL, true);
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([SearchService]);

#include ../../../toolkit/modules/debug.js
