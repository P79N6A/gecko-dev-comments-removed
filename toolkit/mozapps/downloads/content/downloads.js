# -*- indent-tabs-mode: nil; js-indent-level: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

"use strict";




const PREF_BDM_CLOSEWHENDONE = "browser.download.manager.closeWhenDone";
const PREF_BDM_ALERTONEXEOPEN = "browser.download.manager.alertOnEXEOpen";
const PREF_BDM_SCANWHENDONE = "browser.download.manager.scanWhenDone";

const nsLocalFile = Components.Constructor("@mozilla.org/file/local;1",
                                           "nsILocalFile", "initWithPath");

var Cc = Components.classes;
var Ci = Components.interfaces;
let Cu = Components.utils;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/DownloadUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
                                  "resource://gre/modules/PluralForm.jsm");

const nsIDM = Ci.nsIDownloadManager;

let gDownloadManager = Cc["@mozilla.org/download-manager;1"].getService(nsIDM);
let gDownloadManagerUI = Cc["@mozilla.org/download-manager-ui;1"].
                         getService(Ci.nsIDownloadManagerUI);

let gDownloadListener = null;
let gDownloadsView = null;
let gSearchBox = null;
let gSearchTerms = [];
let gBuilder = 0;








let gPerformAllCallback;




const gListBuildDelay = 300;
const gListBuildChunk = 3;


const gSearchAttributes = [
  "target",
  "status",
  "dateTime",
];



var gUserInteracted = false;



let gStr = {
  paused: "paused",
  cannotPause: "cannotPause",
  doneStatus: "doneStatus",
  doneSize: "doneSize",
  doneSizeUnknown: "doneSizeUnknown",
  stateFailed: "stateFailed",
  stateCanceled: "stateCanceled",
  stateBlockedParentalControls: "stateBlocked",
  stateBlockedPolicy: "stateBlockedPolicy",
  stateDirty: "stateDirty",
  downloadsTitleFiles: "downloadsTitleFiles",
  downloadsTitlePercent: "downloadsTitlePercent",
  fileExecutableSecurityWarningTitle: "fileExecutableSecurityWarningTitle",
  fileExecutableSecurityWarningDontAsk: "fileExecutableSecurityWarningDontAsk"
};


let gStmt = null;




function downloadCompleted(aDownload)
{
  
  updateClearListButton();

  
  
  
  try {
    let dl = getDownload(aDownload.id);

    
    dl.setAttribute("startTime", Math.round(aDownload.startTime / 1000));
    dl.setAttribute("endTime", Date.now());
    dl.setAttribute("currBytes", aDownload.amountTransferred);
    dl.setAttribute("maxBytes", aDownload.size);

    
    if (downloadMatchesSearch(dl)) {
      
      let next = dl.nextSibling;
      while (next && next.inProgress)
        next = next.nextSibling;

      
      gDownloadsView.insertBefore(dl, next);
    } else {
      removeFromView(dl);
    }

    
    try {
      
      var mimeService = Cc["@mozilla.org/mime;1"].
                        getService(Ci.nsIMIMEService);
      var contentType = mimeService.getTypeFromFile(aDownload.targetFile);

      var listItem = getDownload(aDownload.id)
      var oldImage = listItem.getAttribute("image");
      
      listItem.setAttribute("image", oldImage + "&contentType=" + contentType);
    } catch (e) { }

    if (gDownloadManager.activeDownloadCount == 0)
      document.title = document.documentElement.getAttribute("statictitle");

    gDownloadManagerUI.getAttention();
  }
  catch (e) { }
}

function autoRemoveAndClose(aDownload)
{
  var pref = Cc["@mozilla.org/preferences-service;1"].
             getService(Ci.nsIPrefBranch);

  if (gDownloadManager.activeDownloadCount == 0) {
    
    
    
    
    
    
    let autoClose = false;
    try {
      autoClose = pref.getBoolPref(PREF_BDM_CLOSEWHENDONE);
    } catch (e) { }
    var autoOpened =
      !window.opener || window.opener.location.href == window.location.href;
    if (autoClose && autoOpened && !gUserInteracted) {
      gCloseDownloadManager();
      return true;
    }
  }

  return false;
}



