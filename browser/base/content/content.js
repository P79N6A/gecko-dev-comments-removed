




let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ContentLinkHandler",
  "resource:///modules/ContentLinkHandler.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "LanguageDetector",
  "resource:///modules/translation/LanguageDetector.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "LoginManagerContent",
  "resource://gre/modules/LoginManagerContent.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "InsecurePasswordUtils",
  "resource://gre/modules/InsecurePasswordUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
  "resource://gre/modules/PrivateBrowsingUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UITour",
  "resource:///modules/UITour.jsm");


function makeURI(uri, originCharset, baseURI) {
  return Services.io.newURI(uri, originCharset, baseURI);
}

addMessageListener("Browser:HideSessionRestoreButton", function (message) {
  
  let doc = content.document;
  let container;
  if (doc.documentURI.toLowerCase() == "about:home" &&
      (container = doc.getElementById("sessionRestoreContainer"))){
    container.hidden = true;
  }
});

if (Services.appinfo.processType == Services.appinfo.PROCESS_TYPE_CONTENT) {
  addEventListener("contextmenu", function (event) {
    sendAsyncMessage("contextmenu", {}, { event: event });
  }, false);
} else {
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

  addEventListener("mozUITour", function(event) {
    if (!Services.prefs.getBoolPref("browser.uitour.enabled"))
      return;

    let handled = UITour.onPageEvent(event);
    if (handled)
      addEventListener("pagehide", UITour);
  }, false, true);
}

let AboutHomeListener = {
  init: function(chromeGlobal) {
    chromeGlobal.addEventListener('AboutHomeLoad', () => this.onPageLoad(), false, true);
  },

  handleEvent: function(aEvent) {
    switch (aEvent.type) {
      case "AboutHomeLoad":
        this.onPageLoad();
        break;
    }
  },

  receiveMessage: function(aMessage) {
    switch (aMessage.name) {
      case "AboutHome:Update":
        this.onUpdate(aMessage.data);
        break;
    }
  },

  onUpdate: function(aData) {
    let doc = content.document;
    if (doc.documentURI.toLowerCase() != "about:home")
      return;

    if (aData.showRestoreLastSession && !PrivateBrowsingUtils.isWindowPrivate(content))
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
    if (doc.documentURI.toLowerCase() != "about:home" ||
        doc.documentElement.hasAttribute("hasBrowserHandlers")) {
      return;
    }

    doc.documentElement.setAttribute("hasBrowserHandlers", "true");
    let self = this;
    addMessageListener("AboutHome:Update", self);
    addEventListener("click", this.onClick, true);
    addEventListener("pagehide", function onPageHide(event) {
      if (event.target.defaultView.frameElement)
        return;
      removeMessageListener("AboutHome:Update", self);
      removeEventListener("click", self.onClick, true);
      removeEventListener("pagehide", onPageHide, true);
      if (event.target.documentElement)
        event.target.documentElement.removeAttribute("hasBrowserHandlers");
    }, true);

    
    
    if (Services.prefs.getPrefType("browser.aboutHome.apps") == Services.prefs.PREF_BOOL &&
        Services.prefs.getBoolPref("browser.aboutHome.apps"))
      doc.getElementById("apps").removeAttribute("hidden");

    sendAsyncMessage("AboutHome:RequestUpdate");

    doc.addEventListener("AboutHomeSearchEvent", function onSearch(e) {
      sendAsyncMessage("AboutHome:Search", { searchData: e.detail });
    }, true, true);
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
    }
  },
};
AboutHomeListener.init(this);

var global = this;


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
    
    
    if (!event.isTrusted || event.defaultPrevented || event.button == 2)
      return;

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
          if (json.bookmark)
            event.preventDefault(); 
        }
      }

      sendAsyncMessage("Content:Click", json);
      return;
    }

    
    if (event.button == 1)
      sendAsyncMessage("Content:Click", json);
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

    
    
    
    return [href ? makeURI(href, null, baseURI).spec : null, null];
  }
};
ClickEventHandler.init();

ContentLinkHandler.init(this);

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
    return docShell.contentViewer.QueryInterface(Ci.nsIMarkupDocumentViewer);
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

let TranslationHandler = {
  init: function() {
    let webProgress = docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                              .getInterface(Ci.nsIWebProgress);
    webProgress.addProgressListener(this, Ci.nsIWebProgress.NOTIFY_STATE_DOCUMENT);
  },

  
  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus) {
    if (!aWebProgress.isTopLevel ||
        !(aStateFlags & Ci.nsIWebProgressListener.STATE_STOP))
      return;

    let url = aRequest.name;
    if (!url.startsWith("http://") && !url.startsWith("https://"))
      return;

    
    let encoder = Cc["@mozilla.org/layout/documentEncoder;1?type=text/plain"]
                    .createInstance(Ci.nsIDocumentEncoder);
    encoder.init(content.document, "text/plain", encoder.SkipInvisibleContent);
    let string = encoder.encodeToStringWithMaxLength(60 * 1024);

    
    if (string.length < 100)
      return;

    LanguageDetector.detectLanguage(string).then(result => {
      if (result.confident)
        sendAsyncMessage("LanguageDetection:Result", result.language);
    });
  },

  
  onProgressChange: function() {},
  onLocationChange: function() {},
  onStatusChange:   function() {},
  onSecurityChange: function() {},

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference])
};

if (Services.prefs.getBoolPref("browser.translation.detectLanguage"))
  TranslationHandler.init();
