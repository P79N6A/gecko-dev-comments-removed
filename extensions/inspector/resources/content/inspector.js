













































var inspector;



const kSearchRegURL        = "resource:///res/inspector/search-registry.rdf";

const kClipboardHelperCID  = "@mozilla.org/widget/clipboardhelper;1";
const kPromptServiceCID    = "@mozilla.org/embedcomp/prompt-service;1";
const nsIWebNavigation     = Components.interfaces.nsIWebNavigation;
const nsIDocShellTreeItem  = Components.interfaces.nsIDocShellTreeItem;
const nsIDocShell          = Components.interfaces.nsIDocShell;



window.addEventListener("load", InspectorApp_initialize, false);
window.addEventListener("unload", InspectorApp_destroy, false);

function InspectorApp_initialize()
{
  inspector = new InspectorApp();

  
  
  
  var initNode, initURI;
  if (window.arguments && window.arguments.length) {
    if (typeof window.arguments[0] == "string") {
      initURI = window.arguments[0];
    }
    else if (window.arguments[0] instanceof Components.interfaces.nsIDOMNode) {
      initNode = window.arguments[0];
    }
  }
  inspector.initialize(initNode, initURI);

  
  if (!/Mac/.test(navigator.platform)) {
    document.getElementById("keyEditDeleteMac")
            .setAttribute("disabled", "true");
  }
}

function InspectorApp_destroy()
{
  inspector.destroy();
}




function InspectorApp()
{
}