function gCloseDownloadManager()
{
  window.close();
}




function cancelDownload(aDownload)
{
  gDownloadManager.cancelDownload(aDownload.getAttribute("dlid"));

  
  
  
  
  
  
  var f = getLocalFileFromNativePathOrUrl(aDownload.getAttribute("file"));

  if (f.exists())
    f.remove(false);
}

function pauseDownload(aDownload)
{
  var id = aDownload.getAttribute("dlid");
  gDownloadManager.pauseDownload(id);
}

function resumeDownload(aDownload)
{
  gDownloadManager.resumeDownload(aDownload.getAttribute("dlid"));
}

function removeDownload(aDownload)
{
  gDownloadManager.removeDownload(aDownload.getAttribute("dlid"));
}

function retryDownload(aDownload)
{
  removeFromView(aDownload);
  gDownloadManager.retryDownload(aDownload.getAttribute("dlid"));
}

function showDownload(aDownload)
{
  var f = getLocalFileFromNativePathOrUrl(aDownload.getAttribute("file"));

  try {
    
    f.reveal();
  } catch (e) {
    
    
    let parent = f.parent.QueryInterface(Ci.nsILocalFile);
    if (!parent)
      return;

    try {
      
      parent.launch();
    } catch (e) {
      
      
      openExternal(parent);
    }
  }
}

function onDownloadDblClick(aEvent)
{
  
  if (aEvent.button == 0 && aEvent.target.selected)
    doDefaultForSelected();
}

function openDownload(aDownload)
{
  var f = getLocalFileFromNativePathOrUrl(aDownload.getAttribute("file"));
  if (f.isExecutable()) {
    var dontAsk = false;
    var pref = Cc["@mozilla.org/preferences-service;1"].
               getService(Ci.nsIPrefBranch);
    try {
      dontAsk = !pref.getBoolPref(PREF_BDM_ALERTONEXEOPEN);
    } catch (e) { }

#ifdef XP_WIN
    
    
    try {
      var sysInfo = Cc["@mozilla.org/system-info;1"].
                    getService(Ci.nsIPropertyBag2);
      if (parseFloat(sysInfo.getProperty("version")) >= 6 &&
          pref.getBoolPref(PREF_BDM_SCANWHENDONE)) {
        dontAsk = true;
      }
    } catch (ex) { }
#endif

    if (!dontAsk) {
      var strings = document.getElementById("downloadStrings");
      var name = aDownload.getAttribute("target");
      var message = strings.getFormattedString("fileExecutableSecurityWarning", [name, name]);

      let title = gStr.fileExecutableSecurityWarningTitle;
      let dontAsk = gStr.fileExecutableSecurityWarningDontAsk;

      var promptSvc = Cc["@mozilla.org/embedcomp/prompt-service;1"].
                      getService(Ci.nsIPromptService);
      var checkbox = { value: false };
      var open = promptSvc.confirmCheck(window, title, message, dontAsk, checkbox);

      if (!open)
        return;
      pref.setBoolPref(PREF_BDM_ALERTONEXEOPEN, !checkbox.value);
    }
  }
  try {
    try {
      let download = gDownloadManager.getDownload(aDownload.getAttribute("dlid"));
      let mimeInfo = download.MIMEInfo;
      if (mimeInfo.preferredAction == mimeInfo.useHelperApp) {
        mimeInfo.launchWithFile(f);
        return;
      }
    } catch (ex) {
    }
    f.launch();
  } catch (ex) {
    
    
    openExternal(f);
  }
}

function openReferrer(aDownload)
{
  openURL(getReferrerOrSource(aDownload));
}

function copySourceLocation(aDownload)
{
  var uri = aDownload.getAttribute("uri");
  var clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"].
                  getService(Ci.nsIClipboardHelper);

  
  if (gPerformAllCallback === null) {
    let uris = [];
    gPerformAllCallback = function(aURI) aURI ? uris.push(aURI) :
      clipboard.copyString(uris.join("\n"));
  }

  
  if (typeof gPerformAllCallback == "function")
    gPerformAllCallback(uri);
  else {
    
    clipboard.copyString(uri);
  }
}




