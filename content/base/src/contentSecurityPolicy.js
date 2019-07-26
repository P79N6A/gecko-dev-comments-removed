














const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const CSP_VIOLATION_TOPIC = "csp-on-violate-policy";



const CSP_TYPE_XMLHTTPREQUEST_SPEC_COMPLIANT = "csp_type_xmlhttprequest_spec_compliant";
const CSP_TYPE_WEBSOCKET_SPEC_COMPLIANT = "csp_type_websocket_spec_compliant";

const WARN_FLAG = Ci.nsIScriptError.warningFlag;
const ERROR_FLAG = Ci.nsIScriptError.ERROR_FLAG;

const INLINE_STYLE_VIOLATION_OBSERVER_SUBJECT = 'violated base restriction: Inline Stylesheets will not apply';
const INLINE_SCRIPT_VIOLATION_OBSERVER_SUBJECT = 'violated base restriction: Inline Scripts will not execute';
const EVAL_VIOLATION_OBSERVER_SUBJECT = 'violated base restriction: Code will not be created from strings';


const CSP_CACHE_URI_CUTOFF_SIZE = 512;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/CSPUtils.jsm");



function ContentSecurityPolicy() {
  CSPdebug("CSP CREATED");
  this._isInitialized = false;

  this._policies = [];

  this._request = "";
  this._requestOrigin = "";
  this._requestPrincipal = "";
  this._referrer = "";
  this._docRequest = null;
  CSPdebug("CSP object initialized, no policies to enforce yet");

  this._cache = { };
}




{
  let cp = Ci.nsIContentPolicy;
  let csp = ContentSecurityPolicy;
  let cspr_sd_old = CSPRep.SRC_DIRECTIVES_OLD;
  let cspr_sd_new = CSPRep.SRC_DIRECTIVES_NEW;

  csp._MAPPINGS=[];

  
  
  csp._MAPPINGS[cp.TYPE_OTHER]             =  cspr_sd_new.DEFAULT_SRC;

  
  csp._MAPPINGS[cp.TYPE_DOCUMENT]          =  null;

  
  csp._MAPPINGS[cp.TYPE_REFRESH]           =  null;

  
  
  csp._MAPPINGS[cp.TYPE_SCRIPT]            = cspr_sd_new.SCRIPT_SRC;
  csp._MAPPINGS[cp.TYPE_IMAGE]             = cspr_sd_new.IMG_SRC;
  csp._MAPPINGS[cp.TYPE_STYLESHEET]        = cspr_sd_new.STYLE_SRC;
  csp._MAPPINGS[cp.TYPE_OBJECT]            = cspr_sd_new.OBJECT_SRC;
  csp._MAPPINGS[cp.TYPE_OBJECT_SUBREQUEST] = cspr_sd_new.OBJECT_SRC;
  csp._MAPPINGS[cp.TYPE_SUBDOCUMENT]       = cspr_sd_new.FRAME_SRC;
  csp._MAPPINGS[cp.TYPE_MEDIA]             = cspr_sd_new.MEDIA_SRC;
  csp._MAPPINGS[cp.TYPE_FONT]              = cspr_sd_new.FONT_SRC;

  



  csp._MAPPINGS[cp.TYPE_XMLHTTPREQUEST]    = cspr_sd_old.XHR_SRC;
  csp._MAPPINGS[cp.TYPE_WEBSOCKET]         = cspr_sd_old.XHR_SRC;

  
  csp._MAPPINGS[cp.TYPE_CSP_REPORT]        = null;

  
  csp._MAPPINGS[cp.TYPE_XBL]               = cspr_sd_new.DEFAULT_SRC;
  csp._MAPPINGS[cp.TYPE_PING]              = cspr_sd_new.DEFAULT_SRC;
  csp._MAPPINGS[cp.TYPE_DTD]               = cspr_sd_new.DEFAULT_SRC;

  
  
  
  
  
  csp._MAPPINGS[CSP_TYPE_XMLHTTPREQUEST_SPEC_COMPLIANT]    = cspr_sd_new.CONNECT_SRC;
  csp._MAPPINGS[CSP_TYPE_WEBSOCKET_SPEC_COMPLIANT]         = cspr_sd_new.CONNECT_SRC;
  
  
  
  
}

