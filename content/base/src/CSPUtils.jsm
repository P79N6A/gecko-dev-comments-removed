











































var EXPORTED_SYMBOLS = ["CSPRep", "CSPSourceList", "CSPSource", 
                        "CSPHost", "CSPWarning", "CSPError", "CSPdebug"];



var gIoService = Components.classes["@mozilla.org/network/io-service;1"]
                 .getService(Components.interfaces.nsIIOService);

var gETLDService = Components.classes["@mozilla.org/network/effective-tld-service;1"]
                   .getService(Components.interfaces.nsIEffectiveTLDService);


function CSPWarning(aMsg) {
  
  aMsg = 'CSP WARN:  ' + aMsg + "\n";
  dump(aMsg);
  Components.classes["@mozilla.org/consoleservice;1"]
                    .getService(Components.interfaces.nsIConsoleService)
                    .logStringMessage(aMsg);
}
function CSPError(aMsg) {
  aMsg = 'CSP ERROR: ' + aMsg + "\n";
  dump(aMsg);
  Components.classes["@mozilla.org/consoleservice;1"]
                    .getService(Components.interfaces.nsIConsoleService)
                    .logStringMessage(aMsg);
}
function CSPdebug(aMsg) {
  aMsg = 'CSP debug: ' + aMsg + "\n";
  dump(aMsg);
  Components.classes["@mozilla.org/consoleservice;1"]
                    .getService(Components.interfaces.nsIConsoleService)
                    .logStringMessage(aMsg);
}






function CSPRep() {
  
  
  this._isInitialized = false;

  this._allowEval = false;
  this._allowInlineScripts = false;

  
  this._directives = {};
}