function clearDownloadList() {
  
  if (gSearchTerms == "") {
    gDownloadManager.cleanUp();
    return;
  }

  
  
  let item;
  while ((item = gDownloadsView.lastChild) && !item.inProgress)
    removeDownload(item);

  
  gSearchBox.value = "";
  gSearchBox.doCommand();
  gDownloadsView.focus();
}


var gLastComputedMean = -1;
var gLastActiveDownloads = 0;
function onUpdateProgress()
{
  let numActiveDownloads = gDownloadManager.activeDownloadCount;

  
  if (numActiveDownloads == 0) {
    document.title = document.documentElement.getAttribute("statictitle");
    gLastComputedMean = -1;
    gLastActiveDownloads = 0;

    return;
  }

  
  var mean = 0;
  var base = 0;
  var dls = gDownloadManager.activeDownloads;
  while (dls.hasMoreElements()) {
    let dl = dls.getNext();
    if (dl.percentComplete < 100 && dl.size > 0) {
      mean += dl.amountTransferred;
      base += dl.size;
    }
  }

  
  let title = gStr.downloadsTitlePercent;
  if (base == 0)
    title = gStr.downloadsTitleFiles;
  else
    mean = Math.floor((mean / base) * 100);

  
  if (mean != gLastComputedMean || gLastActiveDownloads != numActiveDownloads) {
    gLastComputedMean = mean;
    gLastActiveDownloads = numActiveDownloads;

    
    title = PluralForm.get(numActiveDownloads, title);
    title = replaceInsert(title, 1, numActiveDownloads);
    title = replaceInsert(title, 2, mean);

    document.title = title;
  }
}




function Startup()
{
  gDownloadsView = document.getElementById("downloadView");
  gSearchBox = document.getElementById("searchbox");

  
  let sb = document.getElementById("downloadStrings");
  let getStr = function(string) sb.getString(string);
  for (let [name, value] in Iterator(gStr))
    gStr[name] = typeof value == "string" ? getStr(value) : value.map(getStr);

  initStatement();
  buildDownloadList(true);

  
  
  gDownloadListener = new DownloadProgressListener();
  gDownloadManager.addListener(gDownloadListener);

  
  
  if (window.arguments[1] == Ci.nsIDownloadManagerUI.REASON_USER_INTERACTED)
    gUserInteracted = true;

  
  
  if (!autoRemoveAndClose())
    gDownloadsView.focus();

  let obs = Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService);
  obs.addObserver(gDownloadObserver, "download-manager-remove-download", false);
  obs.addObserver(gDownloadObserver, "browser-lastwindow-close-granted", false);

  
  gSearchBox.addEventListener("keypress", function(e) {
    if (e.keyCode == e.DOM_VK_ESCAPE) {
      
      gDownloadsView.focus();
      e.preventDefault();
    }
  }, false);

  let DownloadTaskbarProgress =
    Cu.import("resource://gre/modules/DownloadTaskbarProgress.jsm", {}).DownloadTaskbarProgress;
  DownloadTaskbarProgress.onDownloadWindowLoad(window);
}

function Shutdown()
{
  gDownloadManager.removeListener(gDownloadListener);

  let obs = Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService);
  obs.removeObserver(gDownloadObserver, "download-manager-remove-download");
  obs.removeObserver(gDownloadObserver, "browser-lastwindow-close-granted");

  clearTimeout(gBuilder);
  gStmt.reset();
  gStmt.finalize();
}

let gDownloadObserver = {
  observe: function gdo_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "download-manager-remove-download":
        
        if (!aSubject) {
          
          buildDownloadList(true);
          break;
        }

        
        let id = aSubject.QueryInterface(Ci.nsISupportsPRUint32);
        let dl = getDownload(id.data);
        removeFromView(dl);
        break;
      case "browser-lastwindow-close-granted":
#ifndef XP_MACOSX
        if (gDownloadManager.activeDownloadCount == 0) {
          setTimeout(gCloseDownloadManager, 0);
        }
#endif
        break;
    }
  }
};




