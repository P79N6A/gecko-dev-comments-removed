











const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");


var EXPORTED_SYMBOLS = ["CSPRep", "CSPSourceList", "CSPSource", "CSPHost",
                        "CSPWarning", "CSPError", "CSPdebug",
                        "CSPViolationReportListener", "CSPLocalizer"];

var STRINGS_URI = "chrome://global/locale/security/csp.properties";


var gIoService = Components.classes["@mozilla.org/network/io-service;1"]
                 .getService(Components.interfaces.nsIIOService);

var gETLDService = Components.classes["@mozilla.org/network/effective-tld-service;1"]
                   .getService(Components.interfaces.nsIEffectiveTLDService);



const R_SCHEME     = new RegExp ("([a-zA-Z0-9\\-]+)", 'i');
const R_GETSCHEME  = new RegExp ("^" + R_SCHEME.source + "(?=\\:)", 'i');


const R_SCHEMESRC  = new RegExp ("^" + R_SCHEME.source + "\\:$", 'i');


const R_HOSTCHAR   = new RegExp ("[a-zA-Z0-9\\-]", 'i');


const R_HOST       = new RegExp ("\\*|(((\\*\\.)?" + R_HOSTCHAR.source +
                                      "+)(\\." + R_HOSTCHAR.source +"+)+)",'i');

const R_PORT       = new RegExp ("(\\:([0-9]+|\\*))", 'i');


const R_HOSTSRC    = new RegExp ("^((" + R_SCHEME.source + "\\:\\/\\/)?("
                                       +   R_HOST.source + ")"
                                       +   R_PORT.source + "?)$", 'i');



const R_EXTHOSTSRC = new RegExp ("^" + R_HOSTSRC.source + "\\/[:print:]+$", 'i');


const R_KEYWORDSRC = new RegExp ("^('self'|'unsafe-inline'|'unsafe-eval')$", 'i');


const R_SOURCEEXP  = new RegExp (R_SCHEMESRC.source + "|" +
                                   R_HOSTSRC.source + "|" +
                                R_KEYWORDSRC.source,  'i');


var gPrefObserver = {
  get debugEnabled () {
    if (!this._branch)
      this._initialize();
    return this._debugEnabled;
  },

  _initialize: function() {
    var prefSvc = Components.classes["@mozilla.org/preferences-service;1"]
                    .getService(Components.interfaces.nsIPrefService);
    this._branch = prefSvc.getBranch("security.csp.");
    this._branch.addObserver("", this, false);
    this._debugEnabled = this._branch.getBoolPref("debug");
  },

  unregister: function() {
    if(!this._branch) return;
    this._branch.removeObserver("", this);
  },

  observe: function(aSubject, aTopic, aData) {
    if(aTopic != "nsPref:changed") return;
    if(aData === "debug")
      this._debugEnabled = this._branch.getBoolPref("debug");
  },

};


function CSPWarning(aMsg, aWindowID, aSource, aScriptSample, aLineNum) {
  var textMessage = 'CSP WARN:  ' + aMsg + "\n";

  var consoleMsg = Components.classes["@mozilla.org/scripterror;1"]
                    .createInstance(Components.interfaces.nsIScriptError);
  consoleMsg.initWithWindowID(textMessage, aSource, aScriptSample, aLineNum, 0,
                  Components.interfaces.nsIScriptError.warningFlag,
                  "Content Security Policy", aWindowID);
  Components.classes["@mozilla.org/consoleservice;1"]
                    .getService(Components.interfaces.nsIConsoleService)
                    .logMessage(consoleMsg);
}

function CSPError(aMsg, aWindowID) {
  var textMessage = 'CSP ERROR:  ' + aMsg + "\n";

  var consoleMsg = Components.classes["@mozilla.org/scripterror;1"]
                    .createInstance(Components.interfaces.nsIScriptError);
  consoleMsg.initWithWindowID(textMessage, null, null, 0, 0,
                  Components.interfaces.nsIScriptError.errorFlag,
                  "Content Security Policy", aWindowID);
  Components.classes["@mozilla.org/consoleservice;1"]
                    .getService(Components.interfaces.nsIConsoleService)
                    .logMessage(consoleMsg);
}

