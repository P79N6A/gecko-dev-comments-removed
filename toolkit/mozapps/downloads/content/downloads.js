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
const PREF_BDM_RETENTION = "browser.download.manager.retention";

const nsLocalFile = Components.Constructor("@mozilla.org/file/local;1",
                                           "nsILocalFile", "initWithPath");

var Cc = Components.classes;
var Ci = Components.interfaces;

var gDownloadManager  = Cc["@mozilla.org/download-manager;1"].
                        getService(Ci.nsIDownloadManager);
var gDownloadListener     = null;
var gDownloadsView        = null;
var gDownloadsActiveTitle = null;
var gDownloadsOtherLabel  = null;
var gDownloadsOtherTitle  = null;
var gDownloadInfoPopup    = null;
var gUserInterfered       = false;
var gSearching            = false;



var gUserInteracted = false;



function fireEventForElement(aElement, aEventType)
{
  var e = document.createEvent("Events");
  e.initEvent("download-" + aEventType, true, true);
  
  aElement.dispatchEvent(e);
}

function createDownloadItem(aID, aFile, aTarget, aURI, aState,
                            aStatus, aProgress, aStartTime)
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
  dl.setAttribute("status", aStatus);
  dl.setAttribute("progress", aProgress);
  dl.setAttribute("startTime", aStartTime);

  var ioSvc = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
  var file = ioSvc.newURI(aFile, null, null).QueryInterface(Ci.nsIFileURL).file;
  dl.setAttribute("path", file.nativePath || file.path);
  
  return dl;
}

function getDownload(aID)
{
  return document.getElementById("dl" + aID);
}



function downloadCompleted(aDownload) 
{
  
  
  
  try {
    let dl = getDownload(aDownload.id);

    
    
    if (!gSearching)
      gDownloadsView.insertBefore(dl, gDownloadsOtherTitle.nextSibling);
    else
      gDownloadsView.removeChild(dl);

    
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

    if (gDownloadManager.activeDownloadCount == 0) {
      gDownloadsActiveTitle.hidden = true;
      document.title = document.documentElement.getAttribute("statictitle");
    }
  }
  catch (e) { }
}

function autoRemoveAndClose(aDownload)
{
  var pref = Cc["@mozilla.org/preferences-service;1"].
             getService(Ci.nsIPrefBranch);

  if (aDownload && (pref.getIntPref(PREF_BDM_RETENTION) == 0)) {
    
    var dl = getDownload(aDownload.id);
    if (dl)
      dl.parentNode.removeChild(dl);
  }
  
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
  var newIndex = Math.max(gDownloadsView.selectedIndex - 1, 0);
  gDownloadsView.removeChild(aDownload);
  gDownloadsView.selectedIndex = newIndex;
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
      case Ci.nsIDownloadManager.DOWNLOAD_FINISHED:
        gDownloadViewController.doCommand("cmd_open");
        break;
      case Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING:  
        gDownloadViewController.doCommand("cmd_pause");
        break;
      case Ci.nsIDownloadManager.DOWNLOAD_PAUSED:
        gDownloadViewController.doCommand("cmd_resume");
        break;
      case Ci.nsIDownloadManager.DOWNLOAD_CANCELED:
      case Ci.nsIDownloadManager.DOWNLOAD_FAILED:
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

      var title = strings.getString("fileExecutableSecurityWarningTitle");
      var dontAsk = strings.getString("fileExecutableSecurityWarningDontAsk");

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
  
  var uri = aDownload.getAttribute("uri");
  uriLabel.label = uri;
  uriLabel.setAttribute("tooltiptext", uri);
  var path = aDownload.getAttribute("path");
  locationLabel.label = path;
  locationLabel.setAttribute("tooltiptext", path);

  var button = document.getAnonymousElementByAttribute(aDownload, "anonid", "info");
  gDownloadInfoPopup.openPopup(button, "after_end", 0, 0, false, false);
}

function retryDownload(aDownload)
{
  gDownloadManager.retryDownload(aDownload.getAttribute("dlid"));
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
    mean += dl.amountTransferred;
    base += dl.size;
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
  gDownloadsActiveTitle = document.getElementById("active-downloads-title");
  gDownloadsOtherLabel  = document.getElementById("other-downloads");
  gDownloadsOtherTitle  = document.getElementById("other-downloads-title");
  gDownloadInfoPopup    = document.getElementById("information");

  buildDefaultView();

  
  gDownloadsView.addEventListener("dblclick", onDownloadDblClick, false);

  
  
  gDownloadListener = new DownloadProgressListener();
  gDownloadManager.addListener(gDownloadListener);

  
  
  if (!autoRemoveAndClose())
    gDownloadsView.focus();
}

function Shutdown() 
{
  gDownloadManager.removeListener(gDownloadListener);
}



