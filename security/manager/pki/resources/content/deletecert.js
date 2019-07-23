




































const nsIX509Cert = Components.interfaces.nsIX509Cert;
const nsX509CertDB = "@mozilla.org/security/x509certdb;1";
const nsIX509CertDB = Components.interfaces.nsIX509CertDB;
const nsIPKIParamBlock = Components.interfaces.nsIPKIParamBlock;
const nsIDialogParamBlock = Components.interfaces.nsIDialogParamBlock;

var certdb;
var certs = [];
var gParams;

function setWindowName()
{
  gParams = window.arguments[0].QueryInterface(nsIDialogParamBlock);
  
  
  certdb = Components.classes[nsX509CertDB].getService(nsIX509CertDB);
  
  var typeFlag = gParams.GetString(0);
  var numberOfCerts = gParams.GetInt(0);
  var dbkey;
  for(var x=0; x<numberOfCerts;x++)
  {
     dbkey = gParams.GetString(x+1);
     certs[x] = certdb.findCertByDBKey(dbkey , null);
  }
  
  var bundle = srGetStrBundle("chrome://pippki/locale/pippki.properties");
  var title;
  var confirm;
  var impact;
  
  if(typeFlag == "mine_tab")
  {
     title = bundle.GetStringFromName("deleteUserCertTitle");
     confirm = bundle.GetStringFromName("deleteUserCertConfirm");
     impact = bundle.GetStringFromName("deleteUserCertImpact");
  }
  else if(typeFlag == "websites_tab")
  {
     title = bundle.GetStringFromName("deleteSslCertTitle");
     confirm = bundle.GetStringFromName("deleteSslCertConfirm");
     impact = bundle.GetStringFromName("deleteSslCertImpact");
  }
  else if(typeFlag == "ca_tab")
  {
     title = bundle.GetStringFromName("deleteCaCertTitle");
     confirm = bundle.GetStringFromName("deleteCaCertConfirm");
     impact = bundle.GetStringFromName("deleteCaCertImpact");
  }
  else if(typeFlag == "others_tab")
  {
     title = bundle.GetStringFromName("deleteEmailCertTitle");
     confirm = bundle.GetStringFromName("deleteEmailCertConfirm");
     impact = bundle.GetStringFromName("deleteEmailCertImpactDesc");
  }
  else if(typeFlag == "orphan_tab")
  {
     title = bundle.GetStringFromName("deleteOrphanCertTitle");
     confirm = bundle.GetStringFromName("deleteOrphanCertConfirm");
     impact = "";
  }
  else
  {
     return;
  }
  var confirReference = document.getElementById('confirm');
  var impactReference = document.getElementById('impact');
  document.title = title;
  
  setText("confirm",confirm);

  var box=document.getElementById("certlist");
  var text;
  for(x=0;x<certs.length;x++)
  {
    if (!certs[x])
      continue;
    text = document.createElement("text");
    text.setAttribute("value",certs[x].commonName);
    box.appendChild(text);
  }

  setText("impact",impact);
}

function doOK()
{
  
  

  for(var i=0;i<certs.length;i++)
  {
    if (certs[i]) {
      try {
        certdb.deleteCertificate(certs[i]);
      }
      catch (e) {
        gParams.SetString(i+1, "");
      }
      certs[i] = null;
    }
  }
  gParams.SetInt(1, 1); 
  return true;
}

function doCancel()
{
  var numberOfCerts = gParams.GetInt(0);
  for(var x=0; x<numberOfCerts;x++)
  {
     gParams.SetString(x+1, "");
  }
  gParams.SetInt(1, 0); 
  return true;
}
