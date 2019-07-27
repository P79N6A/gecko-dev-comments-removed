





Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/CharsetMenu.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;

var gLastLineFound = '';
var gGoToLine = 0;

[
  ["gBrowser",          "content"],
  ["gViewSourceBundle", "viewSourceBundle"],
  ["gContextMenu",      "viewSourceContextMenu"]
].forEach(function ([name, id]) {
  window.__defineGetter__(name, function () {
    var element = document.getElementById(id);
    if (!element)
      return null;
    delete window[name];
    return window[name] = element;
  });
});


function getBrowser() {
  return gBrowser;
}

this.__defineGetter__("gPageLoader", function () {
  var webnav = getWebNavigation();
  if (!webnav)
    return null;
  delete this.gPageLoader;
  return this.gPageLoader = webnav.QueryInterface(Ci.nsIWebPageDescriptor);
});

var gSelectionListener = {
  timeout: 0,
  attached: false,
  notifySelectionChanged: function(doc, sel, reason)
  {
    
    if (!this.timeout)
      this.timeout = setTimeout(updateStatusBar, 100);
  }
}

function onLoadViewSource() 
{
  viewSource(window.arguments[0]);
  document.commandDispatcher.focusedWindow = content;
  gBrowser.droppedLinkHandler = function (event, url, name) {
    viewSource(url)
    event.preventDefault();
  }

  if (!isHistoryEnabled()) {
    
    var viewSourceNavigation = document.getElementById("viewSourceNavigation");
    viewSourceNavigation.setAttribute("disabled", "true");
    viewSourceNavigation.setAttribute("hidden", "true");
  }
}

function isHistoryEnabled() {
  return !gBrowser.hasAttribute("disablehistory");
}

function getSelectionController() {
  return gBrowser.docShell
                 .QueryInterface(Ci.nsIInterfaceRequestor)
                 .getInterface(Ci.nsISelectionDisplay)
                 .QueryInterface(Ci.nsISelectionController);
}

function viewSource(url)
{
  if (!url)
    return; 
    
  var viewSrcUrl = "view-source:" + url;

  gBrowser.addEventListener("pagehide", onUnloadContent, true);
  gBrowser.addEventListener("pageshow", onLoadContent, true);
  gBrowser.addEventListener("click", onClickContent, false);

  var loadFromURL = true;

  
  
  
  
  
  

  if ("arguments" in window) {
    var arg;

    
    var charset;
    if (window.arguments.length >= 2) {
      arg = window.arguments[1];

      try {
        if (typeof(arg) == "string" && arg.indexOf('charset=') != -1) {
          var arrayArgComponents = arg.split('=');
          if (arrayArgComponents) {
            
            
            charset = arrayArgComponents[1];
          }
        }
      } catch (ex) {
        
      }
    }
    
    if (window.arguments.length >= 5) {
      arg = window.arguments[4];

      try {
        if (arg === true) {
          gBrowser.docShell.charset = charset;
        }
      } catch (ex) {
        
      }
    }

    
    if (window.arguments.length >= 4) {
      arg = window.arguments[3];
      gGoToLine = parseInt(arg);
    }

    
    
    if (window.arguments.length >= 3) {
      arg = window.arguments[2];

      try {
        if (typeof(arg) == "object" && arg != null) {
          
          
          
          gPageLoader.loadPage(arg, gPageLoader.DISPLAY_AS_SOURCE);

          
          loadFromURL = false;

          
          var shEntrySource = arg.QueryInterface(Ci.nsISHEntry);
          var shEntry = Cc["@mozilla.org/browser/session-history-entry;1"].createInstance(Ci.nsISHEntry);
          shEntry.setURI(makeURI(viewSrcUrl, null, null));
          shEntry.setTitle(viewSrcUrl);
          shEntry.loadType = Ci.nsIDocShellLoadInfo.loadHistory;
          shEntry.cacheKey = shEntrySource.cacheKey;
          gBrowser.sessionHistory
                  .QueryInterface(Ci.nsISHistoryInternal)
                  .addEntry(shEntry, true);
        }
      } catch(ex) {
        
        
      }
    }
  }

  if (loadFromURL) {
    
    var loadFlags = Ci.nsIWebNavigation.LOAD_FLAGS_NONE;
    getWebNavigation().loadURI(viewSrcUrl, loadFlags, null, null, null);
  }

  
  
  var wraplonglinesPrefValue = Services.prefs.getBoolPref("view_source.wrap_long_lines");

  if (wraplonglinesPrefValue)
    document.getElementById("menu_wrapLongLines").setAttribute("checked", "true");

  document.getElementById("menu_highlightSyntax")
          .setAttribute("checked",
                        Services.prefs.getBoolPref("view_source.syntax_highlight"));

  window.addEventListener("AppCommand", HandleAppCommandEvent, true);
  window.addEventListener("MozSwipeGesture", HandleSwipeGesture, true);
  window.content.focus();
}

