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
# The Original Code is Mozilla.org Code.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 2001
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Blake Ross <blakeross@telocity.com> (Original Author)
#   Ben Goodger <ben@bengoodger.com> (v2.0)
#   Dan Mosedale <dmose@mozilla.org>
#   Fredrik Holmqvist <thesuckiestemail@yahoo.se>
#   Josh Aas <josh@mozilla.com>
#   Shawn Wilsher <me@shawnwilsher.com> (v3.0)
#   Edward Lee <edward.lee@engineering.uiuc.edu>
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




const PREF_BDM_CLOSEWHENDONE = "browser.download.manager.closeWhenDone";
const PREF_BDM_ALERTONEXEOPEN = "browser.download.manager.alertOnEXEOpen";
const PREF_BDM_DISPLAYEDHISTORYDAYS =
  "browser.download.manager.displayedHistoryDays";

const nsLocalFile = Components.Constructor("@mozilla.org/file/local;1",
                                           "nsILocalFile", "initWithPath");

var Cc = Components.classes;
var Ci = Components.interfaces;

const nsIDM = Ci.nsIDownloadManager;

let gDownloadManager = Cc["@mozilla.org/download-manager;1"].getService(nsIDM);
var gDownloadListener     = null;
var gDownloadsView        = null;
var gDownloadsActiveArea  = null;
var gDownloadsDoneArea    = null;
var gDownloadInfoPopup    = null;
var gUserInterfered       = false;
var gSearching            = false;



var gUserInteracted = false;



let gStr = {
  paused: "paused",
  statusFormat: "statusFormat2",
  transferSameUnits: "transferSameUnits",
  transferDiffUnits: "transferDiffUnits",
  transferNoTotal: "transferNoTotal",
  timeMinutesLeft: "timeMinutesLeft",
  timeSecondsLeft: "timeSecondsLeft",
  timeFewSeconds: "timeFewSeconds",
  timeUnknown: "timeUnknown",
  doneStatus: "doneStatus",
  doneSize: "doneSize",
  doneSizeUnknown: "doneSizeUnknown",
  stateFailed: "stateFailed",
  stateCanceled: "stateCanceled",
  stateBlocked: "stateBlocked",

  units: ["bytes", "kilobyte", "megabyte", "gigabyte"],

  fileExecutableSecurityWarningTitle: "fileExecutableSecurityWarningTitle",
  fileExecutableSecurityWarningDontAsk: "fileExecutableSecurityWarningDontAsk"
};


let gBaseQuery = "SELECT id, target, name, source, state, startTime, " +
                        "endTime, referrer, currBytes, maxBytes " +
                 "FROM moz_downloads " +
                 "WHERE #1 " +
                 "ORDER BY endTime ASC, startTime ASC";




function fireEventForElement(aElement, aEventType)
{
  var e = document.createEvent("Events");
  e.initEvent("download-" + aEventType, true, true);

  aElement.dispatchEvent(e);
}

function createDownloadItem(aID, aFile, aTarget, aURI, aState, aProgress,
                            aStartTime, aEndTime, aReferrer, aCurrBytes,
                            aMaxBytes)
{
  var dl = document.createElement("richlistitem");
  dl.setAttribute("type", "download");
  dl.setAttribute("id", "dl" + aID);
  dl.setAttribute("dlid", aID);
  dl.setAttribute("image", "moz-icon://" + aFile + "?size=32");
  dl.setAttribute("file", aFile);
  dl.setAttribute("target", aTarget);
  dl.setAttribute("uri", aURI);
  dl.setAttribute("state", aState);
  dl.setAttribute("progress", aProgress);
  dl.setAttribute("startTime", aStartTime);
  dl.setAttribute("endTime", aEndTime);
  if (aReferrer)
    dl.setAttribute("referrer", aReferrer);
  dl.setAttribute("currBytes", aCurrBytes);
  dl.setAttribute("maxBytes", aMaxBytes);
  dl.setAttribute("lastSeconds", Infinity);

  updateStatus(dl);

  try {
    var file = getLocalFileFromNativePathOrUrl(aFile);
    dl.setAttribute("path", file.nativePath || file.path);
    return dl;
  }
  catch (ex) {
    
    
  }
  return null;
}