var gContextMenus = [
  
  [
    "menuitem_pause"
    , "menuitem_cancel"
    , "menuseparator"
    , "menuitem_show"
    , "menuseparator"
    , "menuitem_openReferrer"
    , "menuitem_copyLocation"
    , "menuseparator"
    , "menuitem_selectAll"
  ],
  
  [
    "menuitem_open"
    , "menuitem_show"
    , "menuseparator"
    , "menuitem_openReferrer"
    , "menuitem_copyLocation"
    , "menuseparator"
    , "menuitem_selectAll"
    , "menuseparator"
    , "menuitem_removeFromList"
  ],
  
  [
    "menuitem_retry"
    , "menuseparator"
    , "menuitem_openReferrer"
    , "menuitem_copyLocation"
    , "menuseparator"
    , "menuitem_selectAll"
    , "menuseparator"
    , "menuitem_removeFromList"
  ],
  
  [
    "menuitem_retry"
    , "menuseparator"
    , "menuitem_openReferrer"
    , "menuitem_copyLocation"
    , "menuseparator"
    , "menuitem_selectAll"
    , "menuseparator"
    , "menuitem_removeFromList"
  ],
  
  [
    "menuitem_resume"
    , "menuitem_cancel"
    , "menuseparator"
    , "menuitem_show"
    , "menuseparator"
    , "menuitem_openReferrer"
    , "menuitem_copyLocation"
    , "menuseparator"
    , "menuitem_selectAll"
  ],
  
  [
    "menuitem_cancel"
    , "menuseparator"
    , "menuitem_show"
    , "menuseparator"
    , "menuitem_openReferrer"
    , "menuitem_copyLocation"
    , "menuseparator"
    , "menuitem_selectAll"
  ],
  
  [
    "menuitem_openReferrer"
    , "menuitem_copyLocation"
    , "menuseparator"
    , "menuitem_selectAll"
    , "menuseparator"
    , "menuitem_removeFromList"
  ],
  
  [
    "menuitem_show"
    , "menuseparator"
    , "menuitem_openReferrer"
    , "menuitem_copyLocation"
    , "menuseparator"
    , "menuitem_selectAll"
  ],
  
  [
    "menuitem_openReferrer"
    , "menuitem_copyLocation"
    , "menuseparator"
    , "menuitem_selectAll"
    , "menuseparator"
    , "menuitem_removeFromList"
  ],
  
  [
    "menuitem_openReferrer"
    , "menuitem_copyLocation"
    , "menuseparator"
    , "menuitem_selectAll"
    , "menuseparator"
    , "menuitem_removeFromList"
  ]
];

function buildContextMenu(aEvent)
{
  if (aEvent.target.id != "downloadContextMenu")
    return false;

  var popup = document.getElementById("downloadContextMenu");
  while (popup.hasChildNodes())
    popup.removeChild(popup.firstChild);

  if (gDownloadsView.selectedItem) {
    let dl = gDownloadsView.selectedItem;
    let idx = parseInt(dl.getAttribute("state"));
    if (idx < 0)
      idx = 0;

    var menus = gContextMenus[idx];
    for (let i = 0; i < menus.length; ++i) {
      let menuitem = document.getElementById(menus[i]).cloneNode(true);
      let cmd = menuitem.getAttribute("cmd");
      if (cmd)
        menuitem.disabled = !gDownloadViewController.isCommandEnabled(cmd, dl);

      popup.appendChild(menuitem);
    }

    return true;
  }

  return false;
}