ContentSecurityPolicy.prototype = {
  classID:          Components.ID("{d1680bb4-1ac0-4772-9437-1188375e44f2}"),
  QueryInterface:   XPCOMUtils.generateQI([Ci.nsIContentSecurityPolicy]),

  get isInitialized() {
    return this._isInitialized;
  },

  set isInitialized (foo) {
    this._isInitialized = foo;
  },

  _getPolicyInternal: function(index) {
    if (index < 0 || index >= this._policies.length) {
      throw Cr.NS_ERROR_FAILURE;
    }
    return this._policies[index];
  },

  _buildViolatedDirectiveString:
  function(aDirectiveName, aPolicy) {
    var SD = CSPRep.SRC_DIRECTIVES_NEW;
    var cspContext = (SD[aDirectiveName] in aPolicy._directives) ? SD[aDirectiveName] : SD.DEFAULT_SRC;
    var directive = aPolicy._directives[cspContext];
    return cspContext + ' ' + directive.toString();
  },

  


  getPolicy: function(index) {
    return this._getPolicyInternal(index).toString();
  },

  


  get numPolicies() {
    return this._policies.length;
  },

  getAllowsInlineScript: function(shouldReportViolations) {
    
    shouldReportViolations.value = this._policies.some(function(a) { return !a.allowsInlineScripts; });

    
    return this._policies.every(function(a) {
      return a._reportOnlyMode || a.allowsInlineScripts;
    });
  },

  getAllowsEval: function(shouldReportViolations) {
    
    shouldReportViolations.value = this._policies.some(function(a) { return !a.allowsEvalInScripts; });

    
    return this._policies.every(function(a) {
      return a._reportOnlyMode || a.allowsEvalInScripts;
    });
  },

  getAllowsInlineStyle: function(shouldReportViolations) {
    
    shouldReportViolations.value = this._policies.some(function(a) { return !a.allowsInlineStyles; });

    
    return this._policies.every(function(a) {
      return a._reportOnlyMode || a.allowsInlineStyles;
    });
  },

  












  logViolationDetails:
  function(aViolationType, aSourceFile, aScriptSample, aLineNum, violatedPolicyIndex) {
    for (let policyIndex=0; policyIndex < this._policies.length; policyIndex++) {
      let policy = this._policies[policyIndex];

      
      
      
      
      
      switch (aViolationType) {
      case Ci.nsIContentSecurityPolicy.VIOLATION_TYPE_INLINE_STYLE:
        if (!policy.allowsInlineStyles) {
          var violatedDirective = this._buildViolatedDirectiveString('STYLE_SRC', policy);
          this._asyncReportViolation('self', null, violatedDirective, policyIndex,
                                    INLINE_STYLE_VIOLATION_OBSERVER_SUBJECT,
                                    aSourceFile, aScriptSample, aLineNum);
        }
        break;
      case Ci.nsIContentSecurityPolicy.VIOLATION_TYPE_INLINE_SCRIPT:
        if (!policy.allowsInlineScripts)    {
          var violatedDirective = this._buildViolatedDirectiveString('SCRIPT_SRC', policy);
          this._asyncReportViolation('self', null, violatedDirective, policyIndex,
                                    INLINE_SCRIPT_VIOLATION_OBSERVER_SUBJECT,
                                    aSourceFile, aScriptSample, aLineNum);
          }
        break;
      case Ci.nsIContentSecurityPolicy.VIOLATION_TYPE_EVAL:
        if (!policy.allowsEvalInScripts) {
          var violatedDirective = this._buildViolatedDirectiveString('SCRIPT_SRC', policy);
          this._asyncReportViolation('self', null, violatedDirective, policyIndex,
                                    EVAL_VIOLATION_OBSERVER_SUBJECT,
                                    aSourceFile, aScriptSample, aLineNum);
        }
        break;
      }
    }
  },

  


  scanRequestData:
  function(aChannel) {
    if (!aChannel)
      return;

    
    this._docRequest = aChannel;

    
    let uri = aChannel.URI.cloneIgnoringRef();
    try { 
      uri.userPass = '';
    } catch (ex) {}
    this._request = uri.asciiSpec;
    this._requestOrigin = uri;

    
    this._requestPrincipal = Components.classes["@mozilla.org/scriptsecuritymanager;1"].
    getService(Components.interfaces.nsIScriptSecurityManager).getChannelPrincipal(aChannel);

    if (aChannel.referrer) {
      let referrer = aChannel.referrer.cloneIgnoringRef();
      try { 
        referrer.userPass = '';
      } catch (ex) {}
      this._referrer = referrer.asciiSpec;
    }
  },



  



  appendPolicy:
  function csp_appendPolicy(aPolicy, selfURI, aReportOnly, aSpecCompliant) {
#ifndef MOZ_B2G
    CSPdebug("APPENDING POLICY: " + aPolicy);
    CSPdebug("            SELF: " + selfURI.asciiSpec);
    CSPdebug("CSP 1.0 COMPLIANT : " + aSpecCompliant);
#endif

    
    
    
    if (selfURI instanceof Ci.nsINestedURI) {
#ifndef MOZ_B2G
      CSPdebug("        INNER: " + selfURI.innermostURI.asciiSpec);
#endif
      selfURI = selfURI.innermostURI;
    }

    
    var newpolicy;

    
    
    
    

    
    
    
    if (aSpecCompliant) {
      newpolicy = CSPRep.fromStringSpecCompliant(aPolicy,
                                                 selfURI,
                                                 this._docRequest,
                                                 this,
                                                 aReportOnly);
    } else {
      newpolicy = CSPRep.fromString(aPolicy,
                                    selfURI,
                                    this._docRequest,
                                    this,
                                    aReportOnly);
    }

    newpolicy._specCompliant = !!aSpecCompliant;
    newpolicy._isInitialized = true;
    this._policies.push(newpolicy);
    this._cache = {}; 
  },

  


  removePolicy:
  function csp_removePolicy(index) {
    if (index < 0 || index >= this._policies.length) {
      CSPdebug("Cannot remove policy " + index + "; not enough policies.");
      return;
    }
    this._policies.splice(index, 1);
    this._cache = {}; 
  },

  


  sendReports:
  function(blockedUri, originalUri, violatedDirective,
           violatedPolicyIndex, aSourceFile,
           aScriptSample, aLineNum) {

    let policy = this._getPolicyInternal(violatedPolicyIndex);
    if (!policy) {
      CSPdebug("ERROR in SendReports: policy " + violatedPolicyIndex + " is not defined.");
      return;
    }

    var uriString = policy.getReportURIs();
    var uris = uriString.split(/\s+/);
    if (uris.length > 0) {
      
      let blocked = '';
      if (originalUri) {
        
        try {
          let clone = blockedUri.clone();
          clone.path = '';
          blocked = clone.asciiSpec;
        } catch(e) {
          CSPdebug(".... blockedUri can't be cloned: " + blockedUri);
        }
      }
      else if (blockedUri instanceof Ci.nsIURI) {
        blocked = blockedUri.cloneIgnoringRef().asciiSpec;
      }
      else {
        
        blocked = blockedUri;
      }

      
      
      
      
      
      
      
      
      
      var report = {
        'csp-report': {
          'document-uri': this._request,
          'referrer': this._referrer,
          'blocked-uri': blocked,
          'violated-directive': violatedDirective
        }
      }

      
      if (originalUri)
        report["csp-report"]["original-uri"] = originalUri.cloneIgnoringRef().asciiSpec;
      if (aSourceFile)
        report["csp-report"]["source-file"] = aSourceFile;
      if (aScriptSample)
        report["csp-report"]["script-sample"] = aScriptSample;
      if (aLineNum)
        report["csp-report"]["line-number"] = aLineNum;

      var reportString = JSON.stringify(report);
      CSPdebug("Constructed violation report:\n" + reportString);

      
      
      
      
      for (let i in uris) {
        if (uris[i] === "")
          continue;

        try {
          var chan = Services.io.newChannel(uris[i], null, null);
          if (!chan) {
            CSPdebug("Error creating channel for " + uris[i]);
            continue;
          }

          var content = Cc["@mozilla.org/io/string-input-stream;1"]
                          .createInstance(Ci.nsIStringInputStream);
          content.data = reportString + "\n\n";

          
          
          chan.loadFlags |= Ci.nsIChannel.LOAD_ANONYMOUS;

          
          
          chan.notificationCallbacks = new CSPReportRedirectSink(policy);
          if (this._docRequest) {
            chan.loadGroup = this._docRequest.loadGroup;
          }

          chan.QueryInterface(Ci.nsIUploadChannel)
              .setUploadStream(content, "application/json", content.available());

          try {
            
            chan.QueryInterface(Ci.nsIHttpChannel);
            chan.requestMethod = "POST";
          } catch(e) {} 

          
          
          try {
            var contentPolicy = Cc["@mozilla.org/layout/content-policy;1"]
                                  .getService(Ci.nsIContentPolicy);
            if (contentPolicy.shouldLoad(Ci.nsIContentPolicy.TYPE_CSP_REPORT,
                                         chan.URI, this._requestOrigin,
                                         null, null, null, this._requestPrincipal)
                != Ci.nsIContentPolicy.ACCEPT) {
              continue; 
            }
          } catch(e) {
            continue; 
          }

          
          chan.asyncOpen(new CSPViolationReportListener(uris[i]), null);
          CSPdebug("Sent violation report to " + uris[i]);
        } catch(e) {
          
          
          policy.log(WARN_FLAG, CSPLocalizer.getFormatStr("triedToSendReport", [uris[i]]));
          policy.log(WARN_FLAG, CSPLocalizer.getFormatStr("errorWas", [e.toString()]));
        }
      }
    }
  },

  


  logToConsole:
  function(blockedUri, originalUri, violatedDirective, aViolatedPolicyIndex,
           aSourceFile, aScriptSample, aLineNum, aObserverSubject) {
     let policy = this._policies[aViolatedPolicyIndex];
     switch(aObserverSubject.data) {
      case INLINE_STYLE_VIOLATION_OBSERVER_SUBJECT:
        violatedDirective = CSPLocalizer.getStr("inlineStyleBlocked");
        break;
      case INLINE_SCRIPT_VIOLATION_OBSERVER_SUBJECT:
        violatedDirective = CSPLocalizer.getStr("inlineScriptBlocked");
        break;
      case EVAL_VIOLATION_OBSERVER_SUBJECT:
        violatedDirective = CSPLocalizer.getStr("scriptFromStringBlocked");
        break;
    }
    var violationMessage = null;
    if (blockedUri["asciiSpec"]) {
      violationMessage = CSPLocalizer.getFormatStr("CSPViolationWithURI", [violatedDirective, blockedUri.asciiSpec]);
    } else {
      violationMessage = CSPLocalizer.getFormatStr("CSPViolation", [violatedDirective]);
    }
    policy.log(WARN_FLAG, violationMessage,
               (aSourceFile) ? aSourceFile : null,
               (aScriptSample) ? decodeURIComponent(aScriptSample) : null,
               (aLineNum) ? aLineNum : null);
  },










  permitsAncestry:
  function(docShell) {
    
    
    var permitted = true;
    for (let i = 0; i < this._policies.length; i++) {
      if (!this._permitsAncestryInternal(docShell, this._policies[i], i)) {
        permitted = false;
      }
    }
    return permitted;
  },

  _permitsAncestryInternal:
  function(docShell, policy, policyIndex) {
    if (!docShell) { return false; }

    
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
        
        let ancestor = it.currentURI.cloneIgnoringRef();
        try { 
          ancestor.userPass = '';
        } catch (ex) {}

#ifndef MOZ_B2G
        CSPdebug(" found frame ancestor " + ancestor.asciiSpec);
#endif
        ancestors.push(ancestor);
      }
    }

    
    
    
    let cspContext = CSPRep.SRC_DIRECTIVES_NEW.FRAME_ANCESTORS;
    for (let i in ancestors) {
      let ancestor = ancestors[i];
      if (!policy.permits(ancestor, cspContext)) {
        
        let directive = policy._directives[cspContext];
        let violatedPolicy = 'frame-ancestors ' + directive.toString();

        this._asyncReportViolation(ancestors[i], null, violatedPolicy,
                                   policyIndex);

        
        return policy._reportOnlyMode;
      }
    }
    return true;
  },

  


  _createCacheKey:
  function (aContentLocation, aContentType) {
    if (aContentType != Ci.nsIContentPolicy.TYPE_SCRIPT &&
        aContentLocation.scheme == "data") {
      
      return aContentLocation.scheme + ":" + aContentType;
    }

    let uri = aContentLocation.spec;
    if (uri.length > CSP_CACHE_URI_CUTOFF_SIZE) {
      
      return null;
    }
    return uri + "!" + aContentType;
  },

  




  shouldLoad:
  function csp_shouldLoad(aContentType,
                          aContentLocation,
                          aRequestOrigin,
                          aContext,
                          aMimeTypeGuess,
                          aOriginalUri) {
    let key = this._createCacheKey(aContentLocation, aContentType);
    if (key && this._cache[key]) {
      return this._cache[key];
    }

#ifndef MOZ_B2G
    
    CSPdebug("shouldLoad location = " + aContentLocation.asciiSpec);
    CSPdebug("shouldLoad content type = " + aContentType);
#endif
    
    var cspContext = ContentSecurityPolicy._MAPPINGS[aContentType];

    
    
    var cspContext;

    let cp = Ci.nsIContentPolicy;

    
    
    
    let policyAllowsLoadArray = [];
    for (let policyIndex=0; policyIndex < this._policies.length; policyIndex++) {
      let policy = this._policies[policyIndex];

#ifndef MOZ_B2G
      CSPdebug("policy is " + (policy._specCompliant ?
                              "1.0 compliant" : "pre-1.0"));
      CSPdebug("policy is " + (policy._reportOnlyMode ?
                              "report-only" : "blocking"));
#endif

      if (aContentType == cp.TYPE_XMLHTTPREQUEST && this._policies[policyIndex]._specCompliant) {
        cspContext = ContentSecurityPolicy._MAPPINGS[CSP_TYPE_XMLHTTPREQUEST_SPEC_COMPLIANT];
      } else if (aContentType == cp.TYPE_WEBSOCKET && this._policies[policyIndex]._specCompliant) {
        cspContext = ContentSecurityPolicy._MAPPINGS[CSP_TYPE_WEBSOCKET_SPEC_COMPLIANT];
      } else {
        cspContext = ContentSecurityPolicy._MAPPINGS[aContentType];
      }

#ifndef MOZ_B2G
      CSPdebug("shouldLoad cspContext = " + cspContext);
#endif

      
      if (!cspContext) {
        return Ci.nsIContentPolicy.ACCEPT;
      }

      
      
      var res = policy.permits(aContentLocation, cspContext)
                ? cp.ACCEPT : cp.REJECT_SERVER;
      
      policyAllowsLoadArray.push(res == cp.ACCEPT || policy._reportOnlyMode);

      

      
      if (res != Ci.nsIContentPolicy.ACCEPT) {
        CSPdebug("blocking request for " + aContentLocation.asciiSpec);
        try {
          let directive = "unknown directive",
              violatedPolicy = "unknown policy";

          
          
          
          if (policy._directives.hasOwnProperty(cspContext)) {
            directive = policy._directives[cspContext];
            violatedPolicy = cspContext + ' ' + directive.toString();
          } else if (policy._directives.hasOwnProperty("default-src")) {
            directive = policy._directives["default-src"];
            violatedPolicy = "default-src " + directive.toString();
          } else {
            violatedPolicy = "unknown directive";
            CSPdebug('ERROR in blocking content: ' +
                    'CSP is not sure which part of the policy caused this block');
          }

          this._asyncReportViolation(aContentLocation, aOriginalUri,
                                     violatedPolicy, policyIndex);
        } catch(e) {
          CSPdebug('---------------- ERROR: ' + e);
        }
      }
    } 

    
    
    
    let ret = (policyAllowsLoadArray.some(function(a,b) { return !a; }) ?
               cp.REJECT_SERVER : cp.ACCEPT);

    if (key) {
      this._cache[key] = ret;
    }
    return ret;
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
  function(aBlockedContentSource, aOriginalUri, aViolatedDirective,
           aViolatedPolicyIndex, aObserverSubject,
           aSourceFile, aScriptSample, aLineNum) {
    
    
    if (!aObserverSubject)
      aObserverSubject = aBlockedContentSource;

    
    
    
    if (!(aObserverSubject instanceof Ci.nsISupports)) {
      let d = aObserverSubject;
      aObserverSubject = Cc["@mozilla.org/supports-cstring;1"]
                          .createInstance(Ci.nsISupportsCString);
      aObserverSubject.data = d;
    }

    var reportSender = this;
    Services.tm.mainThread.dispatch(
      function() {
        Services.obs.notifyObservers(aObserverSubject,
                                     CSP_VIOLATION_TOPIC,
                                     aViolatedDirective);
        reportSender.sendReports(aBlockedContentSource, aOriginalUri,
                                 aViolatedDirective, aViolatedPolicyIndex,
                                 aSourceFile, aScriptSample, aLineNum);
        reportSender.logToConsole(aBlockedContentSource, aOriginalUri,
                                  aViolatedDirective, aViolatedPolicyIndex,
                                  aSourceFile, aScriptSample,
                                  aLineNum, aObserverSubject);

      }, Ci.nsIThread.DISPATCH_NORMAL);
  },
};