function CSPdebug(aMsg) {
  if (!gPrefObserver.debugEnabled) return;

  aMsg = 'CSP debug: ' + aMsg + "\n";
  Components.classes["@mozilla.org/consoleservice;1"]
                    .getService(Components.interfaces.nsIConsoleService)
                    .logStringMessage(aMsg);
}


function CSPPolicyURIListener(policyURI, docRequest, csp) {
  this._policyURI = policyURI;    
  this._docRequest = docRequest;  
  this._csp = csp;                
  this._policy = "";              
  this._wrapper = null;           
  this._docURI = docRequest.QueryInterface(Components.interfaces.nsIChannel)
                 .URI;    
}

CSPPolicyURIListener.prototype = {

  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIStreamListener) ||
        iid.equals(Components.interfaces.nsIRequestObserver) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  onStartRequest:
  function(request, context) {},

  onDataAvailable:
  function(request, context, inputStream, offset, count) {
    if (this._wrapper == null) {
      this._wrapper = Components.classes["@mozilla.org/scriptableinputstream;1"]
                      .createInstance(Components.interfaces.nsIScriptableInputStream);
      this._wrapper.init(inputStream);
    }
    
    this._policy += this._wrapper.read(count);
  },

  onStopRequest:
  function(request, context, status) {
    if (Components.isSuccessCode(status)) {
      
      
      this._csp.refinePolicy(this._policy, this._docURI, this._docRequest);
    }
    else {
      
      this._csp.refinePolicy("allow 'none'", this._docURI, this._docRequest);
      this._csp.refinePolicy("default-src 'none'", this._docURI, this._docRequest);
    }
    
    this._docRequest.resume();
  }
};






function CSPRep() {
  
  
  this._isInitialized = false;

  this._allowEval = false;
  this._allowInlineScripts = false;

  
  this._directives = {};
}

CSPRep.SRC_DIRECTIVES = {
  DEFAULT_SRC:      "default-src",
  SCRIPT_SRC:       "script-src",
  STYLE_SRC:        "style-src",
  MEDIA_SRC:        "media-src",
  IMG_SRC:          "img-src",
  OBJECT_SRC:       "object-src",
  FRAME_SRC:        "frame-src",
  FRAME_ANCESTORS:  "frame-ancestors",
  FONT_SRC:         "font-src",
  XHR_SRC:          "xhr-src"
};

CSPRep.URI_DIRECTIVES = {
  REPORT_URI:       "report-uri", 
  POLICY_URI:       "policy-uri"  
};

CSPRep.OPTIONS_DIRECTIVE = "options";
CSPRep.ALLOW_DIRECTIVE   = "allow";
















