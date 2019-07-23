# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the Browser Search Service.
#
# The Initial Developer of the Original Code is
# Google Inc.
# Portions created by the Initial Developer are Copyright (C) 2005-2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Ben Goodger <beng@google.com> (Original author)
#   Gavin Sharp <gavin@gavinsharp.com>
#   Joe Hughes  <joe@retrovirus.com>
#   Pamela Greene <pamg.bugs@gmail.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

const PERMS_FILE      = 0644;
const PERMS_DIRECTORY = 0755;

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


const SEARCH_ENGINE_TOPIC        = "browser-search-engine-modified";
const QUIT_APPLICATION_TOPIC     = "quit-application";

const SEARCH_ENGINE_REMOVED      = "engine-removed";
const SEARCH_ENGINE_ADDED        = "engine-added";
const SEARCH_ENGINE_CHANGED      = "engine-changed";
const SEARCH_ENGINE_LOADED       = "engine-loaded";
const SEARCH_ENGINE_CURRENT      = "engine-current";

const SEARCH_TYPE_MOZSEARCH      = Ci.nsISearchEngine.TYPE_MOZSEARCH;
const SEARCH_TYPE_OPENSEARCH     = Ci.nsISearchEngine.TYPE_OPENSEARCH;
const SEARCH_TYPE_SHERLOCK       = Ci.nsISearchEngine.TYPE_SHERLOCK;

const SEARCH_DATA_XML            = Ci.nsISearchEngine.DATA_XML;
const SEARCH_DATA_TEXT           = Ci.nsISearchEngine.DATA_TEXT;


const XML_FILE_EXT      = "xml";
const SHERLOCK_FILE_EXT = "src";


const LAZY_SERIALIZE_DELAY = 100;


const CACHE_INVALIDATION_DELAY = 1000;



const CACHE_VERSION = 5;

const ICON_DATAURL_PREFIX = "data:image/x-icon;base64,";


const SHERLOCK_ICON_EXTENSIONS = [".gif", ".png", ".jpg", ".jpeg"];

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

const USER_DEFINED = "{searchTerms}";


#ifdef OFFICIAL_BUILD
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

__defineGetter__("gObsSvc", function() {
  delete this.gObsSvc;
  return this.gObsSvc = Cc["@mozilla.org/observer-service;1"].
                        getService(Ci.nsIObserverService);
});

__defineGetter__("gIoSvc", function() {
  delete this.gIoSvc;
  return this.gIoSvc = Cc["@mozilla.org/network/io-service;1"].
                       getService(Ci.nsIIOService);
});

__defineGetter__("gPrefSvc", function() {
  delete this.gPrefSvc;
  return this.gPrefSvc = Cc["@mozilla.org/preferences-service;1"].
                         getService(Ci.nsIPrefBranch);
});

__defineGetter__("NetUtil", function() {
  delete this.NetUtil;
  Components.utils.import("resource://gre/modules/NetUtil.jsm");
  return NetUtil;
});




const SEARCH_LOG_PREFIX = "*** Search: ";




