# -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
#
# The contents of this file are subject to the Netscape Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http:
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is Mozilla Communicator client code, released March
# 31, 1998.
#
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are
# Copyright (C) 1998 Netscape Communications Corporation. All
# Rights Reserved.
#
# Contributor(s): 
#   Ben "Count XULula" Goodger
#   Brian Ryner <bryner@brianryner.com>



var kObserverService;
var kSignonBundle;
var gSelectUserInUse = false;


var passwordmanager     = null;


var signons             = [];
var rejects             = [];
var deletedSignons      = [];
var deletedRejects      = [];

var showingPasswords = false;

function Startup() {
  
  passwordmanager = Components.classes["@mozilla.org/passwordmanager;1"].getService(Components.interfaces.nsIPasswordManager);

  kSignonBundle = document.getElementById("signonBundle");

  
  kObserverService = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
  kObserverService.addObserver(signonReloadDisplay, "signonChanged", false);

  
  kObserverService.addObserver(signonReloadDisplay, "signonSelectUser", false);

  signonsTree = document.getElementById("signonsTree");
  rejectsTree = document.getElementById("rejectsTree");

  
  var tabBox = document.getElementById("tabbox");
  tabBox.selectedTab = document.getElementById("signonsTab");

  
  document.getElementById("togglePasswords").label = kSignonBundle.getString(showingPasswords ? "hidePasswords" : "showPasswords");
  document.documentElement.getButton("accept").label = kSignonBundle.getString("close");

  
  if (!LoadSignons()) {
    return; 
  }
  LoadRejects();
}

function Shutdown() {
  kObserverService.removeObserver(signonReloadDisplay, "signonChanged");
  kObserverService.removeObserver(signonReloadDisplay, "signonSelectUser");
}

var signonReloadDisplay = {
  observe: function(subject, topic, state) {
    if (topic == "signonChanged") {
      if (state == "signons") {
        signons.length = 0;
        if (lastSignonSortColumn == "host") {
          lastSignonSortAscending = !lastSignonSortAscending; 
        }
        LoadSignons();
      } else if (state == "rejects") {
        rejects.length = 0;
        if (lastRejectSortColumn == "host") {
          lastRejectSortAscending = !lastRejectSortAscending; 
        }
        LoadRejects();
      }
    } else if (topic == "signonSelectUser") {
      if (state == "suspend") {
        gSelectUserInUse = true;
        document.getElementById("removeSignon").disabled = true;
        document.getElementById("removeAllSignons").disabled = true;
        document.getElementById("togglePasswords").disabled = true;
      } else if (state == "resume") {
        gSelectUserInUse = false;
        var selections = GetTreeSelections(signonsTree);
        if (selections.length > 0) {
          document.getElementById("removeSignon").disabled = false;
        }
        if (signons.length > 0) {
          document.getElementById("removeAllSignons").disabled = false;
          document.getElementById("togglePasswords").disabled = false;
        }
      } else if (state == "inUse") {
        gSelectUserInUse = true;
      }
    }
  }
}



var signonsTreeView = {
  rowCount : 0,
  setTree : function(tree) {},
  getImageSrc : function(row,column) {},
  getProgressMode : function(row,column) {},
  getCellValue : function(row,column) {},
  getCellText : function(row,column) {
    var rv="";
    if (column.id=="siteCol") {
      rv = signons[row].host;
    } else if (column.id=="userCol") {
      rv = signons[row].user;
    } else if (column.id=="passwordCol") {
      rv = signons[row].password;
    }
    return rv;
  },
  isSeparator : function(index) { return false; },
  isSorted : function() { return false; },
  isContainer : function(index) { return false; },
  cycleHeader : function(column) {},
  getRowProperties : function(row,prop) {},
  getColumnProperties : function(column,prop) {},
  getCellProperties : function(row,column,prop) {}
 };
var signonsTree;

function Signon(number, host, user, rawuser, password) {
  this.number = number;
  this.host = host;
  this.user = user;
  this.rawuser = rawuser;
  this.password = password;
}

function LoadSignons() {
  
  var enumerator = passwordmanager.enumerator;
  var count = 0;

  while (enumerator.hasMoreElements()) {
    var nextPassword;
    try {
      nextPassword = enumerator.getNext();
    } catch(e) {
      
      window.close();
      return false;
    }
    nextPassword = nextPassword.QueryInterface(Components.interfaces.nsIPassword);
    var host = nextPassword.host;
    var user;
    var password;
    
    try {
      user = nextPassword.user;
      password = nextPassword.password;
    } catch (e) {
      
      dump("could not decrypt user/password for host " + host + "\n");
      continue;
    }
    var rawuser = user;

    
    if (user == "") {
      var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                    .getService(Components.interfaces.nsIIOService);
      try {
        user = ioService.newURI(host, null, null).username;
        if (user == "") {
          user = "<>";
        }
      } catch(e) {
        user = "<>";
      }
    }

    signons[count] = new Signon(count++, host, user, rawuser, password);
  }
  signonsTreeView.rowCount = signons.length;

  
  signonsTree.treeBoxObject.view = signonsTreeView;
  SignonColumnSort('host');

  
  var element = document.getElementById("removeAllSignons");
  var toggle = document.getElementById("togglePasswords");
  if (signons.length == 0 || gSelectUserInUse) {
    element.setAttribute("disabled","true");
    toggle.setAttribute("disabled","true");
  } else {
    element.removeAttribute("disabled");
    toggle.removeAttribute("disabled");
  }
 
  return true;
}