var gDownloadDNDObserver =
{
  onDragStart: function (aEvent)
  {
    if (!gDownloadsView.selectedItem)
      return;
    var dl = gDownloadsView.selectedItem;
    var f = getLocalFileFromNativePathOrUrl(dl.getAttribute("file"));
    if (!f.exists())
      return;

    var dt = aEvent.dataTransfer;
    dt.mozSetDataAt("application/x-moz-file", f, 0);
    var url = Services.io.newFileURI(f).spec;
    dt.setData("text/uri-list", url);
    dt.setData("text/plain", url);
    dt.effectAllowed = "copyMove";
    dt.addElement(dl);
  },

  onDragOver: function (aEvent)
  {
    var types = aEvent.dataTransfer.types;
    if (types.contains("text/uri-list") ||
        types.contains("text/x-moz-url") ||
        types.contains("text/plain"))
      aEvent.preventDefault();
  },

  onDrop: function(aEvent)
  {
    var dt = aEvent.dataTransfer;
    
    
    if (dt.mozGetDataAt("application/x-moz-file", 0))
      return;

    var url = dt.getData("URL");
    var name;
    if (!url) {
      url = dt.getData("text/x-moz-url") || dt.getData("text/plain");
      [url, name] = url.split("\n");
    }
    if (url) {
      let sourceDoc = dt.mozSourceNode ? dt.mozSourceNode.ownerDocument : document;
      saveURL(url, name ? name : url, null, true, true, null, sourceDoc);
    }
  }
}

function pasteHandler() {
  let trans = Cc["@mozilla.org/widget/transferable;1"].
              createInstance(Ci.nsITransferable);
  trans.init(null);
  let flavors = ["text/x-moz-url", "text/unicode"];
  flavors.forEach(trans.addDataFlavor);

  Services.clipboard.getData(trans, Services.clipboard.kGlobalClipboard);

  
  try {
    let data = {};
    trans.getAnyTransferData({}, data, {});
    let [url, name] = data.value.QueryInterface(Ci.nsISupportsString).data.split("\n");

    if (!url)
      return;

    let uri = Services.io.newURI(url, null, null);

    saveURL(uri.spec, name || uri.spec, null, true, true, null, document);
  } catch (ex) {}
}




var gDownloadViewController = {
  isCommandEnabled: function(aCommand, aItem)
  {
    let dl = aItem;
    let download = null; 

    switch (aCommand) {
      case "cmd_cancel":
        return dl.inProgress;
      case "cmd_open": {
        let file = getLocalFileFromNativePathOrUrl(dl.getAttribute("file"));
        return dl.openable && file.exists();
      }
      case "cmd_show": {
        let file = getLocalFileFromNativePathOrUrl(dl.getAttribute("file"));
        return file.exists();
      }
      case "cmd_pause":
        download = gDownloadManager.getDownload(dl.getAttribute("dlid"));
        return dl.inProgress && !dl.paused && download.resumable;
      case "cmd_pauseResume":
        download = gDownloadManager.getDownload(dl.getAttribute("dlid"));
        return (dl.inProgress || dl.paused) && download.resumable;
      case "cmd_resume":
        download = gDownloadManager.getDownload(dl.getAttribute("dlid"));
        return dl.paused && download.resumable;
      case "cmd_openReferrer":
        return dl.hasAttribute("referrer");
      case "cmd_removeFromList":
      case "cmd_retry":
        return dl.removable;
      case "cmd_copyLocation":
        return true;
    }
    return false;
  },

  doCommand: function(aCommand, aItem)
  {
    if (this.isCommandEnabled(aCommand, aItem))
      this.commands[aCommand](aItem);
  },

  commands: {
    cmd_cancel: function(aSelectedItem) {
      cancelDownload(aSelectedItem);
    },
    cmd_open: function(aSelectedItem) {
      openDownload(aSelectedItem);
    },
    cmd_openReferrer: function(aSelectedItem) {
      openReferrer(aSelectedItem);
    },
    cmd_pause: function(aSelectedItem) {
      pauseDownload(aSelectedItem);
    },
    cmd_pauseResume: function(aSelectedItem) {
      if (aSelectedItem.paused)
        this.cmd_resume(aSelectedItem);
      else
        this.cmd_pause(aSelectedItem);
    },
    cmd_removeFromList: function(aSelectedItem) {
      removeDownload(aSelectedItem);
    },
    cmd_resume: function(aSelectedItem) {
      resumeDownload(aSelectedItem);
    },
    cmd_retry: function(aSelectedItem) {
      retryDownload(aSelectedItem);
    },
    cmd_show: function(aSelectedItem) {
      showDownload(aSelectedItem);
    },
    cmd_copyLocation: function(aSelectedItem) {
      copySourceLocation(aSelectedItem);
    },
  }
};













