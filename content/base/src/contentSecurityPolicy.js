














const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const CSP_VIOLATION_TOPIC = "csp-on-violate-policy";



const CSP_TYPE_XMLHTTPREQUEST_SPEC_COMPLIANT = "csp_type_xmlhttprequest_spec_compliant";
const CSP_TYPE_WEBSOCKET_SPEC_COMPLIANT = "csp_type_websocket_spec_compliant";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/CSPUtils.jsm");



function ContentSecurityPolicy() {
  CSPdebug("CSP CREATED");
  this._isInitialized = false;
  this._reportOnlyMode = false;

  this._policy = CSPRep.fromString("default-src *");

  
  this._policy._allowInlineScripts = true;
  this._policy._allowInlineStyles = true;
  this._policy._allowEval = true;

  this._request = "";
  this._requestOrigin = "";
  this._requestPrincipal = "";
  this._referrer = "";
  this._docRequest = null;
  CSPdebug("CSP POLICY INITED TO 'default-src *'");

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

  get policy () {
    return this._policy.toString();
  },

  get allowsInlineStyle() {
    return this._reportOnlyMode || this._policy.allowsInlineStyles;
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
    case Ci.nsIContentSecurityPolicy.VIOLATION_TYPE_INLINE_STYLE:
      if (!this._policy.allowsInlineStyles)
        this._asyncReportViolation('self',null,'inline style base restriction',
                                   'violated base restriction: Inline Stylesheets will not apply',
                                   aSourceFile, aScriptSample, aLineNum);
      break;
    case Ci.nsIContentSecurityPolicy.VIOLATION_TYPE_INLINE_SCRIPT:
      if (!this._policy.allowsInlineScripts)
        this._asyncReportViolation('self',null,'inline script base restriction',
                                   'violated base restriction: Inline Scripts will not execute',
                                   aSourceFile, aScriptSample, aLineNum);
      break;
    case Ci.nsIContentSecurityPolicy.VIOLATION_TYPE_EVAL:
      if (!this._policy.allowsEvalInScripts)
        this._asyncReportViolation('self',null,'eval script base restriction',
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



  





  refinePolicy:
  function csp_refinePolicy(aPolicy, selfURI, aSpecCompliant) {
    CSPdebug("REFINE POLICY: " + aPolicy);
    CSPdebug("         SELF: " + selfURI.asciiSpec);
    CSPdebug("CSP 1.0 COMPLIANT : " + aSpecCompliant);
    
    
    
    if (selfURI instanceof Ci.nsINestedURI) {
      CSPdebug("        INNER: " + selfURI.innermostURI.asciiSpec);
      selfURI = selfURI.innermostURI;
    }

    
    this._isInitialized = false;

    
    
    
    

    
    
    
    var newpolicy;
    if (aSpecCompliant) {
      newpolicy = CSPRep.fromStringSpecCompliant(aPolicy,
                                                 selfURI,
                                                 this._docRequest,
                                                 this);
    } else {
      newpolicy = CSPRep.fromString(aPolicy,
                                    selfURI,
                                    this._docRequest,
                                    this);
    }

    
    var intersect = this._policy.intersectWith(newpolicy);

    
    this._policy = intersect;

    this._policy._specCompliant = !!aSpecCompliant;

    this._isInitialized = true;
    this._cache = {};
  },

  


  sendReports:
  function(blockedUri, originalUri, violatedDirective,
           aSourceFile, aScriptSample, aLineNum) {
    var uriString = this._policy.getReportURIs();
    var uris = uriString.split(/\s+/);
    if (uris.length > 0) {
      
      let blocked = '';
      if (originalUri) {
        
        let clone = blockedUri.clone();
        clone.path = '';
        blocked = clone.asciiSpec;
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

      var violationMessage = null;
      if (blockedUri["asciiSpec"]) {
         violationMessage = CSPLocalizer.getFormatStr("directiveViolatedWithURI", [violatedDirective, blockedUri.asciiSpec]);
      } else {
         violationMessage = CSPLocalizer.getFormatStr("directiveViolated", [violatedDirective]);
      }
      this._policy.warn(violationMessage,
                        (aSourceFile) ? aSourceFile : null,
                        (aScriptSample) ? decodeURIComponent(aScriptSample) : null,
                        (aLineNum) ? aLineNum : null);

      
      
      
      
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

          
          
          chan.notificationCallbacks = new CSPReportRedirectSink(this._policy);
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
          
          
          this._policy.warn(CSPLocalizer.getFormatStr("triedToSendReport", [uris[i]]));
          this._policy.warn(CSPLocalizer.getFormatStr("errorWas", [e.toString()]));
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
        
        let ancestor = it.currentURI.cloneIgnoringRef();
        try { 
          ancestor.userPass = '';
        } catch (ex) {}

        CSPdebug(" found frame ancestor " + ancestor.asciiSpec);
        ancestors.push(ancestor);
      }
    }

    
    
    
    let cspContext = CSPRep.SRC_DIRECTIVES_NEW.FRAME_ANCESTORS;
    for (let i in ancestors) {
      let ancestor = ancestors[i];
      if (!this._policy.permits(ancestor, cspContext)) {
        
        let directive = this._policy._directives[cspContext];
        let violatedPolicy = (directive._isImplicit
                                ? 'default-src' : 'frame-ancestors ')
                                + directive.toString();

        this._asyncReportViolation(ancestors[i], null, violatedPolicy);

        
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
                          aOriginalUri) {
    let key = aContentLocation.spec + "!" + aContentType;
    if (this._cache[key]) {
      return this._cache[key];
    }

#ifndef MOZ_B2G
    
    CSPdebug("shouldLoad location = " + aContentLocation.asciiSpec);
    CSPdebug("shouldLoad content type = " + aContentType);
#endif
    
    var cspContext = ContentSecurityPolicy._MAPPINGS[aContentType];

    
    
    var cspContext;

    let cp = Ci.nsIContentPolicy;

#ifndef MOZ_B2G
    CSPdebug("policy is " + (this._policy._specCompliant ?
                             "1.0 compliant" : "pre-1.0"));
#endif

    if (aContentType == cp.TYPE_XMLHTTPREQUEST && this._policy._specCompliant) {
      cspContext = ContentSecurityPolicy._MAPPINGS[CSP_TYPE_XMLHTTPREQUEST_SPEC_COMPLIANT];
    } else if (aContentType == cp.TYPE_WEBSOCKET && this._policy._specCompliant) {
      cspContext = ContentSecurityPolicy._MAPPINGS[CSP_TYPE_WEBSOCKET_SPEC_COMPLIANT];
    } else {
      cspContext = ContentSecurityPolicy._MAPPINGS[aContentType];
    }

    CSPdebug("shouldLoad cspContext = " + cspContext);

    
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
        this._asyncReportViolation(aContentLocation, aOriginalUri, violatedPolicy);
      } catch(e) {
        CSPdebug('---------------- ERROR: ' + e);
      }
    }

    let ret = this._cache[key] =
      (this._reportOnlyMode ? Ci.nsIContentPolicy.ACCEPT : res);
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
  function(blockedContentSource, originalUri, violatedDirective, observerSubject,
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
        reportSender.sendReports(blockedContentSource, originalUri,
                                 violatedDirective,
                                 aSourceFile, aScriptSample, aLineNum);
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
    this._policy.warn(CSPLocalizer.getFormatStr("reportPostRedirect", [oldChannel.URI.asciiSpec]));

    
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
