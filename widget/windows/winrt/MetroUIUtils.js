



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function MetroUIUtils() {
}

const URLElements = {
  "a": "href",
  "applet": ["archive", "code", "codebase"],
  "area": "href",
  "audio": "src",
  "base": "href",
  "blockquote": ["cite"],
  "body": "background",
  "button": "formaction",
  "command": "icon",
  "del": ["cite"],
  "embed": "src",
  "form": "action",
  "frame": ["longdesc", "src"],
  "iframe": ["longdesc", "src"],
  "img": ["longdesc", "src"],
  "input": ["formaction", "src"],
  "ins": ["cite"],
  "link": "href",
  "object": ["archive", "codebase", "data"],
  "q": ["cite"],
  "script": "src",
  "source": "src",
};

MetroUIUtils.prototype = {
  classID : Components.ID("e4626085-17f7-4068-a225-66c1acc0485c"),
  QueryInterface : XPCOMUtils.generateQI([Ci.nsIMetroUIUtils]),
  



  showPanel: function(aPanelId) {
    let browserWin = Services.wm.getMostRecentWindow("navigator:browser");
    browserWin.PanelUI.show(aPanelId);
  },

  


  get hasSelectedContent() {
    try {
      let browserWin = Services.wm.getMostRecentWindow("navigator:browser");
      let tabBrowser = browserWin.getBrowser();
      if (!browserWin || !tabBrowser || !tabBrowser.contentWindow) {
        return false;
      }

      let sel = tabBrowser.contentWindow.getSelection();
      return sel && sel.toString();
    } catch(e) {
      return false;
    }
  },

  


  get currentPageTitle() {
    let browserWin = Services.wm.getMostRecentWindow("navigator:browser");
    if (!browserWin || !browserWin.content || !browserWin.content.document) {
      throw Cr.NS_ERROR_FAILURE;
    }
    return browserWin.content.document.title || "";
  },

  


  get currentPageURI() {
    let browserWin = Services.wm.getMostRecentWindow("navigator:browser");
    if (!browserWin || !browserWin.content || !browserWin.content.document) {
      throw Cr.NS_ERROR_FAILURE;
    }
    return browserWin.content.document.URL || "";
  },

  


  get shareText() {
    let browserWin = Services.wm.getMostRecentWindow("navigator:browser");
    let tabBrowser = browserWin.getBrowser();
    if (browserWin && tabBrowser && tabBrowser.contentWindow) {
      let sel = tabBrowser.contentWindow.getSelection();
      if (sel && sel.rangeCount)
        return sel;
    }

    throw Cr.NS_ERROR_FAILURE;
  },

  


  _expandAttribute : function(ioService, doc, node, attrName) {
    let attrValue = node.getAttribute(attrName);
    if (!attrValue)
      return;

    try {
      let uri = ioService.newURI(attrValue, null, doc.baseURIObject);
      node.setAttribute(attrName, uri.spec);
    } catch (e) {
    }
  },

  



  _expandURLs: function(doc, n) {
    let ioService = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService);
    for (let i = 0; i < n.children.length; i++) {
      let child = n.children[i];
      let childTagName = child.tagName.toLowerCase();
      
      
      
      for (let tagName in URLElements) {
        if (tagName === childTagName) {
          if (URLElements[tagName] instanceof Array) {
            URLElements[tagName].forEach(function(attrName) {
              this._expandAttribute(ioService ,doc, child, attrName);
            }, this);
          } else {
            this._expandAttribute(ioService ,doc, child, URLElements[tagName]);
          }
        }
      }

      this._expandURLs(doc, child);
    }
  },

  


  get shareHTML() {
    let browserWin = Services.wm.getMostRecentWindow("navigator:browser");
    let tabBrowser = browserWin.getBrowser();
    let sel;
    if (browserWin && tabBrowser && tabBrowser.contentWindow &&
        (sel = tabBrowser.contentWindow.getSelection()) && sel.rangeCount) {
      let div = tabBrowser.contentWindow.document.createElement("DIV");
      for (let i = 0; i < sel.rangeCount; i++) {
        let contents = sel.getRangeAt(i).cloneContents(true);
        div.appendChild(contents);
      }
      this._expandURLs(tabBrowser.contentWindow.document, div);
      return div.outerHTML;
    }

    throw Cr.NS_ERROR_FAILURE;
  }
};

var component = [MetroUIUtils];
this.NSGetFactory = XPCOMUtils.generateNSGetFactory(component);