function performCommand(aCmd, aItem)
{
  let elm = aItem;
  if (!elm) {
    
    
    
    
    gPerformAllCallback = null;

    
    let items = [];
    for (let i = gDownloadsView.selectedItems.length; --i >= 0; )
      items.unshift(gDownloadsView.selectedItems[i]);

    
    for (let item of items)
      performCommand(aCmd, item);

    
    if (typeof gPerformAllCallback == "function")
      gPerformAllCallback();
    gPerformAllCallback = undefined;

    return;
  } else {
    while (elm.nodeName != "richlistitem" ||
           elm.getAttribute("type") != "download")
      elm = elm.parentNode;
  }

  gDownloadViewController.doCommand(aCmd, elm);
}

function setSearchboxFocus()
{
  gSearchBox.focus();
  gSearchBox.select();
}

function openExternal(aFile)
{
  var uri = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService).newFileURI(aFile);

  var protocolSvc = Cc["@mozilla.org/uriloader/external-protocol-service;1"].
                    getService(Ci.nsIExternalProtocolService);
  protocolSvc.loadUrl(uri);

  return;
}














function createDownloadItem(aAttrs)
{
  let dl = document.createElement("richlistitem");

  
  for (let attr in aAttrs)
    dl.setAttribute(attr, aAttrs[attr]);

  
  dl.setAttribute("type", "download");
  dl.setAttribute("id", "dl" + aAttrs.dlid);
  dl.setAttribute("image", "moz-icon://" + aAttrs.file + "?size=32");
  dl.setAttribute("lastSeconds", Infinity);

  
  updateTime(dl);
  updateStatus(dl);

  try {
    let file = getLocalFileFromNativePathOrUrl(aAttrs.file);
    dl.setAttribute("path", file.nativePath || file.path);
    return dl;
  } catch (e) {
    
    
  }
  return null;
}







function updateButtons(aItem)
{
  let buttons = aItem.buttons;

  for (let i = 0; i < buttons.length; ++i) {
    let cmd = buttons[i].getAttribute("cmd");
    let enabled = gDownloadViewController.isCommandEnabled(cmd, aItem);
    buttons[i].disabled = !enabled;

    if ("cmd_pause" == cmd && !enabled) {
      
      
      buttons[i].setAttribute("tooltiptext", gStr.cannotPause);
    }
  }
}