CSPRep.SRC_DIRECTIVES = {
  ALLOW:            "allow",
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











CSPRep.fromString = function(aStr, self) {
  var SD = CSPRep.SRC_DIRECTIVES;
  var UD = CSPRep.URI_DIRECTIVES;
  var aCSPR = new CSPRep();
  aCSPR._originalText = aStr;

  var dirs = aStr.split(";");

  directive:
  for each(var dir in dirs) {
    dir = dir.trim();
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
          CSPWarning("don't understand option '" + opt + "'.  Ignoring it.");
      }
      continue directive;
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
      var selfUri = self ? gIoService.newURI(self.toString(),null,null) : null;

      
      
      for (let i in uriStrings) {
        try {
          var uri = gIoService.newURI(uriStrings[i],null,null);
          if (self) {
            if (gETLDService.getBaseDomain(uri) ===
                gETLDService.getBaseDomain(selfUri)) {
              okUriStrings.push(uriStrings[i]);
            } else {
              CSPWarning("can't use report URI from non-matching eTLD+1: "
                         + gETLDService.getBaseDomain(uri));
            }
          }
        } catch(e) {
          switch (e.result) {
            case Components.results.NS_ERROR_INSUFFICIENT_DOMAIN_LEVELS:
            case Components.results.NS_ERROR_HOST_IS_IP_ADDRESS:
              if (uri.host === selfUri.host) {
                okUriStrings.push(uriStrings[i]);
              } else {
                CSPWarning("page on " + selfUri.host + " cannot send reports to " + uri.host);
              }
              break;

            default:
              CSPWarning("couldn't parse report URI: " + uriStrings[i]);
              break;
          }
        }
      }
      aCSPR._directives[UD.REPORT_URI] = okUriStrings.join(' ');
      continue directive;
    }

    
    if (dirname === UD.POLICY_URI) {
      
      if (aCSPR._directives.length > 0 || dirs.length > 1) {
        CSPError("policy-uri directive can only appear alone");
        return CSPRep.fromString("allow 'none'");
      }

      var uri = '';
      try {
        uri = gIoService.newURI(dirvalue, null, null);
      } catch(e) {
        CSPError("could not parse URI in policy URI: " + dirvalue);
        return CSPRep.fromString("allow 'none'");
      }
      
      
      if (self) {
        var selfUri = gIoService.newURI(self.toString(), null, null);
        if (selfUri.host !== uri.host){
          CSPError("can't fetch policy uri from non-matching hostname: " + uri.host);
          return CSPRep.fromString("allow 'none'");
        }
        if (selfUri.port !== uri.port){
          CSPError("can't fetch policy uri from non-matching port: " + uri.port);
          return CSPRep.fromString("allow 'none'");
        }
        if (selfUri.scheme !== uri.scheme){
          CSPError("can't fetch policy uri from non-matching scheme: " + uri.scheme);
          return CSPRep.fromString("allow 'none'");
        }
      }

      var req = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"]  
                  .createInstance(Components.interfaces.nsIXMLHttpRequest);  

      
      req.onerror = CSPError;

      
      
      
      req.open("GET", dirvalue, false);

      
      
      
      req.channel.loadFlags |= Components.interfaces.nsIChannel.LOAD_ANONYMOUS;

      req.send(null);  
      if (req.status == 200) {
        aCSPR = CSPRep.fromString(req.responseText, self);
        
        aCSPR._directives[UD.POLICY_URI] = dirvalue;
        return aCSPR;
      }
      CSPError("Error fetching policy URI: server response was " + req.status);
      return CSPRep.fromString("allow 'none'");
    }

    
    CSPWarning("Couldn't process unknown directive '" + dirname + "'");

  } 

  
  
  if (aCSPR.makeExplicit())
    return aCSPR;
  return CSPRep.fromString("allow 'none'", self);
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
      dirs.push("options " + (this._allowEval ? "eval-script" : "")
                           + (this._allowInlineScripts ? "inline-script" : ""));
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
    var allowDir = this._directives[SD.ALLOW];
    if (!allowDir) {
      CSPWarning("'allow' directive required but not present.  Reverting to \"allow 'none'\"");
      return false;
    }

    for (var dir in SD) {
      var dirv = SD[dir];
      if (dirv === SD.ALLOW) continue;
      if (!this._directives[dirv]) {
        
        
        if (dirv === SD.FRAME_ANCESTORS)
          this._directives[dirv] = CSPSourceList.fromString("*");
        else
          this._directives[dirv] = allowDir.clone();
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
  
  
  
  
  

  var slObj = new CSPSourceList();
  if (aStr === "'none'")
    return slObj;

  if (aStr === "*") {
    slObj._permitAllSources = true;
    return slObj;
  }

  var tokens = aStr.split(/\s+/);
  for (var i in tokens) {
    if (tokens[i] === "") continue;
    var src = CSPSource.create(tokens[i], self, enforceSelfChecks);
    if (!src) {
      CSPWarning("Failed to parse unrecoginzied source " + tokens[i]);
      continue;
    }
    slObj._sources.push(src);
  }

  return slObj;
};

CSPSourceList.prototype = {
  







  equals:
  function(that) {
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
    CSPError("Provided argument is not an nsIURI");
    return null;
  }

  if (!self && enforceSelfChecks) {
    CSPError("Can't use 'self' if self data is not provided");
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
    CSPError("can't parse a URI without a scheme: " + aURI.asciiSpec);
    return null;
  }

  
  try {
    
    
    sObj._host = CSPHost.fromString(aURI.host);
  } catch(e) {
    sObj._host = undefined;
  }

  
  
  
  
  
  try {
    
    
    if (aURI.port > 0) {
      sObj._port = aURI.port;
    } else {
      
      
      
      if (sObj._scheme) {
        sObj._port = gIoService.getProtocolHandler(sObj._scheme).defaultPort;
        if (sObj._port < 1) 
          sObj._port = undefined;
      }
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
    CSPError("Provided argument is not a string");
    return null;
  }

  if (!self && enforceSelfChecks) {
    CSPError("Can't use 'self' if self data is not provided");
    return null;
  }

  if (self && !(self instanceof CSPSource)) {
    self = CSPSource.create(self, undefined, false);
  }

  var sObj = new CSPSource();
  sObj._self = self;

  
  if (aStr === "'self'") {
    if (!self) {
      CSPError("self keyword used, but no self data specified");
      return null;
    }
    sObj._isSelf = true;
    sObj._self = self.clone();
    return sObj;
  }

  
  
  

  
  var chunks = aStr.split(":");

  
  if (chunks.length == 1) {
    sObj._host = CSPHost.fromString(chunks[0]);
    if (!sObj._host) {
      CSPError("Couldn't parse invalid source " + aStr);
      return null;
    }

    
    if (enforceSelfChecks) {
      
      if (!sObj.scheme || !sObj.port) {
        CSPError("Can't create host-only source " + aStr + " without 'self' data");
        return null;
      }
    }
    return sObj;
  }

  
  
  
  if (chunks.length == 2) {

    
    if (chunks[1] === "*" || chunks[1].match(/^\d+$/)) {
      sObj._port = chunks[1];
      
      if (chunks[0] !== "") {
        sObj._host = CSPHost.fromString(chunks[0]);
        if (!sObj._host) {
          CSPError("Couldn't parse invalid source " + aStr);
          return null;
        }
      }
      
      
      
      if (enforceSelfChecks) {
        
        if (!sObj.scheme || !sObj.host || !sObj.port) {
          CSPError("Can't create source " + aStr + " without 'self' data");
          return null;
        }
      }
    }
    
    else if (CSPSource.validSchemeName(chunks[0])) {
      sObj._scheme = chunks[0];
      
      if (chunks[1] === "") {
        
        
        
        if (!sObj._host) sObj._host = "*";
        if (!sObj._port) sObj._port = "*";
      } else {
        
        
        var cleanHost = chunks[1].replace(/^\/{0,3}/,"");
        
        sObj._host = CSPHost.fromString(cleanHost);
        if (!sObj._host) {
          CSPError("Couldn't parse invalid host " + cleanHost);
          return null;
        }
      }

      
      
      if (enforceSelfChecks) {
        
        if (!sObj.scheme || !sObj.host || !sObj.port) {
          CSPError("Can't create source " + aStr + " without 'self' data");
          return null;
        }
      }
    }
    else  {
      
      CSPError("Couldn't parse invalid source " + aStr);
      return null;
    }

    return sObj;
  }

  
  if (!CSPSource.validSchemeName(chunks[0])) {
    CSPError("Couldn't parse scheme in " + aStr);
    return null;
  }
  sObj._scheme = chunks[0];
  if (!(chunks[2] === "*" || chunks[2].match(/^\d+$/))) {
    CSPError("Couldn't parse port in " + aStr);
    return null;
  }

  sObj._port = chunks[2];

  
  var cleanHost = chunks[1].replace(/^\/{0,3}/,"");
  sObj._host = CSPHost.fromString(cleanHost);

  return sObj._host ? sObj : null;
};

CSPSource.validSchemeName = function(aStr) {
  
  
  
  
  
  return aStr.match(/^[a-zA-Z][a-zA-Z0-9+.-]*$/);
};

CSPSource.prototype = {

  get scheme () {
    if (!this._scheme && this._self)
      return this._self.scheme;
    return this._scheme;
  },

  get host () {
    if (!this._host && this._self)
      return this._self.host;
    return this._host;
  },

  




  get port () {
    if (this._port) return this._port;
    
    if (this._scheme) {
      try {
        var port = gIoService.getProtocolHandler(this._scheme).defaultPort;
        if (port > 0) return port;
      } catch(e) {
        
      }
    }
    
    if (this._self && this._self.port) return this._self.port;

    return undefined;
  },

  



  toString:
  function() {
    if (this._isSelf) 
      return this._self.toString();

    var s = "";
    if (this._scheme)
      s = s + this._scheme + "://";
    if (this._host)
      s = s + this._host;
    if (this._port)
      s = s + ":" + this._port;
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
      CSPError("Could not intersect " + this + " with " + that
               + " due to port problems.");
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
      CSPError("Could not intersect " + this + " with " + that
               + " due to scheme problems.");
      return null;
    }

    
    if (!this._host)
      newSource._host = that._host;
    else if (!that._host)
      newSource._host = this._host;
    else 
      newSource._host = this._host.intersectWith(that._host);

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
    if (!aHost) return false;

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
