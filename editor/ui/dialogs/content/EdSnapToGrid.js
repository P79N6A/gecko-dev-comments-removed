




































var gPrefs = GetPrefs();
var gEditor;


function Startup()
{
  gEditor = GetCurrentEditor();
  if (!gEditor)
  {
    window.close();
    return;
  }

  gEditor instanceof Components.interfaces.nsIHTMLAbsPosEditor;

  gDialog.enableSnapToGrid = document.getElementById("enableSnapToGrid");
  gDialog.sizeInput        = document.getElementById("size");
  gDialog.sizeLabel        = document.getElementById("sizeLabel");
  gDialog.unitLabel        = document.getElementById("unitLabel");

  
  InitDialog()

  
  SetTextboxFocus(gDialog.sizeInput);

  
  window.sizeToContent();

  SetWindowLocation();
}




function InitDialog()
{
  gDialog.enableSnapToGrid.checked = gEditor.snapToGridEnabled;
  toggleSnapToGrid();

  gDialog.sizeInput.value = gEditor.gridSize;
}

function onAccept()
{
  gEditor.snapToGridEnabled = gDialog.enableSnapToGrid.checked;
  gEditor.gridSize = gDialog.sizeInput.value;

  return true;
}

function toggleSnapToGrid()
{
  SetElementEnabledById("size", gDialog.enableSnapToGrid.checked)
  SetElementEnabledById("sizeLabel", gDialog.enableSnapToGrid.checked)
  SetElementEnabledById("unitLabel", gDialog.enableSnapToGrid.checked)
}
