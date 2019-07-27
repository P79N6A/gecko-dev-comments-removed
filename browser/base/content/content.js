







let {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/ContentWebRTC.jsm");
Cu.import("resource:///modules/ContentObservers.jsm");
Cu.import("resource://gre/modules/InlineSpellChecker.jsm");
Cu.import("resource://gre/modules/InlineSpellCheckerContent.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "BrowserUtils",
  "resource://gre/modules/BrowserUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ContentLinkHandler",
  "resource:///modules/ContentLinkHandler.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "LoginManagerContent",
  "resource://gre/modules/LoginManagerContent.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "InsecurePasswordUtils",
  "resource://gre/modules/InsecurePasswordUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PluginContent",
  "resource:///modules/PluginContent.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
  "resource://gre/modules/PrivateBrowsingUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FormSubmitObserver",
  "resource:///modules/FormSubmitObserver.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PageMetadata",
  "resource://gre/modules/PageMetadata.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUIUtils",
  "resource:///modules/PlacesUIUtils.jsm");
XPCOMUtils.defineLazyGetter(this, "PageMenuChild", function() {
  let tmp = {};
  Cu.import("resource://gre/modules/PageMenu.jsm", tmp);
  return new tmp.PageMenuChild();
});


var global = this;


var formSubmitObserver = new FormSubmitObserver(content, this);

addMessageListener("ContextMenu:DoCustomCommand", function(message) {
  PageMenuChild.executeMenu(message.data);
});

addMessageListener("RemoteLogins:fillForm", function(message) {
  LoginManagerContent.receiveMessage(message, content);
});
addEventListener("DOMFormHasPassword", function(event) {
  LoginManagerContent.onDOMFormHasPassword(event, content);
  InsecurePasswordUtils.checkForInsecurePasswords(event.target);
});
addEventListener("DOMInputPasswordAdded", function(event) {
  LoginManagerContent.onDOMInputPasswordAdded(event, content);
});
addEventListener("pageshow", function(event) {
  LoginManagerContent.onPageShow(event, content);
});
addEventListener("DOMAutoComplete", function(event) {
  LoginManagerContent.onUsernameInput(event);
});
addEventListener("blur", function(event) {
  LoginManagerContent.onUsernameInput(event);
});

let handleContentContextMenu = function (event) {
  let defaultPrevented = event.defaultPrevented;
  if (!Services.prefs.getBoolPref("dom.event.contextmenu.enabled")) {
    let plugin = null;
    try {
      plugin = event.target.QueryInterface(Ci.nsIObjectLoadingContent);
    } catch (e) {}
    if (plugin && plugin.displayedType == Ci.nsIObjectLoadingContent.TYPE_PLUGIN) {
      
      return;
    }

    defaultPrevented = false;
  }

  if (defaultPrevented)
    return;

  let addonInfo = {};
  let subject = {
    event: event,
    addonInfo: addonInfo,
  };
  subject.wrappedJSObject = subject;
  Services.obs.notifyObservers(subject, "content-contextmenu", null);

  let doc = event.target.ownerDocument;
  let docLocation = doc.location.href;
  let charSet = doc.characterSet;
  let baseURI = doc.baseURI;
  let referrer = doc.referrer;
  let referrerPolicy = doc.referrerPolicy;
  let frameOuterWindowID = doc.defaultView.QueryInterface(Ci.nsIInterfaceRequestor)
                                          .getInterface(Ci.nsIDOMWindowUtils)
                                          .outerWindowID;

  
  
  
  if (Services.prefs.getBoolPref("network.http.enablePerElementReferrer")) {
    let referrerAttrValue = Services.netUtils.parseAttributePolicyString(event.target.
                            getAttribute("referrer"));
    if (referrerAttrValue !== Ci.nsIHttpChannel.REFERRER_POLICY_DEFAULT) {
      referrerPolicy = referrerAttrValue;
    }
  }

  let disableSetDesktopBg = null;
  
  let contentType = null;
  let contentDisposition = null;
  if (event.target.nodeType == Ci.nsIDOMNode.ELEMENT_NODE &&
      event.target instanceof Ci.nsIImageLoadingContent &&
      event.target.currentURI) {
    disableSetDesktopBg = disableSetDesktopBackground(event.target);

    try {
      let imageCache = 
        Cc["@mozilla.org/image/tools;1"].getService(Ci.imgITools)
                                        .getImgCacheForDocument(doc);
      let props =
        imageCache.findEntryProperties(event.target.currentURI);
      try {
        contentType = props.get("type", Ci.nsISupportsCString).data;
      } catch(e) {}
      try {
        contentDisposition =
          props.get("content-disposition", Ci.nsISupportsCString).data;
      } catch(e) {}
    } catch(e) {}
  }

  let selectionInfo = BrowserUtils.getSelectionDetails(content);

  if (Services.appinfo.processType == Services.appinfo.PROCESS_TYPE_CONTENT) {
    let editFlags = SpellCheckHelper.isEditable(event.target, content);
    let spellInfo;
    if (editFlags &
        (SpellCheckHelper.EDITABLE | SpellCheckHelper.CONTENTEDITABLE)) {
      spellInfo =
        InlineSpellCheckerContent.initContextMenu(event, editFlags, this);
    }

    
    
    
    docShell.contentViewer.QueryInterface(Ci.nsIContentViewerEdit)
            .setCommandNode(event.target);
    event.target.ownerDocument.defaultView.updateCommands("contentcontextmenu");

    let customMenuItems = PageMenuChild.build(event.target);
    let principal = doc.nodePrincipal;
    sendRpcMessage("contextmenu",
                   { editFlags, spellInfo, customMenuItems, addonInfo,
                     principal, docLocation, charSet, baseURI, referrer,
                     referrerPolicy, contentType, contentDisposition,
                     frameOuterWindowID, selectionInfo, disableSetDesktopBg },
                   { event, popupNode: event.target });
  }
  else {
    
    let browser = docShell.chromeEventHandler;
    let mainWin = browser.ownerDocument.defaultView;
    mainWin.gContextMenuContentData = {
      isRemote: false,
      event: event,
      popupNode: event.target,
      browser: browser,
      addonInfo: addonInfo,
      documentURIObject: doc.documentURIObject,
      docLocation: docLocation,
      charSet: charSet,
      referrer: referrer,
      referrerPolicy: referrerPolicy,
      contentType: contentType,
      contentDisposition: contentDisposition,
      selectionInfo: selectionInfo,
      disableSetDesktopBackground: disableSetDesktopBg,
    };
  }
}

Cc["@mozilla.org/eventlistenerservice;1"]
  .getService(Ci.nsIEventListenerService)
  .addSystemEventListener(global, "contextmenu", handleContentContextMenu, false);


const TLS_ERROR_REPORT_TELEMETRY_UI_SHOWN = 0;
const TLS_ERROR_REPORT_TELEMETRY_EXPANDED = 1;
const TLS_ERROR_REPORT_TELEMETRY_SUCCESS  = 6;
const TLS_ERROR_REPORT_TELEMETRY_FAILURE  = 7;

let AboutNetErrorListener = {
  init: function(chromeGlobal) {
    chromeGlobal.addEventListener('AboutNetErrorLoad', this, false, true);
    chromeGlobal.addEventListener('AboutNetErrorSetAutomatic', this, false, true);
    chromeGlobal.addEventListener('AboutNetErrorSendReport', this, false, true);
    chromeGlobal.addEventListener('AboutNetErrorUIExpanded', this, false, true);
  },

  get isAboutNetError() {
    return content.document.documentURI.startsWith("about:neterror");
  },

  handleEvent: function(aEvent) {
    if (!this.isAboutNetError) {
      return;
    }

    switch (aEvent.type) {
    case "AboutNetErrorLoad":
      this.onPageLoad(aEvent);
      break;
    case "AboutNetErrorSetAutomatic":
      this.onSetAutomatic(aEvent);
      break;
    case "AboutNetErrorSendReport":
      this.onSendReport(aEvent);
      break;
    case "AboutNetErrorUIExpanded":
      sendAsyncMessage("Browser:SSLErrorReportTelemetry",
                       {reportStatus: TLS_ERROR_REPORT_TELEMETRY_EXPANDED});
      break;
    }
  },

  onPageLoad: function(evt) {
    let automatic = Services.prefs.getBoolPref("security.ssl.errorReporting.automatic");
    content.dispatchEvent(new content.CustomEvent("AboutNetErrorOptions", {
            detail: JSON.stringify({
              enabled: Services.prefs.getBoolPref("security.ssl.errorReporting.enabled"),
            automatic: automatic
            })
          }
    ));

    sendAsyncMessage("Browser:SSLErrorReportTelemetry",
                     {reportStatus: TLS_ERROR_REPORT_TELEMETRY_UI_SHOWN});

    if (automatic) {
      this.onSendReport(evt);
    }
    
    let contentDoc = content.document;

    let reportSendingMsg = contentDoc.getElementById("reportSendingMessage");
    let reportSentMsg = contentDoc.getElementById("reportSentMessage");
    let retryBtn = contentDoc.getElementById("reportCertificateErrorRetry");
    reportSendingMsg.style.display = "none";
    reportSentMsg.style.display = "none";
    retryBtn.style.display = "none";
  },

  onSetAutomatic: function(evt) {
    sendAsyncMessage("Browser:SetSSLErrorReportAuto", {
        automatic: evt.detail
      });
  },

  onSendReport: function(evt) {
    let contentDoc = content.document;

    let reportSendingMsg = contentDoc.getElementById("reportSendingMessage");
    let reportSentMsg = contentDoc.getElementById("reportSentMessage");
    let reportBtn = contentDoc.getElementById("reportCertificateError");
    let retryBtn = contentDoc.getElementById("reportCertificateErrorRetry");

    addMessageListener("Browser:SSLErrorReportStatus", function(message) {
      
      
      if (contentDoc.documentURI === message.data.documentURI) {
        switch(message.data.reportStatus) {
        case "activity":
          
          reportBtn.style.display = "none";
          retryBtn.style.display = "none";
          reportSentMsg.style.display = "none";
          reportSendingMsg.style.removeProperty("display");
          break;
        case "error":
          
          retryBtn.style.removeProperty("display");
          reportSendingMsg.style.display = "none";
          sendAsyncMessage("Browser:SSLErrorReportTelemetry",
                           {reportStatus: TLS_ERROR_REPORT_TELEMETRY_FAILURE});
          break;
        case "complete":
          
          reportSentMsg.style.removeProperty("display");
          reportSendingMsg.style.display = "none";
          sendAsyncMessage("Browser:SSLErrorReportTelemetry",
                           {reportStatus: TLS_ERROR_REPORT_TELEMETRY_SUCCESS});
          break;
        }
      }
    });

    let failedChannel = docShell.failedChannel;
    let location = contentDoc.location.href;

    let serhelper = Cc["@mozilla.org/network/serialization-helper;1"]
                     .getService(Ci.nsISerializationHelper);

    let serializable =  docShell.failedChannel.securityInfo
                                .QueryInterface(Ci.nsITransportSecurityInfo)
                                .QueryInterface(Ci.nsISerializable);

    let serializedSecurityInfo = serhelper.serializeToString(serializable);

    sendAsyncMessage("Browser:SendSSLErrorReport", {
        elementId: evt.target.id,
        documentURI: contentDoc.documentURI,
        location: {hostname: contentDoc.location.hostname, port: contentDoc.location.port},
        securityInfo: serializedSecurityInfo
      });
  }
}

AboutNetErrorListener.init(this);


let ClickEventHandler = {
  init: function init() {
    Cc["@mozilla.org/eventlistenerservice;1"]
      .getService(Ci.nsIEventListenerService)
      .addSystemEventListener(global, "click", this, true);
  },

  handleEvent: function(event) {
    if (!event.isTrusted || event.defaultPrevented || event.button == 2) {
      return;
    }

    let originalTarget = event.originalTarget;
    let ownerDoc = originalTarget.ownerDocument;
    if (!ownerDoc) {
      return;
    }

    
    if (ownerDoc.documentURI.startsWith("about:certerror")) {
      this.onAboutCertError(originalTarget, ownerDoc);
      return;
    } else if (ownerDoc.documentURI.startsWith("about:blocked")) {
      this.onAboutBlocked(originalTarget, ownerDoc);
      return;
    } else if (ownerDoc.documentURI.startsWith("about:neterror")) {
      this.onAboutNetError(event, ownerDoc.documentURI);
      return;
    }

    let [href, node] = this._hrefAndLinkNodeForClickEvent(event);

    
    
    
    let referrerPolicy = ownerDoc.referrerPolicy;
    if (Services.prefs.getBoolPref("network.http.enablePerElementReferrer") &&
        node) {
      let referrerAttrValue = Services.netUtils.parseAttributePolicyString(node.
                              getAttribute("referrer"));
      if (referrerAttrValue !== Ci.nsIHttpChannel.REFERRER_POLICY_DEFAULT) {
        referrerPolicy = referrerAttrValue;
      }
    }

    let json = { button: event.button, shiftKey: event.shiftKey,
                 ctrlKey: event.ctrlKey, metaKey: event.metaKey,
                 altKey: event.altKey, href: null, title: null,
                 bookmark: false, referrerPolicy: referrerPolicy };

    if (href) {
      try {
        BrowserUtils.urlSecurityCheck(href, node.ownerDocument.nodePrincipal);
      } catch (e) {
        return;
      }

      json.href = href;
      if (node) {
        json.title = node.getAttribute("title");
        if (event.button == 0 && !event.ctrlKey && !event.shiftKey &&
            !event.altKey && !event.metaKey) {
          json.bookmark = node.getAttribute("rel") == "sidebar";
          if (json.bookmark) {
            event.preventDefault(); 
          }
        }
      }
      json.noReferrer = BrowserUtils.linkHasNoReferrer(node)

      sendAsyncMessage("Content:Click", json);
      return;
    }

    
    if (event.button == 1) {
      sendAsyncMessage("Content:Click", json);
    }
  },

  onAboutCertError: function (targetElement, ownerDoc) {
    let docshell = ownerDoc.defaultView.QueryInterface(Ci.nsIInterfaceRequestor)
                                       .getInterface(Ci.nsIWebNavigation)
                                       .QueryInterface(Ci.nsIDocShell);
    let serhelper = Cc["@mozilla.org/network/serialization-helper;1"]
                     .getService(Ci.nsISerializationHelper);
    let serializedSSLStatus = "";

    try {
      let serializable =  docShell.failedChannel.securityInfo
                                  .QueryInterface(Ci.nsISSLStatusProvider)
                                  .SSLStatus
                                  .QueryInterface(Ci.nsISerializable);
      serializedSSLStatus = serhelper.serializeToString(serializable);
    } catch (e) { }

    sendAsyncMessage("Browser:CertExceptionError", {
      location: ownerDoc.location.href,
      elementId: targetElement.getAttribute("id"),
      isTopFrame: (ownerDoc.defaultView.parent === ownerDoc.defaultView),
      sslStatusAsString: serializedSSLStatus
    });
  },

  onAboutBlocked: function (targetElement, ownerDoc) {
    var reason = 'phishing';
    if (/e=malwareBlocked/.test(ownerDoc.documentURI)) {
      reason = 'malware';
    } else if (/e=unwantedBlocked/.test(ownerDoc.documentURI)) {
      reason = 'unwanted';
    }
    sendAsyncMessage("Browser:SiteBlockedError", {
      location: ownerDoc.location.href,
      reason: reason,
      elementId: targetElement.getAttribute("id"),
      isTopFrame: (ownerDoc.defaultView.parent === ownerDoc.defaultView)
    });
  },

  onAboutNetError: function (event, documentURI) {
    let elmId = event.originalTarget.getAttribute("id");
    if (elmId != "errorTryAgain" || !/e=netOffline/.test(documentURI)) {
      return;
    }
    
    
    
    if (Services.io.offline) {
      event.preventDefault();
      sendAsyncMessage("Browser:EnableOnlineMode", {});
    }
  },

  









  _hrefAndLinkNodeForClickEvent: function(event) {
    function isHTMLLink(aNode) {
      
      return ((aNode instanceof content.HTMLAnchorElement && aNode.href) ||
              (aNode instanceof content.HTMLAreaElement && aNode.href) ||
              aNode instanceof content.HTMLLinkElement);
    }

    let node = event.target;
    while (node && !isHTMLLink(node)) {
      node = node.parentNode;
    }

    if (node)
      return [node.href, node];

    
    let href, baseURI;
    node = event.target;
    while (node && !href) {
      if (node.nodeType == content.Node.ELEMENT_NODE) {
        href = node.getAttributeNS("http://www.w3.org/1999/xlink", "href");
        if (href)
          baseURI = node.ownerDocument.baseURIObject;
      }
      node = node.parentNode;
    }

    
    
    
    return [href ? BrowserUtils.makeURI(href, null, baseURI).spec : null, null];
  }
};
ClickEventHandler.init();

ContentLinkHandler.init(this);


let pluginContent = new PluginContent(global);

addEventListener("DOMWebNotificationClicked", function(event) {
  sendAsyncMessage("DOMWebNotificationClicked", {});
}, false);

ContentWebRTC.init();
addMessageListener("webrtc:Allow", ContentWebRTC);
addMessageListener("webrtc:Deny", ContentWebRTC);
addMessageListener("webrtc:StopSharing", ContentWebRTC);
addMessageListener("webrtc:StartBrowserSharing", () => {
  let windowID = content.QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
  sendAsyncMessage("webrtc:response:StartBrowserSharing", {
    windowID: windowID
  });
});

addEventListener("pageshow", function(event) {
  if (event.target == content.document) {
    sendAsyncMessage("PageVisibility:Show", {
      persisted: event.persisted,
    });
  }
});

let PageMetadataMessenger = {
  init() {
    addMessageListener("PageMetadata:GetPageData", this);
    addMessageListener("PageMetadata:GetMicrodata", this);
  },
  receiveMessage(message) {
    switch(message.name) {
      case "PageMetadata:GetPageData": {
        let result = PageMetadata.getData(content.document);
        sendAsyncMessage("PageMetadata:PageDataResult", result);
        break;
      }

      case "PageMetadata:GetMicrodata": {
        let target = message.objects.target;
        let result = PageMetadata.getMicrodata(content.document, target);
        sendAsyncMessage("PageMetadata:MicrodataResult", result);
        break;
      }
    }
  }
}
PageMetadataMessenger.init();

addEventListener("ActivateSocialFeature", function (aEvent) {
  let document = content.document;
  if (PrivateBrowsingUtils.isContentWindowPrivate(content)) {
    Cu.reportError("cannot use social providers in private windows");
    return;
  }
  let dwu = content.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIDOMWindowUtils);
  if (!dwu.isHandlingUserInput) {
    Cu.reportError("attempt to activate provider without user input from " + document.nodePrincipal.origin);
    return;
  }

  let node = aEvent.target;
  let ownerDocument = node.ownerDocument;
  let data = node.getAttribute("data-service");
  if (data) {
    try {
      data = JSON.parse(data);
    } catch(e) {
      Cu.reportError("Social Service manifest parse error: " + e);
      return;
    }
  } else {
    Cu.reportError("Social Service manifest not available");
    return;
  }

  sendAsyncMessage("Social:Activation", {
    url: ownerDocument.location.href,
    origin: ownerDocument.nodePrincipal.origin,
    manifest: data
  });
}, true, true);

