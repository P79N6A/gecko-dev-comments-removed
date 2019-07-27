









navigator.mozL10n = document.mozL10n = {
  initialize: function(){},

  getDirection: function(){},

  get: function(stringId, vars) {

    
    var readableStringId = stringId.replace(/^./, function(match) {
      "use strict";
      return match.toUpperCase();
    }).replace(/_/g, " ");  

    return "" + readableStringId + (vars ? ";" + JSON.stringify(vars) : "");
  }
};