function updateStatus(aItem, aDownload) {
  let status = "";
  let statusTip = "";

  let state = Number(aItem.getAttribute("state"));
  switch (state) {
    case nsIDM.DOWNLOAD_PAUSED:
    {
      let currBytes = Number(aItem.getAttribute("currBytes"));
      let maxBytes = Number(aItem.getAttribute("maxBytes"));

      let transfer = DownloadUtils.getTransferTotal(currBytes, maxBytes);
      status = replaceInsert(gStr.paused, 1, transfer);

      break;
    }
    case nsIDM.DOWNLOAD_DOWNLOADING:
    {
      let currBytes = Number(aItem.getAttribute("currBytes"));
      let maxBytes = Number(aItem.getAttribute("maxBytes"));
      
      let speed = aDownload ? aDownload.speed : 0;
      let lastSec = Number(aItem.getAttribute("lastSeconds"));

      let newLast;
      [status, newLast] =
        DownloadUtils.getDownloadStatus(currBytes, maxBytes, speed, lastSec);

      
      aItem.setAttribute("lastSeconds", newLast);

      break;
    }
    case nsIDM.DOWNLOAD_FINISHED:
    case nsIDM.DOWNLOAD_FAILED:
    case nsIDM.DOWNLOAD_CANCELED:
    case nsIDM.DOWNLOAD_BLOCKED_PARENTAL:
    case nsIDM.DOWNLOAD_BLOCKED_POLICY:
    case nsIDM.DOWNLOAD_DIRTY:
    {
      let stateSize = {};
      stateSize[nsIDM.DOWNLOAD_FINISHED] = function() {
        
        let fileSize = Number(aItem.getAttribute("maxBytes"));
        let sizeText = gStr.doneSizeUnknown;
        if (fileSize >= 0) {
          let [size, unit] = DownloadUtils.convertByteUnits(fileSize);
          sizeText = replaceInsert(gStr.doneSize, 1, size);
          sizeText = replaceInsert(sizeText, 2, unit);
        }
        return sizeText;
      };
      stateSize[nsIDM.DOWNLOAD_FAILED] = function() gStr.stateFailed;
      stateSize[nsIDM.DOWNLOAD_CANCELED] = function() gStr.stateCanceled;
      stateSize[nsIDM.DOWNLOAD_BLOCKED_PARENTAL] = function() gStr.stateBlockedParentalControls;
      stateSize[nsIDM.DOWNLOAD_BLOCKED_POLICY] = function() gStr.stateBlockedPolicy;
      stateSize[nsIDM.DOWNLOAD_DIRTY] = function() gStr.stateDirty;

      
      status = replaceInsert(gStr.doneStatus, 1, stateSize[state]());

      let [displayHost, fullHost] =
        DownloadUtils.getURIHost(getReferrerOrSource(aItem));

      
      status = replaceInsert(status, 2, displayHost);
      
      statusTip = fullHost;

      break;
    }
  }

  aItem.setAttribute("status", status);
  aItem.setAttribute("statusTip", statusTip != "" ? statusTip : status);
}







function updateTime(aItem)
{
  
  if (aItem.inProgress)
    return;

  let end = new Date(parseInt(aItem.getAttribute("endTime")));
  let [dateCompact, dateComplete] = DownloadUtils.getReadableDates(end);
  aItem.setAttribute("dateTime", dateCompact);
  aItem.setAttribute("dateTimeTip", dateComplete);
}












function replaceInsert(aText, aIndex, aValue)
{
  return aText.replace("#" + aIndex, aValue);
}




function doDefaultForSelected()
{
  
  let item = gDownloadsView.selectedItem;
  if (!item)
    return;

  
  let state = Number(item.getAttribute("state"));
  let menuitem = document.getElementById(gContextMenus[state][0]);

  
  gDownloadViewController.doCommand(menuitem.getAttribute("cmd"), item);
}

function removeFromView(aDownload)
{
  
  if (!aDownload) return;

  let index = gDownloadsView.selectedIndex;
  gDownloadsView.removeChild(aDownload);
  gDownloadsView.selectedIndex = Math.min(index, gDownloadsView.itemCount - 1);

  
  updateClearListButton();
}

function getReferrerOrSource(aDownload)
{
  
  if (aDownload.hasAttribute("referrer"))
    return aDownload.getAttribute("referrer");

  
  return aDownload.getAttribute("uri");
}








function buildDownloadList(aForceBuild)
{
  
  let prevSearch = gSearchTerms.join(" ");

  
  gSearchTerms = gSearchBox.value.replace(/^\s+|\s+$/g, "").
                 toLowerCase().split(/\s+/);

  
  if (!aForceBuild && gSearchTerms.join(" ") == prevSearch)
    return;

  
  clearTimeout(gBuilder);
  gStmt.reset();

  
  let empty = gDownloadsView.cloneNode(false);
  gDownloadsView.parentNode.replaceChild(empty, gDownloadsView);
  gDownloadsView = empty;

  try {
    gStmt.bindByIndex(0, nsIDM.DOWNLOAD_NOTSTARTED);
    gStmt.bindByIndex(1, nsIDM.DOWNLOAD_DOWNLOADING);
    gStmt.bindByIndex(2, nsIDM.DOWNLOAD_PAUSED);
    gStmt.bindByIndex(3, nsIDM.DOWNLOAD_QUEUED);
    gStmt.bindByIndex(4, nsIDM.DOWNLOAD_SCANNING);
  } catch (e) {
    
    gStmt.reset();
    return;
  }

  
  gBuilder = setTimeout(function() {
    
    stepListBuilder(1);

    
    updateClearListButton();
  }, 0);
}