var gContextMenus = [ 
  ["menuitem_pause", "menuitem_cancel"],
  ["menuitem_open", "menuitem_show", "menuitem_remove"],
  ["menuitem_retry", "menuitem_remove"],
  ["menuitem_retry", "menuitem_remove"],
  ["menuitem_resume", "menuitem_cancel"]
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
      case "cmd_remove":
      case "cmd_retry":
        return dl.removable;
      case "cmd_showInfo":
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
    cmd_pause: function(aSelectedItem) {
      pauseDownload(aSelectedItem);
    },
    cmd_pauseResume: function(aSelectedItem) {
      if (aSelectedItem.inProgress)
        this.commands.cmd_pause(aSelectedItem);
      else
        this.commands.cmd_resume(aSelectedItem);
    },
    cmd_remove: function(aSelectedItem) {
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







function buildDefaultView()
{
  buildActiveDownloadsList();
  buildDownloadListWithTime(Date.now() - 24 * 3600 * 1000 * 7); 

  
  var children = gDownloadsView.children;
  if (children.length > 0)
    gDownloadsView.selectedItem = children[0];
}














function buildDownloadList(aStmt, aRef)
{
  while (aRef.nextSibling && aRef.nextSibling.tagName == "richlistitem")
    gDownloadsView.removeChild(aRef.nextSibling);

  while (aStmt.executeStep()) {
    let id = aStmt.getInt64(0);
    let state = aStmt.getInt32(4);
    let percentComplete = 100;
    if (state == Ci.nsIDownloadManager.DOWNLOAD_NOTSTARTED ||
        state == Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING ||
        state == Ci.nsIDownloadManager.DOWNLOAD_PAUSED) {
      
      
      
      let dl = gDownloadManager.getDownload(id);
      percentComplete = dl.percentComplete;
    }
    let dl = createDownloadItem(id, aStmt.getString(1),
                                aStmt.getString(2), aStmt.getString(3),
                                state, "", percentComplete,
                                Math.round(aStmt.getInt64(5) / 1000));
    gDownloadsView.insertBefore(dl, aRef.nextSibling);
  }
}

var gActiveDownloadsQuery = null;
function buildActiveDownloadsList()
{
  
  if (gDownloadManager.activeDownloadCount == 0)
    return;

  
  gDownloadsActiveTitle.hidden = false;

  
  var db = gDownloadManager.DBConnection;
  var stmt = gActiveDownloadsQuery;
  if (!stmt) {
    stmt = gActiveDownloadsQuery =
      db.createStatement("SELECT id, target, name, source, state, startTime " +
                         "FROM moz_downloads " +
                         "WHERE state = ?1 " +
                         "OR state = ?2 " +
                         "OR state = ?3 " +
                         "ORDER BY endTime ASC");
  }

  try {
    stmt.bindInt32Parameter(0, Ci.nsIDownloadManager.DOWNLOAD_NOTSTARTED);
    stmt.bindInt32Parameter(1, Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING);
    stmt.bindInt32Parameter(2, Ci.nsIDownloadManager.DOWNLOAD_PAUSED);
    buildDownloadList(stmt, gDownloadsActiveTitle);
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
    stmt = gDownloadListWithTimeQuery =
      db.createStatement("SELECT id, target, name, source, state, startTime " +
                         "FROM moz_downloads " +
                         "WHERE startTime >= ?1 " +
                         "AND (state = ?2 " +
                         "OR state = ?3 " +
                         "OR state = ?4) " +
                         "ORDER BY endTime ASC");
  }

  try {
    stmt.bindInt64Parameter(0, aTime * 1000);
    stmt.bindInt32Parameter(1, Ci.nsIDownloadManager.DOWNLOAD_FINISHED);
    stmt.bindInt32Parameter(2, Ci.nsIDownloadManager.DOWNLOAD_FAILED);
    stmt.bindInt32Parameter(3, Ci.nsIDownloadManager.DOWNLOAD_CANCELED);
    buildDownloadList(stmt, gDownloadsOtherTitle);
  } finally {
    stmt.reset();
  }
}











function buildDownloadListWithSearch(aTerms)
{
  gSearching = true;
  gDownloadsOtherLabel.value = gDownloadsOtherLabel.getAttribute("searchlabel");

  
  aTerms = aTerms.replace(/^\s+|\s+$/, "");
  if (aTerms.length == 0) {
    gSearching = false;
    gDownloadsOtherLabel.value = gDownloadsOtherLabel.getAttribute("completedlabel");
    buildDefaultView();
    return;
  }

  var sql = "SELECT id, target, name, source, state, startTime " +
            "FROM moz_downloads WHERE name LIKE ?1 ESCAPE '/' " +
            "AND state != ?2 AND state != ?3 ORDER BY endTime ASC";

  var db = gDownloadManager.DBConnection;
  var stmt = db.createStatement(sql);

  try {
    var paramForLike = stmt.escapeStringForLIKE(aTerms, '/');
    stmt.bindStringParameter(0, "%" + paramForLike + "%");
    stmt.bindInt32Parameter(1, Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING);
    stmt.bindInt32Parameter(2, Ci.nsIDownloadManager.DOWNLOAD_PAUSED);
    buildDownloadList(stmt, gDownloadsOtherTitle);
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

