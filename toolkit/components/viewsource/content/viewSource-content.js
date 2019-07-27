



const { utils: Cu, interfaces: Ci, classes: Cc } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "BrowserUtils",
  "resource://gre/modules/BrowserUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DeferredTask",
  "resource://gre/modules/DeferredTask.jsm");

const BUNDLE_URL = "chrome://global/locale/viewSource.properties";

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
  ],

  







  selectionListenerAttached: false,

  


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
    }
  },

  



  handleEvent(event) {
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

    if (lineNumber) {
      let doneLoading = (event) => {
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
    
    if (!event.isTrusted || event.target.localName != "button")
      return;

    let target = event.originalTarget;
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
    content.getSelection()
           .QueryInterface(Ci.nsISelectionPrivate)
           .addSelectionListener(this);
    this.selectionListenerAttached = true;

    content.focus();
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
    
    
    
    this.reload();
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
};
ViewSourceContent.init();
