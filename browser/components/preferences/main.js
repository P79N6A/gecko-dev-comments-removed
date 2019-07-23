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
# The Original Code is the Firefox Preferences System.
#
# The Initial Developer of the Original Code is
# Jeff Walden <jwalden+code@mit.edu>.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Ben Goodger <ben@mozilla.org>
#   Asaf Romano <mozilla.mano@sent.com>
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

var gMainPane = {
  _pane: null,

  


  init: function ()
  {
    this._pane = document.getElementById("paneMain");

    
    this._updateUseCurrentButton();
    window.addEventListener("focus", this._updateUseCurrentButton, false);
  },

  

  


















  




  setHomePageToCurrent: function ()
  {
    var win;
    if (document.documentElement.instantApply) {
      
      var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                         .getService(Components.interfaces.nsIWindowMediator);
      win = wm.getMostRecentWindow("navigator:browser");
    }
    else
      win = window.opener;

    if (win) {
      var homePage = document.getElementById("browser.startup.homepage");
      var browser = win.document.getElementById("content");

      var newVal = browser.browsers[0].currentURI.spec;
      if (browser.browsers.length > 1) {
        
        for (var i = 1; i < browser.browsers.length; i++)
          newVal += "|" + browser.browsers[i].currentURI.spec;
      }

      homePage.value = newVal;
    }
  },

  




  setHomePageToBookmark: function ()
  {
    var rv = { urls: null, names: null };
    document.documentElement.openSubDialog("chrome://browser/content/preferences/selectBookmark.xul",
                                           "resizable", rv);  
    if (rv.urls && rv.names) {
      var homePage = document.getElementById("browser.startup.homepage");

      
      homePage.value = rv.urls.join("|");
    }
  },

  



  _updateUseCurrentButton: function () {
    var useCurrent = document.getElementById("useCurrent");

    var win;
    if (document.documentElement.instantApply) {
      const Cc = Components.classes, Ci = Components.interfaces;
      
      var wm = Cc["@mozilla.org/appshell/window-mediator;1"]
                 .getService(Ci.nsIWindowMediator);
      win = wm.getMostRecentWindow("navigator:browser");
    }
    else
      win = window.opener;

    if (win && win.document.documentElement
                  .getAttribute("windowtype") == "navigator:browser") {
      useCurrent.disabled = false;

      var tabbrowser = win.document.getElementById("content");
      if (tabbrowser.browsers.length > 1)
        useCurrent.label = useCurrent.getAttribute("label2");
      else
        useCurrent.label = useCurrent.getAttribute("label1");
    }
    else {
      useCurrent.label = useCurrent.getAttribute("label1");
      useCurrent.disabled = true;
    }
  },

  


  restoreDefaultHomePage: function ()
  {
    var homePage = document.getElementById("browser.startup.homepage");
    homePage.value = homePage.defaultValue;
  },

  

  
























  




  readShowDownloadsWhenStarting: function ()
  {
    this.showDownloadsWhenStartingPrefChanged();

    
    return undefined;
  },

  



  showDownloadsWhenStartingPrefChanged: function ()
  {
    var showWhenStartingPref = document.getElementById("browser.download.manager.showWhenStarting");
    var closeWhenDonePref = document.getElementById("browser.download.manager.closeWhenDone");
    closeWhenDonePref.disabled = !showWhenStartingPref.value;
  },

  



  readUseDownloadDir: function ()
  {
    var downloadFolder = document.getElementById("downloadFolder");
    var chooseFolder = document.getElementById("chooseFolder");
    var preference = document.getElementById("browser.download.useDownloadDir");
    downloadFolder.disabled = !preference.value;
    chooseFolder.disabled = !preference.value;

    
    return undefined;
  },

  




  chooseFolder: function ()
  {
    const nsIFilePicker = Components.interfaces.nsIFilePicker;
    var fp = Components.classes["@mozilla.org/filepicker;1"]
                       .createInstance(nsIFilePicker);
    var bundlePreferences = document.getElementById("bundlePreferences");
    var title = bundlePreferences.getString("chooseDownloadFolderTitle");
    fp.init(window, title, nsIFilePicker.modeGetFolder);

    const nsILocalFile = Components.interfaces.nsILocalFile;
    var customDirPref = document.getElementById("browser.download.dir");
    if (customDirPref.value)
      fp.displayDirectory = customDirPref.value;
    fp.appendFilters(nsIFilePicker.filterAll);
    if (fp.show() == nsIFilePicker.returnOK) {
      var file = fp.file.QueryInterface(nsILocalFile);
      var currentDirPref = document.getElementById("browser.download.downloadDir");
      customDirPref.value = currentDirPref.value = file;
      var folderListPref = document.getElementById("browser.download.folderList");
      folderListPref.value = this._folderToIndex(file);
    }
  },

  



  readDownloadDirPref: function ()
  {
    var folderListPref = document.getElementById("browser.download.folderList");
    var bundlePreferences = document.getElementById("bundlePreferences");
    var downloadFolder = document.getElementById("downloadFolder");

    var customDirPref = document.getElementById("browser.download.dir");
    var customIndex = customDirPref.value ? this._folderToIndex(customDirPref.value) : 0;
    if (folderListPref.value == 0 || customIndex == 0)
      downloadFolder.label = bundlePreferences.getString("desktopFolderName");
    else if (folderListPref.value == 1 || customIndex == 1)
      downloadFolder.label = bundlePreferences.getString("myDownloadsFolderName");
    else
      downloadFolder.label = this._getDisplayNameOfFile(customDirPref.value);

    var ios = Components.classes["@mozilla.org/network/io-service;1"]
                        .getService(Components.interfaces.nsIIOService);
    var fph = ios.getProtocolHandler("file")
                 .QueryInterface(Components.interfaces.nsIFileProtocolHandler);
    var currentDirPref = document.getElementById("browser.download.downloadDir");
    var downloadDir = currentDirPref.value || this._indexToFolder(folderListPref.value);
    var urlspec = fph.getURLSpecFromFile(downloadDir);
    downloadFolder.image = "moz-icon://" + urlspec + "?size=16";

    
    return undefined;
  },

  









  _folderToIndex: function (aFolder)
  {
    if (!aFolder || aFolder.equals(this._getDownloadsFolder("Desktop")))
      return 0;
    else if (aFolder.equals(this._getDownloadsFolder("Downloads")))
      return 1;
    return 2;
  },

  







  _getDownloadsFolder: function (aFolder)
  {
    var fileLocator = Components.classes["@mozilla.org/file/directory_service;1"]
                                .getService(Components.interfaces.nsIProperties);
    var dir = fileLocator.get(this._getSpecialFolderKey(aFolder),
                              Components.interfaces.nsILocalFile);
    if (aFolder != "Desktop")
      dir.append("My Downloads"); 

    return dir;
  },

  








  _getSpecialFolderKey: function (aFolderType)
  {
    if (aFolderType == "Desktop")
      return "Desk";

    if (aFolderType == "Downloads")
#ifdef XP_WIN
      return "Pers";
#else
#ifdef XP_MACOSX
      return "UsrDocs";
#else
      return "Home";
#endif
#endif

      throw "ASSERTION FAILED: folder type should be 'Desktop' or 'Downloads'";
  },

  


  _getDisplayNameOfFile: function (aFolder)
  {
    
    
    return aFolder ? aFolder.path : "";
  },

  








  _indexToFolder: function (aIndex)
  {
    switch (aIndex) {
      case 0:
        return this._getDownloadsFolder("Desktop");
      case 1:
        return this._getDownloadsFolder("Downloads");
    }

    var customDirPref = document.getElementById("browser.download.dir");
    return customDirPref.value;
  },

  



  writeFolderList: function ()
  {
    var currentDirPref = document.getElementById("browser.download.downloadDir");
    return this._folderToIndex(currentDirPref.value);
  }

#ifdef HAVE_SHELL_SERVICE
  ,

  

  







  





  checkNow: function ()
  {
    var shellSvc = Components.classes["@mozilla.org/browser/shell-service;1"]
                             .getService(Components.interfaces.nsIShellService);
    var brandBundle = document.getElementById("bundleBrand");
    var shellBundle = document.getElementById("bundleShell");
    var brandShortName = brandBundle.getString("brandShortName");
    var promptTitle = shellBundle.getString("setDefaultBrowserTitle");
    var promptMessage;
    const IPS = Components.interfaces.nsIPromptService;
    var psvc = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                         .getService(IPS);
    if (!shellSvc.isDefaultBrowser(false)) {
      promptMessage = shellBundle.getFormattedString("setDefaultBrowserMessage", 
                                                     [brandShortName]);
      var rv = psvc.confirmEx(window, promptTitle, promptMessage, 
                              IPS.STD_YES_NO_BUTTONS,
                              null, null, null, null, { });
      if (rv == 0)
        shellSvc.setDefaultBrowser(true, false);
    }
    else {
      promptMessage = shellBundle.getFormattedString("alreadyDefaultBrowser",
                                                     [brandShortName]);
      psvc.alert(window, promptTitle, promptMessage);
    }
  }
#endif
};
