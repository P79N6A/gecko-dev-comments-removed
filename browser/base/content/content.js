




let {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource:///modules/ContentWebRTC.jsm");
Cu.import("resource:///modules/ContentObservers.jsm");
Cu.import("resource://gre/modules/InlineSpellChecker.jsm");
Cu.import("resource://gre/modules/InlineSpellCheckerContent.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "E10SUtils",
  "resource:///modules/E10SUtils.jsm");
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
XPCOMUtils.defineLazyModuleGetter(this, "AboutReader",
  "resource://gre/modules/AboutReader.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ReaderMode",
  "resource://gre/modules/ReaderMode.jsm");
XPCOMUtils.defineLazyGetter(this, "SimpleServiceDiscovery", function() {
  let ssdp = Cu.import("resource://gre/modules/SimpleServiceDiscovery.jsm", {}).SimpleServiceDiscovery;
  
  ssdp.registerDevice({
    id: "roku:ecp",
    target: "roku:ecp",
    factory: function(aService) {
      Cu.import("resource://gre/modules/RokuApp.jsm");
      return new RokuApp(aService);
    },
    mirror: true,
    types: ["video/mp4"],
    extensions: ["mp4"]
  });
  return ssdp;
});
XPCOMUtils.defineLazyGetter(this, "PageMenuChild", function() {
  let tmp = {};
  Cu.import("resource://gre/modules/PageMenu.jsm", tmp);
  return new tmp.PageMenuChild();
});


var global = this;


var formSubmitObserver = new FormSubmitObserver(content, this);

addMessageListener("Browser:HideSessionRestoreButton", function (message) {
  
  let doc = content.document;
  let container;
  if (doc.documentURI.toLowerCase() == "about:home" &&
      (container = doc.getElementById("sessionRestoreContainer"))) {
    container.hidden = true;
  }
});

addMessageListener("Browser:Reload", function(message) {
  





  let webNav = docShell.QueryInterface(Ci.nsIWebNavigation);
  try {
    let sh = webNav.sessionHistory;
    if (sh)
      webNav = sh.QueryInterface(Ci.nsIWebNavigation);
  } catch (e) {
  }

  let reloadFlags = message.data.flags;
  let handlingUserInput;
  try {
    handlingUserInput = content.QueryInterface(Ci.nsIInterfaceRequestor)
                               .getInterface(Ci.nsIDOMWindowUtils)
                               .setHandlingUserInput(message.data.handlingUserInput);
    webNav.reload(reloadFlags);
  } catch (e) {
  } finally {
    handlingUserInput.destruct();
  }
});

addMessageListener("MixedContent:ReenableProtection", function() {
  docShell.mixedContentChannel = null;
});

addMessageListener("SecondScreen:tab-mirror", function(message) {
  let app = SimpleServiceDiscovery.findAppForService(message.data.service);
  if (app) {
    let width = content.innerWidth;
    let height = content.innerHeight;
    let viewport = {cssWidth: width, cssHeight: height, width: width, height: height};
    app.mirror(function() {}, content, viewport, function() {}, content);
  }
});

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

  if (Services.appinfo.processType == Services.appinfo.PROCESS_TYPE_CONTENT) {
    let editFlags = SpellCheckHelper.isEditable(event.target, content);
    let spellInfo;
    if (editFlags &
        (SpellCheckHelper.EDITABLE | SpellCheckHelper.CONTENTEDITABLE)) {
      spellInfo =
        InlineSpellCheckerContent.initContextMenu(event, editFlags, this);
    }

    let customMenuItems = PageMenuChild.build(event.target);
    let principal = event.target.ownerDocument.nodePrincipal;
    sendSyncMessage("contextmenu",
                    { editFlags, spellInfo, customMenuItems, addonInfo, principal },
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

let AboutHomeListener = {
  init: function(chromeGlobal) {
    chromeGlobal.addEventListener('AboutHomeLoad', this, false, true);
  },

  get isAboutHome() {
    return content.document.documentURI.toLowerCase() == "about:home";
  },

  handleEvent: function(aEvent) {
    if (!this.isAboutHome) {
      return;
    }
    switch (aEvent.type) {
      case "AboutHomeLoad":
        this.onPageLoad();
        break;
      case "AboutHomeSearchEvent":
        this.onSearch(aEvent);
        break;
      case "AboutHomeSearchPanel":
        this.onOpenSearchPanel(aEvent);
        break;
      case "click":
        this.onClick(aEvent);
        break;
      case "pagehide":
        this.onPageHide(aEvent);
        break;
    }
  },

  receiveMessage: function(aMessage) {
    if (!this.isAboutHome) {
      return;
    }
    switch (aMessage.name) {
      case "AboutHome:Update":
        this.onUpdate(aMessage.data);
        break;
      case "AboutHome:FocusInput":
        this.onFocusInput();
        break;
    }
  },

  onUpdate: function(aData) {
    let doc = content.document;
    if (aData.showRestoreLastSession && !PrivateBrowsingUtils.isContentWindowPrivate(content))
      doc.getElementById("launcher").setAttribute("session", "true");

    
    let docElt = doc.documentElement;
    
    
    docElt.setAttribute("snippetsURL", aData.snippetsURL);
    if (aData.showKnowYourRights)
      docElt.setAttribute("showKnowYourRights", "true");
    docElt.setAttribute("snippetsVersion", aData.snippetsVersion);
    docElt.setAttribute("searchEngineName", aData.defaultEngineName);
  },

  onPageLoad: function() {
    let doc = content.document;
    if (doc.documentElement.hasAttribute("hasBrowserHandlers")) {
      return;
    }

    doc.documentElement.setAttribute("hasBrowserHandlers", "true");
    addMessageListener("AboutHome:Update", this);
    addMessageListener("AboutHome:FocusInput", this);
    addEventListener("click", this, true);
    addEventListener("pagehide", this, true);

    if (!Services.prefs.getBoolPref("browser.search.showOneOffButtons")) {
      doc.documentElement.setAttribute("searchUIConfiguration", "oldsearchui");
    }

    sendAsyncMessage("AboutHome:RequestUpdate");
    doc.addEventListener("AboutHomeSearchEvent", this, true, true);
    doc.addEventListener("AboutHomeSearchPanel", this, true, true);
  },

  onClick: function(aEvent) {
    if (!aEvent.isTrusted || 
        aEvent.button == 2 || aEvent.target.localName != "button") {
      return;
    }

    let originalTarget = aEvent.originalTarget;
    let ownerDoc = originalTarget.ownerDocument;
    if (ownerDoc.documentURI != "about:home") {
      
      return;
    }

    let elmId = originalTarget.getAttribute("id");

    switch (elmId) {
      case "restorePreviousSession":
        sendAsyncMessage("AboutHome:RestorePreviousSession");
        ownerDoc.getElementById("launcher").removeAttribute("session");
        break;

      case "downloads":
        sendAsyncMessage("AboutHome:Downloads");
        break;

      case "bookmarks":
        sendAsyncMessage("AboutHome:Bookmarks");
        break;

      case "history":
        sendAsyncMessage("AboutHome:History");
        break;

      case "apps":
        sendAsyncMessage("AboutHome:Apps");
        break;

      case "addons":
        sendAsyncMessage("AboutHome:Addons");
        break;

      case "sync":
        sendAsyncMessage("AboutHome:Sync");
        break;

      case "settings":
        sendAsyncMessage("AboutHome:Settings");
        break;

      case "searchIcon":
        sendAsyncMessage("AboutHome:OpenSearchPanel", null, { anchor: originalTarget });
        break;
    }
  },

  onPageHide: function(aEvent) {
    if (aEvent.target.defaultView.frameElement) {
      return;
    }
    removeMessageListener("AboutHome:Update", this);
    removeEventListener("click", this, true);
    removeEventListener("pagehide", this, true);
    if (aEvent.target.documentElement) {
      aEvent.target.documentElement.removeAttribute("hasBrowserHandlers");
    }
  },

  onSearch: function(aEvent) {
    sendAsyncMessage("AboutHome:Search", { searchData: aEvent.detail });
  },

  onOpenSearchPanel: function(aEvent) {
    sendAsyncMessage("AboutHome:OpenSearchPanel");
  },

  onFocusInput: function () {
    let searchInput = content.document.getElementById("searchText");
    if (searchInput) {
      searchInput.focus();
    }
  },
};
AboutHomeListener.init(this);

let AboutReaderListener = {
  _savedArticle: null,

  init: function() {
    addEventListener("AboutReaderContentLoaded", this, false, true);
    addEventListener("pageshow", this, false);
    addEventListener("pagehide", this, false);
    addMessageListener("Reader:SavedArticleGet", this);
  },

  receiveMessage: function(message) {
    switch (message.name) {
      case "Reader:SavedArticleGet":
        sendAsyncMessage("Reader:SavedArticleData", { article: this._savedArticle });
        break;
    }
  },

  get isAboutReader() {
    return content.document.documentURI.startsWith("about:reader");
  },

  handleEvent: function(aEvent) {
    if (aEvent.originalTarget.defaultView != content) {
      return;
    }

    switch (aEvent.type) {
      case "AboutReaderContentLoaded":
        if (!this.isAboutReader) {
          return;
        }

        if (content.document.body) {
          
          sendAsyncMessage("Reader:UpdateReaderButton");
          new AboutReader(global, content);
        }
        break;

      case "pagehide":
        sendAsyncMessage("Reader:UpdateReaderButton", { isArticle: false });
        break;

      case "pageshow":
        if (!ReaderMode.isEnabledForParseOnLoad || this.isAboutReader) {
          return;
        }

        
        this._savedArticle = null;

        ReaderMode.parseDocument(content.document).then(article => {
          
          if (article === null || content === null) {
            return;
          }

          
          
          let currentURL = Services.io.newURI(content.document.documentURI, null, null).specIgnoringRef;
          if (article.url !== currentURL) {
            return;
          }

          this._savedArticle = article;
          sendAsyncMessage("Reader:UpdateReaderButton", { isArticle: true });

        }).catch(e => Cu.reportError("Error parsing document: " + e));
        break;
    }
  }
};
AboutReaderListener.init();


addEventListener("WebChannelMessageToChrome", function (e) {
  
  let principal = e.target.nodePrincipal ? e.target.nodePrincipal : e.target.document.nodePrincipal;

  if (e.detail) {
    sendAsyncMessage("WebChannelMessageToChrome", e.detail, null, principal);
  } else  {
    Cu.reportError("WebChannel message failed. No message detail.");
  }
}, true, true);


addMessageListener("WebChannelMessageToContent", function (e) {
  if (e.data) {
    content.dispatchEvent(new content.CustomEvent("WebChannelMessageToContent", {
      detail: Cu.cloneInto({
        id: e.data.id,
        message: e.data.message,
      }, content),
    }));
  } else {
    Cu.reportError("WebChannel message failed. No message data.");
  }
});


let ContentSearchMediator = {

  whitelist: new Set([
    "about:home",
    "about:newtab",
  ]),

  init: function (chromeGlobal) {
    chromeGlobal.addEventListener("ContentSearchClient", this, true, true);
    addMessageListener("ContentSearch", this);
  },

  handleEvent: function (event) {
    if (this._contentWhitelisted) {
      this._sendMsg(event.detail.type, event.detail.data);
    }
  },

  receiveMessage: function (msg) {
    if (msg.data.type == "AddToWhitelist") {
      for (let uri of msg.data.data) {
        this.whitelist.add(uri);
      }
      this._sendMsg("AddToWhitelistAck");
      return;
    }
    if (this._contentWhitelisted) {
      this._fireEvent(msg.data.type, msg.data.data);
    }
  },

  get _contentWhitelisted() {
    return this.whitelist.has(content.document.documentURI);
  },

  _sendMsg: function (type, data=null) {
    sendAsyncMessage("ContentSearch", {
      type: type,
      data: data,
    });
  },

  _fireEvent: function (type, data=null) {
    let event = Cu.cloneInto({
      detail: {
        type: type,
        data: data,
      },
    }, content);
    content.dispatchEvent(new content.CustomEvent("ContentSearchService",
                                                  event));
  },
};
ContentSearchMediator.init(this);


addMessageListener("Finder:Initialize", function () {
  let {RemoteFinderListener} = Cu.import("resource://gre/modules/RemoteFinder.jsm", {});
  new RemoteFinderListener(global);
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
                 bookmark: false };

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
        json.noReferrer = BrowserUtils.linkHasNoReferrer(node)
      }

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
    sendAsyncMessage("Browser:SiteBlockedError", {
      location: ownerDoc.location.href,
      isMalware: /e=malwareBlocked/.test(ownerDoc.documentURI),
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

let PageStyleHandler = {
  init: function() {
    addMessageListener("PageStyle:Switch", this);
    addMessageListener("PageStyle:Disable", this);

    
    
    sendSyncMessage("PageStyle:SetSyncHandler", {}, {syncHandler: this});
  },

  get markupDocumentViewer() {
    return docShell.contentViewer;
  },

  
  getStyleSheetInfo: function() {
    let styleSheets = this._filterStyleSheets(this.getAllStyleSheets());
    return {
      styleSheets: styleSheets,
      authorStyleDisabled: this.markupDocumentViewer.authorStyleDisabled,
      preferredStyleSheetSet: content.document.preferredStyleSheetSet
    };
  },

  
  getAllStyleSheets: function(frameset = content) {
    let selfSheets = Array.slice(frameset.document.styleSheets);
    let subSheets = Array.map(frameset.frames, frame => this.getAllStyleSheets(frame));
    return selfSheets.concat(...subSheets);
  },

  receiveMessage: function(msg) {
    switch (msg.name) {
      case "PageStyle:Switch":
        this.markupDocumentViewer.authorStyleDisabled = false;
        this._stylesheetSwitchAll(content, msg.data.title);
        break;

      case "PageStyle:Disable":
        this.markupDocumentViewer.authorStyleDisabled = true;
        break;
    }
  },

  _stylesheetSwitchAll: function (frameset, title) {
    if (!title || this._stylesheetInFrame(frameset, title)) {
      this._stylesheetSwitchFrame(frameset, title);
    }

    for (let i = 0; i < frameset.frames.length; i++) {
      
      this._stylesheetSwitchAll(frameset.frames[i], title);
    }
  },

  _stylesheetSwitchFrame: function (frame, title) {
    var docStyleSheets = frame.document.styleSheets;

    for (let i = 0; i < docStyleSheets.length; ++i) {
      let docStyleSheet = docStyleSheets[i];
      if (docStyleSheet.title) {
        docStyleSheet.disabled = (docStyleSheet.title != title);
      } else if (docStyleSheet.disabled) {
        docStyleSheet.disabled = false;
      }
    }
  },

  _stylesheetInFrame: function (frame, title) {
    return Array.some(frame.document.styleSheets, (styleSheet) => styleSheet.title == title);
  },

  _filterStyleSheets: function(styleSheets) {
    let result = [];

    for (let currentStyleSheet of styleSheets) {
      if (!currentStyleSheet.title)
        continue;

      
      if (currentStyleSheet.media.length > 0) {
        let mediaQueryList = currentStyleSheet.media.mediaText;
        if (!content.matchMedia(mediaQueryList).matches) {
          continue;
        }
      }

      result.push({title: currentStyleSheet.title,
                   disabled: currentStyleSheet.disabled});
    }

    return result;
  },
};
PageStyleHandler.init();


let trHandler = null;
if (Services.prefs.getBoolPref("browser.translation.detectLanguage")) {
  Cu.import("resource:///modules/translation/TranslationContentHandler.jsm");
  trHandler = new TranslationContentHandler(global, docShell);
}

let DOMFullscreenHandler = {
  _fullscreenDoc: null,

  init: function() {
    addMessageListener("DOMFullscreen:Approved", this);
    addMessageListener("DOMFullscreen:CleanUp", this);
    addEventListener("MozEnteredDomFullscreen", this);
  },

  receiveMessage: function(aMessage) {
    switch(aMessage.name) {
      case "DOMFullscreen:Approved": {
        if (this._fullscreenDoc) {
          Services.obs.notifyObservers(this._fullscreenDoc,
                                       "fullscreen-approved",
                                       "");
        }
        break;
      }
      case "DOMFullscreen:CleanUp": {
        this._fullscreenDoc = null;
        break;
      }
    }
  },

  handleEvent: function(aEvent) {
    if (aEvent.type == "MozEnteredDomFullscreen") {
      this._fullscreenDoc = aEvent.target;
      sendAsyncMessage("MozEnteredDomFullscreen", {
        origin: this._fullscreenDoc.nodePrincipal.origin,
      });
    }
  }
};
DOMFullscreenHandler.init();

ContentWebRTC.init();
addMessageListener("webrtc:Allow", ContentWebRTC);
addMessageListener("webrtc:Deny", ContentWebRTC);
addMessageListener("webrtc:StopSharing", ContentWebRTC);

function gKeywordURIFixup(fixupInfo) {
  fixupInfo.QueryInterface(Ci.nsIURIFixupInfo);

  
  let parent = fixupInfo.consumer.QueryInterface(Ci.nsIDocShellTreeItem).sameTypeRootTreeItem;
  if (parent != docShell)
    return;

  let data = {};
  for (let f of Object.keys(fixupInfo)) {
    if (f == "consumer" || typeof fixupInfo[f] == "function")
      continue;

    if (fixupInfo[f] && fixupInfo[f] instanceof Ci.nsIURI) {
      data[f] = fixupInfo[f].spec;
    } else {
      data[f] = fixupInfo[f];
    }
  }

  sendAsyncMessage("Browser:URIFixup", data);
}
Services.obs.addObserver(gKeywordURIFixup, "keyword-uri-fixup", false);
addEventListener("unload", () => {
  Services.obs.removeObserver(gKeywordURIFixup, "keyword-uri-fixup");
}, false);

addMessageListener("Browser:AppTab", function(message) {
  docShell.isAppTab = message.data.isAppTab;
});

let WebBrowserChrome = {
  onBeforeLinkTraversal: function(originalTarget, linkURI, linkNode, isAppTab) {
    return BrowserUtils.onBeforeLinkTraversal(originalTarget, linkURI, linkNode, isAppTab);
  },

  
  shouldLoadURI: function(aDocShell, aURI, aReferrer) {
    if (!E10SUtils.shouldLoadURI(aDocShell, aURI, aReferrer)) {
      E10SUtils.redirectLoad(aDocShell, aURI, aReferrer);
      return false;
    }

    return true;
  },
};

if (Services.appinfo.processType == Services.appinfo.PROCESS_TYPE_CONTENT) {
  let tabchild = docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsITabChild);
  tabchild.webBrowserChrome = WebBrowserChrome;
}

addEventListener("pageshow", function(event) {
  if (event.target == content.document) {
    sendAsyncMessage("PageVisibility:Show", {
      persisted: event.persisted,
    });
  }
});

let SocialMessenger = {
  init: function() {
    addMessageListener("Social:GetPageData", this);
    addMessageListener("Social:GetMicrodata", this);

    XPCOMUtils.defineLazyGetter(this, "og", function() {
      let tmp = {};
      Cu.import("resource:///modules/Social.jsm", tmp);
      return tmp.OpenGraphBuilder;
    });
  },
  receiveMessage: function(aMessage) {
    switch(aMessage.name) {
      case "Social:GetPageData":
        sendAsyncMessage("Social:PageDataResult", this.og.getData(content.document));
        break;
      case "Social:GetMicrodata":
        let target = aMessage.objects;
        sendAsyncMessage("Social:PageDataResult", this.og.getMicrodata(content.document, target));
        break;
    }
  }
}
SocialMessenger.init();

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