addMessageListener("ContextMenu:SaveVideoFrameAsImage", (message) => {
  let video = message.objects.target;
  let canvas = content.document.createElementNS("http://www.w3.org/1999/xhtml", "canvas");
  canvas.width = video.videoWidth;
  canvas.height = video.videoHeight;

  let ctxDraw = canvas.getContext("2d");
  ctxDraw.drawImage(video, 0, 0);
  sendAsyncMessage("ContextMenu:SaveVideoFrameAsImage:Result", {
    dataURL: canvas.toDataURL("image/jpeg", ""),
  });
});

addMessageListener("ContextMenu:MediaCommand", (message) => {
  let media = message.objects.element;

  switch (message.data.command) {
    case "play":
      media.play();
      break;
    case "pause":
      media.pause();
      break;
    case "mute":
      media.muted = true;
      break;
    case "unmute":
      media.muted = false;
      break;
    case "playbackRate":
      media.playbackRate = message.data.data;
      break;
    case "hidecontrols":
      media.removeAttribute("controls");
      break;
    case "showcontrols":
      media.setAttribute("controls", "true");
      break;
    case "hidestats":
    case "showstats":
      let event = media.ownerDocument.createEvent("CustomEvent");
      event.initCustomEvent("media-showStatistics", false, true,
                            message.data.command == "showstats");
      media.dispatchEvent(event);
      break;
    case "fullscreen":
      if (content.document.mozFullScreenEnabled)
        media.mozRequestFullScreen();
      break;
  }
});

