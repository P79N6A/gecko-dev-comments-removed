














































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const CSP_VIOLATION_TOPIC = "csp-on-violate-policy";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/CSPUtils.jsm");



function ContentSecurityPolicy() {
  CSPdebug("CSP CREATED");
  this._isInitialized = false;
  this._enforcedPolicy = null;
  this._reportOnlyPolicy = null;
  this._requestHeaders = [];
  this._request = "";

  this._observerService = Cc['@mozilla.org/observer-service;1']
                            .getService(Ci.nsIObserverService);
}




{
  let cp = Ci.nsIContentPolicy;
  let csp = ContentSecurityPolicy;
  let cspr_sd = CSPRep.SRC_DIRECTIVES;

  csp._MAPPINGS=[];

  
  csp._MAPPINGS[cp.TYPE_OTHER]             =  cspr_sd.ALLOW;

  
  csp._MAPPINGS[cp.TYPE_DOCUMENT]          =  null;

  
  csp._MAPPINGS[cp.TYPE_REFRESH]           =  null;

  
  csp._MAPPINGS[cp.TYPE_SCRIPT]            = cspr_sd.SCRIPT_SRC;
  csp._MAPPINGS[cp.TYPE_IMAGE]             = cspr_sd.IMG_SRC;
  csp._MAPPINGS[cp.TYPE_STYLESHEET]        = cspr_sd.STYLE_SRC;
  csp._MAPPINGS[cp.TYPE_OBJECT]            = cspr_sd.OBJECT_SRC;
  csp._MAPPINGS[cp.TYPE_OBJECT_SUBREQUEST] = cspr_sd.OBJECT_SRC;
  csp._MAPPINGS[cp.TYPE_SUBDOCUMENT]       = cspr_sd.FRAME_SRC;
  csp._MAPPINGS[cp.TYPE_MEDIA]             = cspr_sd.MEDIA_SRC;
  csp._MAPPINGS[cp.TYPE_FONT]              = cspr_sd.FONT_SRC;
  csp._MAPPINGS[cp.TYPE_XMLHTTPREQUEST]    = cspr_sd.XHR_SRC;


  
  csp._MAPPINGS[cp.TYPE_XBL]               = cspr_sd.ALLOW;
  csp._MAPPINGS[cp.TYPE_PING]              = cspr_sd.ALLOW;
  csp._MAPPINGS[cp.TYPE_DTD]               = cspr_sd.ALLOW;
}

