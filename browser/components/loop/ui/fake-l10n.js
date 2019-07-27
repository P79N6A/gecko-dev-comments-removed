



(function() {
  "use strict";

  





  navigator.mozL10n = document.mozL10n = {
    initialize: function(){},

    getDirection: function(){},

    get: function(stringId, vars) {

      
      var readableStringId = stringId.replace(/^./, function(match) {
        return match.toUpperCase();
      }).replace(/_/g, " ");  

      return "" + readableStringId + (vars ? ";" + JSON.stringify(vars) : "");
    },

    
    language: {
      code: "en-US"
    }
  };
})();
