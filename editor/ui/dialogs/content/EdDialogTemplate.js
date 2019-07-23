






































var insertNew = true;
var tagname = "TAG NAME"


function Startup()
{
  if (!GetCurrentEditor())
  {
    window.close();
    return;
  }
  
  
  gDialog.fooButton = document.getElementById("fooButton");

  initDialog();
  
  
  SetWindowLocation();

  
  SetTextboxFocus(gDialog.fooButton);
}

function InitDialog() 
{
  
  
}

function onAccept()
{
  
  
  
  SaveWindowLocation();
  return true; 
}