addMessageListener("ContextMenu:Canvas:ToDataURL", (message) => {
  let dataURL = message.objects.target.toDataURL();
  sendAsyncMessage("ContextMenu:Canvas:ToDataURL:Result", { dataURL });
});

addMessageListener("ContextMenu:ReloadFrame", (message) => {
  message.objects.target.ownerDocument.location.reload();
});

addMessageListener("ContextMenu:ReloadImage", (message) => {
  let image = message.objects.target;
  if (image instanceof Ci.nsIImageLoadingContent)
    image.forceReload();
});

addMessageListener("ContextMenu:BookmarkFrame", (message) => {
  let frame = message.objects.target.ownerDocument;
  sendAsyncMessage("ContextMenu:BookmarkFrame:Result",
                   { title: frame.title,
                     description: PlacesUIUtils.getDescriptionFromDocument(frame) });
});

addMessageListener("ContextMenu:SearchFieldBookmarkData", (message) => {
  let node = message.objects.target;

  let charset = node.ownerDocument.characterSet;

  let formBaseURI = BrowserUtils.makeURI(node.form.baseURI,
                                         charset);

  let formURI = BrowserUtils.makeURI(node.form.getAttribute("action"),
                                     charset,
                                     formBaseURI);

  let spec = formURI.spec;

  let isURLEncoded =
               (node.form.method.toUpperCase() == "POST"
                && (node.form.enctype == "application/x-www-form-urlencoded" ||
                    node.form.enctype == ""));

  let title = node.ownerDocument.title;
  let description = PlacesUIUtils.getDescriptionFromDocument(node.ownerDocument);

  let formData = [];

  function escapeNameValuePair(aName, aValue, aIsFormUrlEncoded) {
    if (aIsFormUrlEncoded)
      return escape(aName + "=" + aValue);
    else
      return escape(aName) + "=" + escape(aValue);
  }

  for (let el of node.form.elements) {
    if (!el.type) 
      continue;

    if (el == node) {
      formData.push((isURLEncoded) ? escapeNameValuePair(el.name, "%s", true) :
                                     
                                     escapeNameValuePair(el.name, "", false) + "%s");
      continue;
    }

    let type = el.type.toLowerCase();

    if (((el instanceof content.HTMLInputElement && el.mozIsTextField(true)) ||
        type == "hidden" || type == "textarea") ||
        ((type == "checkbox" || type == "radio") && el.checked)) {
      formData.push(escapeNameValuePair(el.name, el.value, isURLEncoded));
    } else if (el instanceof content.HTMLSelectElement && el.selectedIndex >= 0) {
      for (let j=0; j < el.options.length; j++) {
        if (el.options[j].selected)
          formData.push(escapeNameValuePair(el.name, el.options[j].value,
                                            isURLEncoded));
      }
    }
  }

  let postData;

  if (isURLEncoded)
    postData = formData.join("&");
  else {
    let separator = spec.includes("?") ? "&" : "?";
    spec += separator + formData.join("&");
  }

  sendAsyncMessage("ContextMenu:SearchFieldBookmarkData:Result",
                   { spec, title, description, postData, charset });
});

