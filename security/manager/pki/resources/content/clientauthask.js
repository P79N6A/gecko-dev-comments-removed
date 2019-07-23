







































const nsIDialogParamBlock = Components.interfaces.nsIDialogParamBlock;

var dialogParams;
var itemCount = 0;

function onLoad()
{
    var cn;
    var org;
    var issuer;

    dialogParams = window.arguments[0].QueryInterface(nsIDialogParamBlock);
    cn = dialogParams.GetString(0);
    org = dialogParams.GetString(1);
    issuer = dialogParams.GetString(2);

    var bundle = srGetStrBundle("chrome://pippki/locale/pippki.properties");
    var message1 = bundle.formatStringFromName("clientAuthMessage1", 
                                             [org],
                                             1);
    var message2 = bundle.formatStringFromName("clientAuthMessage2",
                                             [issuer],
                                             1);
    setText("hostname", cn);
    setText("organization", message1);
    setText("issuer", message2);

    var selectElement = document.getElementById("nicknames");
    itemCount = dialogParams.GetInt(0);
    for (var i=0; i < itemCount; i++) {
        var menuItemNode = document.createElement("menuitem");
        var nick = dialogParams.GetString(i+3);
        menuItemNode.setAttribute("value", i);
        menuItemNode.setAttribute("label", nick); 
        selectElement.firstChild.appendChild(menuItemNode);
        if (i == 0) {
            selectElement.selectedItem = menuItemNode;
        }
    }

    setDetails();
}

function setDetails()
{
  var index = parseInt(document.getElementById("nicknames").value);
  var details = dialogParams.GetString(index+itemCount+3);
  document.getElementById("details").value = details;
}

function onCertSelected()
{
  setDetails();
}

function doOK()
{
  dialogParams.SetInt(0,1);
  var index = parseInt(document.getElementById("nicknames").value);
  dialogParams.SetInt(1, index);
  return true;
}

function doCancel()
{
  dialogParams.SetInt(0,0);
  return true;
}
