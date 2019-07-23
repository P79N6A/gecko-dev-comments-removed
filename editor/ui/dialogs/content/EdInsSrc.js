








































function Startup()
{
  var editor = GetCurrentEditor();
  if (!editor)
  {
    window.close();
    return;
  }

  document.documentElement.getButton("accept").removeAttribute("default");

  
  gDialog.srcInput = document.getElementById("srcInput");

  var selection;
  try {
    selection = editor.outputToString("text/html", kOutputFormatted | kOutputSelectionOnly | kOutputWrap);
  } catch (e) {}
  if (selection)
  {
    selection = (selection.replace(/<body[^>]*>/,"")).replace(/<\/body>/,"");
    if (selection)
      gDialog.srcInput.value = selection;
  }
  
  gDialog.srcInput.focus();
  
  SetWindowLocation();
}

function onAccept()
{
  if (gDialog.srcInput.value)
  {
    try {
      GetCurrentEditor().insertHTML(gDialog.srcInput.value);
    } catch (e) {}
  }
  else
  {
    dump("Null value -- not inserting in HTML Source dialog\n");
    return false;
  }
  SaveWindowLocation();

  return true;
}