function SignonSelected() {
  var selections = GetTreeSelections(signonsTree);
  if (selections.length && !gSelectUserInUse) {
    document.getElementById("removeSignon").removeAttribute("disabled");
  }
}

function DeleteSignon() {
  DeleteSelectedItemFromTree(signonsTree, signonsTreeView,
                             signons, deletedSignons,
                             "removeSignon", "removeAllSignons");
  FinalizeSignonDeletions();
}

function DeleteAllSignons() {
  DeleteAllFromTree(signonsTree, signonsTreeView,
                        signons, deletedSignons,
                        "removeSignon", "removeAllSignons");
  FinalizeSignonDeletions();
}

function TogglePasswordVisible() {
  if (!showingPasswords && !ConfirmShowPasswords())
    return;

  showingPasswords = !showingPasswords;
  document.getElementById("togglePasswords").label = kSignonBundle.getString(showingPasswords ? "hidePasswords" : "showPasswords");
  document.getElementById("passwordCol").hidden = !showingPasswords;
}

function AskUserShowPasswords() {
  var prompter = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
  var dummy = { value: false };

  
  return prompter.confirmEx(window,
          null,
          kSignonBundle.getString("noMasterPasswordPrompt"),
          prompter.BUTTON_TITLE_YES * prompter.BUTTON_POS_0 + prompter.BUTTON_TITLE_NO * prompter.BUTTON_POS_1,
          null, null, null, null, dummy) == 0;    
}

function ConfirmShowPasswords() {
  
  var tokendb = Components.classes["@mozilla.org/security/pk11tokendb;1"]
                    .createInstance(Components.interfaces.nsIPK11TokenDB);
  var token = tokendb.getInternalKeyToken();

  
  if (token.checkPassword(""))
    return AskUserShowPasswords();

  
  try {
    
    token.login(true);  
                        
  } catch (e) {
    
    
  }

  return token.isLoggedIn();
}

function FinalizeSignonDeletions() {
  for (var s=0; s<deletedSignons.length; s++) {
    passwordmanager.removeUser(deletedSignons[s].host, deletedSignons[s].rawuser);
  }
  deletedSignons.length = 0;
}

function HandleSignonKeyPress(e) {
  if (e.keyCode == 46) {
    DeleteSignonSelected();
  }
}

var lastSignonSortColumn = "";
var lastSignonSortAscending = false;

function SignonColumnSort(column) {
  lastSignonSortAscending =
    SortTree(signonsTree, signonsTreeView, signons,
                 column, lastSignonSortColumn, lastSignonSortAscending);
  lastSignonSortColumn = column;
}



var rejectsTreeView = {
  rowCount : 0,
  setTree : function(tree){},
  getImageSrc : function(row,column) {},
  getProgressMode : function(row,column) {},
  getCellValue : function(row,column) {},
  getCellText : function(row,column){
    var rv="";
    if (column.id=="rejectCol") {
      rv = rejects[row].host;
    }
    return rv;
  },
  isSeparator : function(index) {return false;},
  isSorted: function() { return false; },
  isContainer : function(index) {return false;},
  cycleHeader : function(column) {},
  getRowProperties : function(row,prop) {},
  getColumnProperties : function(column,prop) {},
  getCellProperties : function(row,column,prop) {}
 };
var rejectsTree;

function Reject(number, host) {
  this.number = number;
  this.host = host;
}

function LoadRejects() {
  var enumerator = passwordmanager.rejectEnumerator;
  var count = 0;
  while (enumerator.hasMoreElements()) {
    var nextReject = enumerator.getNext();
    nextReject = nextReject.QueryInterface(Components.interfaces.nsIPassword);
    var host = nextReject.host;
    rejects[count] = new Reject(count++, host);
  }
  rejectsTreeView.rowCount = rejects.length;

  
  rejectsTree.treeBoxObject.view = rejectsTreeView;
  RejectColumnSort('host');

  var element = document.getElementById("removeAllRejects");
  if (rejects.length == 0) {
    element.setAttribute("disabled","true");
  } else {
    element.removeAttribute("disabled");
  }
}

function RejectSelected() {
  var selections = GetTreeSelections(rejectsTree);
  if (selections.length) {
    document.getElementById("removeReject").removeAttribute("disabled");
  }
}

function DeleteReject() {
  DeleteSelectedItemFromTree(rejectsTree, rejectsTreeView,
                                 rejects, deletedRejects,
                                 "removeReject", "removeAllRejects");
  FinalizeRejectDeletions();
}