CSPRep.fromString = function(aStr, self, docRequest, csp) {
  var SD = CSPRep.SRC_DIRECTIVES;
  var UD = CSPRep.URI_DIRECTIVES;
  var aCSPR = new CSPRep();
  aCSPR._originalText = aStr;

  var selfUri = null;
  if (self instanceof Components.interfaces.nsIURI)
    selfUri = self.clone();

  var dirs = aStr.split(";");

  directive:
  for each(var dir in dirs) {
    dir = dir.trim();
    if (dir.length < 1) continue;

    var dirname = dir.split(/\s+/)[0];
    var dirvalue = dir.substring(dirname.length).trim();

    
    if (dirname === CSPRep.OPTIONS_DIRECTIVE) {
      
      var options = dirvalue.split(/\s+/);
      for each (var opt in options) {
        if (opt === "inline-script")
          aCSPR._allowInlineScripts = true;
        else if (opt === "eval-script")
          aCSPR._allowEval = true;
        else
          CSPWarning(CSPLocalizer.getFormatStr("doNotUnderstandOption", [opt]));
      }
      continue directive;
    }

    
    
    
    if (dirname === CSPRep.ALLOW_DIRECTIVE) {
      var dv = CSPSourceList.fromString(dirvalue, self, true);
      if (dv) {
        aCSPR._directives[SD.DEFAULT_SRC] = dv;
        continue directive;
      }
    }

    
    for each(var sdi in SD) {
      if (dirname === sdi) {
        
        var dv = CSPSourceList.fromString(dirvalue, self, true);
        if (dv) {
          aCSPR._directives[sdi] = dv;
          continue directive;
        }
      }
    }

    
    if (dirname === UD.REPORT_URI) {
      
      var uriStrings = dirvalue.split(/\s+/);
      var okUriStrings = [];

      for (let i in uriStrings) {
        var uri = null;
        try {
          
          
          
          uri = gIoService.newURI(uriStrings[i],null,selfUri);

          
          
          
          uri.host;

          
          
          
          if (self) {
            if (gETLDService.getBaseDomain(uri) !==
                gETLDService.getBaseDomain(selfUri)) {
              CSPWarning(CSPLocalizer.getFormatStr("notETLDPlus1",
                         [gETLDService.getBaseDomain(uri)]));
              continue;
            }
            if (!uri.schemeIs(selfUri.scheme)) {
              CSPWarning(CSPLocalizer.getFormatStr("notSameScheme",
                         [uri.asciiSpec]));
              continue;
            }
            if (uri.port && uri.port !== selfUri.port) {
              CSPWarning(CSPLocalizer.getFormatStr("notSamePort",
                         [uri.asciiSpec]));
              continue;
            }
          }
        } catch(e) {
          switch (e.result) {
            case Components.results.NS_ERROR_INSUFFICIENT_DOMAIN_LEVELS:
            case Components.results.NS_ERROR_HOST_IS_IP_ADDRESS:
              if (uri.host !== selfUri.host) {
                CSPWarning(CSPLocalizer.getFormatStr("pageCannotSendReportsTo",
                         [selfUri.host, uri.host]));
                continue;
              }
              break;

            default:
              CSPWarning(CSPLocalizer.getFormatStr("couldNotParseReportURI", [uriStrings[i]]));
              continue;
          }
        }
        
        okUriStrings.push(uri.asciiSpec);
      }
      aCSPR._directives[UD.REPORT_URI] = okUriStrings.join(' ');
      continue directive;
    }

    
    if (dirname === UD.POLICY_URI) {
      
      if (aCSPR._directives.length > 0 || dirs.length > 1) {
        CSPError(CSPLocalizer.getStr("policyURINotAlone"));
        return CSPRep.fromString("default-src 'none'");
      }
      
      
      if (!docRequest || !csp) {
        CSPError(CSPLocalizer.getStr("noParentRequest"));
        return CSPRep.fromString("default-src 'none'");
      }

      var uri = '';
      try {
        uri = gIoService.newURI(dirvalue, null, selfUri);
      } catch(e) {
        CSPError(CSPLocalizer.getFormatStr("policyURIParseError", [dirvalue]));
        return CSPRep.fromString("default-src 'none'");
      }

      
      if (selfUri) {
        if (selfUri.host !== uri.host){
          CSPError(CSPLocalizer.getFormatStr("nonMatchingHost", [uri.host]));
          return CSPRep.fromString("default-src 'none'");
        }
        if (selfUri.port !== uri.port){
          CSPError(CSPLocalizer.getFormatStr("nonMatchingPort", [uri.port.toString()]));
          return CSPRep.fromString("default-src 'none'");
        }
        if (selfUri.scheme !== uri.scheme){
          CSPError(CSPLocalizer.getFormatStr("nonMatchingScheme", [uri.scheme]));
          return CSPRep.fromString("default-src 'none'");
        }
      }

      
      try {
        docRequest.suspend();
        var chan = gIoService.newChannel(uri.asciiSpec, null, null);
        
        
        chan.loadFlags |= Components.interfaces.nsIChannel.LOAD_ANONYMOUS;
        chan.asyncOpen(new CSPPolicyURIListener(uri, docRequest, csp), null);
      }
      catch (e) {
        
        docRequest.resume();
        CSPError(CSPLocalizer.getFormatStr("errorFetchingPolicy", [e.toString()]));
        return CSPRep.fromString("default-src 'none'");
      }

      
      
      return CSPRep.fromString("default-src *");
    }

    
    CSPWarning(CSPLocalizer.getFormatStr("couldNotProcessUnknownDirective", [dirname]));

  } 

  
  
  if (aCSPR.makeExplicit())
    return aCSPR;
  return CSPRep.fromString("default-src 'none'", self);
};

