# -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 2001
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Doron Rosenberg (doronr@naboonline.com)
#   Neil Rashbrook (neil@parkwaycc.co.uk)
#   DÃ£o Gottwald (dao@design-noir.de)
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

const Cc = Components.classes;
const Ci = Components.interfaces;
var gPrefs = null;

var gLastLineFound = '';
var gGoToLine = 0;

[
  ["gBrowser",          "content"],
  ["gViewSourceBundle", "viewSourceBundle"]
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

__defineGetter__("gPageLoader", function () {
  var webnav = getWebNavigation();
  if (!webnav)
    return null;
  delete this.gPageLoader;
  return this.gPageLoader = webnav.QueryInterface(Ci.nsIWebPageDescriptor);
});

try {
  var prefService = Components.classes["@mozilla.org/preferences-service;1"]
                              .getService(Components.interfaces.nsIPrefService);
  gPrefs = prefService.getBranch(null);
} catch (ex) {
}

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
            gBrowser.markupDocumentViewer.defaultCharacterSet = charset;
          }
        }
      } catch (ex) {
        
      }
    }
    
    if (window.arguments.length >= 5) {
      arg = window.arguments[4];

      try {
        if (arg === true) {
          var docCharset = gBrowser.docShell.QueryInterface(Ci.nsIDocCharset);
          docCharset.charset = charset;
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

  
  
  if (gPrefs) {
    try {
      var wraplonglinesPrefValue = gPrefs.getBoolPref("view_source.wrap_long_lines");

      if (wraplonglinesPrefValue)
        document.getElementById("menu_wrapLongLines").setAttribute("checked", "true");
    } catch (ex) {
    }
    try {
      document.getElementById("menu_highlightSyntax").setAttribute("checked",
        gPrefs.getBoolPref("view_source.syntax_highlight"));
    } catch (ex) {
    }
  } else {
    document.getElementById("menu_highlightSyntax").setAttribute("hidden", "true");
  }

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


function ViewSourceEditPage()
{
  editPage(window.content.location.href.substring(12), window, false);
}


function ViewSourceSavePage()
{
  saveURL(window.content.location.href.substring(12), null, "SaveLinkTitle");
}

function onEnterPP()
{
  var toolbox = document.getElementById("viewSource-toolbox");
  toolbox.hidden = true;
}

function onExitPP()
{
  var toolbox = document.getElementById("viewSource-toolbox");
  toolbox.hidden = false;
}

function getPPBrowser() {
  return gBrowser;
}


function getNavToolbox()
{
  return document.getElementById("appcontent");
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
  var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"]
                        .getService(Ci.nsIPromptService);

  var input = {value:gLastLineFound};
  for (;;) {
    var ok = promptService.prompt(
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
      promptService.alert(window,
                          gViewSourceBundle.getString("invalidInputTitle"),
                          gViewSourceBundle.getString("invalidInputText"));

      continue;
    }

    var found = goToLine(line);

    if (found)
      break;

    promptService.alert(window,
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

    var firstLine = parseInt(pre.id.substring(4));

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

  
  
  
  var curLine = parseInt(pre.id.substring(4));

  
  var treewalker = window.content.document
      .createTreeWalker(pre, NodeFilter.SHOW_TEXT, null, false);

  
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

  if (myWrap.className == '')
    myWrap.className = 'wrap';
  else myWrap.className = '';

  
  
  
  if (gPrefs) {
    try {
      if (myWrap.className == '') {
        gPrefs.setBoolPref("view_source.wrap_long_lines", false);
      }
      else {
        gPrefs.setBoolPref("view_source.wrap_long_lines", true);
      }
    } catch (ex) {
    }
  }
}



function highlightSyntax()
{
  var highlightSyntaxMenu = document.getElementById("menu_highlightSyntax");
  var highlightSyntax = (highlightSyntaxMenu.getAttribute("checked") == "true");
  gPrefs.setBoolPref("view_source.syntax_highlight", highlightSyntax);

  gPageLoader.loadPage(gPageLoader.currentDescriptor, gPageLoader.DISPLAY_NORMAL);
}



function BrowserSetForcedCharacterSet(aCharset)
{
  var docCharset = gBrowser.docShell.QueryInterface(Ci.nsIDocCharset);
  docCharset.charset = aCharset;
  if (isHistoryEnabled()) {
    gPageLoader.loadPage(gPageLoader.currentDescriptor,
                         gPageLoader.DISPLAY_NORMAL);
  } else {
    gBrowser.reloadWithFlags(Ci.nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
  }
}






function BrowserSetForcedDetector(doReload)
{
  gBrowser.documentCharsetInfo.forcedDetector = true; 
  if (doReload)
  {
    if (isHistoryEnabled()) {
      gPageLoader.loadPage(gPageLoader.currentDescriptor,
                           gPageLoader.DISPLAY_NORMAL);
    } else {
      gBrowser.reloadWithFlags(Ci.nsIWebNavigation.LOAD_FLAGS_CHARSET_CHANGE);
    }
  }
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