function DO_LOG(aText) {
  dump(SEARCH_LOG_PREFIX + aText + "\n");
  var consoleService = Cc["@mozilla.org/consoleservice;1"].
                       getService(Ci.nsIConsoleService);
  consoleService.logStringMessage(aText);
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
        aIID.equals(Ci.nsIBadCertListener2)   ||
        aIID.equals(Ci.nsISSLErrorListener)   ||
        
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

  
  onChannelRedirect: function SRCH_loadCRedirect(aOldChannel, aNewChannel,
                                                 aFlags) {
    this._channel = aNewChannel;
  },

  
  getInterface: function SRCH_load_GI(aIID) {
    return this.QueryInterface(aIID);
  },

  
  notifyCertProblem: function SRCH_certProblem(socketInfo, status, targetSite) {
    return true;
  },

  
  notifySSLError: function SRCH_SSLError(socketInfo, error, targetSite) {
    return true;
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






let _dirSvc = null;
function getDir(aKey, aIFace) {
  if (!aKey)
    FAIL("getDir requires a directory key!");

  if (!_dirSvc)
    _dirSvc = Cc["@mozilla.org/file/directory_service;1"].
               getService(Ci.nsIProperties);
  return _dirSvc.get(aKey, aIFace || Ci.nsIFile);
}





function queryCharsetFromCode(aCode) {
  const codes = [];
  codes[0] = "x-mac-roman";
  codes[6] = "x-mac-greek";
  codes[35] = "x-mac-turkish";
  codes[513] = "ISO-8859-1";
  codes[514] = "ISO-8859-2";
  codes[517] = "ISO-8859-5";
  codes[518] = "ISO-8859-6";
  codes[519] = "ISO-8859-7";
  codes[520] = "ISO-8859-8";
  codes[521] = "ISO-8859-9";
  codes[1049] = "IBM864";
  codes[1280] = "windows-1252";
  codes[1281] = "windows-1250";
  codes[1282] = "windows-1251";
  codes[1283] = "windows-1253";
  codes[1284] = "windows-1254";
  codes[1285] = "windows-1255";
  codes[1286] = "windows-1256";
  codes[1536] = "us-ascii";
  codes[1584] = "GB2312";
  codes[1585] = "x-gbk";
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

  return getLocalizedPref("intl.charset.default", DEFAULT_QUERY_CHARSET);
}
function fileCharsetFromCode(aCode) {
  const codes = [
    "x-mac-roman",           
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
  const localePref = "general.useragent.locale";
  var locale = getLocalizedPref(localePref);
  if (locale)
    return locale;

  
  return gPrefSvc.getCharPref(localePref);
}







function getLocalizedPref(aPrefName, aDefault) {
  const nsIPLS = Ci.nsIPrefLocalizedString;
  try {
    return gPrefSvc.getComplexValue(aPrefName, nsIPLS).data;
  } catch (ex) {}

  return aDefault;
}






function setLocalizedPref(aPrefName, aValue) {
  const nsIPLS = Ci.nsIPrefLocalizedString;
  try {
    var pls = Components.classes["@mozilla.org/pref-localizedstring;1"]
                        .createInstance(Ci.nsIPrefLocalizedString);
    pls.data = aValue;
    gPrefSvc.setComplexValue(aPrefName, nsIPLS, pls);
  } catch (ex) {}
}







function getBoolPref(aName, aDefault) {
  try {
    return gPrefSvc.getBoolPref(aName);
  } catch (ex) {
    return aDefault;
  }
}









function getSanitizedFile(aName) {
  var fileName = sanitizeName(aName) + "." + XML_FILE_EXT;
  var file = getDir(NS_APP_USER_SEARCH_DIR);
  file.append(fileName);
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, PERMS_FILE);
  return file;
}








function sanitizeName(aName) {
  const chars = "-abcdefghijklmnopqrstuvwxyz0123456789";
  const maxLength = 60;

  var name = aName.toLowerCase();
  name = name.replace(/ /g, "-");
  name = name.split("").filter(function (el) {
                                 return chars.indexOf(el) != -1;
                               }).join("");

  if (!name) {
    
    var cl = chars.length - 1;
    for (var i = 0; i < 8; ++i)
      name += chars.charAt(Math.round(Math.random() * cl));
  }

  if (name.length > maxLength)
    name = name.substring(0, maxLength);

  return name;
}







function getMozParamPref(prefName)
  gPrefSvc.getCharPref(BROWSER_SEARCH_PREF + "param." + prefName);












function notifyAction(aEngine, aVerb) {
  LOG("NOTIFY: Engine: \"" + aEngine.name + "\"; Verb: \"" + aVerb + "\"");
  gObsSvc.notifyObservers(aEngine, SEARCH_ENGINE_TOPIC, aVerb);
}




function QueryParameter(aName, aValue) {
  if (!aName || (aValue == null))
    FAIL("missing name or value for QueryParameter!");

  this.name = aName;
  this.value = aValue;
}

















function ParamSubstitution(aParamValue, aSearchTerms, aEngine) {
  var value = aParamValue;

  var distributionID = MOZ_DISTRIBUTION_ID;
  try {
    distributionID = gPrefSvc.getCharPref(BROWSER_SEARCH_PREF + "distributionID");
  }
  catch (ex) { }

  
  
  if (aEngine._isDefault) {
    value = value.replace(MOZ_PARAM_LOCALE, getLocale());
    value = value.replace(MOZ_PARAM_DIST_ID, distributionID);
    value = value.replace(MOZ_PARAM_OFFICIAL, MOZ_OFFICIAL);
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








function createStatement (dbconn, sql) {
  var stmt = dbconn.createStatement(sql);
  var wrapper = Cc["@mozilla.org/storage/statement-wrapper;1"].
                createInstance(Ci.mozIStorageStatementWrapper);

  wrapper.initialize(stmt);
  return wrapper;
}



















function EngineURL(aType, aMethod, aTemplate) {
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
}
EngineURL.prototype = {

  addParam: function SRCH_EURL_addParam(aName, aValue) {
    this.params.push(new QueryParameter(aName, aValue));
  },

  _addMozParam: function SRCH_EURL__addMozParam(aObj) {
    aObj.mozparam = true;
    this.mozparams[aObj.name] = aObj;
  },

  getSubmission: function SRCH_EURL_getSubmission(aSearchTerms, aEngine) {
    var url = ParamSubstitution(this.template, aSearchTerms, aEngine);

    
    
    var dataString = "";
    for (var i = 0; i < this.params.length; ++i) {
      var param = this.params[i];
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
        this.addParam(param.name, param.value);
    }
  },

  



  _serializeToJSON: function SRCH_EURL__serializeToJSON() {
    var json = {
      template: this.template,
      rels: this.rels
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

  if (aLocation.cached) {
    this._file = aLocation.value;
  } else if (aLocation instanceof Ci.nsILocalFile) {
    
    this._file = aLocation;
  } else if (aLocation instanceof Ci.nsIURI) {
    this._uri = aLocation;
    switch (aLocation.scheme) {
      case "https":
      case "http":
      case "ftp":
      case "data":
      case "file":
      case "resource":
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
  
  _alias: null,
  
  
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
  
  
  _uri: null,
  
  
  _confirm: false,
  
  
  _useNow: true,
  
  
  __installLocation: null,
  
  _updateInterval: null,
  
  _updateURL: null,
  
  _iconUpdateURL: null,
  
  _serializeTimer: null,
  
  __used: null,
  get _used() {
    if (!this.__used)
      this.__used = !!engineMetadataService.getAttr(this, "used");
    return this.__used;
  },
  set _used(aValue) {
    this.__used = aValue
    engineMetadataService.setAttr(this, "used", aValue);
  },

  





  _initFromFile: function SRCH_ENG_initFromFile() {
    if (!this._file || !this._file.exists())
      FAIL("File must exist before calling initFromFile!", Cr.NS_ERROR_UNEXPECTED);

    var fileInStream = Cc["@mozilla.org/network/file-input-stream;1"].
                       createInstance(Ci.nsIFileInputStream);

    fileInStream.init(this._file, MODE_RDONLY, PERMS_FILE, false);

    switch (this._dataType) {
      case SEARCH_DATA_XML:
        var domParser = Cc["@mozilla.org/xmlextras/domparser;1"].
                        createInstance(Ci.nsIDOMParser);
        var doc = domParser.parseFromStream(fileInStream, "UTF-8",
                                            this._file.fileSize,
                                            "text/xml");

        this._data = doc.documentElement;
        break;
      case SEARCH_DATA_TEXT:
        var binaryInStream = Cc["@mozilla.org/binaryinputstream;1"].
                             createInstance(Ci.nsIBinaryInputStream);
        binaryInStream.setInputStream(fileInStream);

        var bytes = binaryInStream.readByteArray(binaryInStream.available());
        this._data = bytes;

        break;
      default:
        ERROR("Bogus engine _dataType: \"" + this._dataType + "\"",
              Cr.NS_ERROR_UNEXPECTED);
    }
    fileInStream.close();

    
    this._initFromData();
  },

  


  _initFromURI: function SRCH_ENG_initFromURI() {
    ENSURE_WARN(this._uri instanceof Ci.nsIURI,
                "Must have URI when calling _initFromURI!",
                Cr.NS_ERROR_UNEXPECTED);

    LOG("_initFromURI: Downloading engine from: \"" + this._uri.spec + "\".");

    var chan = gIoSvc.newChannelFromURI(this._uri);

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

  








  _getURLOfType: function SRCH_ENG__getURLOfType(aType) {
    for (var i = 0; i < this._urls.length; ++i) {
      if (this._urls[i].type == aType)
        return this._urls[i];
    }

    return null;
  },

  _confirmAddEngine: function SRCH_SVC_confirmAddEngine() {
    var sbs = Cc["@mozilla.org/intl/stringbundle;1"].
              getService(Ci.nsIStringBundleService);
    var stringBundle = sbs.createBundle(SEARCH_BUNDLE);
    var titleMessage = stringBundle.GetStringFromName("addEngineConfirmTitle");

    
    var dialogMessage =
        stringBundle.formatStringFromName("addEngineConfirmation",
                                          [this._name, this._uri.host], 2);
    var checkboxMessage = stringBundle.GetStringFromName("addEngineUseNowText");
    var addButtonLabel =
        stringBundle.GetStringFromName("addEngineAddButtonLabel");

    var ps = Cc["@mozilla.org/embedcomp/prompt-service;1"].
             getService(Ci.nsIPromptService);
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
    



    function onError(aErrorString, aTitleString) {
      if (aEngine._engineToUpdate) {
        
        LOG("updating " + aEngine._engineToUpdate.name + " failed");
        return;
      }
      var sbs = Cc["@mozilla.org/intl/stringbundle;1"].
                getService(Ci.nsIStringBundleService);

      var brandBundle = sbs.createBundle(BRAND_BUNDLE);
      var brandName = brandBundle.GetStringFromName("brandShortName");

      var searchBundle = sbs.createBundle(SEARCH_BUNDLE);
      var msgStringName = aErrorString || "error_loading_engine_msg2";
      var titleStringName = aTitleString || "error_loading_engine_title";
      var title = searchBundle.GetStringFromName(titleStringName);
      var text = searchBundle.formatStringFromName(msgStringName,
                                                   [brandName, aEngine._location],
                                                   2);

      var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
               getService(Ci.nsIWindowWatcher);
      ww.getNewPrompter(null).alert(title, text);
    }

    if (!aBytes) {
      onError();
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
        onError();
        LOG("_onLoad: Bogus engine _dataType: \"" + this._dataType + "\"");
        return;
    }

    try {
      
      aEngine._initFromData();
    } catch (ex) {
      LOG("_onLoad: Failed to init engine!\n" + ex);
      
      onError();
      return;
    }

    
    
    
    if (!engineToUpdate) {
      var ss = Cc["@mozilla.org/browser/search-service;1"].
               getService(Ci.nsIBrowserSearchService);
      if (ss.getEngineByName(aEngine.name)) {
        if (aEngine._confirm)
          onError("error_duplicate_engine_msg", "error_invalid_engine_title");

        LOG("_onLoad: duplicate engine found, bailing");
        return;
      }
    }

    
    
    
    if (aEngine._confirm) {
      var confirmation = aEngine._confirmAddEngine();
      LOG("_onLoad: confirm is " + confirmation.confirmed +
          "; useNow is " + confirmation.useNow);
      if (!confirmation.confirmed)
        return;
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
        let oldSelfURL = engineToUpdate._getURLOfType(URLTYPE_OPENSEARCH);
        if (oldSelfURL && oldSelfURL._hasRelation("self")) {
          oldUpdateURL = oldSelfURL.template;
          let newSelfURL = aEngine._getURLOfType(URLTYPE_OPENSEARCH);
          if (!newSelfURL || !newSelfURL._hasRelation("self")) {
            LOG("_onLoad: updateURL missing in updated engine for " +
                aEngine.name + " aborted");
            return;
          }
          newUpdateURL = newSelfURL.template;
        }

        if (oldUpdateURL != newUpdateURL) {
          LOG("_onLoad: updateURLs do not match! Update of " + aEngine.name + " aborted");
          return;
        }
      }

      
      if (!aEngine._iconURI && engineToUpdate._iconURI)
        aEngine._iconURI = engineToUpdate._iconURI;

      
      
      aEngine._useNow = false;
    }

    
    
    if (!aEngine._readOnly)
      aEngine._serializeToFile();

    
    
    notifyAction(aEngine, SEARCH_ENGINE_LOADED);
  },

  











  _setIcon: function SRCH_ENG_setIcon(aIconURL, aIsPreferred) {
    
    
    if (this._hasPreferredIcon && !aIsPreferred)
      return;

    var uri = makeURI(aIconURL);

    
    if (!uri)
      return;

    LOG("_setIcon: Setting icon url \"" + uri.spec + "\" for engine \""
        + this.name + "\".");
    
    switch (uri.scheme) {
      case "data":
        this._iconURI = uri;
        notifyAction(this, SEARCH_ENGINE_CHANGED);
        this._hasPreferredIcon = aIsPreferred;
        break;
      case "http":
      case "https":
      case "ftp":
        
        if (!this._readOnly) {
          LOG("_setIcon: Downloading icon: \"" + uri.spec +
              "\" for engine: \"" + this.name + "\"");
          var chan = gIoSvc.newChannelFromURI(uri);

          function iconLoadCallback(aByteArray, aEngine) {
            
            
            if (aEngine._hasPreferredIcon && !aIsPreferred)
              return;

            if (!aByteArray || aByteArray.length > MAX_ICON_SIZE) {
              LOG("iconLoadCallback: load failed, or the icon was too large!");
              return;
            }

            var str = btoa(String.fromCharCode.apply(null, aByteArray));
            aEngine._iconURI = makeURI(ICON_DATAURL_PREFIX + str);

            
            
            
            
            
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
                                                    aTemplate) {
    ENSURE_WARN(!this._readOnly,
                "Can't call _initFromMetaData on a readonly engine!",
                Cr.NS_ERROR_FAILURE);

    this._urls.push(new EngineURL("text/html", aMethod, aTemplate));

    this._name = aName;
    this.alias = aAlias;
    this._description = aDescription;
    this._setIcon(aIconURL, true);

    this._serializeToFile();
  },

  








  _parseURL: function SRCH_ENG_parseURL(aElement) {
    var type     = aElement.getAttribute("type");
    
    
    var method   = aElement.getAttribute("method") || "GET";
    var template = aElement.getAttribute("template");

    try {
      var url = new EngineURL(type, method, template);
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
        switch (param.getAttribute("condition")) {
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
        }
      }
    }

    this._urls.push(url);
  },

  _isDefaultEngine: function SRCH_ENG__isDefaultEngine() {
    let defaultPrefB = gPrefSvc.QueryInterface(Ci.nsIPrefService)
                               .getDefaultBranch(BROWSER_SEARCH_PREF);
    let nsIPLS = Ci.nsIPrefLocalizedString;
    let defaultEngine;
    try {
      defaultEngine = defaultPrefB.getComplexValue("defaultenginename", nsIPLS).data;
    } catch (ex) {}
    return this.name == defaultEngine;
  },

  



  _parseImage: function SRCH_ENG_parseImage(aElement) {
    LOG("_parseImage: Image textContent: \"" + aElement.textContent + "\"");
    if (aElement.getAttribute("width")  == "16" &&
        aElement.getAttribute("height") == "16") {
      this._setIcon(aElement.textContent, true);
    }
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
          
          valueEnd = lLine.indexOf("\"", valueStart);
          
          if (valueEnd == -1)
            valueEnd = aLine.length;
        }
        return aLine.substring(valueStart, valueEnd);
      }

      var inputs = [];

      LOG("_parseAsSherlock::getInputs: Lines:\n" + aLines);
      
      lines = aLines.filter(function (line) {
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
      url = new EngineURL("text/html", method, template);

    } else if (method == "POST") {
      
      url = new EngineURL("text/html", method, template);
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
    this._hidden = aJson.hidden || null;
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
    for (let i = 0; i < aJson._urls.length; ++i) {
      let url = aJson._urls[i];
      let engineURL = new EngineURL(url.type || URLTYPE_SEARCH_HTML,
                                    url.method || "GET", url.template);
      engineURL._initWithJSON(url, this);
      this._urls.push(engineURL);
    }
  },

  






  _serializeToJSON: function SRCH_ENG__serializeToJSON(aFilter) {
    var json = {
      _id: this._id,
      _name: this._name,
      description: this.description,
      filePath: this._file.QueryInterface(Ci.nsILocalFile).persistentDescriptor,
      __searchForm: this.__searchForm,
      _iconURL: this._iconURL,
      _urls: [url._serializeToJSON() for each(url in this._urls)] 
    };

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
    if (this.hidden || !aFilter)
      json.hidden = this.hidden;
    if (this.type != SEARCH_TYPE_MOZSEARCH || !aFilter)
      json.type = this.type;
    if (this.queryCharset != DEFAULT_QUERY_CHARSET || !aFilter)
      json.queryCharset = this.queryCharset;
    if (this._dataType != SEARCH_DATA_XML || !aFilter)
      json._dataType = this._dataType;
    if (!this._readOnly || !aFilter)
      json._readOnly = this._readOnly;

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

    for (var i = 0; i < this._urls.length; ++i)
      this._urls[i]._serializeToElement(doc, docElem);
    docElem.appendChild(doc.createTextNode("\n"));

    return doc;
  },

  _lazySerializeToFile: function SRCH_ENG_serializeToFile() {
    if (this._serializeTimer) {
      
      this._serializeTimer.delay = LAZY_SERIALIZE_DELAY;
    } else {
      this._serializeTimer = Cc["@mozilla.org/timer;1"].
                             createInstance(Ci.nsITimer);
      var timerCallback = {
        self: this,
        notify: function SRCH_ENG_notify(aTimer) {
          try {
            this.self._serializeToFile();
          } catch (ex) {
            LOG("Serialization from timer callback failed:\n" + ex);
          }
          this.self._serializeTimer = null;
        }
      };
      this._serializeTimer.initWithCallback(timerCallback,
                                            LAZY_SERIALIZE_DELAY,
                                            Ci.nsITimer.TYPE_ONE_SHOT);
    }
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

    fos.init(file, (MODE_WRONLY | MODE_TRUNCATE), PERMS_FILE, 0);

    try {
      var serializer = Cc["@mozilla.org/xmlextras/xmlserializer;1"].
                       createInstance(Ci.nsIDOMSerializer);
      serializer.serializeToStream(doc.documentElement, fos, null);
    } catch (e) {
      LOG("_serializeToFile: Error serializing engine:\n" + e);
    }

    closeSafeOutputStream(fos);
  },

  




  _remove: function SRCH_ENG_remove() {
    if (this._readOnly)
      FAIL("Can't remove read only engine!", Cr.NS_ERROR_FAILURE);
    if (!this._file || !this._file.exists())
      FAIL("Can't remove engine: file doesn't exist!", Cr.NS_ERROR_FILE_NOT_FOUND);

    this._file.remove(false);
  },

  
  get alias() {
    if (this._alias === null)
      this._alias = engineMetadataService.getAttr(this, "alias");

    return this._alias;
  },
  set alias(val) {
    this._alias = val;
    engineMetadataService.setAttr(this, "alias", val);
    notifyAction(this, SEARCH_ENGINE_CHANGED);
  },

  get description() {
    return this._description;
  },

  get hidden() {
    if (this._hidden === null)
      this._hidden = engineMetadataService.getAttr(this, "hidden");
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
    return this._iconURI;
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

  
  
  __id: null,
  get _id() {
    if (!this.__id) {
      ENSURE_WARN(this._file, "No _file for id!", Cr.NS_ERROR_FAILURE);
  
      if (this._isInProfile)
        return this.__id = "[profile]/" + this._file.leafName;
      if (this._isInAppDir)
        return this.__id = "[app]/" + this._file.leafName;
  
      
      
      return this.__id = this._file.path;
    }
    return this.__id;
  },

  get _installLocation() {
    ENSURE_WARN(this._file && this._file.exists(),
                "_installLocation: engine has no file!",
                Cr.NS_ERROR_FAILURE);

    if (this.__installLocation === null) {
      if (this._file.parent.equals(getDir(NS_APP_SEARCH_DIR)))
        this.__installLocation = SEARCH_APP_DIR;
      else if (this._file.parent.equals(getDir(NS_APP_USER_SEARCH_DIR)))
        this.__installLocation = SEARCH_PROFILE_DIR;
      else
        this.__installLocation = SEARCH_IN_EXTENSION;
    }

    return this.__installLocation;
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
    
    let selfURL = this._getURLOfType(URLTYPE_OPENSEARCH);
    return !!(this._updateURL || this._iconUpdateURL || (selfURL &&
              selfURL._hasRelation("self")));
  },

  get name() {
    return this._name;
  },

  get type() {
    return this._type;
  },

  get searchForm() {
    if (!this._searchForm) {
      
      
      var htmlUrl = this._getURLOfType(URLTYPE_SEARCH_HTML);
      ENSURE_WARN(htmlUrl, "Engine has no HTML URL!", Cr.NS_ERROR_UNEXPECTED);
      this._searchForm = makeURI(htmlUrl.template).prePath;
    }

    return this._searchForm;
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

    
    this._lazySerializeToFile();
  },

  
  getSubmission: function SRCH_ENG_getSubmission(aData, aResponseType) {
    if (!aResponseType)
      aResponseType = URLTYPE_SEARCH_HTML;

    
    if (this._isInAppDir && aResponseType == URLTYPE_SEARCH_HTML && !this._used) {
      this._used = true;
      engineUpdateService.update(this);
    }

    var url = this._getURLOfType(aResponseType);

    if (!url)
      return null;

    if (!aData) {
      
      return new Submission(makeURI(this.searchForm), null);
    }

    LOG("getSubmission: In data: \"" + aData + "\"");
    var textToSubURI = Cc["@mozilla.org/intl/texttosuburi;1"].
                       getService(Ci.nsITextToSubURI);
    var data = "";
    try {
      data = textToSubURI.ConvertAndEscape(this.queryCharset, aData);
    } catch (ex) {
      LOG("getSubmission: Falling back to default queryCharset!");
      data = textToSubURI.ConvertAndEscape(DEFAULT_QUERY_CHARSET, aData);
    }
    LOG("getSubmission: Out data: \"" + data + "\"");
    return url.getSubmission(data, this);
  },

  
  supportsResponseType: function SRCH_ENG_supportsResponseType(type) {
    return (this._getURLOfType(type) != null);
  },

  
  QueryInterface: function SRCH_ENG_QI(aIID) {
    if (aIID.equals(Ci.nsISearchEngine) ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  get wrappedJSObject() {
    return this;
  }

};


function Submission(aURI, aPostData) {
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


function SearchService() {
  this._init();
}
SearchService.prototype = {
  _engines: { },
  _sortedEngines: null,
  
  
  _needToSetOrderPrefs: false,

  _init: function() {
    
    if (getBoolPref(BROWSER_SEARCH_PREF + "log", false))
      LOG = DO_LOG;

    engineMetadataService.init();
    engineUpdateService.init();

    this._loadEngines();
    this._addObservers();

    
    this._buildSortedEngineList();

    let selectedEngineName = getLocalizedPref(BROWSER_SEARCH_PREF +
                                              "selectedEngine");
    this._currentEngine = this.getEngineByName(selectedEngineName) ||
                          this.defaultEngine;
  },

  _buildCache: function SRCH_SVC__buildCache() {
    if (!getBoolPref(BROWSER_SEARCH_PREF + "cache.enabled", true))
      return;

    let cache = {};
    let locale = getLocale();
    let buildID = Cc["@mozilla.org/xre/app-info;1"].
                  getService(Ci.nsIXULAppInfo).platformBuildID;

    
    cache.version = CACHE_VERSION;
    
    
    
    
    
    
    cache.buildID = buildID;
    cache.locale = locale;

    cache.directories = {};

    for each (let engine in this._engines) {
      let parent = engine._file.parent;
      if (!cache.directories[parent.path]) {
        let cacheEntry = {};
        cacheEntry.lastModifiedTime = parent.lastModifiedTime;
        cacheEntry.engines = [];
        cache.directories[parent.path] = cacheEntry;
      }
      cache.directories[parent.path].engines.push(engine._serializeToJSON(true));
    }

    let ostream = Cc["@mozilla.org/network/file-output-stream;1"].
                  createInstance(Ci.nsIFileOutputStream);
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                    createInstance(Ci.nsIScriptableUnicodeConverter);
    let cacheFile = getDir(NS_APP_USER_PROFILE_50_DIR);
    cacheFile.append("search.json");

    try {
      LOG("_buildCache: Writing to cache file.");
      ostream.init(cacheFile, (MODE_WRONLY | MODE_CREATE | MODE_TRUNCATE), PERMS_FILE, 0);
      converter.charset = "UTF-8";
      let data = converter.convertToInputStream(JSON.stringify(cache));

      
      NetUtil.asyncCopy(data, ostream, function(rv) {
        if (!Components.isSuccessCode(rv))
          LOG("_buildCache: failure during asyncCopy: " + rv);
      });
    } catch (ex) {
      LOG("_buildCache: Could not write to cache file: " + ex);
    }
  },

  _loadEngines: function SRCH_SVC__loadEngines() {
    
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

    function modifiedDir(aDir) {
      return (!cache.directories[aDir.path] ||
              cache.directories[aDir.path].lastModifiedTime != aDir.lastModifiedTime);
    }

    function notInLoadDirs(aCachePath, aIndex)
      aCachePath != loadDirs[aIndex].path;

    let buildID = Cc["@mozilla.org/xre/app-info;1"].
                  getService(Ci.nsIXULAppInfo).platformBuildID;
    let cachePaths = [path for (path in cache.directories)];

    let rebuildCache = !cache.directories ||
                       cache.version != CACHE_VERSION ||
                       cache.locale != getLocale() ||
                       cache.buildID != buildID ||
                       cachePaths.length != loadDirs.length ||
                       cachePaths.some(notInLoadDirs) ||
                       loadDirs.some(modifiedDir);

    if (!cacheEnabled || rebuildCache) {
      LOG("_loadEngines: Absent or outdated cache. Loading engines from disk.");
      loadDirs.forEach(this._loadEnginesFromDir, this);

      if (cacheEnabled)
        this._buildCache();
      return;
    }

    for each (let dir in cache.directories)
      this._loadEnginesFromCache(dir);
  },

  _readCacheFile: function SRCH_SVC__readCacheFile(aFile) {
    let stream = Cc["@mozilla.org/network/file-input-stream;1"].
                 createInstance(Ci.nsIFileInputStream);
    let json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);

    try {
      stream.init(aFile, MODE_RDONLY, PERMS_FILE, 0);
      return json.decodeFromStream(stream, stream.available());
    } catch(ex) {
      LOG("_readCacheFile: Error reading cache file: " + ex);
    } finally {
      stream.close();
    }
    return false;
  },

  _batchTimer: null,
  _batchCacheInvalidation: function SRCH_SVC__batchCacheInvalidation() {
    let callback = {
      self: this,
      notify: function SRCH_SVC_batchTimerNotify(aTimer) {
        LOG("_batchCacheInvalidation: Invalidating engine cache");
        this.self._buildCache();
        this.self._batchTimer = null;
      }
    };

    if (!this._batchTimer) {
      this._batchTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      this._batchTimer.initWithCallback(callback, CACHE_INVALIDATION_DELAY,
                                        Ci.nsITimer.TYPE_ONE_SHOT);
    } else {
      this._batchTimer.delay = CACHE_INVALIDATION_DELAY;
      LOG("_batchCacheInvalidation: Batch timer reset");
    }
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
      
      
      
      
      if (this._sortedEngines) {
        this._sortedEngines.push(aEngine);
        this._needToSetOrderPrefs = true;
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
      let engine = new Engine({cached: true, value: json.filePath}, json._dataType,
                              json._readOnly);
      engine._initWithJSON(json);
      this._addEngineToStore(engine);
    }
  },

  _loadEnginesFromDir: function SRCH_SVC__loadEnginesFromDir(aDir) {
    LOG("_loadEnginesFromDir: Searching in " + aDir.path + " for search engines.");

    
    var isInProfile = aDir.equals(getDir(NS_APP_USER_SEARCH_DIR));

    var files = aDir.directoryEntries
                    .QueryInterface(Ci.nsIDirectoryEnumerator);

    var addedEngines = [];
    while (files.hasMoreElements()) {
      var file = files.nextFile;

      
      if (!file.isFile() || file.fileSize == 0 || file.isHidden())
        continue;

      var fileURL = gIoSvc.newFileURI(file).QueryInterface(Ci.nsIURL);
      var fileExtension = fileURL.fileExtension.toLowerCase();
      var isWritable = isInProfile && file.isWritable();

      var dataType;
      switch (fileExtension) {
        case XML_FILE_EXT:
          dataType = SEARCH_DATA_XML;
          break;
        case SHERLOCK_FILE_EXT:
          dataType = SEARCH_DATA_TEXT;
          break;
        default:
          
          continue;
      }

      var addedEngine = null;
      try {
        addedEngine = new Engine(file, dataType, !isWritable);
        addedEngine._initFromFile();
        if (addedEngine._used)
          addedEngine._used = false;
      } catch (ex) {
        LOG("_loadEnginesFromDir: Failed to load " + file.path + "!\n" + ex);
        continue;
      }

      if (fileExtension == SHERLOCK_FILE_EXT) {
        if (isWritable) {
          try {
            this._convertSherlockFile(addedEngine, fileURL.fileBaseName);
          } catch (ex) {
            LOG("_loadEnginesFromDir: Failed to convert: " + fileURL.path + "\n" + ex);
            
            addedEngine._readOnly = true;
          }
        }

        
        if (!addedEngine._iconURI) {
          var icon = this._findSherlockIcon(file, fileURL.fileBaseName);
          if (icon)
            addedEngine._iconURI = gIoSvc.newFileURI(icon);
        }
      }

      this._addEngineToStore(addedEngine);
      addedEngines.push(addedEngine);
    }
    return addedEngines;
  },

  _saveSortedEngineList: function SRCH_SVC_saveSortedEngineList() {
    
    if (!this._needToSetOrderPrefs)
      return;

    
    
    gPrefSvc.setBoolPref(BROWSER_SEARCH_PREF + "useDBForOrder", true);

    var engines = this._getSortedEngines(true);
    var values = [];
    var names = [];

    for (var i = 0; i < engines.length; ++i) {
      names[i] = "order";
      values[i] = i + 1;
    }

    engineMetadataService.setAttrs(engines, names, values);
  },

  _buildSortedEngineList: function SRCH_SVC_buildSortedEngineList() {
    var addedEngines = { };
    this._sortedEngines = [];
    var engine;

    
    
    
    if (getBoolPref(BROWSER_SEARCH_PREF + "useDBForOrder", false)) {
      for each (engine in this._engines) {
        var orderNumber = engineMetadataService.getAttr(engine, "order");

        
        
        
        
        
        if (orderNumber && !this._sortedEngines[orderNumber-1]) {
          this._sortedEngines[orderNumber-1] = engine;
          addedEngines[engine.name] = engine;
        } else {
          
          this._needToSetOrderPrefs = true;
        }
      }

      
      var filteredEngines = this._sortedEngines.filter(function(a) { return !!a; });
      if (this._sortedEngines.length != filteredEngines.length)
        this._needToSetOrderPrefs = true;
      this._sortedEngines = filteredEngines;

    } else {
      
      var i = 0;
      var engineName;
      var prefName;

      try {
        var extras =
          gPrefSvc.getChildList(BROWSER_SEARCH_PREF + "order.extra.", { });

        for each (prefName in extras) {
          engineName = gPrefSvc.getCharPref(prefName);

          engine = this._engines[engineName];
          if (!engine || engine.name in addedEngines)
            continue;

          this._sortedEngines.push(engine);
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
        
        this._sortedEngines.push(engine);
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
    this._sortedEngines = this._sortedEngines.concat(alphaEngines);
  },

  














  _convertSherlockFile: function SRCH_SVC_convertSherlock(aEngine, aBaseName) {
    var oldSherlockFile = aEngine._file;

    
    try {
      var backupDir = oldSherlockFile.parent;
      backupDir.append("searchplugins-backup");

      if (!backupDir.exists())
        backupDir.create(Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);

      oldSherlockFile.copyTo(backupDir, null);
    } catch (ex) {
      
      
      FAIL("_convertSherlockFile: Couldn't back up " + oldSherlockFile.path +
           ":\n" + ex, Cr.NS_ERROR_FAILURE);
    }

    
    var newXMLFile = oldSherlockFile.parent.clone();
    newXMLFile.append(aBaseName + "." + XML_FILE_EXT);

    if (newXMLFile.exists()) {
      
      newXMLFile.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, PERMS_FILE);
    }

    
    oldSherlockFile.moveTo(null, newXMLFile.leafName);

    aEngine._file = newXMLFile;

    
    aEngine._serializeToFile();

    
    aEngine._type = SEARCH_TYPE_MOZSEARCH;

    
    try {
      var icon = this._findSherlockIcon(aEngine._file, aBaseName);
      if (icon && icon.fileSize < MAX_ICON_SIZE) {
        
        var bStream = Cc["@mozilla.org/binaryinputstream;1"].
                        createInstance(Ci.nsIBinaryInputStream);
        var fileInStream = Cc["@mozilla.org/network/file-input-stream;1"].
                           createInstance(Ci.nsIFileInputStream);

        fileInStream.init(icon, MODE_RDONLY, PERMS_FILE, 0);
        bStream.setInputStream(fileInStream);

        var bytes = [];
        while (bStream.available() != 0)
          bytes = bytes.concat(bStream.readByteArray(bStream.available()));
        bStream.close();

        
        var str = btoa(String.fromCharCode.apply(null, bytes));

        aEngine._iconURI = makeURI(ICON_DATAURL_PREFIX + str);
        LOG("_importSherlockEngine: Set sherlock iconURI to: \"" +
            aEngine._iconURL + "\"");

        
        aEngine._serializeToFile();

        
        icon.remove(false);
      }
    } catch (ex) { LOG("_convertSherlockFile: Error setting icon:\n" + ex); }
  },

  










  _findSherlockIcon: function SRCH_SVC_findSherlock(aEngineFile, aBaseName) {
    for (var i = 0; i < SHERLOCK_ICON_EXTENSIONS.length; i++) {
      var icon = aEngineFile.parent.clone();
      icon.append(aBaseName + SHERLOCK_ICON_EXTENSIONS[i]);
      if (icon.exists() && icon.isFile())
        return icon;
    }
    return null;
  },

  




  _getSortedEngines: function SRCH_SVC_getSorted(aWithHidden) {
    if (aWithHidden)
      return this._sortedEngines;

    return this._sortedEngines.filter(function (engine) {
                                        return !engine.hidden;
                                      });
  },

  
  getEngines: function SRCH_SVC_getEngines(aCount) {
    LOG("getEngines: getting all engines");
    var engines = this._getSortedEngines(true);
    aCount.value = engines.length;
    return engines;
  },

  getVisibleEngines: function SRCH_SVC_getVisible(aCount) {
    LOG("getVisibleEngines: getting all visible engines");
    var engines = this._getSortedEngines(false);
    aCount.value = engines.length;
    return engines;
  },

  getDefaultEngines: function SRCH_SVC_getDefault(aCount) {
    function isDefault(engine) {
      return engine._isDefault;
    };
    var engines = this._sortedEngines.filter(isDefault);
    var engineOrder = {};
    var engineName;
    var i = 1;

    
    
    

    
    try {
      var extras = gPrefSvc.getChildList(BROWSER_SEARCH_PREF + "order.extra.",
                                         {});

      for each (var prefName in extras) {
        engineName = gPrefSvc.getCharPref(prefName);

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
    return this._engines[aEngineName] || null;
  },

  getEngineByAlias: function SRCH_SVC_getEngineByAlias(aAlias) {
    for (var engineName in this._engines) {
      var engine = this._engines[engineName];
      if (engine && engine.alias == aAlias)
        return engine;
    }
    return null;
  },

  addEngineWithDetails: function SRCH_SVC_addEWD(aName, aIconURL, aAlias,
                                                 aDescription, aMethod,
                                                 aTemplate) {
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
                             aMethod, aTemplate);
    this._addEngineToStore(engine);
    this._batchCacheInvalidation();
  },

  addEngine: function SRCH_SVC_addEngine(aEngineURL, aDataType, aIconURL,
                                         aConfirm) {
    LOG("addEngine: Adding \"" + aEngineURL + "\".");
    try {
      var uri = makeURI(aEngineURL);
      var engine = new Engine(uri, aDataType, false);
      engine._initFromURI();
    } catch (ex) {
      FAIL("addEngine: Error adding engine:\n" + ex, Cr.NS_ERROR_FAILURE);
    }
    engine._setIcon(aIconURL, false);
    engine._confirm = aConfirm;
  },

  removeEngine: function SRCH_SVC_removeEngine(aEngine) {
    if (!aEngine)
      FAIL("no engine passed to removeEngine!");

    var engineToRemove = null;
    for (var e in this._engines)
      if (aEngine.wrappedJSObject == this._engines[e])
        engineToRemove = this._engines[e];

    if (!engineToRemove)
      FAIL("removeEngine: Can't find engine to remove!", Cr.NS_ERROR_FILE_NOT_FOUND);

    if (engineToRemove == this.currentEngine)
      this._currentEngine = null;

    if (engineToRemove._readOnly) {
      
      
      engineToRemove.hidden = true;
      engineToRemove.alias = null;
    } else {
      
      if (engineToRemove._serializeTimer) {
        engineToRemove._serializeTimer.cancel();
        engineToRemove._serializeTimer = null;
      }

      
      engineToRemove._remove();
      engineToRemove._file = null;

      
      var index = this._sortedEngines.indexOf(engineToRemove);
      if (index == -1)
        FAIL("Can't find engine to remove in _sortedEngines!", Cr.NS_ERROR_FAILURE);
      this._sortedEngines.splice(index, 1);

      
      delete this._engines[engineToRemove.name];

      notifyAction(engineToRemove, SEARCH_ENGINE_REMOVED);

      
      this._needToSetOrderPrefs = true;
    }
  },

  moveEngine: function SRCH_SVC_moveEngine(aEngine, aNewIndex) {
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

    
    var movedEngine = this._sortedEngines.splice(currentIndex, 1)[0];
    this._sortedEngines.splice(aNewIndex, 0, movedEngine);

    notifyAction(engine, SEARCH_ENGINE_CHANGED);

    
    this._needToSetOrderPrefs = true;
  },

  restoreDefaultEngines: function SRCH_SVC_resetDefaultEngines() {
    for each (var e in this._engines) {
      
      if (e.hidden && e._isDefault)
        e.hidden = false;
    }
  },

  get defaultEngine() {
    const defPref = BROWSER_SEARCH_PREF + "defaultenginename";
    
    
    this._defaultEngine = this.getEngineByName(getLocalizedPref(defPref, ""));
    if (!this._defaultEngine || this._defaultEngine.hidden)
      this._defaultEngine = this._getSortedEngines(false)[0] || null;
    return this._defaultEngine;
  },

  get currentEngine() {
    if (!this._currentEngine || this._currentEngine.hidden)
      this._currentEngine = this.defaultEngine;
    return this._currentEngine;
  },
  set currentEngine(val) {
    if (!(val instanceof Ci.nsISearchEngine))
      FAIL("Invalid argument passed to currentEngine setter");

    var newCurrentEngine = this.getEngineByName(val.name);
    if (!newCurrentEngine)
      FAIL("Can't find engine in store!", Cr.NS_ERROR_UNEXPECTED);

    this._currentEngine = newCurrentEngine;

    var currentEnginePref = BROWSER_SEARCH_PREF + "selectedEngine";

    if (this._currentEngine == this.defaultEngine) {
      gPrefSvc.clearUserPref(currentEnginePref);
    }
    else {
      setLocalizedPref(currentEnginePref, this._currentEngine.name);
    }

    notifyAction(this._currentEngine, SEARCH_ENGINE_CURRENT);
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
            this._batchCacheInvalidation();
            break;
          case SEARCH_ENGINE_CHANGED:
          case SEARCH_ENGINE_REMOVED:
            this._batchCacheInvalidation();
            break;
        }
        break;

      case QUIT_APPLICATION_TOPIC:
        this._removeObservers();
        this._saveSortedEngineList();
        if (this._batchTimer) {
          
          this._batchTimer.cancel();
          this._buildCache();
        }
        break;
    }
  },

  _addObservers: function SRCH_SVC_addObservers() {
    gObsSvc.addObserver(this, SEARCH_ENGINE_TOPIC, false);
    gObsSvc.addObserver(this, QUIT_APPLICATION_TOPIC, false);
  },

  _removeObservers: function SRCH_SVC_removeObservers() {
    gObsSvc.removeObserver(this, SEARCH_ENGINE_TOPIC);
    gObsSvc.removeObserver(this, QUIT_APPLICATION_TOPIC);
  },

  QueryInterface: function SRCH_SVC_QI(aIID) {
    if (aIID.equals(Ci.nsIBrowserSearchService) ||
        aIID.equals(Ci.nsIObserver)             ||
        aIID.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};

var engineMetadataService = {
  init: function epsInit() {
    var engineDataTable = "id INTEGER PRIMARY KEY, engineid STRING, name STRING, value STRING";
    var file = getDir(NS_APP_USER_PROFILE_50_DIR);
    file.append("search.sqlite");
    var dbService = Cc["@mozilla.org/storage/service;1"].
                    getService(Ci.mozIStorageService);
    try {
        this.mDB = dbService.openDatabase(file);
    } catch (ex) {
        if (ex.result == 0x8052000b) { 
            
            file.remove(false);
            this.mDB = dbService.openDatabase(file);
        } else {
            throw ex;
        }
    }

    try {
      this.mDB.createTable("engine_data", engineDataTable);
    } catch (ex) {
      
    }

    this.mGetData = createStatement (
      this.mDB,
      "SELECT value FROM engine_data WHERE engineid = :engineid AND name = :name");
    this.mDeleteData = createStatement (
      this.mDB,
      "DELETE FROM engine_data WHERE engineid = :engineid AND name = :name");
    this.mInsertData = createStatement (
      this.mDB,
      "INSERT INTO engine_data (engineid, name, value) " +
      "VALUES (:engineid, :name, :value)");
  },
  getAttr: function epsGetAttr(engine, name) {
     
     name = name.toLowerCase();

    var stmt = this.mGetData;
    stmt.reset();
    var pp = stmt.params;
    pp.engineid = engine._id;
    pp.name = name;

    var value = null;
    if (stmt.step())
      value = stmt.row.value;
    stmt.reset();
    return value;
  },

  setAttr: function epsSetAttr(engine, name, value) {
    
    name = name.toLowerCase();

    this.mDB.beginTransaction();

    var pp = this.mDeleteData.params;
    pp.engineid = engine._id;
    pp.name = name;
    this.mDeleteData.step();
    this.mDeleteData.reset();

    pp = this.mInsertData.params;
    pp.engineid = engine._id;
    pp.name = name;
    pp.value = value;
    this.mInsertData.step();
    this.mInsertData.reset();

    this.mDB.commitTransaction();
  },

  setAttrs: function epsSetAttrs(engines, names, values) {
    this.mDB.beginTransaction();

    for (var i = 0; i < engines.length; i++) {
      
      var name = names[i].toLowerCase();

      var pp = this.mDeleteData.params;
      pp.engineid = engines[i]._id;
      pp.name = names[i];
      this.mDeleteData.step();
      this.mDeleteData.reset();

      pp = this.mInsertData.params;
      pp.engineid = engines[i]._id;
      pp.name = names[i];
      pp.value = values[i];
      this.mInsertData.step();
      this.mInsertData.reset();
    }

    this.mDB.commitTransaction();
  },

  deleteEngineData: function epsDelData(engine, name) {
    
    name = name.toLowerCase();

    var pp = this.mDeleteData.params;
    pp.engineid = engine._id;
    pp.name = name;
    this.mDeleteData.step();
    this.mDeleteData.reset();
  }
}

const SEARCH_UPDATE_LOG_PREFIX = "*** Search update: ";





function ULOG(aText) {
  if (getBoolPref(BROWSER_SEARCH_PREF + "update.log", false)) {
    dump(SEARCH_UPDATE_LOG_PREFIX + aText + "\n");
    var consoleService = Cc["@mozilla.org/consoleservice;1"].
                         getService(Ci.nsIConsoleService);
    consoleService.logStringMessage(aText);
  }
}

var engineUpdateService = {
  init: function eus_init() {
    var tm = Cc["@mozilla.org/updates/timer-manager;1"].
             getService(Ci.nsIUpdateTimerManager);
    
    var interval = gPrefSvc.getIntPref(BROWSER_SEARCH_PREF + "updateinterval");

    
    var seconds = interval * 3600;
    tm.registerTimer("search-engine-update-timer", engineUpdateService,
                     seconds);
  },

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
      testEngine._initFromURI();
    } else
      ULOG("invalid updateURI");

    if (engine._iconUpdateURL) {
      
      
      (testEngine || engine)._setIcon(engine._iconUpdateURL, true);
    }
  },

  notify: function eus_Notify(aTimer) {
    ULOG("notify called");

    if (!getBoolPref(BROWSER_SEARCH_PREF + "update", true))
      return;

    
    
    var searchService = Cc["@mozilla.org/browser/search-service;1"].
                        getService(Ci.nsIBrowserSearchService);
    var currentTime = Date.now();
    ULOG("currentTime: " + currentTime);
    for each (engine in searchService.getEngines({})) {
      engine = engine.wrappedJSObject;
      if (!engine._hasUpdates)
        continue;

      ULOG("checking " + engine.name);

      var expirTime = engineMetadataService.getAttr(engine, "updateexpir");
      ULOG("expirTime: " + expirTime + "\nupdateURL: " + engine._updateURL +
           "\niconUpdateURL: " + engine._iconUpdateURL);

      var engineExpired = expirTime <= currentTime;

      if (!expirTime || !engineExpired) {
        ULOG("skipping engine");
        continue;
      }

      ULOG(engine.name + " has expired");

      this.update(engine);

      
      this.scheduleNextUpdate(engine);

    } 
  }
};

const kClassID    = Components.ID("{7319788a-fe93-4db3-9f39-818cf08f4256}");
const kClassName  = "Browser Search Service";
const kContractID = "@mozilla.org/browser/search-service;1";


const kFactory = {
  createInstance: function (outer, iid) {
    if (outer != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return (new SearchService()).QueryInterface(iid);
  }
};


const gModule = {
  registerSelf: function (componentManager, fileSpec, location, type) {
    componentManager.QueryInterface(Ci.nsIComponentRegistrar);
    componentManager.registerFactoryLocation(kClassID,
                                             kClassName,
                                             kContractID,
                                             fileSpec, location, type);
  },

  unregisterSelf: function(componentManager, fileSpec, location) {
    componentManager.QueryInterface(Ci.nsIComponentRegistrar);
    componentManager.unregisterFactoryLocation(kClassID, fileSpec);
  },

  getClassObject: function (componentManager, cid, iid) {
    if (!cid.equals(kClassID))
      throw Cr.NS_ERROR_NO_INTERFACE;
    if (!iid.equals(Ci.nsIFactory))
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;
    return kFactory;
  },

  canUnload: function (componentManager) {
    return true;
  }
};

function NSGetModule(componentManager, fileSpec) {
  return gModule;
}

#include ../../../toolkit/content/debug.js