ContentSecurityPolicy.prototype = {
  classID:          Components.ID("{AB36A2BF-CB32-4AA6-AB41-6B4E4444A221}"),
  QueryInterface:   XPCOMUtils.generateQI([Ci.nsIContentSecurityPolicy]),

  get isInitialized() {
    return this._isInitialized;
  },

  set isInitialized (foo) {
    this._isInitialized = foo;
  },

  get policy () {
    return this._policy.toString();
  },

  




  get allowsInlineScript() {
    
    
    var violation = Cc["@mozilla.org/supports-cstring;1"]
                      .createInstance(Ci.nsISupportsCString);
    violation.data = 'base restriction: no inline scripts';

    
    
    if (this._reportOnlyPolicy && !this._reportOnlyPolicy.allowsInlineScripts) {
      this._observerService.notifyObservers(
                              violation,
                              CSP_VIOLATION_TOPIC,
                              'inline script base restriction');
      this._sendReports(this._reportOnlyPolicy, 'self', violation.data);
    }

    
    
    if (this._enforcedPolicy && !this._enforcedPolicy.allowsInlineScripts) {
      this._observerService.notifyObservers(
                              violation,
                              CSP_VIOLATION_TOPIC,
                              'inline script base restriction');
      this._sendReports(this._enforcedPolicy, 'self', violation.data);
      return false;
    }

    
    return true;
  },

  




  get allowsEval() {
    
    
    var violation = Cc["@mozilla.org/supports-cstring;1"]
                             .createInstance(Ci.nsISupportsCString);
    violation.data = 'base restriction: no eval-like function calls';

    
    
    if (this._reportOnlyPolicy && !this._reportOnlyPolicy.allowsEvalInScripts) {
      this._observerService.notifyObservers(
                              violation,
                              CSP_VIOLATION_TOPIC,
                              'eval script base restriction');
      this._sendReports(this._reportOnlyPolicy, 'self', violation.data);
    }

    
    if (this._enforcedPolicy && !this._enforcedPolicy.allowsEvalInScripts) {
      this._observerService.notifyObservers(
                              violation,
                              CSP_VIOLATION_TOPIC,
                              'eval script base restriction');
      this._sendReports(this._enforcedPolicy, 'self', violation.data);
      return false;
    }

    return true;
  },

  







  


  scanRequestData:
  function(aChannel) {
    if (!aChannel)
      return;
    
    var internalChannel = aChannel.QueryInterface(Ci.nsIHttpChannelInternal);
    var reqMaj = {};
    var reqMin = {};
    var reqVersion = internalChannel.getRequestVersion(reqMaj, reqMin);
    this._request = aChannel.requestMethod + " " 
                  + aChannel.URI.asciiSpec
                  + " HTTP/" + reqMaj.value + "." + reqMin.value;

    
    var self = this;
    aChannel.visitRequestHeaders({
      visitHeader: function(aHeader, aValue) {
        self._requestHeaders.push(aHeader + ": " + aValue);
      }});
  },



  function csp_refinePolicyInternal(policyToRefine, aPolicy, selfURI) {
    CSPdebug("     REFINING: " + policyToRefine);
    CSPdebug("REFINE POLICY: " + aPolicy);
    CSPdebug("         SELF: " + selfURI.asciiSpec);

    
    this._isInitialized = false;

    
    
    var newpolicy = CSPRep.fromString(aPolicy,
                                      selfURI.scheme + "://" + selfURI.hostPort);

    
    if (!this[policyToRefine])
      this[policyToRefine] = newpolicy;
    else
      this[policyToRefine] = this._policy.intersectWith(newpolicy);

    
    this._isInitialized = true;
  },

  





  refineEnforcedPolicy:
  function csp_refineEnforcedPolicy(aPolicy, selfURI) {
    return this._refinePolicyInternal("_enforcedPolicy", aPolicy, selfURI);
  },

  


  refineReportOnlyPolicy:
  function csp_refineReportOnlyPolicy(aPolicy, selfURI) {
    return this._refinePolicyInternal("_reportOnlyPolicy", aPolicy, selfURI);
  },

  


  _sendReports:
  function(policyViolated, blockedUri, violatedDirective) {
    var uriString = policyViolated.getReportURIs();
    var uris = uriString.split(/\s+/);
    if (uris.length > 0) {
      
      
      
      
      
      
      
      
      
      
      
      var strHeaders = "";
      for (let i in this._requestHeaders) {
        strHeaders += this._requestHeaders[i] + "\n";
      }
      var report = {
        'csp-report': {
          'request': this._request,
          'request-headers': strHeaders,
          'blocked-uri': (blockedUri instanceof Ci.nsIURI ?
                          blockedUri.asciiSpec : blockedUri),
          'violated-directive': violatedDirective
        }
      }
      CSPdebug("Constructed violation report:\n" + JSON.stringify(report));

      
      for (let i in uris) {
        if (uris[i] === "")
          continue;

        var failure = function(aEvt) {  
          if (req.readyState == 4 && req.status != 200) {
            CSPError("Failed to send report to " + reportURI);
          }  
        };  
        var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]  
                    .createInstance(Ci.nsIXMLHttpRequest);  

        try {
          req.open("POST", uris[i], true);
          req.setRequestHeader('Content-Type', 'application/json');
          req.upload.addEventListener("error", failure, false);
          req.upload.addEventListener("abort", failure, false);
          
 
          
          
          
          
          req.channel.loadFlags |= Ci.nsIChannel.LOAD_ANONYMOUS;

          req.send(JSON.stringify(report));
          CSPdebug("Sent violation report to " + uris[i]);
        } catch(e) {
          
          
          CSPWarning("Tried to send report to invalid URI: \"" + uris[i] + "\"");
        }
      }
    }
  },

  







  permitsAncestry:
  function(docShell) {
    if (!docShell) { return false; }
    CSPdebug(" in permitsAncestry(), docShell = " + docShell);

    
    var dst = docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                      .getInterface(Ci.nsIDocShellTreeItem);

    
    var ancestors = [];
    while (dst.parent) {
      dst = dst.parent;
      let it = dst.QueryInterface(Ci.nsIInterfaceRequestor)
                  .getInterface(Ci.nsIWebNavigation);
      if (it.currentURI) {
        if (it.currentURI.scheme === "chrome") {
          break;
        }
        let ancestor = it.currentURI;
        CSPdebug(" found frame ancestor " + ancestor.asciiSpec);
        ancestors.push(ancestor);
      }
    } 

    
    let cspContext = CSPRep.SRC_DIRECTIVES.FRAME_ANCESTORS;
    for (let i in ancestors) {
      let ancestor = ancestors[i].prePath;

      
      if (this._reportOnlyPolicy) {
        let directive = this._reportOnlyPolicy._directives[cspContext];
        let violatedPolicy = (directive._isImplicit
                              ? 'allow' : 'frame-ancestors ')
                              + directive.toString();
        if (!this._reportOnlyPolicy.permits(ancestor, cspContext)) {
          this._observerService.notifyObservers(
                                  ancestors[i],
                                  CSP_VIOLATION_TOPIC,
                                  violatedPolicy);
          this._sendReports(this._enforcedPolicy, ancestors[i].asciiSpec, violatedPolicy);
        }
      }

      
      if (this._enforcedPolicy) {
        let directive = this._enforcedPolicy._directives[cspContext];
        let violatedPolicy = (directive._isImplicit
                              ? 'allow' : 'frame-ancestors ')
                              + directive.toString();
        if(!this._enforcedPolicy.permits(ancestor, cspContext)) {
          this._observerService.notifyObservers(
                                  ancestors[i],
                                  CSP_VIOLATION_TOPIC,
                                  violatedPolicy);
          this._sendReports(this._enforcedPolicy, ancestors[i].asciiSpec, violatedPolicy);
          return false;
        }
      }
    }

    
    return true;
  },

  




  shouldLoad:
  function csp_shouldLoad(aContentType, 
                          aContentLocation, 
                          aRequestOrigin, 
                          aContext, 
                          aMimeTypeGuess, 
                          aExtra) {

    
    if (aContentLocation.scheme === 'chrome') {
      return Ci.nsIContentPolicy.ACCEPT;
    }

    
    CSPdebug("shouldLoad location = " + aContentLocation.asciiSpec);
    CSPdebug("shouldLoad content type = " + aContentType);
    var cspContext = ContentSecurityPolicy._MAPPINGS[aContentType];
    

    
    if (!cspContext) {
      return Ci.nsIContentPolicy.ACCEPT;
    }

    
    

    
    if (this._reportOnlyPolicy) {
      var ro_res = this._reportOnlyPolicy.permits(aContentLocation, cspContext)
                    ? Ci.nsIContentPolicy.ACCEPT
                    : Ci.nsIContentPolicy.REJECT_SERVER;

      if (ro_res != Ci.nsIContentPolicy.ACCEPT) {
        try {
          let directive = this._reportOnlyPolicy._directives[cspContext];
          let violatedPolicy = (directive._isImplicit
                                  ? 'allow' : cspContext)
                                  + ' ' + directive.toString();
          this._observerService.notifyObservers(
                                  aContentLocation,
                                  CSP_VIOLATION_TOPIC,
                                  violatedPolicy);
          this._sendReports(this._reportOnlyPolicy, aContentLocation, violatedPolicy);
        } catch(e) {
          CSPdebug('---------------- ERROR: ' + e);
        }
      }
    }

    
    if (this._enforcedPolicy) {
      var en_res = this._enforcedPolicy.permits(aContentLocation, cspContext)
                    ? Ci.nsIContentPolicy.ACCEPT
                    : Ci.nsIContentPolicy.REJECT_SERVER;

      if (en_res != Ci.nsIContentPolicy.ACCEPT) {
        CSPdebug("blocking request for " + aContentLocation.asciiSpec);
        try {
          let directive = this._enforcedPolicy._directives[cspContext];
          let violatedPolicy = (directive._isImplicit
                                  ? 'allow' : cspContext)
                                  + ' ' + directive.toString();
          this._observerService.notifyObservers(
                                  aContentLocation,
                                  CSP_VIOLATION_TOPIC,
                                  violatedPolicy);
          this._sendReports(this._enforcedPolicy, aContentLocation, violatedPolicy);
        } catch(e) {
          CSPdebug('---------------- ERROR: ' + e);
        }
        return en_res;
      }
    }

    return Ci.nsIContentPolicy.ACCEPT;
  },

  shouldProcess:
  function csp_shouldProcess(aContentType,
                             aContentLocation,
                             aRequestOrigin,
                             aContext,
                             aMimeType,
                             aExtra) {
    
    var res = Ci.nsIContentPolicy.ACCEPT;
    CSPdebug("shouldProcess aContext=" + aContext);
    return res;
  },

};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([ContentSecurityPolicy]);
