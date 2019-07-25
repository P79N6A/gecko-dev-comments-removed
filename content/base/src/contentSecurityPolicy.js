














































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const CSP_VIOLATION_TOPIC = "csp-on-violate-policy";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/CSPUtils.jsm");



function ContentSecurityPolicy() {
  CSPdebug("CSP CREATED");
  this._isInitialized = false;
  this._reportOnlyMode = false;
  this._policy = CSPRep.fromString("default-src *");

  
  this._policy._allowInlineScripts = true;
  this._policy._allowEval = true;

  this._requestHeaders = []; 
  this._request = "";
  this._docRequest = null;
  CSPdebug("CSP POLICY INITED TO 'default-src *'");
}




{
  let cp = Ci.nsIContentPolicy;
  let csp = ContentSecurityPolicy;
  let cspr_sd = CSPRep.SRC_DIRECTIVES;

  csp._MAPPINGS=[];

  
  csp._MAPPINGS[cp.TYPE_OTHER]             =  cspr_sd.DEFAULT_SRC;

  
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


  
  csp._MAPPINGS[cp.TYPE_XBL]               = cspr_sd.DEFAULT_SRC;
  csp._MAPPINGS[cp.TYPE_PING]              = cspr_sd.DEFAULT_SRC;
  csp._MAPPINGS[cp.TYPE_DTD]               = cspr_sd.DEFAULT_SRC;
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
    return this._reportOnlyMode || this._policy.allowsInlineScripts;
  },

  get allowsEval() {
    return this._reportOnlyMode || this._policy.allowsEvalInScripts;
  },

  












  logViolationDetails:
  function(aViolationType, aSourceFile, aScriptSample, aLineNum) {
    
    
    
    switch (aViolationType) {
    case Ci.nsIContentSecurityPolicy.VIOLATION_TYPE_INLINE_SCRIPT:
      if (!this._policy.allowsInlineScripts)
        this._asyncReportViolation('self','inline script base restriction',
                                   'violated base restriction: Inline Scripts will not execute',
                                   aSourceFile, aScriptSample, aLineNum);
      break;
    case Ci.nsIContentSecurityPolicy.VIOLATION_TYPE_EVAL:
      if (!this._policy.allowsEvalInScripts)
        this._asyncReportViolation('self','eval script base restriction',
                                   'violated base restriction: Code will not be created from strings',
                                   aSourceFile, aScriptSample, aLineNum);
      break;
    }
  },

  set reportOnlyMode(val) {
    this._reportOnlyMode = val;
  },

  get reportOnlyMode () {
    return this._reportOnlyMode;
  },

  







  


  scanRequestData:
  function(aChannel) {
    if (!aChannel)
      return;
    
    var internalChannel = null;
    try {
      internalChannel = aChannel.QueryInterface(Ci.nsIHttpChannelInternal);
    } catch (e) {
      CSPdebug("No nsIHttpChannelInternal for " + aChannel.URI.asciiSpec);
    }

    this._request = aChannel.requestMethod + " " + aChannel.URI.asciiSpec;
    this._docRequest = aChannel;

    
    
    if (internalChannel) {
      var reqMaj = {};
      var reqMin = {};
      var reqVersion = internalChannel.getRequestVersion(reqMaj, reqMin);
      this._request += " HTTP/" + reqMaj.value + "." + reqMin.value;
    }

    
    var self = this;
    aChannel.visitRequestHeaders({
      visitHeader: function(aHeader, aValue) {
        self._requestHeaders.push(aHeader + ": " + aValue);
      }});
  },



  





  refinePolicy:
  function csp_refinePolicy(aPolicy, selfURI) {
    CSPdebug("REFINE POLICY: " + aPolicy);
    CSPdebug("         SELF: " + selfURI.asciiSpec);
    
    
    
    if (selfURI instanceof Ci.nsINestedURI) {
      CSPdebug("        INNER: " + selfURI.innermostURI.asciiSpec);
      selfURI = selfURI.innermostURI;
    }

    
    this._isInitialized = false;

    
    
    var newpolicy = CSPRep.fromString(aPolicy,
                                      selfURI.scheme + "://" + selfURI.hostPort,
                                      this._docRequest,
                                      this);

    
    var intersect = this._policy.intersectWith(newpolicy);
 
    
    this._policy = intersect;
    this._isInitialized = true;
  },

  


  sendReports:
  function(blockedUri, violatedDirective, aSourceFile, aScriptSample, aLineNum) {
    var uriString = this._policy.getReportURIs();
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
      
      if (aSourceFile)
        report["csp-report"]["source-file"] = aSourceFile;
      if (aScriptSample)
        report["csp-report"]["script-sample"] = aScriptSample;
      if (aLineNum)
        report["csp-report"]["line-number"] = aLineNum;

      CSPdebug("Constructed violation report:\n" + JSON.stringify(report));

      CSPWarning("Directive \"" + violatedDirective + "\" violated"
               + (blockedUri['asciiSpec'] ? " by " + blockedUri.asciiSpec : ""),
                 (aSourceFile) ? aSourceFile : null,
                 (aScriptSample) ? decodeURIComponent(aScriptSample) : null,
                 (aLineNum) ? aLineNum : null);

      
      
      
      
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
      if (!this._policy.permits(ancestor, cspContext)) {
        
        let directive = this._policy._directives[cspContext];
        let violatedPolicy = (directive._isImplicit
                                ? 'default-src' : 'frame-ancestors ')
                                + directive.toString();

        this._asyncReportViolation(ancestors[i], violatedPolicy);

        
        return this._reportOnlyMode;
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

    
    if (aContentLocation.scheme === 'chrome' ||
        aContentLocation.scheme === 'resource') {
      return Ci.nsIContentPolicy.ACCEPT;
    }

    
    CSPdebug("shouldLoad location = " + aContentLocation.asciiSpec);
    CSPdebug("shouldLoad content type = " + aContentType);
    var cspContext = ContentSecurityPolicy._MAPPINGS[aContentType];

    
    if (!cspContext) {
      return Ci.nsIContentPolicy.ACCEPT;
    }

    
    
    var res = this._policy.permits(aContentLocation, cspContext)
              ? Ci.nsIContentPolicy.ACCEPT 
              : Ci.nsIContentPolicy.REJECT_SERVER;

    

    
    if (res != Ci.nsIContentPolicy.ACCEPT) { 
      CSPdebug("blocking request for " + aContentLocation.asciiSpec);
      try {
        let directive = this._policy._directives[cspContext];
        let violatedPolicy = (directive._isImplicit
                                ? 'default-src' : cspContext)
                                + ' ' + directive.toString();
        this._asyncReportViolation(aContentLocation, violatedPolicy);
      } catch(e) {
        CSPdebug('---------------- ERROR: ' + e);
      }
    }

    return (this._reportOnlyMode ? Ci.nsIContentPolicy.ACCEPT : res);
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

  



















  _asyncReportViolation:
  function(blockedContentSource, violatedDirective, observerSubject,
           aSourceFile, aScriptSample, aLineNum) {
    
    
    if (!observerSubject)
      observerSubject = blockedContentSource;

    
    
    
    if (!(observerSubject instanceof Ci.nsISupports)) {
      let d = observerSubject;
      observerSubject = Cc["@mozilla.org/supports-cstring;1"]
                          .createInstance(Ci.nsISupportsCString);
      observerSubject.data = d;
    }

    var reportSender = this;
    Services.tm.mainThread.dispatch(
      function() {
        Services.obs.notifyObservers(observerSubject,
                                     CSP_VIOLATION_TOPIC,
                                     violatedDirective);
        reportSender.sendReports(blockedContentSource, violatedDirective,
                                 aSourceFile, aScriptSample, aLineNum);
      }, Ci.nsIThread.DISPATCH_NORMAL);
  },
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([ContentSecurityPolicy]);