CSPRep.prototype = {
  


  getReportURIs:
  function() {
    if (!this._directives[CSPRep.URI_DIRECTIVES.REPORT_URI])
      return "";
    return this._directives[CSPRep.URI_DIRECTIVES.REPORT_URI];
  },

  


  equals:
  function(that) {
    if (this._directives.length != that._directives.length) {
      return false;
    }
    for (var i in this._directives) {
      if (!that._directives[i] || !this._directives[i].equals(that._directives[i])) {
        return false;
      }
    }
    return (this.allowsInlineScripts === that.allowsInlineScripts)
        && (this.allowsEvalInScripts === that.allowsEvalInScripts);
  },

  


  toString:
  function csp_toString() {
    var dirs = [];

    if (this._allowEval || this._allowInlineScripts) {
      dirs.push("options" + (this._allowEval ? " eval-script" : "")
                           + (this._allowInlineScripts ? " inline-script" : ""));
    }
    for (var i in this._directives) {
      if (this._directives[i]) {
        dirs.push(i + " " + this._directives[i].toString());
      }
    }
    return dirs.join("; ");
  },

  






  permits:
  function csp_permits(aURI, aContext) {
    if (!aURI) return false;

    
    if (aURI instanceof String && aURI.substring(0,6) === "about:")
      return true;
    if (aURI instanceof Components.interfaces.nsIURI && aURI.scheme === "about")
      return true;

    
    for (var i in CSPRep.SRC_DIRECTIVES) {
      if (CSPRep.SRC_DIRECTIVES[i] === aContext) {
        return this._directives[aContext].permits(aURI);
      }
    }
    return false;
  },

  







  intersectWith:
  function cspsd_intersectWith(aCSPRep) {
    var newRep = new CSPRep();

    for (var dir in CSPRep.SRC_DIRECTIVES) {
      var dirv = CSPRep.SRC_DIRECTIVES[dir];
      newRep._directives[dirv] = this._directives[dirv]
               .intersectWith(aCSPRep._directives[dirv]);
    }

    
    var reportURIDir = CSPRep.URI_DIRECTIVES.REPORT_URI;
    if (this._directives[reportURIDir] && aCSPRep._directives[reportURIDir]) {
      newRep._directives[reportURIDir] =
        this._directives[reportURIDir].concat(aCSPRep._directives[reportURIDir]);
    }
    else if (this._directives[reportURIDir]) {
      
      newRep._directives[reportURIDir] = this._directives[reportURIDir].concat();
    }
    else if (aCSPRep._directives[reportURIDir]) {
      
      newRep._directives[reportURIDir] = aCSPRep._directives[reportURIDir].concat();
    }

    for (var dir in CSPRep.SRC_DIRECTIVES) {
      var dirv = CSPRep.SRC_DIRECTIVES[dir];
      newRep._directives[dirv] = this._directives[dirv]
               .intersectWith(aCSPRep._directives[dirv]);
    }

    newRep._allowEval =          this.allowsEvalInScripts
                           && aCSPRep.allowsEvalInScripts;

    newRep._allowInlineScripts = this.allowsInlineScripts 
                           && aCSPRep.allowsInlineScripts;

    return newRep;
  },

  





  makeExplicit:
  function cspsd_makeExplicit() {
    var SD = CSPRep.SRC_DIRECTIVES;
    var defaultSrcDir = this._directives[SD.DEFAULT_SRC];
    if (!defaultSrcDir) {
      CSPWarning(CSPLocalizer.getStr("allowOrDefaultSrcRequired"));
      return false;
    }

    for (var dir in SD) {
      var dirv = SD[dir];
      if (dirv === SD.DEFAULT_SRC) continue;
      if (!this._directives[dirv]) {
        
        
        if (dirv === SD.FRAME_ANCESTORS)
          this._directives[dirv] = CSPSourceList.fromString("*");
        else
          this._directives[dirv] = defaultSrcDir.clone();
        this._directives[dirv]._isImplicit = true;
      }
    }
    this._isInitialized = true;
    return true;
  },

  


  get allowsEvalInScripts () {
    return this._allowEval;
  },

  



  get allowsInlineScripts () {
    return this._allowInlineScripts;
  },
};





