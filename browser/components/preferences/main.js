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
#   Ehsan Akhgari <ehsan.akhgari@gmail.com>
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

    this.updateBrowserStartupLastSession();

    
    Components.classes["@mozilla.org/observer-service;1"]
              .getService(Components.interfaces.nsIObserverService)
              .notifyObservers(window, "main-pane-loaded", null);
  },

  

  


















  syncFromHomePref: function ()
  {
    let homePref = document.getElementById("browser.startup.homepage");

    
    
    if (homePref.value.toLowerCase() == "about:home")
      return "";

    
    
    
    if (homePref.value == "")
      return "about:blank";

    
    return undefined;
  },

  syncToHomePref: function (value)
  {
    
    if (value == "")
      return "about:home";

    
    return undefined;
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
      var tabs = win.gBrowser.visibleTabs;
      function getTabURI(t) t.linkedBrowser.currentURI.spec;
      
      homePage.value = tabs.map(getTabURI).join("|");
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

    var windowIsPresent;
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
      windowIsPresent = true;

      var tabbrowser = win.document.getElementById("content");
      if (tabbrowser.browsers.length > 1)
        useCurrent.label = useCurrent.getAttribute("label2");
      else
        useCurrent.label = useCurrent.getAttribute("label1");
    }
    else {
      windowIsPresent = false;
      useCurrent.label = useCurrent.getAttribute("label1");
    }

    
    if (document.getElementById
        ("pref.browser.homepage.disable_button.current_page").locked)
      return;

    useCurrent.disabled = !windowIsPresent;
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
    const nsILocalFile = Components.interfaces.nsILocalFile;

    var fp = Components.classes["@mozilla.org/filepicker;1"]
                       .createInstance(nsIFilePicker);
    var bundlePreferences = document.getElementById("bundlePreferences");
    var title = bundlePreferences.getString("chooseDownloadFolderTitle");
    fp.init(window, title, nsIFilePicker.modeGetFolder);
    fp.appendFilters(nsIFilePicker.filterAll);

    var folderListPref = document.getElementById("browser.download.folderList");
    var currentDirPref = this._indexToFolder(folderListPref.value); 
    var defDownloads = this._indexToFolder(1); 

    
    if (currentDirPref && currentDirPref.exists()) {
      fp.displayDirectory = currentDirPref;
    } 
    else if (defDownloads && defDownloads.exists()) {
      fp.displayDirectory = defDownloads;
    } 
    else {
      fp.displayDirectory = this._indexToFolder(0);
    }

    if (fp.show() == nsIFilePicker.returnOK) {
      var file = fp.file.QueryInterface(nsILocalFile);
      var currentDirPref = document.getElementById("browser.download.dir");
      currentDirPref.value = file;
      var folderListPref = document.getElementById("browser.download.folderList");
      folderListPref.value = this._folderToIndex(file);
      
      
      
      
    }
  },

  



  displayDownloadDirPref: function ()
  {
    var folderListPref = document.getElementById("browser.download.folderList");
    var bundlePreferences = document.getElementById("bundlePreferences");
    var downloadFolder = document.getElementById("downloadFolder");
    var currentDirPref = document.getElementById("browser.download.dir");

    
    var ios = Components.classes["@mozilla.org/network/io-service;1"]
                        .getService(Components.interfaces.nsIIOService);
    var fph = ios.getProtocolHandler("file")
                 .QueryInterface(Components.interfaces.nsIFileProtocolHandler);
    var iconUrlSpec;
      
    
    if (folderListPref.value == 2) {
      
      downloadFolder.label = this._getDisplayNameOfFile(currentDirPref.value);
      iconUrlSpec = fph.getURLSpecFromFile(currentDirPref.value);
    } else if (folderListPref.value == 1) {
      
      
      
      
      
      
      
      
      
      
      downloadFolder.label = bundlePreferences.getString("downloadsFolderName");
      iconUrlSpec = fph.getURLSpecFromFile(this._indexToFolder(1));
    } else {
      
      downloadFolder.label = bundlePreferences.getString("desktopFolderName");
      iconUrlSpec = fph.getURLSpecFromFile(desk);
    }
    downloadFolder.image = "moz-icon://" + iconUrlSpec + "?size=16";
    
    
    return undefined;
  },

  


  _getDisplayNameOfFile: function (aFolder)
  {
    
    
    return aFolder ? aFolder.path : "";
  },

  








  _getDownloadsFolder: function (aFolder)
  {
    switch (aFolder) {
      case "Desktop":
        var fileLoc = Components.classes["@mozilla.org/file/directory_service;1"]
                                    .getService(Components.interfaces.nsIProperties);
        return fileLoc.get("Desk", Components.interfaces.nsILocalFile);
      break;
      case "Downloads":
        var dnldMgr = Components.classes["@mozilla.org/download-manager;1"]
                                .getService(Components.interfaces.nsIDownloadManager);
        return dnldMgr.defaultDownloadsDirectory;
      break;
    }
    throw "ASSERTION FAILED: folder type should be 'Desktop' or 'Downloads'";
  },

  









  _folderToIndex: function (aFolder)
  {
    if (!aFolder || aFolder.equals(this._getDownloadsFolder("Desktop")))
      return 0;
    else if (aFolder.equals(this._getDownloadsFolder("Downloads")))
      return 1;
    return 2;
  },

  








  _indexToFolder: function (aIndex)
  {
    switch (aIndex) {
      case 0:
        return this._getDownloadsFolder("Desktop");
      case 1:
        return this._getDownloadsFolder("Downloads");
    }
    var currentDirPref = document.getElementById("browser.download.dir");
    return currentDirPref.value;
  },

  


  getFolderListPref: function ()
  {
    var folderListPref = document.getElementById("browser.download.folderList");
    switch (folderListPref.value) {
      case 0: 
      case 1: 
        return folderListPref.value;
      break;
      case 2: 
        var currentDirPref = document.getElementById("browser.download.dir");
        if (currentDirPref.value) {
          
          
          return this._folderToIndex(currentDirPref.value);
        }
        return 0;
      break;
    }
  },

  


  showAddonsMgr: function ()
  {
    openUILinkIn("about:addons", "window");
  },

  



  updateBrowserStartupLastSession: function()
  {
    let pbAutoStartPref = document.getElementById("browser.privatebrowsing.autostart");
    let startupPref = document.getElementById("browser.startup.page");
    let menu = document.getElementById("browserStartupPage");
    let option = document.getElementById("browserStartupLastSession");
    if (pbAutoStartPref.value) {
      option.setAttribute("disabled", "true");
      if (option.selected) {
        menu.selectedItem = document.getElementById("browserStartupHomePage");
      }
    } else {
      option.removeAttribute("disabled");
      startupPref.updateElements(); 
    }
  }
};
