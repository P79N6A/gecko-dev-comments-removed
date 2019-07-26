



Components.utils.import("resource://gre/modules/Services.jsm");

let Cc = Components.classes;
let Ci = Components.interfaces;


function populateResetPane(aContainerID) {
  let resetProfileItems = document.getElementById(aContainerID);
  try {
    let dataTypes = getMigratedData();
    for (let dataType of dataTypes) {
      let label = document.createElement("label");
      label.setAttribute("value", dataType);
      resetProfileItems.appendChild(label);
    }
  } catch (ex) {
    Cu.reportError(ex);
  }
}

function onResetProfileLoad() {
  populateResetPane("migratedItems");
}






function resetSupported() {
  let profileService = Cc["@mozilla.org/toolkit/profile-service;1"].
                       getService(Ci.nsIToolkitProfileService);
  let currentProfileDir = Services.dirsvc.get("ProfD", Ci.nsIFile);

#expand const MOZ_APP_NAME = "__MOZ_APP_NAME__";
#expand const MOZ_BUILD_APP = "__MOZ_BUILD_APP__";

  
  try {
    return currentProfileDir.equals(profileService.selectedProfile.rootDir) &&
             ("@mozilla.org/profile/migrator;1?app=" + MOZ_BUILD_APP + "&type=" + MOZ_APP_NAME in Cc);
  } catch (e) {
    
    Cu.reportError(e);
  }
  return false;
}

function getMigratedData() {
  Components.utils.import("resource:///modules/MigrationUtils.jsm");

#expand const MOZ_BUILD_APP = "__MOZ_BUILD_APP__";
#expand const MOZ_APP_NAME = "__MOZ_APP_NAME__";

  
  const MIGRATED_TYPES = [
    128,
    4,  
    16, 
    8,  
    2,  
  ];

  
  let dataTypes = [];
  for (let itemID of MIGRATED_TYPES) {
    try {
      let typeName = MigrationUtils.getLocalizedString(itemID + "_" + MOZ_APP_NAME);
      dataTypes.push(typeName);
    } catch (x) {
      
      Components.utils.reportError(x);
    }
  }
  return dataTypes;
}

function onResetProfileAccepted() {
  let retVals = window.arguments[0];
  retVals.reset = true;
}