function CSPSourceList() {
  this._sources = [];
  this._permitAllSources = false;

  
  
  this._isImplicit = false;
}














CSPSourceList.fromString = function(aStr, self, enforceSelfChecks) {
  
  

  

  if(self && !(self instanceof CSPSource)) {
     self = CSPSource.create(self);
  }

  var slObj = new CSPSourceList();
  aStr = aStr.trim();
  
  if (aStr.toUpperCase() === "'NONE'"){
    slObj._permitAllSources = false;
    return slObj;
  }

  var tokens = aStr.split(/\s+/);
  for (var i in tokens) {
    if (!R_SOURCEEXP.test(tokens[i])){
      CSPWarning(CSPLocalizer.getFormatStr("failedToParseUnrecognizedSource",
                                           [tokens[i]]));
      continue;
    }
    var src = CSPSource.create(tokens[i], self, enforceSelfChecks);
    if (!src) {
      CSPWarning(CSPLocalizer.getFormatStr("failedToParseUnrecognizedSource",
                                           [tokens[i]]));
      continue;
    }
    
    if (src.permitAll){
      slObj._permitAllSources = true;
      return slObj;
    } else {
      slObj._sources.push(src);
    }
  }

  return slObj;
};

CSPSourceList.prototype = {
  







  equals:
  function(that) {
    
    
    if (that._permitAllSources != this._permitAllSources) {
      return false;
    }
    if (that._sources.length != this._sources.length) {
      return false;
    }
    
    
    var sortfn = function(a,b) {
      return a.toString() > b.toString();
    };
    var a_sorted = this._sources.sort(sortfn);
    var b_sorted = that._sources.sort(sortfn);
    for (var i in a_sorted) {
      if (!a_sorted[i].equals(b_sorted[i])) {
        return false;
      }
    }
    return true;
  },

  


  toString:
  function() {
    if (this.isNone()) {
      return "'none'";
    }
    if (this._permitAllSources) {
      return "*";
    }
    return this._sources.map(function(x) { return x.toString(); }).join(" ");
  },

  



  isNone:
  function() {
    return (!this._permitAllSources) && (this._sources.length < 1);
  },

  


  isAll:
  function() {
    return this._permitAllSources;
  },

  




  clone:
  function() {
    var aSL = new CSPSourceList();
    aSL._permitAllSources = this._permitAllSources;
    for (var i in this._sources) {
      aSL._sources[i] = this._sources[i].clone();
    }
    return aSL;
  },

  






  permits:
  function cspsd_permits(aURI) {
    if (this.isNone())    return false;
    if (this.isAll())     return true;

    for (var i in this._sources) {
      if (this._sources[i].permits(aURI)) {
        return true;
      }
    }
    return false;
  },

  







  intersectWith:
  function cspsd_intersectWith(that) {

    var newCSPSrcList = null;

    if (this.isNone() || that.isNone())
      newCSPSrcList = CSPSourceList.fromString("'none'");

    if (this.isAll()) newCSPSrcList = that.clone();
    if (that.isAll()) newCSPSrcList = this.clone();

    if (!newCSPSrcList) {
      
      

      
      
      var isrcs = [];
      for (var i in this._sources) {
        for (var j in that._sources) {
          var s = that._sources[j].intersectWith(this._sources[i]);
          if (s) {
            isrcs.push(s);
          }
        }
      }
      
      dup: for (var i = 0; i < isrcs.length; i++) {
        for (var j = 0; j < i; j++) {
          if (isrcs[i].equals(isrcs[j])) {
            isrcs.splice(i, 1);
            i--;
            continue dup;
          }
        }
      }
      newCSPSrcList = new CSPSourceList();
      newCSPSrcList._sources = isrcs;
    }

    
    newCSPSrcList._isImplicit = this._isImplicit && that._isImplicit;

    return newCSPSrcList;
  }
}





function CSPSource() {
  this._scheme = undefined;
  this._port = undefined;
  this._host = undefined;

  
  this._permitAll = false;

  
  this._isSelf = false;
}

