function getDownload(aID)
{
  return document.getElementById("dl" + aID);
}




function downloadCompleted(aDownload)
{
  
  
  
  try {
    let dl = getDownload(aDownload.id);

    
    dl.setAttribute("startTime", Math.round(aDownload.startTime / 1000));
    dl.setAttribute("endTime", Date.now());
    dl.setAttribute("currBytes", aDownload.amountTransferred);
    dl.setAttribute("maxBytes", aDownload.size);

    
    
    if (!gSearching) {
      gDownloadsView.insertBefore(dl, gDownloadsDoneArea.nextSibling);
      evenOddCellAttribution();
    } else
      removeFromView(dl);

    
    try {
      
      const kExternalHelperAppServContractID =
        "@mozilla.org/uriloader/external-helper-app-service;1";
      var mimeService = Cc[kExternalHelperAppServContractID].
                        getService(Ci.nsIMIMEService);
      var contentType = mimeService.getTypeFromFile(aDownload.targetFile);

      var listItem = getDownload(aDownload.id)
      var oldImage = listItem.getAttribute("image");
      
      listItem.setAttribute("image", oldImage + "&contentType=" + contentType);
    } catch (e) { }

    if (gDownloadManager.activeDownloadCount == 0)
      document.title = document.documentElement.getAttribute("statictitle");
  }
  catch (e) { }
}

function autoRemoveAndClose(aDownload)
{
  var pref = Cc["@mozilla.org/preferences-service;1"].
             getService(Ci.nsIPrefBranch);

  if (gDownloadManager.activeDownloadCount == 0) {
    
    
    
    
    var autoClose = pref.getBoolPref(PREF_BDM_CLOSEWHENDONE);
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
  } catch (ex) {
    
    
    
    var parent = f.parent;
    if (parent)
      openExternal(parent);
  }
}

function onDownloadDblClick(aEvent)
{
  var item = aEvent.target;
  if (item.getAttribute("type") == "download" && aEvent.button == 0) {
    var state = parseInt(item.getAttribute("state"));
    switch (state) {
      case nsIDM.DOWNLOAD_FINISHED:
        gDownloadViewController.doCommand("cmd_open");
        break;
      case nsIDM.DOWNLOAD_DOWNLOADING:
        gDownloadViewController.doCommand("cmd_pause");
        break;
      case nsIDM.DOWNLOAD_PAUSED:
        gDownloadViewController.doCommand("cmd_resume");
        break;
      case nsIDM.DOWNLOAD_CANCELED:
      case nsIDM.DOWNLOAD_BLOCKED:
      case nsIDM.DOWNLOAD_FAILED:
        gDownloadViewController.doCommand("cmd_retry");
        break;
    }
  }
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
    f.launch();
  } catch (ex) {
    
    
    openExternal(f);
  }
}

function openReferrer(aDownload)
{
  openURL(getReferrerOrSource(aDownload));
}

function showDownloadInfo(aDownload)
{
  gUserInteracted = true;

  var popupTitle    = document.getElementById("information-title");
  var uriLabel      = document.getElementById("information-uri");
  var locationLabel = document.getElementById("information-location");

  
  var dts = Cc["@mozilla.org/intl/scriptabledateformat;1"].
            getService(Ci.nsIScriptableDateFormat);
  var dateStarted = new Date(parseInt(aDownload.getAttribute("startTime")));
  dateStarted = dts.FormatDateTime("",
                                   dts.dateFormatLong,
                                   dts.timeFormatNoSeconds,
                                   dateStarted.getFullYear(),
                                   dateStarted.getMonth() + 1,
                                   dateStarted.getDate(),
                                   dateStarted.getHours(),
                                   dateStarted.getMinutes(), 0);
  popupTitle.setAttribute("value", dateStarted);
  
  let uri = getReferrerOrSource(aDownload);
  uriLabel.label = uri;
  uriLabel.setAttribute("tooltiptext", uri);
  var path = aDownload.getAttribute("path");
  locationLabel.label = path;
  locationLabel.setAttribute("tooltiptext", path);

  var button = document.getAnonymousElementByAttribute(aDownload, "anonid", "info");
  gDownloadInfoPopup.openPopup(button, "after_end", 0, 0, false, false);
}