function onLoadContent()
{
  
  if (gGoToLine > 0) {
    goToLine(gGoToLine);
    gGoToLine = 0;
  }
  document.getElementById('cmd_goToLine').removeAttribute('disabled');

  
  window.content.getSelection()
   .QueryInterface(Ci.nsISelectionPrivate)
   .addSelectionListener(gSelectionListener);
  gSelectionListener.attached = true;

  if (isHistoryEnabled())
    UpdateBackForwardCommands();
}

function onUnloadContent()
{
  
  
  document.getElementById('cmd_goToLine').setAttribute('disabled', 'true');

  
  
  
  if (gSelectionListener.attached) {
    window.content.getSelection().QueryInterface(Ci.nsISelectionPrivate)
          .removeSelectionListener(gSelectionListener);
    gSelectionListener.attached = false;
  }
}




function onClickContent(event) {
  
  if (!event.isTrusted || event.target.localName != "button")
    return;

  var target = event.originalTarget;
  var errorDoc = target.ownerDocument;

  if (/^about:blocked/.test(errorDoc.documentURI)) {
    

    if (target == errorDoc.getElementById('getMeOutButton')) {
      
      window.close();
    } else if (target == errorDoc.getElementById('reportButton')) {
      
      
      let url = Services.urlFormatter.formatURLPref("app.support.baseURL");
      openURL(url + "phishing-malware");
    } else if (target == errorDoc.getElementById('ignoreWarningButton')) {
      
      gBrowser.loadURIWithFlags(content.location.href,
                                Ci.nsIWebNavigation.LOAD_FLAGS_BYPASS_CLASSIFIER,
                                null, null, null);
    }
  }
}

function HandleAppCommandEvent(evt)
{
  evt.stopPropagation();
  switch (evt.command) {
    case "Back":
      BrowserBack();
      break;
    case "Forward":
      BrowserForward();
      break;
  }
}

function HandleSwipeGesture(evt) {
  evt.stopPropagation();
  switch (evt.direction) {
    case SimpleGestureEvent.DIRECTION_LEFT:
      BrowserBack();
      break;
    case SimpleGestureEvent.DIRECTION_RIGHT:
      BrowserForward();
      break;
    case SimpleGestureEvent.DIRECTION_UP:
      goDoCommand("cmd_scrollTop");
      break;
    case SimpleGestureEvent.DIRECTION_DOWN:
      goDoCommand("cmd_scrollBottom");
      break;
  }
}

function ViewSourceClose()
{
  window.close();
}

function ViewSourceReload()
{
  gBrowser.reloadWithFlags(Ci.nsIWebNavigation.LOAD_FLAGS_BYPASS_PROXY |
                           Ci.nsIWebNavigation.LOAD_FLAGS_BYPASS_CACHE);
}


function ViewSourceSavePage()
{
  internalSave(window.content.location.href.replace(/^view-source:/i, ""),
               null, null, null, null, null, "SaveLinkTitle",
               null, null, window.content.document, null, gPageLoader);
}

