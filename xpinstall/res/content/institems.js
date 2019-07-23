







































var gParam;
var gBundle;

function addTreeItem(num, aName, aUrl, aCertName)
{
  
  var item = document.createElement("description");
  item.setAttribute("value", aName);
  item.setAttribute("tooltiptext", aName);
  item.setAttribute("class", "confirmName");
  item.setAttribute("crop", "center");

  
  var certName = document.createElement("description");
  var certNameValue = aCertName ? aCertName : gBundle.getString("Unsigned");
  certName.setAttribute("value", certNameValue);
  certName.setAttribute("tooltiptext", certNameValue);
  certName.setAttribute("crop", "center");

  
  var urltext = aUrl.replace(/^([^:]*:\/*[^\/]+).*/, "$1");
  var url = document.createElement("description");
  url.setAttribute("value", aUrl);
  url.setAttribute("tooltiptext", aUrl);
  url.setAttribute("class", "confirmURL");
  url.setAttribute("crop", "center");

  
  var row = document.createElement("row");
  row.appendChild(item);
  row.appendChild(certName);
  row.appendChild(url);

  document.getElementById("xpirows").appendChild(row);
}

function onLoad()
{
  var row = 0;
  var moduleName, URL, IconURL, certName, numberOfDialogTreeElements;

  gBundle = document.getElementById("xpinstallBundle");
  gParam = window.arguments[0].QueryInterface(Components.interfaces.nsIDialogParamBlock);

  gParam.SetInt(0, 1); 

  numberOfDialogTreeElements = gParam.GetInt(1);

  for (var i=0; i < numberOfDialogTreeElements; i++)
  {
    moduleName = gParam.GetString(i);
    URL = gParam.GetString(++i);
    IconURL = gParam.GetString(++i); 
    certName = gParam.GetString(++i);

    addTreeItem(row++, moduleName, URL, certName);
  }

  
  var aButton = document.documentElement.getButton("accept");
  aButton.setAttribute("default", false);
  aButton.setAttribute("label", gBundle.getString("OK"));
  aButton.setAttribute("disabled", true);

  aButton = document.documentElement.getButton("cancel");
  aButton.focus();
  aButton.setAttribute("default", true);

  
  var delayInterval = 2000;
  try {
    var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                .getService(Components.interfaces.nsIPrefBranch);
    delayInterval = prefs.getIntPref("security.dialog_enable_delay");
  } catch (e) {}
  setTimeout(reenableInstallButtons, delayInterval);
}

function reenableInstallButtons()
{
    document.documentElement.getButton("accept").setAttribute("disabled", false);
}

function onAccept()
{
  
  if (gParam)
    gParam.SetInt(0, 0);

  return true;
}

function onCancel()
{
  
  if (gParam)
    gParam.SetInt(0, 1);

  return true;
}
