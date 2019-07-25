



Components.utils.import("resource://gre/modules/Services.jsm");


function onResetProfileLoad() {
#expand const MOZ_BUILD_APP = "__MOZ_BUILD_APP__";
#expand const MOZ_APP_NAME = "__MOZ_APP_NAME__";
  const MAX_MIGRATED_TYPES = 16;

  var migratedItems = document.getElementById("migratedItems");
  var bundle = Services.strings.createBundle("chrome://" + MOZ_BUILD_APP +
                                             "/locale/migration/migration.properties");

  
  
  
  for (var i = 1; i < MAX_MIGRATED_TYPES; ++i) {
    var itemID = Math.pow(2, i);
    try {
      var checkbox = document.createElement("label");
      checkbox.setAttribute("value", bundle.GetStringFromName(itemID + "_" + MOZ_APP_NAME));
      migratedItems.appendChild(checkbox);
    } catch (x) {
      
    }
  }
}

function onResetProfileAccepted() {
  var retVals = window.arguments[0];
  retVals.reset = true;
}
