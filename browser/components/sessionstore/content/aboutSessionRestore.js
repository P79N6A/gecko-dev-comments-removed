



































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

var gStateObject;
var gTreeData;



window.onload = function() {
  
  
  var sessionData = document.getElementById("sessionData");
  if (!sessionData.value) {
    var ss = Cc["@mozilla.org/browser/sessionstartup;1"].getService(Ci.nsISessionStartup);
    sessionData.value = ss.state;
    if (!sessionData.value) {
      document.getElementById("errorTryAgain").disabled = true;
      return;
    }
  }

  
  if (sessionData.value.charAt(0) == '(')
    sessionData.value = sessionData.value.slice(1, -1);
  try {
    gStateObject = JSON.parse(sessionData.value);
  }
  catch (exJSON) {
    var s = new Cu.Sandbox("about:blank");
    gStateObject = Cu.evalInSandbox("(" + sessionData.value + ")", s);
    
    
    sessionData.value = JSON.stringify(gStateObject);
  }

  
  var event = document.createEvent("UIEvents");
  event.initUIEvent("input", true, true, window, 0);
  sessionData.dispatchEvent(event);

  initTreeView();
  
  document.getElementById("errorTryAgain").focus();
};

function initTreeView() {
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
    for each (var tab in winState.tabs)
      gTreeData.push(tab);
  }, this);
  
  tabList.view = treeView;
  tabList.view.selection.select(0);
}



function restoreSession() {
  document.getElementById("errorTryAgain").disabled = true;
  
  
  var ix = gStateObject.windows.length - 1;
  for (var t = gTreeData.length - 1; t >= 0; t--) {
    if (treeView.isContainer(t)) {
      if (gTreeData[t].checked === 0)
        
        gStateObject.windows[ix].tabs =
          gStateObject.windows[ix].tabs.filter(function(aTabData, aIx)
                                                 gTreeData[t].tabs[aIx].checked);
      else if (!gTreeData[t].checked)
        
        gStateObject.windows.splice(ix, 1);
      ix--;
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
  newWindow.addEventListener("load", function() {
    newWindow.removeEventListener("load", arguments.callee, true);
    ss.setWindowState(newWindow, stateString, true);
    
    var tabbrowser = top.gBrowser;
    var tabIndex = tabbrowser.getBrowserIndexForDocument(document);
    tabbrowser.removeTab(tabbrowser.tabs[tabIndex]);
  }, true);
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
  
  var row = {}, col = {};
  treeView.treeBox.getCellAt(aEvent.clientX, aEvent.clientY, row, col, {});
  if (col.value) {
    
    
#ifdef XP_MACOSX
    let accelKey = aEvent.metaKey;
#else
    let accelKey = aEvent.ctrlKey;
#endif
    if ((aEvent.button == 1 || aEvent.button == 0 && aEvent.detail == 2 || accelKey) &&
        col.value.id == "title" &&
        !treeView.isContainer(row.value)) {
      restoreSingleTab(row.value, aEvent.shiftKey);
      aEvent.stopPropagation();
    }
    else if (col.value.id == "restore")
      toggleRowChecked(row.value);
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
  case KeyEvent.DOM_VK_UP:
  case KeyEvent.DOM_VK_DOWN:
  case KeyEvent.DOM_VK_PAGE_UP:
  case KeyEvent.DOM_VK_PAGE_DOWN:
  case KeyEvent.DOM_VK_HOME:
  case KeyEvent.DOM_VK_END:
    aEvent.preventDefault(); 
    break;
  }
}



function getBrowserWindow() {
  return window.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIWebNavigation)
               .QueryInterface(Ci.nsIDocShellTreeItem).rootTreeItem
               .QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindow);
}

function toggleRowChecked(aIx) {
  var item = gTreeData[aIx];
  item.checked = !item.checked;
  treeView.treeBox.invalidateRow(aIx);
  
  function isChecked(aItem) aItem.checked;
  
  if (treeView.isContainer(aIx)) {
    
    for each (var tab in item.tabs) {
      tab.checked = item.checked;
      treeView.treeBox.invalidateRow(gTreeData.indexOf(tab));
    }
  }
  else {
    
    item.parent.checked = item.parent.tabs.every(isChecked) ? true :
                          item.parent.tabs.some(isChecked) ? 0 : false;
    treeView.treeBox.invalidateRow(gTreeData.indexOf(item.parent));
  }
  
  document.getElementById("errorTryAgain").disabled = !gTreeData.some(isChecked);
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
  _atoms: {},
  _getAtom: function(aName)
  {
    if (!this._atoms[aName]) {
      var as = Cc["@mozilla.org/atom-service;1"].getService(Ci.nsIAtomService);
      this._atoms[aName] = as.getAtom(aName);
    }
    return this._atoms[aName];
  },

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

  getCellProperties: function(idx, column, prop) {
    if (column.id == "restore" && this.isContainer(idx) && gTreeData[idx].checked === 0)
      prop.AppendElement(this._getAtom("partial"));
    if (column.id == "title")
      prop.AppendElement(this._getAtom(this.getImageSrc(idx, column) ? "icon" : "noicon"));
  },

  getRowProperties: function(idx, prop) {
    var winState = gTreeData[idx].parent || gTreeData[idx];
    if (winState.ix % 2 != 0)
      prop.AppendElement(this._getAtom("alternate"));
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
  getColumnProperties: function(column, prop) { }
};
