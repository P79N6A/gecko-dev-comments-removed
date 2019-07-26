



const Cu = Components.utils;

let TargetFactory = (function() {
  let tempScope = {};
  Components.utils.import("resource:///modules/devtools/Target.jsm", tempScope);
  return tempScope.TargetFactory;
})();


function clearUserPrefs()
{
  Services.prefs.clearUserPref("devtools.inspector.htmlPanelOpen");
  Services.prefs.clearUserPref("devtools.inspector.sidebarOpen");
  Services.prefs.clearUserPref("devtools.inspector.activeSidebar");
}

registerCleanupFunction(clearUserPrefs);