CSPSource.create = function(aData, self, enforceSelfChecks) {
  if (typeof aData === 'string')
    return CSPSource.fromString(aData, self, enforceSelfChecks);

  if (aData instanceof Components.interfaces.nsIURI)
    return CSPSource.fromURI(aData, self, enforceSelfChecks);

  if (aData instanceof CSPSource) {
    var ns = aData.clone();
    ns._self = CSPSource.create(self);
    return ns;
  }

  return null;
}
















CSPSource.fromURI = function(aURI, self, enforceSelfChecks) {
  if (!(aURI instanceof Components.interfaces.nsIURI)){
    CSPError(CSPLocalizer.getStr("cspSourceNotURI"));
    return null;
  }

  if (!self && enforceSelfChecks) {
    CSPError(CSPLocalizer.getStr("selfDataNotProvided"));
    return null;
  }

  if (self && !(self instanceof CSPSource)) {
    self = CSPSource.create(self, undefined, false);
  }

  var sObj = new CSPSource();
  sObj._self = self;

  
  

  
  try {
    sObj._scheme = aURI.scheme;
  } catch(e) {
    sObj._scheme = undefined;
    CSPError(CSPLocalizer.getFormatStr("uriWithoutScheme", [aURI.asciiSpec]));
    return null;
  }

  
  try {
    
    
    sObj._host = CSPHost.fromString(aURI.host);
  } catch(e) {
    sObj._host = undefined;
  }

  
  
  
  
  

  
  
  
  
  
  sObj._port = undefined;
  try {
    
    
    if (aURI.port > 0) {
      sObj._port = aURI.port;
    }
  } catch(e) {
    sObj._port = undefined;
  }

  return sObj;
};














CSPSource.fromString = function(aStr, self, enforceSelfChecks) {
  if (!aStr)
    return null;

  if (!(typeof aStr === 'string')) {
    CSPError(CSPLocalizer.getStr("argumentIsNotString"));
    return null;
  }

  var sObj = new CSPSource();
  sObj._self = self;

  
  if (aStr === "*"){
    sObj._permitAll = true;
    return sObj;
  }

  if (!self && enforceSelfChecks) {
    CSPError(CSPLocalizer.getStr("selfDataNotProvided"));
    return null;
  }

  if (self && !(self instanceof CSPSource)) {
    self = CSPSource.create(self, undefined, false);
  }

  
  if (R_SCHEMESRC.test(aStr)){
    var schemeSrcMatch = R_GETSCHEME.exec(aStr);
    sObj._scheme = schemeSrcMatch[0];
    if (!sObj._host) sObj._host = CSPHost.fromString("*");
    if (!sObj._port) sObj._port = "*";
    return sObj;
  }

  
  if (R_HOSTSRC.test(aStr) || R_EXTHOSTSRC.test(aStr)){
    var schemeMatch = R_GETSCHEME.exec(aStr);
    if (!schemeMatch)
      sObj._scheme = self.scheme;
    else {
      sObj._scheme = schemeMatch[0];
    }

    var hostMatch = R_HOST.exec(aStr);
    if (!hostMatch) {
      CSPError(CSPLocalizer.getFormatStr("couldntParseInvalidSource", [aStr]));
      return null;
    }
    sObj._host = CSPHost.fromString(hostMatch[0]);
    var portMatch = R_PORT.exec(aStr);
    if (!portMatch) {
      
      defPort = Services.io.getProtocolHandler(sObj._scheme).defaultPort;
      if (!defPort) {
        CSPError(CSPLocalizer.getFormatStr("couldntParseInvalidSource", [aStr]));
        return null;
      }
      sObj._port = defPort;
    }
    else {
      
      sObj._port = portMatch[0].substr(1);
    }
    return sObj;
  }

  
  if (aStr.toUpperCase() === "'SELF'"){
    if (!self){
      CSPError(CSPLocalizer.getStr("selfKeywordNoSelfData"));
      return null;
    }
    sObj._self = self.clone();
    sObj._isSelf = true;
    return sObj;
  }
  CSPError(CSPLocalizer.getFormatStr("couldntParseInvalidSource",[aStr]));
  return null;
};