var PrintPreviewListener = {
  getPrintPreviewBrowser: function () {
    var browser = document.getElementById("ppBrowser");
    if (!browser) {
      browser = document.createElement("browser");
      browser.setAttribute("id", "ppBrowser");
      browser.setAttribute("flex", "1");
      browser.setAttribute("type", "content");
      document.getElementById("appcontent").
        insertBefore(browser, document.getElementById("FindToolbar"));
    }

    return browser;
  },
  getSourceBrowser: function () {
    return gBrowser;
  },
  getNavToolbox: function () {
    return document.getElementById("appcontent");
  },
  onEnter: function () {
    var toolbox = document.getElementById("viewSource-toolbox");
    toolbox.hidden = true;
    gBrowser.collapsed = true;
  },
  onExit: function () {
    document.getElementById("ppBrowser").collapsed = true;
    gBrowser.collapsed = false;
    document.getElementById("viewSource-toolbox").hidden = false;
  }
}

function getWebNavigation()
{
  try {
    return gBrowser.webNavigation;
  } catch (e) {
    return null;
  }
}

function ViewSourceGoToLine()
{
  var input = {value:gLastLineFound};
  for (;;) {
    var ok = Services.prompt.prompt(
        window,
        gViewSourceBundle.getString("goToLineTitle"),
        gViewSourceBundle.getString("goToLineText"),
        input,
        null,
        {value:0});

    if (!ok)
      return;

    var line = parseInt(input.value, 10);

    if (!(line > 0)) {
      Services.prompt.alert(window,
                            gViewSourceBundle.getString("invalidInputTitle"),
                            gViewSourceBundle.getString("invalidInputText"));

      continue;
    }

    var found = goToLine(line);

    if (found)
      break;

    Services.prompt.alert(window,
                          gViewSourceBundle.getString("outOfRangeTitle"),
                          gViewSourceBundle.getString("outOfRangeText"));
  }
}

function goToLine(line)
{
  var viewsource = window.content.document.body;

  
  
  
  
  
  

  var pre;
  for (var lbound = 0, ubound = viewsource.childNodes.length; ; ) {
    var middle = (lbound + ubound) >> 1;
    pre = viewsource.childNodes[middle];

    var firstLine = pre.id ? parseInt(pre.id.substring(4)) : 1;

    if (lbound == ubound - 1) {
      break;
    }

    if (line >= firstLine) {
      lbound = middle;
    } else {
      ubound = middle;
    }
  }

  var result = {};
  var found = findLocation(pre, line, null, -1, false, result);

  if (!found) {
    return false;
  }

  var selection = window.content.getSelection();
  selection.removeAllRanges();

  
  
  

  selection.QueryInterface(Ci.nsISelectionPrivate)
    .interlinePosition = true;

  selection.addRange(result.range);

  if (!selection.isCollapsed) {
    selection.collapseToEnd();

    var offset = result.range.startOffset;
    var node = result.range.startContainer;
    if (offset < node.data.length) {
      
      selection.extend(node, offset);
    }
    else {
      
      
      
      
      node = node.nextSibling ? node.nextSibling : node.parentNode.nextSibling;
      selection.extend(node, 0);
    }
  }

  var selCon = getSelectionController();
  selCon.setDisplaySelection(Ci.nsISelectionController.SELECTION_ON);
  selCon.setCaretVisibilityDuringSelection(true);

  
  selCon.scrollSelectionIntoView(
    Ci.nsISelectionController.SELECTION_NORMAL,
    Ci.nsISelectionController.SELECTION_FOCUS_REGION,
    true);

  gLastLineFound = line;

  document.getElementById("statusbar-line-col").label =
    gViewSourceBundle.getFormattedString("statusBarLineCol", [line, 1]);

  return true;
}

function updateStatusBar()
{
  
  gSelectionListener.timeout = 0;

  var statusBarField = document.getElementById("statusbar-line-col");

  var selection = window.content.getSelection();
  if (!selection.focusNode) {
    statusBarField.label = '';
    return;
  }
  if (selection.focusNode.nodeType != Node.TEXT_NODE) {
    return;
  }

  var selCon = getSelectionController();
  selCon.setDisplaySelection(Ci.nsISelectionController.SELECTION_ON);
  selCon.setCaretVisibilityDuringSelection(true);

  var interlinePosition = selection.QueryInterface(Ci.nsISelectionPrivate)
                                   .interlinePosition;

  var result = {};
  findLocation(null, -1, 
      selection.focusNode, selection.focusOffset, interlinePosition, result);

  statusBarField.label = gViewSourceBundle.getFormattedString(
                           "statusBarLineCol", [result.line, result.col]);
}