function stepListBuilder(aNumItems) {
  try {
    
    if (!gStmt.executeStep()) {
      
      updateClearListButton();
      setTimeout(function() Cc["@mozilla.org/observer-service;1"].
        getService(Ci.nsIObserverService).
        notifyObservers(window, "download-manager-ui-done", null), 0);

      return;
    }

    
    let attrs = {
      dlid: gStmt.getInt64(0),
      file: gStmt.getString(1),
      target: gStmt.getString(2),
      uri: gStmt.getString(3),
      state: gStmt.getInt32(4),
      startTime: Math.round(gStmt.getInt64(5) / 1000),
      endTime: Math.round(gStmt.getInt64(6) / 1000),
      currBytes: gStmt.getInt64(8),
      maxBytes: gStmt.getInt64(9)
    };

    
    let referrer = gStmt.getString(7);
    if (referrer)
      attrs.referrer = referrer;

    
    let isActive = gStmt.getInt32(10);
    attrs.progress = isActive ? gDownloadManager.getDownload(attrs.dlid).
      percentComplete : 100;

    
    let item = createDownloadItem(attrs);
    if (item && (isActive || downloadMatchesSearch(item))) {
      
      gDownloadsView.appendChild(item);

      
      
      updateButtons(item);
    } else {
      
      
      aNumItems += .9;
    }
  } catch (e) {
    
    gStmt.reset();
    return;
  }

  
  
  if (aNumItems > 1) {
    stepListBuilder(aNumItems - 1);
  } else {
    
    let delay = Math.min(gDownloadsView.itemCount * 10, gListBuildDelay);
    gBuilder = setTimeout(stepListBuilder, delay, gListBuildChunk);
  }
}







function prependList(aDownload)
{
  let attrs = {
    dlid: aDownload.id,
    file: aDownload.target.spec,
    target: aDownload.displayName,
    uri: aDownload.source.spec,
    state: aDownload.state,
    progress: aDownload.percentComplete,
    startTime: Math.round(aDownload.startTime / 1000),
    endTime: Date.now(),
    currBytes: aDownload.amountTransferred,
    maxBytes: aDownload.size
  };

  
  let item = createDownloadItem(attrs);
  if (item) {
    
    gDownloadsView.insertBefore(item, gDownloadsView.firstChild);
    
    
    
    updateButtons(item);

    
    updateClearListButton();
  }
}










function downloadMatchesSearch(aItem)
{
  
  
  let combinedSearch = "";
  for (let attr of gSearchAttributes)
    combinedSearch += aItem.getAttribute(attr).toLowerCase() + " ";

  
  for (let term of gSearchTerms)
    if (combinedSearch.indexOf(term) == -1)
      return false;

  return true;
}







function getLocalFileFromNativePathOrUrl(aPathOrUrl)
{
  if (aPathOrUrl.substring(0,7) == "file://") {
    
    let ioSvc = Cc["@mozilla.org/network/io-service;1"].
                getService(Ci.nsIIOService);

    
    const fileUrl = ioSvc.newURI(aPathOrUrl, null, null).
                    QueryInterface(Ci.nsIFileURL);
    return fileUrl.file.clone().QueryInterface(Ci.nsILocalFile);
  } else {
    
    var f = new nsLocalFile(aPathOrUrl);

    return f;
  }
}





function updateClearListButton()
{
  let button = document.getElementById("clearListButton");
  
  button.disabled = !(gDownloadsView.itemCount && gDownloadManager.canCleanUp);
}

function getDownload(aID)
{
  return document.getElementById("dl" + aID);
}




function initStatement()
{
  if (gStmt)
    gStmt.finalize();

  gStmt = gDownloadManager.DBConnection.createStatement(
    "SELECT id, target, name, source, state, startTime, endTime, referrer, " +
           "currBytes, maxBytes, state IN (?1, ?2, ?3, ?4, ?5) isActive " +
    "FROM moz_downloads " +
    "ORDER BY isActive DESC, endTime DESC, startTime DESC");
}