InspectorApp.prototype = 
{
  
  

  mSearchService: null,
  mShowBrowser: false,
  mClipboardHelper: null,
  mPromptService: null,
  
  get document() { return this.mDocPanel.viewer.subject },
  get searchRegistry() { return this.mSearchService },
  get panelset() { return this.mPanelSet; },
  
  initialize: function(aTarget, aURI)
  {
    this.mInitTarget = aTarget;
    
    

    var el = document.getElementById("bxBrowser");
    el.addEventListener("pageshow", BrowserPageShowListener, true);

    this.setBrowser(false, true);
    

    this.mClipboardHelper = XPCU.getService(kClipboardHelperCID, "nsIClipboardHelper");
    this.mPromptService = XPCU.getService(kPromptServiceCID, "nsIPromptService");

    this.mPanelSet = document.getElementById("bxPanelSet");
    this.mPanelSet.addObserver("panelsetready", this, false);
    this.mPanelSet.initialize();

    
    var cmd = document.getElementById("cmd:toggleAccessibleNodes");
    if (cmd) {
      if (!("@mozilla.org/accessibilityService;1" in Components.classes))
        cmd.setAttribute("disabled", "true");
    }

    if (aURI) {
      this.gotoURL(aURI);
    }
  },

  destroy: function()
  {
    InsUtil.persistAll("bxDocPanel");
    InsUtil.persistAll("bxObjectPanel");
  },
  
  
  
  
  initViewerPanels: function()
  {
    this.mDocPanel = this.mPanelSet.getPanel(0);
    this.mDocPanel.addObserver("subjectChange", this, false);
    this.mObjectPanel = this.mPanelSet.getPanel(1);

    if (this.mInitTarget) {
      if (this.mInitTarget.nodeType == Node.DOCUMENT_NODE)
        this.setTargetDocument(this.mInitTarget);
      else if (this.mInitTarget.nodeType == Node.ELEMENT_NODE) {
        this.setTargetDocument(this.mInitTarget.ownerDocument);
        this.mDocPanel.params = this.mInitTarget;
      }
      this.mInitTarget = null;
    }
  },

  onEvent: function(aEvent)
  {
    switch (aEvent.type) {
      case "panelsetready":
        this.initViewerPanels();
        break;
      case "subjectChange":
        if (aEvent.target == this.mDocPanel.viewer &&
            aEvent.subject && "location" in aEvent.subject) {
          this.locationText = aEvent.subject.location; 

          var docTitle = aEvent.subject.title || aEvent.subject.location; 
          if (/Mac/.test(navigator.platform)) {
            document.title = docTitle;
          } else {
            document.title = docTitle + " - " + 
              document.documentElement.getAttribute("title");
          }
        }
        break;
    }
  },
  
  
  

  doViewerCommand: function(aCommand)
  {
    this.mPanelSet.execCommand(aCommand);
  },
  
  showOpenURLDialog: function()
  {
    var bundle = this.mPanelSet.stringBundle;
    var msg = bundle.getString("inspectURL.message");
    var title = bundle.getString("inspectURL.title");
    var url = { value: "http://" };
    var dummy = { value: false };
    var go = this.mPromptService.prompt(window, title, msg, url, null, dummy);
    if (go) {
      this.gotoURL(url.value);
    }
  },

  showPrefsDialog: function()
  {
    goPreferences("advancedItem", "chrome://inspector/content/prefs/pref-inspector.xul", "inspector");
  },
  
  toggleBrowser: function(aToggleSplitter)
  {
    this.setBrowser(!this.mShowBrowser, aToggleSplitter)
  },

  setBrowser: function(aValue, aToggleSplitter)
  {
    this.mShowBrowser = aValue;
    if (aToggleSplitter)
      this.openSplitter("Browser", aValue);
    var cmd = document.getElementById("cmdToggleBrowser");
    cmd.setAttribute("checked", aValue);
  },

  toggleSearch: function(aToggleSplitter)
  {
    this.setSearch(!this.mShowSearch, aToggleSplitter);
  },

  setSearch: function(aValue, aToggleSplitter)
  {
    this.mShowSearch = aValue;
    if (aToggleSplitter)
      this.openSplitter("Search", aValue);
    var cmd = document.getElementById("cmdToggleSearch");
    cmd.setAttribute("checked", aValue);
  },

  openSplitter: function(aName, aTruth)
  {
    var splitter = document.getElementById("spl" + aName);
    if (aTruth)
      splitter.open();
    else
      splitter.close();
  },
























































































  exit: function()
  {
    window.close();
    
  },

  
  
  
  gotoTypedURL: function()
  {
    var url = document.getElementById("tfURLBar").value;
    this.gotoURL(url);
  },

  gotoURL: function(aURL, aNoSaveHistory)
  {
    this.mPendingURL = aURL;
    this.mPendingNoSave = aNoSaveHistory;
    this.browseToURL(aURL);
    this.setBrowser(true, true);
  },

  browseToURL: function(aURL)
  {
    try {
      this.webNavigation.loadURI(aURL, nsIWebNavigation.LOAD_FLAGS_NONE, null, null, null);
    }
    catch(ex) {
      
      
    }
  },

 


  showInspectDocumentList: function showInspectDocumentList(aEvent, aChrome)
  {
    const XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    var menu = aEvent.target;
    var ww = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                       .getService(Components.interfaces.nsIWindowMediator);
    var windows = ww.getXULWindowEnumerator(null);
    var docs = [];

    while (windows.hasMoreElements()) {
      try {
        
        var windowDocShell = windows.getNext()
                            .QueryInterface(Components.interfaces.nsIXULWindow)
                            .docShell;
        this.appendContainedDocuments(docs, windowDocShell,
                                      aChrome ? nsIDocShellTreeItem.typeChrome
                                              : nsIDocShellTreeItem.typeContent);
      }
      catch (ex) {
        
        
        Components.utils.reportError(ex);
      }
    }

    
    this.emptyChildren(menu);

    
    if (!docs.length) {
      var noneMenuItem = document.createElementNS(XULNS, "menuitem");
      noneMenuItem.setAttribute("label",
                                this.mPanelSet.stringBundle
                                    .getString("inspectWindow.noDocuments.message"));
      noneMenuItem.setAttribute("disabled", true);
      menu.appendChild(noneMenuItem);
    } else {
      for (var i = 0; i < docs.length; i++)
        this.addInspectDocumentMenuItem(menu, docs[i], i + 1);
    }
  },

 







  appendContainedDocuments: function appendContainedDocuments(array, docShell, type)
  {
    
    var containedDocShells = docShell.getDocShellEnumerator(type, 
                                      nsIDocShell.ENUMERATE_FORWARDS);
    while (containedDocShells.hasMoreElements()) {
      try {
        
        var childDoc = containedDocShells.getNext().QueryInterface(nsIDocShell)
                                         .contentViewer.DOMDocument;

        
        if (docShell.contentViewer.DOMDocument.location.href != document.location.href ||
            childDoc.location.href != "about:blank") {
          array.push(childDoc);
        }
      }
      catch (ex) {
        
        
        dump(ex + "\n");
      }
    }
  },

 





  addInspectDocumentMenuItem: function addInspectDocumentMenuItem(parent, doc, docNumber)
  {
    const XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    var menuItem = document.createElementNS(XULNS, "menuitem");
    menuItem.doc = doc;
    
    var title = doc.title || doc.location.href;
    
    if (docNumber < 10) {
      menuItem.setAttribute("label", docNumber + " " + title);
      menuItem.setAttribute("accesskey", docNumber);
    } else {
      menuItem.setAttribute("label", title);
    }
    parent.appendChild(menuItem);
  },

  setTargetWindow: function(aWindow)
  {
    this.setTargetDocument(aWindow.document);
  },

  setTargetDocument: function(aDoc)
  {
    this.mDocPanel.subject = aDoc;
  },

  get webNavigation()
  {
    var browser = document.getElementById("ifBrowser");
    return browser.webNavigation;
  },

  
  

  get locationText() { return document.getElementById("tfURLBar").value; },
  set locationText(aText) { document.getElementById("tfURLBar").value = aText; },

  get statusText() { return document.getElementById("txStatus").value; },
  set statusText(aText) { document.getElementById("txStatus").value = aText; },

  get progress() { return document.getElementById("pmStatus").value; },
  set progress(aPct) { document.getElementById("pmStatus").value = aPct; },












  
  

  documentLoaded: function()
  {
    this.setTargetWindow(_content);

    var url = this.webNavigation.currentURI.spec;
    
    
    this.locationText = url;

    
    if (!this.mPendingNoSave)
      this.addToHistory(url);

    this.mPendingURL = null;
    this.mPendingNoSave = null;
  },

  
  


























































  
  

  addToHistory: function(aURL)
  {
  },

  
  
  
  get isViewingContent() { return this.mPanelSet.getPanel(0).subject != null; },
  
  fillInTooltip: function(tipElement)
  {
    var retVal = false;
    var textNode = document.getElementById("txTooltip");
    if (textNode) {
      try {  
        var tipText = tipElement.getAttribute("tooltiptext");
        if (tipText != "") {
          textNode.setAttribute("value", tipText);
          retVal = true;
        }
      }
      catch (e) { }
    }
    
    return retVal;
  },

  initPopup: function(aPopup)
  {
    var items = aPopup.getElementsByTagName("menuitem");
    var js, fn, item;
    for (var i = 0; i < items.length; i++) {
      item = items[i];
      fn = "isDisabled" in item ? item.isDisabled : null;
      if (!fn) {
        js = item.getAttribute("isDisabled");
        if (js) {
          fn = new Function(js);
          item.isDisabled = fn;
        } else {
          item.isDisabled = null; 
        }
      } 
      if (fn) {
        if (item.isDisabled())
          item.setAttribute("disabled", "true");
        else
          item.removeAttribute("disabled");
      }

      fn = null;
    }
  },

  emptyChildren: function(aNode)
  {
    while (aNode.hasChildNodes()) {
      aNode.removeChild(aNode.lastChild);
    }
  },

  onSplitterOpen: function(aSplitter)
  {
    if (aSplitter.id == "splBrowser") {
      this.setBrowser(aSplitter.isOpened, false);
    } else if (aSplitter.id == "splSearch") {
      this.setSearch(aSplitter.isOpened, false);
    }
  },
  
  
  
  getViewer: function(aUID)
  {
    return this.mPanelSet.registry.getViewerByUID(aUID);
  }
  
};




function BrowserPageShowListener(aEvent) 
{
  
  
  if (aEvent.target.defaultView == _content)
    inspector.documentLoaded();
}

function UtilWindowOpenListener(aWindow)
{
  inspector.doViewSearchItem(aWindow);
}