function findLocation(pre, line, node, offset, interlinePosition, result)
{
  if (node && !pre) {
    
    for (pre = node;
         pre.nodeName != "PRE";
         pre = pre.parentNode);
  }

  
  
  
  
  
  var curLine = pre.id ? parseInt(pre.id.substring(4)) : 1;

  
  var treewalker = window.content.document
      .createTreeWalker(pre, NodeFilter.SHOW_TEXT, null);

  
  var firstCol = 1;

  var found = false;
  for (var textNode = treewalker.firstChild();
       textNode && !found;
       textNode = treewalker.nextNode()) {

    
    var lineArray = textNode.data.split(/\n/);
    var lastLineInNode = curLine + lineArray.length - 1;

    
    if (node ? (textNode != node) : (lastLineInNode < line)) {
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
        if (curLine == line && !("range" in result)) {
          result.range = document.createRange();
          result.range.setStart(textNode, curPos);

          
          
          
          result.range.setEndAfter(pre.lastChild);

        } else if (curLine == line + 1) {
          result.range.setEnd(textNode, curPos - 1);
          found = true;
          break;
        }
      }
    }
  }

  return found || ("range" in result);
}



function wrapLongLines()
{
  var myWrap = window.content.document.body;
  myWrap.classList.toggle("wrap");

  
  
  
  Services.prefs.setBoolPref("view_source.wrap_long_lines", myWrap.classList.contains("wrap"));
}



function highlightSyntax()
{
  var highlightSyntaxMenu = document.getElementById("menu_highlightSyntax");
  var highlightSyntax = (highlightSyntaxMenu.getAttribute("checked") == "true");
  Services.prefs.setBoolPref("view_source.syntax_highlight", highlightSyntax);

  gPageLoader.loadPage(gPageLoader.currentDescriptor, gPageLoader.DISPLAY_NORMAL);
}





function BrowserCharsetReload()
{
  if (isHistoryEnabled()) {
    gPageLoader.loadPage(gPageLoader.currentDescriptor,
                         gPageLoader.DISPLAY_NORMAL);
  } else {
    gBrowser.reloadWithFlags(Ci.nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
  }
}

function BrowserSetCharacterSet(aEvent)
{
  if (aEvent.target.hasAttribute("charset"))
    gBrowser.docShell.charset = aEvent.target.getAttribute("charset");
  BrowserCharsetReload();
}

function BrowserForward(aEvent) {
  try {
    gBrowser.goForward();
  }
  catch(ex) {
  }
}

function BrowserBack(aEvent) {
  try {
    gBrowser.goBack();
  }
  catch(ex) {
  }
}

function UpdateBackForwardCommands() {
  var backBroadcaster = document.getElementById("Browser:Back");
  var forwardBroadcaster = document.getElementById("Browser:Forward");

  if (getWebNavigation().canGoBack)
    backBroadcaster.removeAttribute("disabled");
  else
    backBroadcaster.setAttribute("disabled", "true");

  if (getWebNavigation().canGoForward)
    forwardBroadcaster.removeAttribute("disabled");
  else
    forwardBroadcaster.setAttribute("disabled", "true");
}

function contextMenuShowing() {
  var isLink = false;
  var isEmail = false;
  if (gContextMenu.triggerNode && gContextMenu.triggerNode.localName == 'a') {
    if (gContextMenu.triggerNode.href.indexOf('view-source:') == 0)
      isLink = true;
    if (gContextMenu.triggerNode.href.indexOf('mailto:') == 0)
      isEmail = true;
  }
  document.getElementById('context-copyLink').hidden = !isLink;
  document.getElementById('context-copyEmail').hidden = !isEmail;
}

function contextMenuCopyLinkOrEmail() {
  if (!gContextMenu.triggerNode)
    return;

  var href = gContextMenu.triggerNode.href;
  var clipboard = Cc['@mozilla.org/widget/clipboardhelper;1'].
                  getService(Ci.nsIClipboardHelper);
  clipboard.copyString(href.substring(href.indexOf(':') + 1), document);
}