function copySourceLocation(aDownload)
{
  var uri = aDownload.getAttribute("uri");
  var clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"].
                  getService(Ci.nsIClipboardHelper);

  clipboard.copyString(uri);
}



var gLastComputedMean = -1;
var gLastActiveDownloads = 0;
function onUpdateProgress()
{
  if (gDownloadManager.activeDownloads == 0) {
    document.title = document.documentElement.getAttribute("statictitle");
    gLastComputedMean = -1;
    return;
  }

  
  var mean = 0;
  var base = 0;
  var numActiveDownloads = 0;
  var dls = gDownloadManager.activeDownloads;
  while (dls.hasMoreElements()) {
    let dl = dls.getNext();
    dl.QueryInterface(Ci.nsIDownload);
    if (dl.percentComplete < 100 && dl.size > 0) {
      mean += dl.amountTransferred;
      base += dl.size;
    }
    numActiveDownloads++;
  }

  
  
  if (base == 0) {
    mean = 100;
  } else {
    mean = Math.floor((mean / base) * 100);
  }

  
  if (mean != gLastComputedMean || gLastActiveDownloads != numActiveDownloads) {
    gLastComputedMean = mean;
    gLastActiveDownloads = numActiveDownloads;

    let strings = document.getElementById("downloadStrings");
    if (numActiveDownloads > 1) {
      document.title = strings.getFormattedString("downloadsTitleMultiple",
                                                  [mean, numActiveDownloads]);
    } else {
      document.title = strings.getFormattedString("downloadsTitle", [mean]);
    }
  }
}




function Startup()
{
  gDownloadsView        = document.getElementById("downloadView");
  gDownloadsActiveArea  = document.getElementById("active-downloads-area");
  gDownloadsDoneArea    = document.getElementById("done-downloads-area");
  gDownloadInfoPopup    = document.getElementById("information");

  
  let (sb = document.getElementById("downloadStrings")) {
    let getStr = function(string) sb.getString(string);
    for (let [name, value] in Iterator(gStr))
      gStr[name] = typeof value == "string" ? getStr(value) : value.map(getStr);
  }

  buildDefaultView();

  
  gDownloadsView.addEventListener("dblclick", onDownloadDblClick, false);

  
  
  gDownloadListener = new DownloadProgressListener();
  gDownloadManager.addListener(gDownloadListener);

  
  
  if (!autoRemoveAndClose())
    gDownloadsView.focus();

  let obs = Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService);
  obs.addObserver(gDownloadObserver, "download-manager-remove-download", false);
}

function Shutdown()
{
  gDownloadManager.removeListener(gDownloadListener);

  let obs = Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService);
  obs.removeObserver(gDownloadObserver, "download-manager-remove-download");
}

let gDownloadObserver = {
  observe: function gdo_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "download-manager-remove-download":
        
        if (!aSubject) {
          
          buildDefaultView();
          break;
        }

        
        let id = aSubject.QueryInterface(Ci.nsISupportsPRUint32);
        let dl = getDownload(id.data);
        removeFromView(dl);
        break;
    }
  }
};




