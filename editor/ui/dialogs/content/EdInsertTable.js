





































var gTableElement = null;
var gRows;
var gColumns;
var gActiveEditor;


function Startup()
{
  gActiveEditor = GetCurrentTableEditor();
  if (!gActiveEditor)
  {
    dump("Failed to get active editor!\n");
    window.close();
    return;
  }

  try {
    gTableElement = gActiveEditor.createElementWithDefaults("table");
  } catch (e) {}

  if(!gTableElement)
  {
    dump("Failed to create a new table!\n");
    window.close();
    return;
  }
  gDialog.rowsInput    = document.getElementById("rowsInput");
  gDialog.columnsInput = document.getElementById("columnsInput");
  gDialog.widthInput = document.getElementById("widthInput");
  gDialog.borderInput = document.getElementById("borderInput");
  gDialog.widthPixelOrPercentMenulist = document.getElementById("widthPixelOrPercentMenulist");
  gDialog.OkButton = document.documentElement.getButton("accept");

  
  globalElement = gTableElement.cloneNode(false);
  try {
    if (GetPrefs().getBoolPref("editor.use_css") && IsHTMLEditor()
        && !(gActiveEditor.flags & Components.interfaces.nsIPlaintextEditor.eEditorMailMask))
    {
      
      globalElement.setAttribute("style", "text-align: left;");
    }
  } catch (e) {}

  
  InitDialog();

  
  
  
  
  
  gDialog.rowsInput.value = 2;
  gDialog.columnsInput.value = 2;

  
  if (gDialog.widthInput.value.length == 0)
  {
    gDialog.widthInput.value = "100";
    gDialog.widthPixelOrPercentMenulist.selectedIndex = 1;
  }

  SetTextboxFocusById("rowsInput");

  SetWindowLocation();
}




function InitDialog()
{  
  
  
  
  
  gDialog.widthInput.value = InitPixelOrPercentMenulist(globalElement, null, "width", "widthPixelOrPercentMenulist", gPercent);
  gDialog.borderInput.value = globalElement.getAttribute("border");
}

function ChangeRowOrColumn(id)
{
  
  forceInteger(id);

  
  var enable = gDialog.rowsInput.value.length > 0 && 
                              gDialog.rowsInput.value > 0 &&
                              gDialog.columnsInput.value.length > 0 &&
                              gDialog.columnsInput.value > 0;

  SetElementEnabled(gDialog.OkButton, enable);
  SetElementEnabledById("AdvancedEditButton1", enable);
}




function ValidateData()
{
  gRows = ValidateNumber(gDialog.rowsInput, null, 1, gMaxRows, null, null, true)
  if (gValidationError)
    return false;

  gColumns = ValidateNumber(gDialog.columnsInput, null, 1, gMaxColumns, null, null, true)
  if (gValidationError)
    return false;

  
  ValidateNumber(gDialog.borderInput, null, 0, gMaxPixels, globalElement, "border", false);
  
  if (gValidationError) return false;

  ValidateNumber(gDialog.widthInput, gDialog.widthPixelOrPercentMenulist,
                 1, gMaxTableSize, globalElement, "width", false);
  if (gValidationError)
    return false;

  return true;
}


function onAccept()
{
  if (ValidateData())
  {
    gActiveEditor.beginTransaction();
    try {
      gActiveEditor.cloneAttributes(gTableElement, globalElement);

      
      var tableBody = gActiveEditor.createElementWithDefaults("tbody");
      if (tableBody)
      {
        gTableElement.appendChild(tableBody);

        
        for (var i = 0; i < gRows; i++)
        {
          var newRow = gActiveEditor.createElementWithDefaults("tr");
          if (newRow)
          {
            tableBody.appendChild(newRow);
            for (var j = 0; j < gColumns; j++)
            {
              var newCell = gActiveEditor.createElementWithDefaults("td");
              if (newCell)
              {
                newRow.appendChild(newCell);
              }
            }
          }
        }
      }
      
        
      var tagNameObj = { value: "" };
      var countObj = { value: 0 };
      var element = gActiveEditor.getSelectedOrParentTableElement(tagNameObj, countObj);
      var deletePlaceholder = false;

      if (tagNameObj.value == "table")
      {
        
        gActiveEditor.deleteTable();
      }
      else if (tagNameObj.value == "td")
      {
        if (countObj.value >= 1)
        {
          if (countObj.value > 1)
          {
            
            
            
            gActiveEditor.joinTableCells(false);
          
            
            element = gActiveEditor.getFirstSelectedCell();
          
            
            gActiveEditor.selection.collapse(element,0);
          }

          if (element)
          {
            
            gActiveEditor.deleteTableCellContents();
          
            
            gActiveEditor.selection.collapse(element,0);
            
            deletePlaceholder = true;
          }
        }
      }

      
      gActiveEditor.insertElementAtSelection(gTableElement, true);

      if (deletePlaceholder && gTableElement && gTableElement.nextSibling)
      {
        
        gActiveEditor.deleteNode(gTableElement.nextSibling);
      }
    } catch (e) {}

    gActiveEditor.endTransaction();

    SaveWindowLocation();
    return true;
  }
  return false;
}
