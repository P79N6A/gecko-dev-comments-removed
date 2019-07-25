




































const Cu = Components.utils;

Cu.import("resource://testpilot/modules/setup.js");

function showdbcontents() {
  
  var experimentId = window.arguments[0];
  var experiment = TestPilotSetup.getTaskById(experimentId);
  var dataStore = experiment.dataStore;
  var experimentTitle = experiment.title;
  var listbox = document.getElementById("raw-data-listbox");
  var columnNames = dataStore.getHumanReadableColumnNames();
  var propertyNames = dataStore.getPropertyNames();

  
  var dialog = document.getElementById("raw-data-dialog");
  var dialogTitle = dialog.getAttribute("title");
  dialogTitle = dialogTitle + ": " + experimentTitle;
  dialog.setAttribute("title", dialogTitle);

  var i,j;
  
  
  var listcols = document.getElementById("raw-data-listcols");
  var listhead = document.getElementById("raw-data-listhead");
  for (j = 0; j < columnNames.length; j++) {
    listcols.appendChild(document.createElement("listcol"));
    var newHeader = document.createElement("listheader");
    newHeader.setAttribute("label", columnNames[j]);
    listhead.appendChild(newHeader);
  }

  dataStore.getAllDataAsJSON(true, function(rawData) {
    
    for (i = 0; i < rawData.length; i++) {
      var row = document.createElement("listitem");
      for (j = 0; j < columnNames.length; j++) {
        var cell = document.createElement("listcell");
        var value = rawData[i][propertyNames[j]];
        cell.setAttribute("label", value);
        row.appendChild(cell);
      }
      listbox.appendChild(row);
    }
  });
}


function onAccept() {
  return true;
}
