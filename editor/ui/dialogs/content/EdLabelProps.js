



































var labelElement;



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
  gDialog.labelText = document.getElementById("LabelText");
  gDialog.labelFor = document.getElementById("LabelFor");
  gDialog.labelAccessKey = document.getElementById("LabelAccessKey");

  labelElement = window.arguments[0];

  
  globalElement = labelElement.cloneNode(false);

  InitDialog();

  var range = editor.document.createRange();
  range.selectNode(labelElement);
  gDialog.labelText.value = range.toString();

  if (/</.test(labelElement.innerHTML))
  {
    gDialog.editText.checked = false;
    gDialog.editText.disabled = false;
    gDialog.labelText.disabled = true;
    gDialog.editText.addEventListener("command", onEditText, false);
    SetTextboxFocus(gDialog.labelFor);
  }
  else
    SetTextboxFocus(gDialog.labelText);

  SetWindowLocation();
}

function InitDialog()
{
  gDialog.labelFor.value = globalElement.getAttribute("for");
  gDialog.labelAccessKey.value = globalElement.getAttribute("accesskey");
}

function onEditText()
{
  gDialog.editText.removeEventListener("command", onEditText, false);
  AlertWithTitle(GetString("Alert"), GetString("EditTextWarning"));
}

function RemoveLabel()
{
  RemoveContainer(labelElement);
  SaveWindowLocation();
  window.close();
}

function ValidateData()
{
  if (gDialog.labelFor.value)
    globalElement.setAttribute("for", gDialog.labelFor.value);
  else
    globalElement.removeAttribute("for");
  if (gDialog.labelAccessKey.value)
    globalElement.setAttribute("accesskey", gDialog.labelAccessKey.value);
  else
    globalElement.removeAttribute("accesskey");
  return true;
}

function onAccept()
{
  
  ValidateData();

  var editor = GetCurrentEditor();

  editor.beginTransaction();

  try {
    if (gDialog.editText.checked)
    {
      editor.setShouldTxnSetSelection(false);

      while (labelElement.firstChild)
        editor.deleteNode(labelElement.firstChild);
      if (gDialog.labelText.value)
        editor.insertNode(editor.document.createTextNode(gDialog.labelText.value), labelElement, 0);

      editor.setShouldTxnSetSelection(true);
    }

    editor.cloneAttributes(labelElement, globalElement);
  } catch(e) {}

  editor.endTransaction();

  SaveWindowLocation();

  return true;
}