var gContextMenus = [
  
  ["menuitem_pause", "menuitem_cancel", "menuseparator_copy_location",
   "menuitem_copyLocation"],
  
  ["menuitem_open", "menuitem_show", "menuitem_removeFromList", "menuitem_clearList",
   "menuseparator_copy_location", "menuitem_copyLocation"],
  
  ["menuitem_retry", "menuitem_removeFromList", "menuitem_clearList",
   "menuseparator_copy_location", "menuitem_copyLocation"],
  
  ["menuitem_retry", "menuitem_removeFromList", "menuitem_clearList",
   "menuseparator_copy_location", "menuitem_copyLocation"],
  
  ["menuitem_resume", "menuitem_cancel", "menuseparator_copy_location",
   "menuitem_copyLocation"],
  
  ["menuitem_cancel", "menuseparator_copy_location",
   "menuitem_copyLocation"],
  
  ["menuitem_retry", "menuitem_removeFromList", "menuitem_clearList",
   "menuseparator_copy_location", "menuitem_copyLocation"],
  
  ["menuitem_copyLocation"]
];

function buildContextMenu(aEvent)
{
  if (aEvent.target.id != "downloadContextMenu")
    return false;

  var popup = document.getElementById("downloadContextMenu");
  while (popup.hasChildNodes())
    popup.removeChild(popup.firstChild);

  if (gDownloadsView.selectedItem) {
    var idx = parseInt(gDownloadsView.selectedItem.getAttribute("state"));
    if (idx < 0)
      idx = 0;

    var menus = gContextMenus[idx];
    for (var i = 0; i < menus.length; ++i)
      popup.appendChild(document.getElementById(menus[i]).cloneNode(true));

    return true;
  }

  return false;
}




var gDownloadDNDObserver =
{
  onDragOver: function (aEvent, aFlavour, aDragSession)
  {
    aDragSession.canDrop = true;
  },

  onDrop: function(aEvent, aXferData, aDragSession)
  {
    var split = aXferData.data.split("\n");
    var url = split[0];
    if (url != aXferData.data) {  
      var name = split[1];
      saveURL(url, name, null, true, true);
    }
  },
  _flavourSet: null,
  getSupportedFlavours: function ()
  {
    if (!this._flavourSet) {
      this._flavourSet = new FlavourSet();
      this._flavourSet.appendFlavour("text/x-moz-url");
      this._flavourSet.appendFlavour("text/unicode");
    }
    return this._flavourSet;
  }
}




var gDownloadViewController = {
  supportsCommand: function(aCommand)
  {
    var commandNode = document.getElementById(aCommand);
    return commandNode && commandNode.parentNode ==
                            document.getElementById("downloadsCommands");
  },

  isCommandEnabled: function(aCommand)
  {
    if (!window.gDownloadsView)
      return false;

    
    switch (aCommand) {
      case "cmd_clearList":
        return gDownloadManager.canCleanUp;
    }

    var dl = gDownloadsView.selectedItem;
    if (!dl)
      return false;

    switch (aCommand) {
      case "cmd_cancel":
        return dl.inProgress;
      case "cmd_open":
      case "cmd_show":
        let file = getLocalFileFromNativePathOrUrl(dl.getAttribute("file"));
        return dl.openable && file.exists();
      case "cmd_pause":
        return dl.inProgress && !dl.paused;
      case "cmd_pauseResume":
        return dl.inProgress || dl.paused;
      case "cmd_resume":
        return dl.paused;
      case "cmd_openReferrer":
      case "cmd_removeFromList":
      case "cmd_retry":
        return dl.removable;
      case "cmd_showInfo":
      case "cmd_copyLocation":
        return true;
    }
    return false;
  },

  doCommand: function(aCommand)
  {
    if (this.isCommandEnabled(aCommand))
      this.commands[aCommand](gDownloadsView.selectedItem);
  },

  onCommandUpdate: function ()
  {
    var downloadsCommands = document.getElementById("downloadsCommands");
    for (var i = 0; i < downloadsCommands.childNodes.length; ++i)
      this.updateCommand(downloadsCommands.childNodes[i]);
  },

  updateCommand: function (command)
  {
    if (this.isCommandEnabled(command.id))
      command.removeAttribute("disabled");
    else
      command.setAttribute("disabled", "true");
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
      if (aSelectedItem.inProgress)
        this.commands.cmd_pause(aSelectedItem);
      else
        this.commands.cmd_resume(aSelectedItem);
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
    cmd_showInfo: function(aSelectedItem) {
      showDownloadInfo(aSelectedItem);
    },
    cmd_copyLocation: function(aSelectedItem) {
      copySourceLocation(aSelectedItem);
    },
    cmd_clearList: function() {
      gDownloadManager.cleanUp();
    }
  }
};