function CSPReportRedirectSink(policy) {
  this._policy = policy;
}

CSPReportRedirectSink.prototype = {
  QueryInterface: function requestor_qi(iid) {
    if (iid.equals(Ci.nsISupports) ||
        iid.equals(Ci.nsIInterfaceRequestor) ||
        iid.equals(Ci.nsIChannelEventSink))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  },

  
  getInterface: function requestor_gi(iid) {
    if (iid.equals(Ci.nsIChannelEventSink))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  
  asyncOnChannelRedirect: function channel_redirect(oldChannel, newChannel,
                                                    flags, callback) {
    this._policy.log(WARN_FLAG, CSPLocalizer.getFormatStr("reportPostRedirect", [oldChannel.URI.asciiSpec]));

    
    oldChannel.cancel(Cr.NS_ERROR_ABORT);

    
    
    Services.tm.mainThread.dispatch(
      function() {
        observerSubject = Cc["@mozilla.org/supports-cstring;1"]
                             .createInstance(Ci.nsISupportsCString);
        observerSubject.data = oldChannel.URI.asciiSpec;

        Services.obs.notifyObservers(observerSubject,
                                     CSP_VIOLATION_TOPIC,
                                     "denied redirect while sending violation report");
      }, Ci.nsIThread.DISPATCH_NORMAL);

    
    throw Cr.NS_BINDING_REDIRECTED;
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ContentSecurityPolicy]);
