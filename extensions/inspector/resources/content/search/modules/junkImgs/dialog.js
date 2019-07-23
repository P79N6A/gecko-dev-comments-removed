








var dialog;



window.addEventListener("load", JunkImgsDialog_initialize, false);

function JunkImgsDialog_initialize()
{
  dialog = new JunkImgsDialog();
  doSetOKCancel(gDoOKFunc);
}




function JunkImgsDialog()
{
}

JunkImgsDialog.prototype = 
{
  browse: function()
  {
    var txf = document.getElementById("txfSearchPath");
    var defaultPath = txf.getAttribute("value");
    var file = FilePickerUtils.pickDir("Select Search Path", defaultPath);
    if (file)
      txf.setAttribute("value", file.path);
  }
};




function gDoOKFunc() {
  window.close();
  window.arguments[0].processDialog(window);

  return true;
}

