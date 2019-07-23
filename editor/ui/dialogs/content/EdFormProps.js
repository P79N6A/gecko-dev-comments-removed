



































var gForm;
var insertNew;
var formElement;
var formActionWarning;

function Startup()
{
  var editor = GetCurrentEditor();
  if (!editor)
  {
    dump("Failed to get active editor!\n");
    window.close();
    return;
  }

  gForm = {
    Name:     document.getElementById("FormName"),
    Action:   document.getElementById("FormAction"),
    Method:   document.getElementById("FormMethod"),
    EncType:  document.getElementById("FormEncType"),
    Target:   document.getElementById("FormTarget")
  }
  gDialog.MoreSection = document.getElementById("MoreSection");
  gDialog.MoreFewerButton = document.getElementById("MoreFewerButton");
  gDialog.RemoveForm = document.getElementById("RemoveForm")

  
  const kTagName = "form";
  try {
    formElement = editor.getSelectedElement(kTagName);
    if (!formElement)
      formElement = editor.getElementOrParentByTagName(kTagName, editor.selection.anchorNode);
    if (!formElement)
      formElement = editor.getElementOrParentByTagName(kTagName, editor.selection.focusNode);
  } catch (e) {}

  if (formElement)
  {
    
    insertNew = false;
    formActionWarning = formElement.hasAttribute("action");
  }
  else
  {
    insertNew = true;
    formActionWarning = true;

    
    
    try {
      formElement = editor.createElementWithDefaults(kTagName);
    } catch (e) {}

    if (!formElement)
    {
      dump("Failed to get selected element or create a new one!\n");
      window.close();
      return;
    }
    
    gDialog.RemoveForm.hidden = true;
  }

  
  globalElement = formElement.cloneNode(false);

  InitDialog();

  InitMoreFewer();

  SetTextboxFocus(gForm.Name);

  SetWindowLocation();
}

function InitDialog()
{
  for (var attribute in gForm)
    gForm[attribute].value = globalElement.getAttribute(attribute);
}

function RemoveForm()
{
  RemoveBlockContainer(formElement);
  SaveWindowLocation();
  window.close();
}

function ValidateData()
{
  for (var attribute in gForm)
  {
    if (gForm[attribute].value)
      globalElement.setAttribute(attribute, gForm[attribute].value);
    else
      globalElement.removeAttribute(attribute);
  }
  return true;
}

function onAccept()
{
  if (formActionWarning && !gForm.Action.value)
  {
    AlertWithTitle(GetString("Alert"), GetString("NoFormAction"));
    gForm.Action.focus();
    formActionWarning = false;
    return false;
  }
  
  
  ValidateData();

  var editor = GetCurrentEditor();

  editor.cloneAttributes(formElement, globalElement);

  if (insertNew)
    InsertElementAroundSelection(formElement);

  SaveWindowLocation();

  return true;
}