function onDownloadShowInfo()
{
  if (gDownloadsView.selectedItem)
    fireEventForElement(gDownloadsView.selectedItem, "properties");
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













function updateStatus(aItem, aDownload) {
  let status = "";

  let state = Number(aItem.getAttribute("state"));
  switch (state) {
    case nsIDM.DOWNLOAD_PAUSED:
    case nsIDM.DOWNLOAD_DOWNLOADING:
      let currBytes = Number(aItem.getAttribute("currBytes"));
      let maxBytes = Number(aItem.getAttribute("maxBytes"));

      
      let ([progress, progressUnits] = convertByteUnits(currBytes),
           [total, totalUnits] = convertByteUnits(maxBytes),
           transfer) {
        if (total < 0)
          transfer = gStr.transferNoTotal;
        else if (progressUnits == totalUnits)
          transfer = gStr.transferSameUnits;
        else
          transfer = gStr.transferDiffUnits;

        transfer = replaceInsert(transfer, 1, progress);
        transfer = replaceInsert(transfer, 2, progressUnits);
        transfer = replaceInsert(transfer, 3, total);
        transfer = replaceInsert(transfer, 4, totalUnits);

        if (state == nsIDM.DOWNLOAD_PAUSED) {
          status = replaceInsert(gStr.paused, 1, transfer);

          
          break;
        }

        
        status = replaceInsert(gStr.statusFormat, 1, transfer);
      }

      
      let speed = aDownload ? aDownload.speed : 0;

      
      let ([rate, unit] = convertByteUnits(speed)) {
        
        status = replaceInsert(status, 2, rate);
        
        status = replaceInsert(status, 3, unit);
      }

      
      let (remain) {
        if ((speed > 0) && (maxBytes > 0)) {
          let seconds = Math.ceil((maxBytes - currBytes) / speed);
          let lastSec = Number(aItem.getAttribute("lastSeconds"));

          
          
          
          let (diff = seconds - lastSec) {
            if (diff > 0 && diff <= 10)
              seconds = lastSec;
            else
              aItem.setAttribute("lastSeconds", seconds);
          }

          
          if (seconds <= 3)
            remain = gStr.timeFewSeconds;
          
          else if (seconds <= 60)
            remain = replaceInsert(gStr.timeSecondsLeft, 1, seconds);
          else
            remain = replaceInsert(gStr.timeMinutesLeft, 1,
                                   Math.ceil(seconds / 60));
        } else {
          remain = gStr.timeUnknown;
        }

        
        status = replaceInsert(status, 4, remain);
      }

      break;
    case nsIDM.DOWNLOAD_FINISHED:
    case nsIDM.DOWNLOAD_FAILED:
    case nsIDM.DOWNLOAD_CANCELED:
    case nsIDM.DOWNLOAD_BLOCKED:
      let (stateSize = {}) {
        stateSize[nsIDM.DOWNLOAD_FINISHED] = function() {
          
          let fileSize = Number(aItem.getAttribute("maxBytes"));
          let sizeText = gStr.doneSizeUnknown;
          if (fileSize >= 0) {
            let [size, unit] = convertByteUnits(fileSize);
            sizeText = replaceInsert(gStr.doneSize, 1, size);
            sizeText = replaceInsert(sizeText, 2, unit);
          }
          return sizeText;
        };
        stateSize[nsIDM.DOWNLOAD_FAILED] = function() gStr.stateFailed;
        stateSize[nsIDM.DOWNLOAD_CANCELED] = function() gStr.stateCanceled;
        stateSize[nsIDM.DOWNLOAD_BLOCKED] = function() gStr.stateBlocked;

        
        status = replaceInsert(gStr.doneStatus, 1, stateSize[state]());
      }

      let (displayHost,
           ioService = Cc["@mozilla.org/network/io-service;1"].
                       getService(Ci.nsIIOService),
           eTLDService = Cc["@mozilla.org/network/effective-tld-service;1"].
                         getService(Ci.nsIEffectiveTLDService)) {
        
        let uri = ioService.newURI(getReferrerOrSource(aItem), null, null);

        try {
          
          displayHost = eTLDService.getBaseDomain(uri);
        } catch (e) {
          
          displayHost = uri.host;
        }

        
        if (displayHost.length == 0)
          displayHost = uri.spec;

        
        else if (uri.port != -1)
          displayHost += ":" + uri.port;

        
        status = replaceInsert(status, 2, displayHost);
      }

      break;
  }

  aItem.setAttribute("status", status);
}







function convertByteUnits(aBytes)
{
  let unitIndex = 0;

  
  
  while ((aBytes >= 999.5) && (unitIndex < gStr.units.length - 1)) {
    aBytes /= 1024;
    unitIndex++;
  }

  
  
  aBytes = aBytes.toFixed((aBytes > 0) && (aBytes < 100) ? 1 : 0);

  return [aBytes, gStr.units[unitIndex]];
}

function replaceInsert(aText, aIndex, aValue)
{
  return aText.replace("#" + aIndex, aValue);
}

function removeFromView(aDownload)
{
  
  if (!aDownload) return;

  let index = gDownloadsView.selectedIndex;
  gDownloadsView.removeChild(aDownload);
  gDownloadsView.selectedIndex = Math.min(index, gDownloadsView.itemCount - 1);
  evenOddCellAttribution();
}

function getReferrerOrSource(aDownload)
{
  
  if (aDownload.hasAttribute("referrer"))
    return aDownload.getAttribute("referrer");

  
  return aDownload.getAttribute("uri");
}




function buildDefaultView()
{
  buildActiveDownloadsList();

  let pref = Cc["@mozilla.org/preferences-service;1"].
             getService(Ci.nsIPrefBranch);
  let days = pref.getIntPref(PREF_BDM_DISPLAYEDHISTORYDAYS);
  buildDownloadListWithTime(Date.now() - days * 24 * 60 * 60 * 1000);

  
  var children = gDownloadsView.children;
  if (children.length > 0)
    gDownloadsView.selectedItem = children[0];
}

 



function evenOddCellAttribution()
{
  let alternateCell = false;
  let allDownloadsInView = gDownloadsView.getElementsByTagName("richlistitem");

  for (let i = 0; i < allDownloadsInView.length; i++) {
    if (alternateCell)
      allDownloadsInView[i].setAttribute("alternate", "true");
    else 
      allDownloadsInView[i].removeAttribute("alternate");

    alternateCell = !alternateCell;
  }
}















function buildDownloadList(aStmt, aRef)
{
  while (aRef.nextSibling && aRef.nextSibling.tagName == "richlistitem")
    gDownloadsView.removeChild(aRef.nextSibling);

  while (aStmt.executeStep()) {
    let id = aStmt.getInt64(0);
    let state = aStmt.getInt32(4);
    let percentComplete = 100;
    if (state == nsIDM.DOWNLOAD_NOTSTARTED ||
        state == nsIDM.DOWNLOAD_DOWNLOADING ||
        state == nsIDM.DOWNLOAD_PAUSED) {
      
      
      
      let dl = gDownloadManager.getDownload(id);
      percentComplete = dl.percentComplete;
    }
    let dl = createDownloadItem(id,
                                aStmt.getString(1),
                                aStmt.getString(2),
                                aStmt.getString(3),
                                state,
                                percentComplete,
                                Math.round(aStmt.getInt64(5) / 1000),
                                Math.round(aStmt.getInt64(6) / 1000),
                                aStmt.getString(7),
                                aStmt.getInt64(8),
                                aStmt.getInt64(9));
    if (dl)
      gDownloadsView.insertBefore(dl, aRef.nextSibling);
  }
  evenOddCellAttribution();
}

var gActiveDownloadsQuery = null;
function buildActiveDownloadsList()
{
  
  if (gDownloadManager.activeDownloadCount == 0)
    return;

  
  var db = gDownloadManager.DBConnection;
  var stmt = gActiveDownloadsQuery;
  if (!stmt) {
    stmt = db.createStatement(replaceInsert(gBaseQuery, 1,
      "state = ?1 OR state = ?2 OR state = ?3 OR state = ?4 OR state = ?5"));
    gActiveDownloadsQuery = stmt;
  }

  try {
    stmt.bindInt32Parameter(0, nsIDM.DOWNLOAD_NOTSTARTED);
    stmt.bindInt32Parameter(1, nsIDM.DOWNLOAD_DOWNLOADING);
    stmt.bindInt32Parameter(2, nsIDM.DOWNLOAD_PAUSED);
    stmt.bindInt32Parameter(3, nsIDM.DOWNLOAD_QUEUED);
    stmt.bindInt32Parameter(4, nsIDM.DOWNLOAD_SCANNING);
    buildDownloadList(stmt, gDownloadsActiveArea);
  } finally {
    stmt.reset();
  }
}








var gDownloadListWithTimeQuery = null;
function buildDownloadListWithTime(aTime)
{
  var db = gDownloadManager.DBConnection;
  var stmt = gDownloadListWithTimeQuery;
  if (!stmt) {
    stmt = db.createStatement(replaceInsert(gBaseQuery, 1, "startTime >= ?1 " +
      "AND (state = ?2 OR state = ?3 OR state = ?4 OR state = ?5)"));
    gDownloadListWithTimeQuery = stmt;
  }

  try {
    stmt.bindInt64Parameter(0, aTime * 1000);
    stmt.bindInt32Parameter(1, nsIDM.DOWNLOAD_FINISHED);
    stmt.bindInt32Parameter(2, nsIDM.DOWNLOAD_FAILED);
    stmt.bindInt32Parameter(3, nsIDM.DOWNLOAD_CANCELED);
    stmt.bindInt32Parameter(4, nsIDM.DOWNLOAD_BLOCKED);
    buildDownloadList(stmt, gDownloadsDoneArea);
  } finally {
    stmt.reset();
  }
}











function buildDownloadListWithSearch(aTerms)
{
  gSearching = true;

  
  aTerms = aTerms.replace(/^\s+|\s+$/, "");
  if (aTerms.length == 0) {
    gSearching = false;
    buildDefaultView();
    return;
  }

  var db = gDownloadManager.DBConnection;
  let stmt = db.createStatement(replaceInsert(gBaseQuery, 1,
    "name LIKE ?1 ESCAPE '/' AND state != ?2 AND state != ?3"));

  try {
    var paramForLike = stmt.escapeStringForLIKE(aTerms, '/');
    stmt.bindStringParameter(0, "%" + paramForLike + "%");
    stmt.bindInt32Parameter(1, nsIDM.DOWNLOAD_DOWNLOADING);
    stmt.bindInt32Parameter(2, nsIDM.DOWNLOAD_PAUSED);
    buildDownloadList(stmt, gDownloadsDoneArea);
  } finally {
    stmt.reset();
  }
}

function performSearch() {
  buildDownloadListWithSearch(document.getElementById("searchbox").value);
}

function onSearchboxBlur() {
  var searchbox = document.getElementById("searchbox");
  if (searchbox.value == "") {
    searchbox.setAttribute("empty", "true");
    searchbox.value = searchbox.getAttribute("defaultValue");
  }
}

function onSearchboxFocus() {
  var searchbox = document.getElementById("searchbox");
  if (searchbox.hasAttribute("empty")) {
    searchbox.value = "";
    searchbox.removeAttribute("empty");
  }
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
