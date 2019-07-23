





































function Startup()
{
  if (!GetCurrentEditor())
  {
    window.close();
    return;
  }
  
  SetWindowLocation();
}

function KeepCurrentPage()
{
  
  
  SaveWindowLocation();
  return true;
}

function UseOtherPage()
{
  
  setTimeout("window.opener.EditorLoadUrl(GetDocumentUrl())", 10);
  SaveWindowLocation();
  return true;
}

function PreventCancel()
{
  SaveWindowLocation();

  
  return false;
}
