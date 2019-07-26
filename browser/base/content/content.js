




let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this,
  "LoginManagerContent", "resource://gre/modules/LoginManagerContent.jsm");
XPCOMUtils.defineLazyModuleGetter(this,
  "InsecurePasswordUtils", "resource://gre/modules/InsecurePasswordUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
  "resource://gre/modules/PrivateBrowsingUtils.jsm");



this.__defineGetter__("webNavigation", function () {
  return docShell.QueryInterface(Ci.nsIWebNavigation);
});

addMessageListener("WebNavigation:LoadURI", function (message) {
  let flags = message.json.flags || webNavigation.LOAD_FLAGS_NONE;

  webNavigation.loadURI(message.json.uri, flags, null, null, null);
});

addMessageListener("Browser:HideSessionRestoreButton", function (message) {
  
  let doc = content.document;
  let container;
  if (doc.documentURI.toLowerCase() == "about:home" &&
      (container = doc.getElementById("sessionRestoreContainer"))){
    container.hidden = true;
  }
});

if (Services.prefs.getBoolPref("browser.tabs.remote")) {
  addEventListener("contextmenu", function (event) {
    sendAsyncMessage("contextmenu", {}, { event: event });
  }, false);
} else {
  addEventListener("DOMContentLoaded", function(event) {
    LoginManagerContent.onContentLoaded(event);
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
}

let AboutHomeListener = {
  init: function(chromeGlobal) {
    let self = this;
    chromeGlobal.addEventListener('AboutHomeLoad', function(e) { self.onPageLoad(); }, false, true);
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
    docElt.setAttribute("searchEngineName", Services.search.defaultEngine.name);
  },

  onPageLoad: function() {
    let doc = content.document;
    if (doc.documentURI.toLowerCase() != "about:home" ||
        doc.documentElement.hasAttribute("hasBrowserHandlers")) {
      return;
    }

    doc.documentElement.setAttribute("hasBrowserHandlers", "true");
    let updateListener = this;
    addMessageListener("AboutHome:Update", updateListener);
    addEventListener("click", this.onClick, true);
    addEventListener("pagehide", function onPageHide(event) {
      if (event.target.defaultView.frameElement)
        return;
      removeMessageListener("AboutHome:Update", updateListener);
      removeEventListener("click", this.onClick, true);
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

    function makeURLAbsolute(aBase, aUrl) {
      
      return makeURI(aUrl, null, makeURI(aBase)).spec;
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
          baseURI = node.baseURI;
      }
      node = node.parentNode;
    }

    
    
    return [href ? makeURLAbsolute(baseURI, href) : null, null];
  }
};
ClickEventHandler.init();
