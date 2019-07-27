



const { utils: Cu, interfaces: Ci, classes: Cc } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "BrowserUtils",
  "resource://gre/modules/BrowserUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DeferredTask",
  "resource://gre/modules/DeferredTask.jsm");

const NS_XHTML = "http://www.w3.org/1999/xhtml";
const BUNDLE_URL = "chrome://global/locale/viewSource.properties";






const MARK_SELECTION_START = "\uFDD0";
const MARK_SELECTION_END = "\uFDEF";

let global = this;





let ViewSourceContent = {
  



  QueryInterface: XPCOMUtils.generateQI([Ci.nsISelectionListener]),

  




  messages: [
    "ViewSource:LoadSource",
    "ViewSource:LoadSourceDeprecated",
    "ViewSource:GoToLine",
    "ViewSource:ToggleWrapping",
    "ViewSource:ToggleSyntaxHighlighting",
    "ViewSource:SetCharacterSet",
    "ViewSource:ScheduleDrawSelection",
  ],

  




  needsDrawSelection: false,

  







  selectionListenerAttached: false,

  get isViewSource() {
    let uri = content.document.documentURI;
    return uri == "about:blank" || uri.startsWith("view-source:") ||
           (uri.startsWith("data:") && uri.includes("MathML"));
  },

  


  init() {
    this.messages.forEach((msgName) => {
      addMessageListener(msgName, this);
    });

    addEventListener("pagehide", this, true);
    addEventListener("pageshow", this, true);
    addEventListener("click", this);
    addEventListener("unload", this);
    Services.els.addSystemEventListener(global, "contextmenu", this, false);
  },

  



  uninit() {
    this.messages.forEach((msgName) => {
      removeMessageListener(msgName, this);
    });

    removeEventListener("pagehide", this, true);
    removeEventListener("pageshow", this, true);
    removeEventListener("click", this);
    removeEventListener("unload", this);

    Services.els.removeSystemEventListener(global, "contextmenu", this, false);

    
    if (this.updateStatusTask) {
      this.updateStatusTask.disarm();
    }
  },

  



  receiveMessage(msg) {
    if (!this.isViewSource) {
      return;
    }
    let data = msg.data;
    let objects = msg.objects;
    switch(msg.name) {
      case "ViewSource:LoadSource":
        this.viewSource(data.URL, data.outerWindowID, data.lineNumber,
                        data.shouldWrap);
        break;
      case "ViewSource:LoadSourceDeprecated":
        this.viewSourceDeprecated(data.URL, objects.pageDescriptor, data.lineNumber,
                                  data.forcedCharSet);
        break;
      case "ViewSource:GoToLine":
        this.goToLine(data.lineNumber);
        break;
      case "ViewSource:ToggleWrapping":
        this.toggleWrapping();
        break;
      case "ViewSource:ToggleSyntaxHighlighting":
        this.toggleSyntaxHighlighting();
        break;
      case "ViewSource:SetCharacterSet":
        this.setCharacterSet(data.charset, data.doPageLoad);
        break;
      case "ViewSource:ScheduleDrawSelection":
        this.scheduleDrawSelection();
        break;
    }
  },

  



  handleEvent(event) {
    if (!this.isViewSource) {
      return;
    }
    switch(event.type) {
      case "pagehide":
        this.onPageHide(event);
        break;
      case "pageshow":
        this.onPageShow(event);
        break;
      case "click":
        this.onClick(event);
        break;
      case "unload":
        this.uninit();
        break;
      case "contextmenu":
        this.onContextMenu(event);
        break;
    }
  },

  


  get bundle() {
    delete this.bundle;
    this.bundle = Services.strings.createBundle(BUNDLE_URL);
    return this.bundle;
  },

  


  get selectionController() {
    return docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsISelectionDisplay)
                   .QueryInterface(Ci.nsISelectionController);
  },

  


  get webBrowserFind() {
    return docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIWebBrowserFind);
  },

  












  viewSource(URL, outerWindowID, lineNumber) {
    let pageDescriptor, forcedCharSet;

    if (outerWindowID) {
      let contentWindow = Services.wm.getOuterWindowWithId(outerWindowID);
      let requestor = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor);

      try {
        let otherWebNav = requestor.getInterface(Ci.nsIWebNavigation);
        pageDescriptor = otherWebNav.QueryInterface(Ci.nsIWebPageDescriptor)
                                    .currentDescriptor;
      } catch(e) {
        
        
      }

      let utils = requestor.getInterface(Ci.nsIDOMWindowUtils);
      let doc = contentWindow.document;
      let forcedCharSet = utils.docCharsetIsForced ? doc.characterSet
                                                   : null;
    }

    this.loadSource(URL, pageDescriptor, lineNumber, forcedCharSet);
  },

  















  viewSourceDeprecated(URL, pageDescriptor, lineNumber, forcedCharSet) {
    
    
    if (Services.appinfo.processType != Services.appinfo.PROCESS_TYPE_DEFAULT) {
      throw new Error("ViewSource deprecated API should not be used with " +
                      "remote browsers.");
    }

    this.loadSource(URL, pageDescriptor, lineNumber, forcedCharSet);
  },

  















  loadSource(URL, pageDescriptor, lineNumber, forcedCharSet) {
    const viewSrcURL = "view-source:" + URL;
    let loadFromURL = false;

    if (forcedCharSet) {
      docShell.charset = forcedCharSet;
    }

    if (lineNumber && lineNumber > 0) {
      let doneLoading = (event) => {
        let uri = content.document.documentURI;
        
        if (uri == "about:blank" ||
            !content.document.body) {
          return;
        }
        this.goToLine(lineNumber);
        removeEventListener("pageshow", doneLoading);
      };

      addEventListener("pageshow", doneLoading);
    }

    if (!pageDescriptor) {
      this.loadSourceFromURL(viewSrcURL);
      return;
    }

    try {
      let pageLoader = docShell.QueryInterface(Ci.nsIWebPageDescriptor);
      pageLoader.loadPage(pageDescriptor,
                          Ci.nsIWebPageDescriptor.DISPLAY_AS_SOURCE);
    } catch(e) {
      
      this.loadSourceFromURL(viewSrcURL);
      return;
    }

    let shEntrySource = pageDescriptor.QueryInterface(Ci.nsISHEntry);
    let shEntry = Cc["@mozilla.org/browser/session-history-entry;1"]
                    .createInstance(Ci.nsISHEntry);
    shEntry.setURI(BrowserUtils.makeURI(viewSrcURL, null, null));
    shEntry.setTitle(viewSrcURL);
    shEntry.loadType = Ci.nsIDocShellLoadInfo.loadHistory;
    shEntry.cacheKey = shEntrySource.cacheKey;
    docShell.QueryInterface(Ci.nsIWebNavigation)
            .sessionHistory
            .QueryInterface(Ci.nsISHistoryInternal)
            .addEntry(shEntry, true);
  },

  





  loadSourceFromURL(URL) {
    let loadFlags = Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
    let webNav = docShell.QueryInterface(Ci.nsIWebNavigation);
    webNav.loadURI(URL, loadFlags, null, null, null);
  },

  





  onClick(event) {
    let target = event.originalTarget;
    
    if (target.id) {
      this.contextMenuItems.forEach(itemSpec => {
        if (itemSpec.id !== target.id) {
          return;
        }
        itemSpec.handler.call(this, event);
        event.stopPropagation();
      });
    }

    
    if (!event.isTrusted || event.target.localName != "button")
      return;

    let errorDoc = target.ownerDocument;

    if (/^about:blocked/.test(errorDoc.documentURI)) {
      

      if (target == errorDoc.getElementById("getMeOutButton")) {
        
        sendAsyncMessage("ViewSource:Close");
      } else if (target == errorDoc.getElementById("reportButton")) {
        
        
        let URL = Services.urlFormatter.formatURLPref("app.support.baseURL");
        sendAsyncMessage("ViewSource:OpenURL", { URL })
      } else if (target == errorDoc.getElementById("ignoreWarningButton")) {
        
        docShell.QueryInterface(Ci.nsIWebNavigation)
                .loadURIWithOptions(content.location.href,
                                    Ci.nsIWebNavigation.LOAD_FLAGS_BYPASS_CLASSIFIER,
                                    null, Ci.nsIHttpChannel.REFERRER_POLICY_DEFAULT,
                                    null, null, null);
      }
    }
  },

  





  onPageShow(event) {
    let selection = content.getSelection();
    if (selection) {
      selection.QueryInterface(Ci.nsISelectionPrivate)
               .addSelectionListener(this);
      this.selectionListenerAttached = true;
    }
    content.focus();

    
    
    if (this.needsDrawSelection &&
        content.document.documentURI.startsWith("view-source:")) {
      this.needsDrawSelection = false;
      this.drawSelection();
    }

    if (content.document.body) {
      this.injectContextMenu();
    }

    sendAsyncMessage("ViewSource:SourceLoaded");
  },

  





  onPageHide(event) {
    
    
    
    if (this.selectionListenerAttached) {
      content.getSelection()
             .QueryInterface(Ci.nsISelectionPrivate)
             .removeSelectionListener(this);
      this.selectionListenerAttached = false;
    }
    sendAsyncMessage("ViewSource:SourceUnloaded");
  },

  onContextMenu(event) {
    let addonInfo = {};
    let subject = {
      event: event,
      addonInfo: addonInfo,
    };

    subject.wrappedJSObject = subject;
    Services.obs.notifyObservers(subject, "content-contextmenu", null);

    let node = event.target;

    let result = {
      isEmail: false,
      isLink: false,
      href: "",
      
      
      
      screenX: event.screenX,
      screenY: event.screenY,
    };

    if (node && node.localName == "a") {
      result.isLink = node.href.startsWith("view-source:");
      result.isEmail = node.href.startsWith("mailto:");
      result.href = node.href.substring(node.href.indexOf(":") + 1);
    }

    sendSyncMessage("ViewSource:ContextMenuOpening", result);
  },

  









  goToLine(lineNumber) {
    let body = content.document.body;

    
    
    
    
    
    
    let pre;
    for (let lbound = 0, ubound = body.childNodes.length; ; ) {
      let middle = (lbound + ubound) >> 1;
      pre = body.childNodes[middle];

      let firstLine = pre.id ? parseInt(pre.id.substring(4)) : 1;

      if (lbound == ubound - 1) {
        break;
      }

      if (lineNumber >= firstLine) {
        lbound = middle;
      } else {
        ubound = middle;
      }
    }

    let result = {};
    let found = this.findLocation(pre, lineNumber, null, -1, false, result);

    if (!found) {
      sendAsyncMessage("ViewSource:GoToLine:Failed");
      return;
    }

    let selection = content.getSelection();
    selection.removeAllRanges();

    
    
    
    selection.QueryInterface(Ci.nsISelectionPrivate)
      .interlinePosition = true;

    selection.addRange(result.range);

    if (!selection.isCollapsed) {
      selection.collapseToEnd();

      let offset = result.range.startOffset;
      let node = result.range.startContainer;
      if (offset < node.data.length) {
        
        selection.extend(node, offset);
      }
      else {
        
        
        
        
        node = node.nextSibling ? node.nextSibling : node.parentNode.nextSibling;
        selection.extend(node, 0);
      }
    }

    let selCon = this.selectionController;
    selCon.setDisplaySelection(Ci.nsISelectionController.SELECTION_ON);
    selCon.setCaretVisibilityDuringSelection(true);

    
    selCon.scrollSelectionIntoView(
      Ci.nsISelectionController.SELECTION_NORMAL,
      Ci.nsISelectionController.SELECTION_FOCUS_REGION,
      true);

    sendAsyncMessage("ViewSource:GoToLine:Success", { lineNumber });
  },


  











  findLocation(pre, lineNumber, node, offset, interlinePosition, result) {
    if (node && !pre) {
      
      for (pre = node;
           pre.nodeName != "PRE";
           pre = pre.parentNode);
    }

    
    
    
    
    
    let curLine = pre.id ? parseInt(pre.id.substring(4)) : 1;

    
    let treewalker = content.document
        .createTreeWalker(pre, Ci.nsIDOMNodeFilter.SHOW_TEXT, null);

    
    let firstCol = 1;

    let found = false;
    for (let textNode = treewalker.firstChild();
         textNode && !found;
         textNode = treewalker.nextNode()) {

      
      let lineArray = textNode.data.split(/\n/);
      let lastLineInNode = curLine + lineArray.length - 1;

      
      if (node ? (textNode != node) : (lastLineInNode < lineNumber)) {
        if (lineArray.length > 1) {
          firstCol = 1;
        }
        firstCol += lineArray[lineArray.length - 1].length;
        curLine = lastLineInNode;
        continue;
      }

      
      
      for (var i = 0, curPos = 0;
           i < lineArray.length;
           curPos += lineArray[i++].length + 1) {

        if (i > 0) {
          curLine++;
        }

        if (node) {
          if (offset >= curPos && offset <= curPos + lineArray[i].length) {
            
            
            

            if (i > 0 && offset == curPos && !interlinePosition) {
              result.line = curLine - 1;
              var prevPos = curPos - lineArray[i - 1].length;
              result.col = (i == 1 ? firstCol : 1) + offset - prevPos;
            } else {
              result.line = curLine;
              result.col = (i == 0 ? firstCol : 1) + offset - curPos;
            }
            found = true;

            break;
          }

        } else {
          if (curLine == lineNumber && !("range" in result)) {
            result.range = content.document.createRange();
            result.range.setStart(textNode, curPos);

            
            
            
            result.range.setEndAfter(pre.lastChild);

          } else if (curLine == lineNumber + 1) {
            result.range.setEnd(textNode, curPos - 1);
            found = true;
            break;
          }
        }
      }
    }

    return found || ("range" in result);
  },

  



  toggleWrapping() {
    let body = content.document.body;
    body.classList.toggle("wrap");
  },

  



  toggleSyntaxHighlighting() {
    let body = content.document.body;
    body.classList.toggle("highlight");
  },

  









  setCharacterSet(charset, doPageLoad) {
    docShell.charset = charset;
    if (doPageLoad) {
      this.reload();
    }
  },

  


  reload() {
    let pageLoader = docShell.QueryInterface(Ci.nsIWebPageDescriptor);
    try {
      pageLoader.loadPage(pageLoader.currentDescriptor,
                          Ci.nsIWebPageDescriptor.DISPLAY_NORMAL);
    } catch(e) {
      let webNav = docShell.QueryInterface(Ci.nsIWebNavigation);
      webNav.reload(Ci.nsIWebNavigation.LOAD_FLAGS_NONE);
    }
  },

  



  updateStatusTask: null,

  



  updateStatus() {
    let selection = content.getSelection();

    if (!selection.focusNode) {
      sendAsyncMessage("ViewSource:UpdateStatus", { label: "" });
      return;
    }
    if (selection.focusNode.nodeType != Ci.nsIDOMNode.TEXT_NODE) {
      return;
    }

    let selCon = this.selectionController;
    selCon.setDisplaySelection(Ci.nsISelectionController.SELECTION_ON);
    selCon.setCaretVisibilityDuringSelection(true);

    let interlinePosition = selection.QueryInterface(Ci.nsISelectionPrivate)
                                     .interlinePosition;

    let result = {};
    this.findLocation(null, -1,
        selection.focusNode, selection.focusOffset, interlinePosition, result);

    let label = this.bundle.formatStringFromName("statusBarLineCol",
                                                 [result.line, result.col], 2);
    sendAsyncMessage("ViewSource:UpdateStatus", { label });
  },

  



  




  notifySelectionChanged(doc, sel, reason) {
    if (!this.updateStatusTask) {
      this.updateStatusTask = new DeferredTask(() => {
        this.updateStatus();
      }, 100);
    }

    this.updateStatusTask.arm();
  },

  




  scheduleDrawSelection() {
    this.needsDrawSelection = true;
  },

  




  drawSelection() {
    content.document.title =
      this.bundle.GetStringFromName("viewSelectionSourceTitle");

    
    
    var findService = null;
    try {
      
      findService = Cc["@mozilla.org/find/find_service;1"]
                    .getService(Ci.nsIFindService);
    } catch(e) { }
    if (!findService)
      return;

    
    var matchCase     = findService.matchCase;
    var entireWord    = findService.entireWord;
    var wrapFind      = findService.wrapFind;
    var findBackwards = findService.findBackwards;
    var searchString  = findService.searchString;
    var replaceString = findService.replaceString;

    
    var findInst = this.webBrowserFind;
    findInst.matchCase = true;
    findInst.entireWord = false;
    findInst.wrapFind = true;
    findInst.findBackwards = false;

    
    findInst.searchString = MARK_SELECTION_START;
    var startLength = MARK_SELECTION_START.length;
    findInst.findNext();

    var selection = content.getSelection();
    if (!selection.rangeCount)
      return;

    var range = selection.getRangeAt(0);

    var startContainer = range.startContainer;
    var startOffset = range.startOffset;

    
    findInst.searchString = MARK_SELECTION_END;
    var endLength = MARK_SELECTION_END.length;
    findInst.findNext();

    var endContainer = selection.anchorNode;
    var endOffset = selection.anchorOffset;

    
    selection.removeAllRanges();

    
    endContainer.deleteData(endOffset, endLength);
    startContainer.deleteData(startOffset, startLength);
    if (startContainer == endContainer)
      endOffset -= startLength; 
    range.setEnd(endContainer, endOffset);

    
    selection.addRange(range);
    
    
    
    try {
      this.selectionController.scrollSelectionIntoView(
                                 Ci.nsISelectionController.SELECTION_NORMAL,
                                 Ci.nsISelectionController.SELECTION_ANCHOR_REGION,
                                 true);
    }
    catch(e) { }

    
    findService.matchCase     = matchCase;
    findService.entireWord    = entireWord;
    findService.wrapFind      = wrapFind;
    findService.findBackwards = findBackwards;
    findService.searchString  = searchString;
    findService.replaceString = replaceString;

    findInst.matchCase     = matchCase;
    findInst.entireWord    = entireWord;
    findInst.wrapFind      = wrapFind;
    findInst.findBackwards = findBackwards;
    findInst.searchString  = searchString;
  },

  


  contextMenuItems: [
    {
      id: "goToLine",
      handler() {
        sendAsyncMessage("ViewSource:PromptAndGoToLine");
      }
    },
    {
      id: "wrapLongLines",
      get checked() {
        return Services.prefs.getBoolPref("view_source.wrap_long_lines");
      },
      handler() {
        this.toggleWrapping();
      }
    },
    {
      id: "highlightSyntax",
      get checked() {
        return Services.prefs.getBoolPref("view_source.syntax_highlight");
      },
      handler() {
        this.toggleSyntaxHighlighting();
      }
    },
  ],

  


  injectContextMenu() {
    let doc = content.document;

    let menu = doc.createElementNS(NS_XHTML, "menu");
    menu.setAttribute("type", "context");
    menu.setAttribute("id", "actions");
    doc.body.appendChild(menu);
    doc.body.setAttribute("contextmenu", "actions");

    this.contextMenuItems.forEach(itemSpec => {
      let item = doc.createElementNS(NS_XHTML, "menuitem");
      item.setAttribute("id", itemSpec.id);
      let labelName = `context_${itemSpec.id}_label`;
      let label = this.bundle.GetStringFromName(labelName);
      item.setAttribute("label", label);
      if ("checked" in itemSpec) {
        item.setAttribute("type", "checkbox");
      }
      menu.appendChild(item);
    });

    this.updateContextMenu();
  },

  


  updateContextMenu() {
    let doc = content.document;
    this.contextMenuItems.forEach(itemSpec => {
      if (!("checked" in itemSpec)) {
        return;
      }
      let item = doc.getElementById(itemSpec.id);
      if (itemSpec.checked) {
        item.setAttribute("checked", true);
      } else {
        item.removeAttribute("checked");
      }
    });
  },
};
ViewSourceContent.init();