function DeleteAllRejects() {
  DeleteAllFromTree(rejectsTree, rejectsTreeView,
                        rejects, deletedRejects,
                        "removeReject", "removeAllRejects");
  FinalizeRejectDeletions();
}

function FinalizeRejectDeletions() {
  for (var r=0; r<deletedRejects.length; r++) {
    passwordmanager.removeReject(deletedRejects[r].host);
  }
  deletedRejects.length = 0;
}

function HandleRejectKeyPress(e) {
  if (e.keyCode == 46) {
    DeleteRejectSelected();
  }
}

var lastRejectSortColumn = "";
var lastRejectSortAscending = false;

function RejectColumnSort(column) {
  lastRejectSortAscending =
    SortTree(rejectsTree, rejectsTreeView, rejects,
                 column, lastRejectSortColumn, lastRejectSortAscending);
  lastRejectSortColumn = column;
}




function TrimString(string)
{
  if (!string) {
    return "";
  }
  return string.replace(/(^\s+)|(\s+$)/g, '')
}

function DeleteAllFromTree
    (tree, view, table, deletedTable, removeButton, removeAllButton) {

  
  for (var i=0; i<table.length; i++) {
    deletedTable[deletedTable.length] = table[i];
  }
  table.length = 0;

  
  view.selection.select(-1); 

  
  view.rowCount = 0;

  var box = tree.treeBoxObject;
  box.rowCountChanged(0, -deletedTable.length);
  box.invalidate();


  
  document.getElementById(removeButton).setAttribute("disabled", "true")
  document.getElementById(removeAllButton).setAttribute("disabled","true");
}

function DeleteSelectedItemFromTree
    (tree, view, table, deletedTable, removeButton, removeAllButton) {

  var box = tree.treeBoxObject;

  
  

  var selection = box.view.selection;
  var oldSelectStart = table.length;
  box.beginUpdateBatch();

  var selCount = selection.getRangeCount();
  var min = new Object();
  var max = new Object();

  for (var s = 0; s < selCount; ++s) {
    selection.getRangeAt(s, min, max);
    var minVal = min.value;
    var maxVal = max.value;

    oldSelectStart = minVal < oldSelectStart ? minVal : oldSelectStart;

    var rowCount = maxVal - minVal + 1;
    view.rowCount -= rowCount;
    box.rowCountChanged(minVal, -rowCount);

    for (var i = minVal; i <= maxVal; ++i) {
      deletedTable[deletedTable.length] = table[i];
      table[i] = null;
    }
  }

  
  for (var j = 0; j < table.length; ++j) {
    if (!table[j]) {
      var k = j;
      while (k < table.length && !table[k])
        k++;

      table.splice(j, k-j);
    }
  }

  box.endUpdateBatch();

  
  var removeButton = document.getElementById(removeButton);
  var removeAllButton = document.getElementById(removeAllButton);

  if (table.length) {
    removeButton.removeAttribute("disabled");
    removeAllButton.removeAttribute("disabled");

    selection.select(oldSelectStart < table.length ? oldSelectStart : table.length - 1);
  } else {
    removeButton.setAttribute("disabled", "true");
    removeAllButton.setAttribute("disabled", "true");
  }
}

function GetTreeSelections(tree) {
  var selections = [];
  var select = tree.view.selection;
  if (select) {
    var count = select.getRangeCount();
    var min = new Object();
    var max = new Object();
    for (var i=0; i<count; i++) {
      select.getRangeAt(i, min, max);
      for (var k=min.value; k<=max.value; k++) {
        if (k != -1) {
          selections[selections.length] = k;
        }
      }
    }
  }
  return selections;
}

function SortTree(tree, view, table, column, lastSortColumn, lastSortAscending, updateSelection) {

  
  var selections = GetTreeSelections(tree);
  var selectedNumber = selections.length ? table[selections[0]].number : -1;

  
  var ascending = (column == lastSortColumn) ? !lastSortAscending : true;

  
  var compareFunc;
  if (ascending) {
    compareFunc = function compare(first, second) {
      return CompareLowerCase(first[column], second[column]);
    }
  } else {
    compareFunc = function compare(first, second) {
      return CompareLowerCase(second[column], first[column]);
    }
  }
  table.sort(compareFunc);

  
  var selectedRow = -1;
  if (selectedNumber>=0 && updateSelection) {
    for (var s=0; s<table.length; s++) {
      if (table[s].number == selectedNumber) {
        
        
        tree.view.selection.select(-1);
        tree.view.selection.select(s);
        selectedRow = s;
        break;
      }
    }
  }

  
  tree.treeBoxObject.invalidate();
  if (selectedRow >= 0) {
    tree.treeBoxObject.ensureRowIsVisible(selectedRow)
  }

  return ascending;
}




function CompareLowerCase(first, second) {

  var firstLower  = first.toLowerCase();
  var secondLower = second.toLowerCase();

  if (firstLower < secondLower) {
    return -1;
  }

  if (firstLower > secondLower) {
    return 1;
  }

  return 0;
}
