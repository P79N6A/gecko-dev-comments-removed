











































function setText(id, value) {
  var element = document.getElementById(id);
  if (!element) return;
     if (element.hasChildNodes())
       element.removeChild(element.firstChild);
  var textNode = document.createTextNode(value);
  element.appendChild(textNode);
}

const nsICertificateDialogs = Components.interfaces.nsICertificateDialogs;
const nsCertificateDialogs = "@mozilla.org/nsCertificateDialogs;1"

function viewCertHelper(parent, cert) {
  if (!cert)
    return;

  var cd = Components.classes[nsCertificateDialogs].getService(nsICertificateDialogs);
  cd.viewCert(parent, cert);
}
