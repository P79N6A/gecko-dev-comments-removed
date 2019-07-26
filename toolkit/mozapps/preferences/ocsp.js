





var gOCSPDialog = {
  _updateUI: function (called_by) {
    var securityOCSPEnabled = document.getElementById("security.OCSP.enabled");
    var enableOCSP = document.getElementById("enableOCSP");
    var requireOCSP = document.getElementById("requireOCSP");

    if (called_by) {
      securityOCSPEnabled.value = enableOCSP.checked ? 1 : 0
    } else {
      enableOCSP.checked = parseInt(securityOCSPEnabled.value) != 0;
    }

    requireOCSP.disabled = !enableOCSP.checked;
    return undefined;
  }
};
