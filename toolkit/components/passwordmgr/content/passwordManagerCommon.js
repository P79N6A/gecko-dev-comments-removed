







var kObserverService;


var passwordmanager     = null;


var signons             = [];
var rejects             = [];
var deletedSignons      = [];
var deletedRejects      = [];

var signonsTree;
var rejectsTree;

var showingPasswords = false;

function Startup() {
  
  passwordmanager = Components.classes["@mozilla.org/login-manager;1"]
                        .getService(Components.interfaces.nsILoginManager);

  
  kObserverService = Components.classes["@mozilla.org/observer-service;1"].getService(Components.interfaces.nsIObserverService);
  kObserverService.addObserver(signonReloadDisplay, "passwordmgr-storage-changed", false);

  signonsTree = document.getElementById("signonsTree");
  rejectsTree = document.getElementById("rejectsTree");
}

function Shutdown() {
  kObserverService.removeObserver(signonReloadDisplay, "passwordmgr-storage-changed");
}

var signonReloadDisplay = {
  observe: function(subject, topic, data) {
    if (topic == "passwordmgr-storage-changed") {
      switch (data) {
        case "addLogin":
        case "modifyLogin":
        case "removeLogin":
        case "removeAllLogins":
          if (!signonsTree) {
            return;
          }
          signons.length = 0;
          LoadSignons();
          
          if (document.getElementById("filter") && document.getElementById("filter").value != "") {
            _filterPasswords();
          }
          break;
        case "hostSavingEnabled":
        case "hostSavingDisabled":
          if (!rejectsTree) {
            return;
          }
          rejects.length = 0;
          if (lastRejectSortColumn == "hostname") {
            lastRejectSortAscending = !lastRejectSortAscending; 
          }
          LoadRejects();
          break;
      }
      kObserverService.notifyObservers(null, "passwordmgr-dialog-updated", null);
    }
  }
}



function DeleteAllFromTree(tree, view, table, deletedTable, removeButton, removeAllButton) {

  
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

  
  tree.view.selection.selectEventsSuppressed = true;

  
  var selections = GetTreeSelections(tree);
  for (var s=selections.length-1; s>= 0; s--) {
    var i = selections[s];
    deletedTable[deletedTable.length] = table[i];
    table[i] = null;
  }

  
  for (var j=0; j<table.length; j++) {
    if (table[j] == null) {
      var k = j;
      while ((k < table.length) && (table[k] == null)) {
        k++;
      }
      table.splice(j, k-j);
      view.rowCount -= k - j;
      tree.treeBoxObject.rowCountChanged(j, j - k);
    }
  }

  
  if (table.length) {
    
    var nextSelection = (selections[0] < table.length) ? selections[0] : table.length-1;
    tree.view.selection.select(nextSelection);
    tree.treeBoxObject.ensureRowIsVisible(nextSelection);
  } else {
    
    document.getElementById(removeButton).setAttribute("disabled", "true")
    document.getElementById(removeAllButton).setAttribute("disabled","true");
  }
  tree.view.selection.selectEventsSuppressed = false;
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
  var firstLower, secondLower;

  
  if (first.hostname) {
    firstLower  = first.hostname.toLowerCase();
    secondLower = second.hostname.toLowerCase();
  } else {
    firstLower  = first.toLowerCase();
    secondLower = second.toLowerCase();
  }

  if (firstLower < secondLower) {
    return -1;
  }

  if (firstLower > secondLower) {
    return 1;
  }

  return 0;
}
