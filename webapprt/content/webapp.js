



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://webapprt/modules/WebappRT.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function onLoad() {
  window.removeEventListener("load", onLoad, false);

  
  let manifest = WebappRT.config.app.manifest;
  document.documentElement.setAttribute("title", manifest.name);

  
  
  
  document.getElementById("content").addEventListener("click", onContentClick,
                                                      false, true);

  
  if ("arguments" in window) {
    
    let installRecord = WebappRT.config.app;
    let url = Services.io.newURI(installRecord.origin, null, null);
    if (manifest.launch_path)
      url = Services.io.newURI(manifest.launch_path, null, url);
    document.getElementById("content").setAttribute("src", url.spec);
  }
}
window.addEventListener("load", onLoad, false);









function onContentClick(event) {
  let target = event.target;

  if (!(target instanceof HTMLAnchorElement) ||
      target.getAttribute("target") != "_blank") {
    return;
  }

  let uri = Services.io.newURI(target.href,
                               target.ownerDocument.characterSet,
                               null);

  
  Cc["@mozilla.org/uriloader/external-protocol-service;1"].
    getService(Ci.nsIExternalProtocolService).
    getProtocolHandlerInfo(uri.scheme).
    launchWithURI(uri);

  
  
  
  event.preventDefault();
}

#ifdef XP_MACOSX


window.addEventListener("load", function onLoadUpdateMenuItems() {
  window.removeEventListener("load", onLoadUpdateMenuItems, false);
  let installRecord = WebappRT.config.app;
  let manifest = WebappRT.config.app.manifest;
  let bundle =
    Services.strings.createBundle("chrome://webapprt/locale/webapp.properties");
  let quitLabel = bundle.formatStringFromName("quitApplicationCmdMac.label",
                                              [manifest.name], 1);
  let hideLabel = bundle.formatStringFromName("hideApplicationCmdMac.label",
                                              [manifest.name], 1);
  document.getElementById("menu_FileQuitItem").setAttribute("label", quitLabel);
  document.getElementById("menu_mac_hide_app").setAttribute("label", hideLabel);
}, false);
#endif

function updateEditUIVisibility() {
#ifndef XP_MACOSX
  let editMenuPopupState = document.getElementById("menu_EditPopup").state;

  
  
  
  gEditUIVisible = editMenuPopupState == "showing" ||
                   editMenuPopupState == "open";

  
  
  if (gEditUIVisible) {
    goUpdateGlobalEditMenuItems();
  }

  
  
  
  else {
    goSetCommandEnabled("cmd_undo", true);
    goSetCommandEnabled("cmd_redo", true);
    goSetCommandEnabled("cmd_cut", true);
    goSetCommandEnabled("cmd_copy", true);
    goSetCommandEnabled("cmd_paste", true);
    goSetCommandEnabled("cmd_selectAll", true);
    goSetCommandEnabled("cmd_delete", true);
    goSetCommandEnabled("cmd_switchTextDirection", true);
  }
#endif
}
