





const { utils: Cu, interfaces: Ci, classes: Cc } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Deprecated",
  "resource://gre/modules/Deprecated.jsm");

const NS_XHTML = "http://www.w3.org/1999/xhtml";
const VIEW_SOURCE_CSS = "resource://gre-resources/viewsource.css";
const BUNDLE_URL = "chrome://global/locale/viewSource.properties";






const MARK_SELECTION_START = "\uFDD0";
const MARK_SELECTION_END = "\uFDEF";

const FRAME_SCRIPT = "chrome://global/content/viewSource-content.js";

this.EXPORTED_SYMBOLS = ["ViewSourceBrowser"];



let gKnownBrowsers = new WeakSet();














this.ViewSourceBrowser = function ViewSourceBrowser(aBrowser) {
  this._browser = aBrowser;
  this.init();
}

ViewSourceBrowser.prototype = {
  


  get browser() {
    return this._browser;
  },

  




  lastLineFound: null,

  





  messages: [
    "ViewSource:PromptAndGoToLine",
    "ViewSource:GoToLine:Success",
    "ViewSource:GoToLine:Failed",
    "ViewSource:StoreWrapping",
    "ViewSource:StoreSyntaxHighlighting",
  ],

  



  init() {
    this.messages.forEach((msgName) => {
      this.mm.addMessageListener(msgName, this);
    });

    
    
    
    
    if (this._browser) {
      this.loadFrameScript();
    }
  },

  



  uninit() {
    this.messages.forEach((msgName) => {
      this.mm.removeMessageListener(msgName, this);
    });
  },

  


  loadFrameScript() {
    if (!gKnownBrowsers.has(this.browser)) {
      gKnownBrowsers.add(this.browser);
      this.mm.loadFrameScript(FRAME_SCRIPT, false);
    }
  },

  



  receiveMessage(message) {
    let data = message.data;

    switch(message.name) {
      case "ViewSource:PromptAndGoToLine":
        this.promptAndGoToLine();
        break;
      case "ViewSource:GoToLine:Success":
        this.onGoToLineSuccess(data.lineNumber);
        break;
      case "ViewSource:GoToLine:Failed":
        this.onGoToLineFailed();
        break;
      case "ViewSource:StoreWrapping":
        this.storeWrapping(data.state);
        break;
      case "ViewSource:StoreSyntaxHighlighting":
        this.storeSyntaxHighlighting(data.state);
        break;
    }
  },

  


  get mm() {
    return this.browser.messageManager;
  },

  


  sendAsyncMessage(...args) {
    this.browser.messageManager.sendAsyncMessage(...args);
  },

  


  get webNav() {
    return this.browser.webNavigation;
  },

  


  get wrapLongLines() {
    return Services.prefs.getBoolPref("view_source.wrap_long_lines");
  },

  


  get bundle() {
    if (this._bundle) {
      return this._bundle;
    }
    return this._bundle = Services.strings.createBundle(BUNDLE_URL);
  },

  





















  loadViewSource({ URL, browser, outerWindowID, lineNumber }) {
    if (!URL) {
      throw new Error("Must supply a URL when opening view source.");
    }

    if (browser) {
      
      
      this.updateBrowserRemoteness(browser.isRemoteBrowser);
    } else {
      if (outerWindowID) {
        throw new Error("Must supply the browser if passing the outerWindowID");
      }
    }

    this.sendAsyncMessage("ViewSource:LoadSource",
                          { URL, outerWindowID, lineNumber });
  },

  









  updateBrowserRemoteness(shouldBeRemote) {
    if (this.browser.isRemoteBrowser != shouldBeRemote) {
      
      
      
      
      throw new Error("View source browser's remoteness mismatch");
    }
  },

  





  loadViewSourceFromSelection(selection) {
    var range = selection.getRangeAt(0);
    var ancestorContainer = range.commonAncestorContainer;
    var doc = ancestorContainer.ownerDocument;

    var startContainer = range.startContainer;
    var endContainer = range.endContainer;
    var startOffset = range.startOffset;
    var endOffset = range.endOffset;

    
    var Node = doc.defaultView.Node;
    if (ancestorContainer.nodeType == Node.TEXT_NODE ||
        ancestorContainer.nodeType == Node.CDATA_SECTION_NODE)
      ancestorContainer = ancestorContainer.parentNode;

    
    
    try {
      if (ancestorContainer == doc.body)
        ancestorContainer = doc.documentElement;
    } catch (e) { }

    
    
    var startPath = this._getPath(ancestorContainer, startContainer);
    var endPath = this._getPath(ancestorContainer, endContainer);

    
    
    
    
    
    
    var isHTML = (doc.createElement("div").tagName == "DIV");
    var dataDoc = isHTML ?
      ancestorContainer.ownerDocument.implementation.createHTMLDocument("") :
      ancestorContainer.ownerDocument.implementation.createDocument("", "", null);
    ancestorContainer = dataDoc.importNode(ancestorContainer, true);
    startContainer = ancestorContainer;
    endContainer = ancestorContainer;

    
    
    
    var canDrawSelection = ancestorContainer.hasChildNodes();
    var tmpNode;
    if (canDrawSelection) {
      var i;
      for (i = startPath ? startPath.length-1 : -1; i >= 0; i--) {
        startContainer = startContainer.childNodes.item(startPath[i]);
      }
      for (i = endPath ? endPath.length-1 : -1; i >= 0; i--) {
        endContainer = endContainer.childNodes.item(endPath[i]);
      }

      
      
      
      
      if (endContainer.nodeType == Node.TEXT_NODE ||
          endContainer.nodeType == Node.CDATA_SECTION_NODE) {
        
        
        
        
        
        if ((endOffset > 0 && endOffset < endContainer.data.length) ||
            !endContainer.parentNode || !endContainer.parentNode.parentNode)
          endContainer.insertData(endOffset, MARK_SELECTION_END);
        else {
          tmpNode = dataDoc.createTextNode(MARK_SELECTION_END);
          endContainer = endContainer.parentNode;
          if (endOffset === 0)
            endContainer.parentNode.insertBefore(tmpNode, endContainer);
          else
            endContainer.parentNode.insertBefore(tmpNode, endContainer.nextSibling);
        }
      }
      else {
        tmpNode = dataDoc.createTextNode(MARK_SELECTION_END);
        endContainer.insertBefore(tmpNode, endContainer.childNodes.item(endOffset));
      }

      if (startContainer.nodeType == Node.TEXT_NODE ||
          startContainer.nodeType == Node.CDATA_SECTION_NODE) {
        
        
        
        
        
        if ((startOffset > 0 && startOffset < startContainer.data.length) ||
            !startContainer.parentNode || !startContainer.parentNode.parentNode ||
            startContainer != startContainer.parentNode.lastChild)
          startContainer.insertData(startOffset, MARK_SELECTION_START);
        else {
          tmpNode = dataDoc.createTextNode(MARK_SELECTION_START);
          startContainer = startContainer.parentNode;
          if (startOffset === 0)
            startContainer.parentNode.insertBefore(tmpNode, startContainer);
          else
            startContainer.parentNode.insertBefore(tmpNode, startContainer.nextSibling);
        }
      }
      else {
        tmpNode = dataDoc.createTextNode(MARK_SELECTION_START);
        startContainer.insertBefore(tmpNode, startContainer.childNodes.item(startOffset));
      }
    }

    
    tmpNode = dataDoc.createElementNS(NS_XHTML, "div");
    tmpNode.appendChild(ancestorContainer);

    
    if (canDrawSelection) {
      this.sendAsyncMessage("ViewSource:ScheduleDrawSelection");
    }

    
    var loadFlags = Components.interfaces.nsIWebNavigation.LOAD_FLAGS_NONE;
    var referrerPolicy = Components.interfaces.nsIHttpChannel.REFERRER_POLICY_DEFAULT;
    this.webNav.loadURIWithOptions((isHTML ?
                                    "view-source:data:text/html;charset=utf-8," :
                                    "view-source:data:application/xml;charset=utf-8,")
                                   + encodeURIComponent(tmpNode.innerHTML),
                                   loadFlags,
                                   null, referrerPolicy,  
                                   null, null,  
                                   Services.io.newURI(doc.baseURI, null, null));
  },

  




  _getPath(ancestor, node) {
    var n = node;
    var p = n.parentNode;
    if (n == ancestor || !p)
      return null;
    var path = new Array();
    if (!path)
      return null;
    do {
      for (var i = 0; i < p.childNodes.length; i++) {
        if (p.childNodes.item(i) == n) {
          path.push(i);
          break;
        }
      }
      n = p;
      p = n.parentNode;
    } while (n != ancestor && p);
    return path;
  },

  









  loadViewSourceFromFragment(node, context) {
    var Node = node.ownerDocument.defaultView.Node;
    this._lineCount = 0;
    this._startTargetLine = 0;
    this._endTargetLine = 0;
    this._targetNode = node;
    if (this._targetNode && this._targetNode.nodeType == Node.TEXT_NODE)
      this._targetNode = this._targetNode.parentNode;

    
    var topTag;
    if (context == "mathml")
      topTag = "math";
    else
      throw "not reached";
    var topNode = this._targetNode;
    while (topNode && topNode.localName != topTag)
      topNode = topNode.parentNode;
    if (!topNode)
      return;

    
    var title = this.bundle.GetStringFromName("viewMathMLSourceTitle");
    var wrapClass = this.wrapLongLines ? ' class="wrap"' : '';
    var source =
      '<!DOCTYPE html>'
    + '<html>'
    + '<head><title>' + title + '</title>'
    + '<link rel="stylesheet" type="text/css" href="' + VIEW_SOURCE_CSS + '">'
    + '<style type="text/css">'
    + '#target { border: dashed 1px; background-color: lightyellow; }'
    + '</style>'
    + '</head>'
    + '<body id="viewsource"' + wrapClass
    +        ' onload="document.title=\''+title+'\'; document.getElementById(\'target\').scrollIntoView(true)">'
    + '<pre>'
    + this._getOuterMarkup(topNode, 0)
    + '</pre></body></html>'
    ; 

    
    this.browser.loadURI("data:text/html;charset=utf-8," +
                         encodeURIComponent(source));
  },

  _getInnerMarkup(node, indent) {
    var str = '';
    for (var i = 0; i < node.childNodes.length; i++) {
      str += this._getOuterMarkup(node.childNodes.item(i), indent);
    }
    return str;
  },

  _getOuterMarkup(node, indent) {
    var Node = node.ownerDocument.defaultView.Node;
    var newline = "";
    var padding = "";
    var str = "";
    if (node == this._targetNode) {
      this._startTargetLine = this._lineCount;
      str += '</pre><pre id="target">';
    }

    switch (node.nodeType) {
    case Node.ELEMENT_NODE: 
      
      
      if (this._lineCount > 0 &&
          this._lineCount != this._startTargetLine &&
          this._lineCount != this._endTargetLine) {
        newline = "\n";
      }
      this._lineCount++;
      for (var k = 0; k < indent; k++) {
        padding += " ";
      }
      str += newline + padding
          +  '&lt;<span class="start-tag">' + node.nodeName + '</span>';
      for (var i = 0; i < node.attributes.length; i++) {
        var attr = node.attributes.item(i);
        if (attr.nodeName.match(/^[-_]moz/)) {
          continue;
        }
        str += ' <span class="attribute-name">'
            +  attr.nodeName
            +  '</span>=<span class="attribute-value">"'
            +  this._unicodeToEntity(attr.nodeValue)
            +  '"</span>';
      }
      if (!node.hasChildNodes()) {
        str += "/&gt;";
      }
      else {
        str += "&gt;";
        var oldLine = this._lineCount;
        str += this._getInnerMarkup(node, indent + 2);
        if (oldLine == this._lineCount) {
          newline = "";
          padding = "";
        }
        else {
          newline = (this._lineCount == this._endTargetLine) ? "" : "\n";
          this._lineCount++;
        }
        str += newline + padding
            +  '&lt;/<span class="end-tag">' + node.nodeName + '</span>&gt;';
      }
      break;
    case Node.TEXT_NODE: 
      var tmp = node.nodeValue;
      tmp = tmp.replace(/(\n|\r|\t)+/g, " ");
      tmp = tmp.replace(/^ +/, "");
      tmp = tmp.replace(/ +$/, "");
      if (tmp.length != 0) {
        str += '<span class="text">' + this._unicodeToEntity(tmp) + '</span>';
      }
      break;
    default:
      break;
    }

    if (node == this._targetNode) {
      this._endTargetLine = this._lineCount;
      str += '</pre><pre>';
    }
    return str;
  },

  _unicodeToEntity(text) {
    const charTable = {
      '&': '&amp;<span class="entity">amp;</span>',
      '<': '&amp;<span class="entity">lt;</span>',
      '>': '&amp;<span class="entity">gt;</span>',
      '"': '&amp;<span class="entity">quot;</span>'
    };

    function charTableLookup(letter) {
      return charTable[letter];
    }

    function convertEntity(letter) {
      try {
        var unichar = this._entityConverter
                          .ConvertToEntity(letter, entityVersion);
        var entity = unichar.substring(1); 
        return '&amp;<span class="entity">' + entity + '</span>';
      } catch (ex) {
        return letter;
      }
    }

    if (!this._entityConverter) {
      try {
        this._entityConverter = Cc["@mozilla.org/intl/entityconverter;1"]
                                  .createInstance(Ci.nsIEntityConverter);
      } catch(e) { }
    }

    const entityVersion = Ci.nsIEntityConverter.entityW3C;

    var str = text;

    
    str = str.replace(/[<>&"]/g, charTableLookup);

    
    str = str.replace(/[^\0-\u007f]/g, convertEntity);

    return str;
  },

  




  promptAndGoToLine() {
    let input = { value: this.lastLineFound };
    let window = Services.wm.getMostRecentWindow(null);

    let ok = Services.prompt.prompt(
        window,
        this.bundle.GetStringFromName("goToLineTitle"),
        this.bundle.GetStringFromName("goToLineText"),
        input,
        null,
        {value:0});

    if (!ok)
      return;

    let line = parseInt(input.value, 10);

    if (!(line > 0)) {
      Services.prompt.alert(window,
                            this.bundle.GetStringFromName("invalidInputTitle"),
                            this.bundle.GetStringFromName("invalidInputText"));
      this.promptAndGoToLine();
    } else {
      this.goToLine(line);
    }
  },

  





  goToLine(lineNumber) {
    this.sendAsyncMessage("ViewSource:GoToLine", { lineNumber });
  },

  






  onGoToLineSuccess(lineNumber) {
    
    
    this.lastLineFound = lineNumber;
  },

  




  onGoToLineFailed() {
    let window = Services.wm.getMostRecentWindow(null);
    Services.prompt.alert(window,
                          this.bundle.GetStringFromName("outOfRangeTitle"),
                          this.bundle.GetStringFromName("outOfRangeText"));
    this.promptAndGoToLine();
  },

  




  storeWrapping(state) {
    Services.prefs.setBoolPref("view_source.wrap_long_lines", state);
  },

  




  storeSyntaxHighlighting(state) {
    Services.prefs.setBoolPref("view_source.syntax_highlight", state);
  },

};






ViewSourceBrowser.isViewSource = function(uri) {
  return uri.startsWith("view-source:") ||
         (uri.startsWith("data:") && uri.includes("MathML"));
};
