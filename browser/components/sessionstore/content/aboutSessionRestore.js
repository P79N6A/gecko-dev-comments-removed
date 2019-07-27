



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AppConstants",
  "resource://gre/modules/AppConstants.jsm");

var gStateObject;
var gTreeData;



window.onload = function() {
  
  
  let anchor = document.getElementById("linkMoreTroubleshooting");
  if (anchor) {
    let baseURL = Services.urlFormatter.formatURLPref("app.support.baseURL");
    anchor.setAttribute("href", baseURL + "troubleshooting");
  }

  
  for (let radioId of ["radioRestoreAll", "radioRestoreChoose"]) {
    let button = document.getElementById(radioId);
    if (button) {
      button.addEventListener("click", updateTabListVisibility);
    }
  }

  
  
  var sessionData = document.getElementById("sessionData");
  if (!sessionData.value) {
    document.getElementById("errorTryAgain").disabled = true;
    return;
  }

  gStateObject = JSON.parse(sessionData.value);

  
  var event = document.createEvent("UIEvents");
  event.initUIEvent("input", true, true, window, 0);
  sessionData.dispatchEvent(event);

  initTreeView();

  document.getElementById("errorTryAgain").focus();
};

function isTreeViewVisible() {
  let tabList = document.getElementById("tabList");
  return tabList.hasAttribute("available");
}

function initTreeView() {
  
  
  if (!isTreeViewVisible()) {
    return;
  }
  var tabList = document.getElementById("tabList");
  var winLabel = tabList.getAttribute("_window_label");

  gTreeData = [];
  gStateObject.windows.forEach(function(aWinData, aIx) {
    var winState = {
      label: winLabel.replace("%S", (aIx + 1)),
      open: true,
      checked: true,
      ix: aIx
    };
    winState.tabs = aWinData.tabs.map(function(aTabData) {
      var entry = aTabData.entries[aTabData.index - 1] || { url: "about:blank" };
      var iconURL = aTabData.attributes && aTabData.attributes.image || null;
      
      if (/^https?:/.test(iconURL))
        iconURL = "moz-anno:favicon:" + iconURL;
      return {
        label: entry.title || entry.url,
        checked: true,
        src: iconURL,
        parent: winState
      };
    });
    gTreeData.push(winState);
    for (let tab of winState.tabs)
      gTreeData.push(tab);
  }, this);

  tabList.view = treeView;
  tabList.view.selection.select(0);
}


function updateTabListVisibility() {
  let tabList = document.getElementById("tabList");
  if (document.getElementById("radioRestoreChoose").checked) {
    tabList.setAttribute("available", "true");
  } else {
    tabList.removeAttribute("available");
  }
  initTreeView();
}

function restoreSession() {
  document.getElementById("errorTryAgain").disabled = true;

  if (isTreeViewVisible()) {
    if (!gTreeData.some(aItem => aItem.checked)) {
      
      
      
      startNewSession();
      return;
    }

    
    var ix = gStateObject.windows.length - 1;
    for (var t = gTreeData.length - 1; t >= 0; t--) {
      if (treeView.isContainer(t)) {
        if (gTreeData[t].checked === 0)
          
          gStateObject.windows[ix].tabs =
            gStateObject.windows[ix].tabs.filter((aTabData, aIx) =>
                                                   gTreeData[t].tabs[aIx].checked);
        else if (!gTreeData[t].checked)
          
          gStateObject.windows.splice(ix, 1);
        ix--;
      }
    }
  }
  var stateString = JSON.stringify(gStateObject);

  var ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  var top = getBrowserWindow();

  
  if (top.gBrowser.tabs.length == 1) {
    ss.setWindowState(top, stateString, true);
    return;
  }

  
  var newWindow = top.openDialog(top.location, "_blank", "chrome,dialog=no,all");

  var obs = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
  obs.addObserver(function observe(win, topic) {
    if (win != newWindow) {
      return;
    }

    obs.removeObserver(observe, topic);
    ss.setWindowState(newWindow, stateString, true);

    var tabbrowser = top.gBrowser;
    var tabIndex = tabbrowser.getBrowserIndexForDocument(document);
    tabbrowser.removeTab(tabbrowser.tabs[tabIndex]);
  }, "browser-delayed-startup-finished", false);
}

function startNewSession() {
  var prefBranch = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
  if (prefBranch.getIntPref("browser.startup.page") == 0)
    getBrowserWindow().gBrowser.loadURI("about:blank");
  else
    getBrowserWindow().BrowserHome();
}

function onListClick(aEvent) {
  
  if (aEvent.button == 2)
    return;

  var cell = treeView.treeBox.getCellAt(aEvent.clientX, aEvent.clientY);
  if (cell.col) {
    
    
    let accelKey = AppConstants.platform == "macosx" ?
                   aEvent.metaKey :
                   aEvent.ctrlKey;
    if ((aEvent.button == 1 || aEvent.button == 0 && aEvent.detail == 2 || accelKey) &&
        cell.col.id == "title" &&
        !treeView.isContainer(cell.row)) {
      restoreSingleTab(cell.row, aEvent.shiftKey);
      aEvent.stopPropagation();
    }
    else if (cell.col.id == "restore")
      toggleRowChecked(cell.row);
  }
}

