









document.webL10n = document.mozL10n = {
  get: function(sringId, vars) {
    return "" + sringId + (vars ? ";" + JSON.stringify(vars) : "");
  }
};