CSPSource.validSchemeName = function(aStr) {
  
  
  
  
  
  return aStr.match(/^[a-zA-Z][a-zA-Z0-9+.-]*$/);
};

CSPSource.prototype = {

  get scheme () {
    if (this._isSelf && this._self)
      return this._self.scheme;
    if (!this._scheme && this._self)
      return this._self.scheme;
    return this._scheme;
  },

  get host () {
    if (this._isSelf && this._self)
      return this._self.host;
    if (!this._host && this._self)
      return this._self.host;
    return this._host;
  },

  get permitAll () {
    if (this._isSelf && this._self)
      return this._self.permitAll;
    return this._permitAll;
  },

  



  get port () {
    if (this._isSelf && this._self)
      return this._self.port;
    if (this._port) return this._port;
    
    
    if (this.scheme) {
      try {
        var port = gIoService.getProtocolHandler(this.scheme).defaultPort;
        if (port > 0) return port;
      } catch(e) {
        
      }
    }

    return undefined;
  },

  


  toString:
  function() {
    if (this._isSelf) 
      return this._self.toString();

    var s = "";
    if (this.scheme)
      s = s + this.scheme + "://";
    if (this._host)
      s = s + this._host;
    if (this.port)
      s = s + ":" + this.port;
    return s;
  },

  




  clone:
  function() {
    var aClone = new CSPSource();
    aClone._self = this._self ? this._self.clone() : undefined;
    aClone._scheme = this._scheme;
    aClone._port = this._port;
    aClone._host = this._host ? this._host.clone() : undefined;
    aClone._isSelf = this._isSelf;
    return aClone;
  },

  






  permits:
  function(aSource) {
    if (!aSource) return false;

    if (!(aSource instanceof CSPSource))
      return this.permits(CSPSource.create(aSource));

    
    if (this.scheme != aSource.scheme)
      return false;

    
    
    
    if (this.port && this.port !== "*" && this.port != aSource.port)
      return false;

    
    
    
    if (this.host && !this.host.permits(aSource.host))
      return false;

    
    return true;
  },

  








  intersectWith:
  function(that) {
    var newSource = new CSPSource();

    
    
    
    
    
    

    
    if (!this._port)
      newSource._port = that._port;
    else if (!that._port)
      newSource._port = this._port;
    else if (this._port === "*") 
      newSource._port = that._port;
    else if (that._port === "*")
      newSource._port = this._port;
    else if (that._port === this._port)
      newSource._port = this._port;
    else {
      CSPError(CSPLocalizer.getFormatStr("notIntersectPort", [this.toString(), that.toString()]));
      return null;
    }

    
    if (!this._scheme)
      newSource._scheme = that._scheme;
    else if (!that._scheme)
      newSource._scheme = this._scheme;
    if (this._scheme === "*")
      newSource._scheme = that._scheme;
    else if (that._scheme === "*")
      newSource._scheme = this._scheme;
    else if (that._scheme === this._scheme)
      newSource._scheme = this._scheme;
    else {
      CSPError(CSPLocalizer.getFormatStr("notIntersectScheme", [this.toString(), that.toString()]));
      return null;
    }

    
    
    
    
    
    
    

    
    if (this._host && that._host) {
      newSource._host = this._host.intersectWith(that._host);
    } else if (this._host) {
      CSPError(CSPLocalizer.getFormatStr("intersectingSourceWithUndefinedHost", [that.toString()]));
      newSource._host = this._host.clone();
    } else if (that._host) {
      CSPError(CSPLocalizer.getFormatStr("intersectingSourceWithUndefinedHost", [this.toString()]));
      newSource._host = that._host.clone();
    } else {
      CSPError(CSPLocalizer.getFormatStr("intersectingSourcesWithUndefinedHosts", [this.toString(), that.toString()]));
      newSource._host = CSPHost.fromString("*");
    }

    return newSource;
  },

  










  equals:
  function(that, resolveSelf) {
    
    
    
    if (resolveSelf)
      return this.scheme === that.scheme
          && this.port   === that.port
          && (!(this.host || that.host) ||
               (this.host && this.host.equals(that.host)));

    
    return this._scheme === that._scheme
        && this._port   === that._port
        && (!(this._host || that._host) ||
              (this._host && this._host.equals(that._host)));
  },

};





