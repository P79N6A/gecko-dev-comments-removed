



































var insertNew;
var fieldsetElement;
var newLegend;
var legendElement;



function Startup()
{
  var editor = GetCurrentEditor();
  if (!editor)
  {
    dump("Failed to get active editor!\n");
    window.close();
    return;
  }

  gDialog.editText = document.getElementById("EditText");
  gDialog.legendText = document.getElementById("LegendText");
  gDialog.legendAlign = document.getElementById("LegendAlign");
  gDialog.RemoveFieldSet = document.getElementById("RemoveFieldSet");

  
  const kTagName = "fieldset";
  try {
    
    fieldsetElement = editor.getSelectedElement(kTagName);
    if (!fieldsetElement)
      fieldsetElement = editor.getElementOrParentByTagName(kTagName, editor.selection.anchorNode);
    if (!fieldsetElement)
      fieldsetElement = editor.getElementOrParentByTagName(kTagName, editor.selection.focusNode);
  } catch (e) {}

  if (fieldsetElement)
    
    insertNew = false;
  else
  {
    insertNew = true;

    
    
    try {
      fieldsetElement = editor.createElementWithDefaults(kTagName);
    } catch (e) {}

    if (!fieldsetElement)
    {
      dump("Failed to get selected element or create a new one!\n");
      window.close();
      return;
    }
    
    gDialog.RemoveFieldSet.hidden = true;
  }

  legendElement = fieldsetElement.firstChild;
  if (legendElement && legendElement.localName == "LEGEND")
  {
    newLegend = false;
    var range = editor.document.createRange();
    range.selectNode(legendElement);
    gDialog.legendText.value = range.toString();
    if (/</.test(legendElement.innerHTML))
    {
      gDialog.editText.checked = false;
      gDialog.editText.disabled = false;
      gDialog.legendText.disabled = true;
      gDialog.editText.addEventListener("command", onEditText, false);
      gDialog.RemoveFieldSet.focus();
    }
    else
      SetTextboxFocus(gDialog.legendText);
  }
  else
  {
    newLegend = true;

    
    

    legendElement = editor.createElementWithDefaults("legend");
    if (!legendElement)
    {
      dump("Failed to get selected element or create a new one!\n");
      window.close();
      return;
    }
    SetTextboxFocus(gDialog.legendText);
  }

  
  globalElement = legendElement.cloneNode(false);

  InitDialog();

  SetWindowLocation();
}

function InitDialog()
{
  gDialog.legendAlign.value = GetHTMLOrCSSStyleValue(globalElement, "align", "caption-side");
}

function onEditText()
{
  gDialog.editText.removeEventListener("command", onEditText, false);
  AlertWithTitle(GetString("Alert"), GetString("EditTextWarning"));
}

function RemoveFieldSet()
{
  var editor = GetCurrentEditor();
  editor.beginTransaction();
  try {
    if (!newLegend)
      editor.deleteNode(legendElement);
    RemoveBlockContainer(fieldsetElement);
  } finally {
    editor.endTransaction();
  }
  SaveWindowLocation();
  window.close();
}

function ValidateData()
{
  if (gDialog.legendAlign.value)
    globalElement.setAttribute("align", gDialog.legendAlign.value);
  else
    globalElement.removeAttribute("align");
  return true;
}

function onAccept()
{
  
  ValidateData();

  var editor = GetCurrentEditor();

  editor.beginTransaction();

  try {
    editor.cloneAttributes(legendElement, globalElement);
 
    if (insertNew)
    {
      if (gDialog.legendText.value)
      {
        fieldsetElement.appendChild(legendElement);
        legendElement.appendChild(editor.document.createTextNode(gDialog.legendText.value));
      }
      InsertElementAroundSelection(fieldsetElement);
    }
    else if (gDialog.editText.checked)
    {
      editor.setShouldTxnSetSelection(false);

      if (gDialog.legendText.value)
      {
        if (newLegend)
          editor.insertNode(legendElement, fieldsetElement, 0, true);
        else while (legendElement.firstChild)
          editor.deleteNode(legendElement.lastChild);
        editor.insertNode(editor.document.createTextNode(gDialog.legendText.value), legendElement, 0);
      }
      else if (!newLegend)
        editor.deleteNode(legendElement);

      editor.setShouldTxnSetSelection(true);
    }
  }
  finally {
    editor.endTransaction();
  }

  SaveWindowLocation();

  return true;
}