let LightWeightThemeWebInstallListener = {
  _previewWindow: null,

  init: function() {
    addEventListener("InstallBrowserTheme", this, false, true);
    addEventListener("PreviewBrowserTheme", this, false, true);
    addEventListener("ResetBrowserThemePreview", this, false, true);
  },

  handleEvent: function (event) {
    switch (event.type) {
      case "InstallBrowserTheme": {
        sendAsyncMessage("LightWeightThemeWebInstaller:Install", {
          baseURI: event.target.baseURI,
          themeData: event.target.getAttribute("data-browsertheme"),
        });
        break;
      }
      case "PreviewBrowserTheme": {
        sendAsyncMessage("LightWeightThemeWebInstaller:Preview", {
          baseURI: event.target.baseURI,
          themeData: event.target.getAttribute("data-browsertheme"),
        });
        this._previewWindow = event.target.ownerDocument.defaultView;
        this._previewWindow.addEventListener("pagehide", this, true);
        break;
      }
      case "pagehide": {
        sendAsyncMessage("LightWeightThemeWebInstaller:ResetPreview");
        this._resetPreviewWindow();
        break;
      }
      case "ResetBrowserThemePreview": {
        if (this._previewWindow) {
          sendAsyncMessage("LightWeightThemeWebInstaller:ResetPreview",
                           {baseURI: event.target.baseURI});
          this._resetPreviewWindow();
        }
        break;
      }
    }
  },

  _resetPreviewWindow: function () {
    this._previewWindow.removeEventListener("pagehide", this, true);
    this._previewWindow = null;
  }
};

