let Ci = Components.interfaces;
let Cc = Components.classes;
let Cu = Components.utils;


let isMainProcess = function() {
  try {
    return Cc["@mozilla.org/xre/app-info;1"].
        getService(Ci.nsIXULRuntime).
        processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
  } catch (e) { }
  return true;
};

if (isMainProcess()) {
  Components.utils.import("resource://gre/modules/SettingsRequestManager.jsm");
}