function onListKeyDown(aEvent) {
  switch (aEvent.keyCode)
  {
  case KeyEvent.DOM_VK_SPACE:
    toggleRowChecked(document.getElementById("tabList").currentIndex);
    break;
  case KeyEvent.DOM_VK_RETURN:
    var ix = document.getElementById("tabList").currentIndex;
    if (aEvent.ctrlKey && !treeView.isContainer(ix))
      restoreSingleTab(ix, aEvent.shiftKey);
    break;
  }
}



function getBrowserWindow() {
  return window.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIWebNavigation)
               .QueryInterface(Ci.nsIDocShellTreeItem).rootTreeItem
               .QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindow);
}

function toggleRowChecked(aIx) {
  function isChecked(aItem) {
    return aItem.checked;
  }

  var item = gTreeData[aIx];
  item.checked = !item.checked;
  treeView.treeBox.invalidateRow(aIx);

  if (treeView.isContainer(aIx)) {
    
    for (let tab of item.tabs) {
      tab.checked = item.checked;
      treeView.treeBox.invalidateRow(gTreeData.indexOf(tab));
    }
  }
  else {
    
    item.parent.checked = item.parent.tabs.every(isChecked) ? true :
                          item.parent.tabs.some(isChecked) ? 0 : false;
    treeView.treeBox.invalidateRow(gTreeData.indexOf(item.parent));
  }

  
  if (document.getElementById("errorCancel")) {
    document.getElementById("errorTryAgain").disabled = !gTreeData.some(isChecked);
  }
}

function restoreSingleTab(aIx, aShifted) {
  var tabbrowser = getBrowserWindow().gBrowser;
  var newTab = tabbrowser.addTab();
  var item = gTreeData[aIx];

  var ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  var tabState = gStateObject.windows[item.parent.ix]
                             .tabs[aIx - gTreeData.indexOf(item.parent) - 1];
  
  tabState.hidden = false;
  ss.setTabState(newTab, JSON.stringify(tabState));

  
  var prefBranch = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
  if (prefBranch.getBoolPref("browser.tabs.loadInBackground") != !aShifted)
    tabbrowser.selectedTab = newTab;
}



var treeView = {
  treeBox: null,
  selection: null,

  get rowCount()                     { return gTreeData.length; },
  setTree: function(treeBox)         { this.treeBox = treeBox; },
  getCellText: function(idx, column) { return gTreeData[idx].label; },
  isContainer: function(idx)         { return "open" in gTreeData[idx]; },
  getCellValue: function(idx, column){ return gTreeData[idx].checked; },
  isContainerOpen: function(idx)     { return gTreeData[idx].open; },
  isContainerEmpty: function(idx)    { return false; },
  isSeparator: function(idx)         { return false; },
  isSorted: function()               { return false; },
  isEditable: function(idx, column)  { return false; },
  canDrop: function(idx, orientation, dt) { return false; },
  getLevel: function(idx)            { return this.isContainer(idx) ? 0 : 1; },

  getParentIndex: function(idx) {
    if (!this.isContainer(idx))
      for (var t = idx - 1; t >= 0 ; t--)
        if (this.isContainer(t))
          return t;
    return -1;
  },

  hasNextSibling: function(idx, after) {
    var thisLevel = this.getLevel(idx);
    for (var t = after + 1; t < gTreeData.length; t++)
      if (this.getLevel(t) <= thisLevel)
        return this.getLevel(t) == thisLevel;
    return false;
  },

  toggleOpenState: function(idx) {
    if (!this.isContainer(idx))
      return;
    var item = gTreeData[idx];
    if (item.open) {
      
      var thisLevel = this.getLevel(idx);
      for (var t = idx + 1; t < gTreeData.length && this.getLevel(t) > thisLevel; t++);
      var deletecount = t - idx - 1;
      gTreeData.splice(idx + 1, deletecount);
      this.treeBox.rowCountChanged(idx + 1, -deletecount);
    }
    else {
      
      var toinsert = gTreeData[idx].tabs;
      for (var i = 0; i < toinsert.length; i++)
        gTreeData.splice(idx + i + 1, 0, toinsert[i]);
      this.treeBox.rowCountChanged(idx + 1, toinsert.length);
    }
    item.open = !item.open;
    this.treeBox.invalidateRow(idx);
  },

  getCellProperties: function(idx, column) {
    if (column.id == "restore" && this.isContainer(idx) && gTreeData[idx].checked === 0)
      return "partial";
    if (column.id == "title")
      return this.getImageSrc(idx, column) ? "icon" : "noicon";

    return "";
  },

  getRowProperties: function(idx) {
    var winState = gTreeData[idx].parent || gTreeData[idx];
    if (winState.ix % 2 != 0)
      return "alternate";

    return "";
  },

  getImageSrc: function(idx, column) {
    if (column.id == "title")
      return gTreeData[idx].src || null;
    return null;
  },

  getProgressMode : function(idx, column) { },
  cycleHeader: function(column) { },
  cycleCell: function(idx, column) { },
  selectionChanged: function() { },
  performAction: function(action) { },
  performActionOnCell: function(action, index, column) { },
  getColumnProperties: function(column) { return ""; }
};
