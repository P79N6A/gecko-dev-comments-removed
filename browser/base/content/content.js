







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

addEventListener("DOMFormHasPassword", function(event) {
  InsecurePasswordUtils.checkForInsecurePasswords(event.target);
  LoginManagerContent.onFormPassword(event);
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

  
  let contentType = null;
  let contentDisposition = null;
  if (event.target.nodeType == Ci.nsIDOMNode.ELEMENT_NODE &&
      event.target instanceof Ci.nsIImageLoadingContent &&
      event.target.currentURI) {
    try {
      let imageCache = Cc["@mozilla.org/image/tools;1"].getService(Ci.imgITools)
                                                       .getImgCacheForDocument(doc);
      let props =
        imageCache.findEntryProperties(event.target.currentURI);
      if (props) {
        contentType = props.get("type", Ci.nsISupportsCString).data;
        contentDisposition = props.get("content-disposition", Ci.nsISupportsCString).data;
      }
    } catch (e) {
      Cu.reportError(e);
    }
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
    sendSyncMessage("contextmenu",
                    { editFlags, spellInfo, customMenuItems, addonInfo,
                      principal, docLocation, charSet, baseURI, referrer,
                      referrerPolicy, contentType, contentDisposition,
                      frameOuterWindowID, selectionInfo },
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
    };
  }
}

Cc["@mozilla.org/eventlistenerservice;1"]
  .getService(Ci.nsIEventListenerService)
  .addSystemEventListener(global, "contextmenu", handleContentContextMenu, false);

let AboutNetErrorListener = {
  init: function(chromeGlobal) {
    chromeGlobal.addEventListener('AboutNetErrorLoad', this, false, true);
    chromeGlobal.addEventListener('AboutNetErrorSetAutomatic', this, false, true);
    chromeGlobal.addEventListener('AboutNetErrorSendReport', this, false, true);
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
    if (automatic) {
      this.onSendReport(evt);
    }
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
          reportSendingMsg.style.display = "inline";
          break;
        case "error":
          
          retryBtn.style.display = "inline";
          reportSendingMsg.style.display = "none";
          break;
        case "complete":
          
          reportSentMsg.style.display = "inline";
          reportSendingMsg.style.display = "none";
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
        location: contentDoc.location,
        securityInfo: serializedSecurityInfo
      });
  }
}

AboutNetErrorListener.init(this);


addEventListener("WebChannelMessageToChrome", function (e) {
  
  let principal = e.target.nodePrincipal ? e.target.nodePrincipal : e.target.document.nodePrincipal;

  if (e.detail) {
    sendAsyncMessage("WebChannelMessageToChrome", e.detail, { eventTarget: e.target }, principal);
  } else  {
    Cu.reportError("WebChannel message failed. No message detail.");
  }
}, true, true);


addMessageListener("WebChannelMessageToContent", function (e) {
  if (e.data) {
    
    
    
    
    let eventTarget = e.objects.eventTarget || content;

    
    
    let targetPrincipal = eventTarget instanceof Ci.nsIDOMWindow ? eventTarget.document.nodePrincipal : eventTarget.nodePrincipal;

    if (e.principal.subsumes(targetPrincipal)) {
      
      
      let targetWindow = eventTarget instanceof Ci.nsIDOMWindow ? eventTarget : eventTarget.ownerDocument.defaultView;

      eventTarget.dispatchEvent(new targetWindow.CustomEvent("WebChannelMessageToContent", {
        detail: Cu.cloneInto({
          id: e.data.id,
          message: e.data.message,
        }, targetWindow),
      }));
    } else {
      Cu.reportError("WebChannel message failed. Principal mismatch.");
    }
  } else {
    Cu.reportError("WebChannel message failed. No message data.");
  }
});


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

    let json = { button: event.button, shiftKey: event.shiftKey,
                 ctrlKey: event.ctrlKey, metaKey: event.metaKey,
                 altKey: event.altKey, href: null, title: null,
                 bookmark: false, referrerPolicy: ownerDoc.referrerPolicy };

    if (href) {
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