function CSPHost() {
  this._segments = [];
}









CSPHost.fromString = function(aStr) {
  if (!aStr) return null;

  
  var invalidChar = aStr.match(/[^a-zA-Z0-9\-\.\*]/);
  if (invalidChar) {
    CSPdebug("Invalid character '" + invalidChar + "' in host " + aStr);
    return null;
  }

  var hObj = new CSPHost();
  hObj._segments = aStr.split(/\./);
  if (hObj._segments.length < 1)
    return null;

  
  for (var i in hObj._segments) {
    var seg = hObj._segments[i];
    if (seg == "*") {
      if (i > 0) {
        
        CSPdebug("Wildcard char located at invalid position in '" + aStr + "'");
        return null;
      }
    } 
    else if (seg.match(/[^a-zA-Z0-9\-]/)) {
      
      CSPdebug("Invalid segment '" + seg + "' in host value");
      return null;
    }
  }
  return hObj;
};

CSPHost.prototype = {
  


  toString:
  function() {
    return this._segments.join(".");
  },

  




  clone:
  function() {
    var aHost = new CSPHost();
    for (var i in this._segments) {
      aHost._segments[i] = this._segments[i];
    }
    return aHost;
  },

  






  permits:
  function(aHost) {
    if (!aHost) aHost = CSPHost.fromString("*");

    if (!(aHost instanceof CSPHost)) {
      
      return this.permits(CSPHost.fromString(aHost));
    }
    var thislen = this._segments.length;
    var thatlen = aHost._segments.length;

    
    
    if (thatlen < thislen) { return false; }

    
    
    
    if ((thatlen > thislen) && this._segments[0] != "*") {
      return false;
    }

    
    
    
    
    for (var i=1; i <= thislen; i++) {
      if (this._segments[thislen-i] != "*" && 
          (this._segments[thislen-i] != aHost._segments[thatlen-i])) {
        return false;
      }
    }

    
    return true;
  },

  








  intersectWith:
  function(that) {
    if (!(this.permits(that) || that.permits(this))) {
      
      
      return null;
    } 

    
    if (this._segments.length == that._segments.length) {
      
      return (this._segments[0] === "*") ? that.clone() : this.clone();
    }

    
    
    
    return (this._segments.length > that._segments.length) ?
            this.clone() : that.clone();
  },

  







  equals:
  function(that) {
    if (this._segments.length != that._segments.length)
      return false;

    for (var i=0; i<this._segments.length; i++) {
      if (this._segments[i] != that._segments[i])
        return false;
    }
    return true;
  }
};






function CSPViolationReportListener(reportURI) {
  this._reportURI = reportURI;
}

CSPViolationReportListener.prototype = {
  _reportURI:   null,

  QueryInterface: function(iid) {
    if(iid.equals(Ci.nsIStreamListener) ||
        iid.equals(Ci.nsIRequestObserver) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  onStopRequest:
  function(request, context, status) {
    if (!Components.isSuccessCode(status)) {
      CSPdebug("error " + status.toString(16) +
                " while sending violation report to " +
                this._reportURI);
    }
  },

  onStartRequest:
  function(request, context) { },

  onDataAvailable:
  function(request, context, inputStream, offset, count) { },

};



CSPLocalizer = {
  







  getStr: function CSPLoc_getStr(aName)
  {
    let result;
    try {
      result = this.stringBundle.GetStringFromName(aName);
    }
    catch (ex) {
      Cu.reportError("Failed to get string: " + aName);
      throw ex;
    }
    return result;
  },

  










  getFormatStr: function CSPLoc_getFormatStr(aName, aArray)
  {
    let result;
    try {
      result = this.stringBundle.formatStringFromName(aName, aArray, aArray.length);
    }
    catch (ex) {
      Cu.reportError("Failed to format string: " + aName);
      throw ex;
    }
    return result;
  },
};

XPCOMUtils.defineLazyGetter(CSPLocalizer, "stringBundle", function() {
  return Services.strings.createBundle(STRINGS_URI);
});
