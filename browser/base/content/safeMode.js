




let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

const appStartup = Services.startup;

let defaultToReset = false;

function restartApp() {
  appStartup.quit(appStartup.eForceQuit | appStartup.eRestart);
}

function resetProfile() {
  
  let env = Cc["@mozilla.org/process/environment;1"]
              .getService(Ci.nsIEnvironment);
  env.set("MOZ_RESET_PROFILE_RESTART", "1");
}

function showResetDialog() {
  
  let retVals = {
    reset: false,
  };
  window.openDialog("chrome://global/content/resetProfile.xul", null,
                    "chrome,modal,centerscreen,titlebar,dialog=yes", retVals);
  if (!retVals.reset)
    return;
  resetProfile();
  restartApp();
}

function onDefaultButton() {
  if (defaultToReset) {
    
    resetProfile();
    restartApp();
    
    return false;
  } else {
    
    return true;
  }
}

function onCancel() {
  appStartup.quit(appStartup.eForceQuit);
}

function onExtra1() {
  if (defaultToReset) {
    
    window.close();
    return true;
  } else {
    
    showResetDialog();
  }
  return false;
}

function onLoad() {
  let dialog = document.documentElement;
  if (appStartup.automaticSafeModeNecessary) {
    document.getElementById("autoSafeMode").hidden = false;
    document.getElementById("safeMode").hidden = true;
    if (resetSupported()) {
      populateResetPane("resetProfileItems");
      document.getElementById("resetProfile").hidden = false;
    } else {
      
      document.documentElement.getButton("extra1").hidden = true;
    }
  } else {
    if (!resetSupported()) {
      
      document.documentElement.getButton("extra1").hidden = true;
      document.getElementById("resetProfileInstead").hidden = true;
    }
  }
}