LightWeightThemeWebInstallListener.init();

function disableSetDesktopBackground(aTarget) {
  
  
  if (!(aTarget instanceof Ci.nsIImageLoadingContent))
    return true;

  if (("complete" in aTarget) && !aTarget.complete)
    return true;

  if (aTarget.currentURI.schemeIs("javascript"))
    return true;

  let request = aTarget.QueryInterface(Ci.nsIImageLoadingContent)
                       .getRequest(Ci.nsIImageLoadingContent.CURRENT_REQUEST);
  if (!request)
    return true;

  return false;
}

addMessageListener("ContextMenu:SetAsDesktopBackground", (message) => {
  let target = message.objects.target;

  
  
  let disable = disableSetDesktopBackground(target);

  if (!disable) {
    try {
      BrowserUtils.urlSecurityCheck(target.currentURI.spec, target.ownerDocument.nodePrincipal);
      let canvas = content.document.createElement("canvas");
      canvas.width = target.naturalWidth;
      canvas.height = target.naturalHeight;
      let ctx = canvas.getContext("2d");
      ctx.drawImage(target, 0, 0);
      let dataUrl = canvas.toDataURL();
      sendAsyncMessage("ContextMenu:SetAsDesktopBackground:Result",
                       { dataUrl });
    }
    catch (e) {
      Cu.reportError(e);
      disable = true;
    }
  }

  if (disable)
    sendAsyncMessage("ContextMenu:SetAsDesktopBackground:Result", { disable });
});
