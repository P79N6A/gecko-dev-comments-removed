




































function test()
{
  const PREF_BDM_CLOSEWHENDONE = "browser.download.manager.closeWhenDone";
  var dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  var db = dm.DBConnection;

  
  db.executeSimpleSQL("DELETE FROM moz_downloads");

  
  var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
           getService(Ci.nsIWindowMediator);
  var win = wm.getMostRecentWindow("Download:Manager");
  if (win)
    win.close();

  
  Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch).
  setBoolPref(PREF_BDM_CLOSEWHENDONE, true);

  
  
  var key = navigator.platform.match("Linux") ? "y" : "j";
  EventUtils.synthesizeKey(key, {metaKey: true}, window.opener);

  
  function finishUp() {
    var dmui = Cc["@mozilla.org/download-manager-ui;1"].
               getService(Ci.nsIDownloadManagerUI);
    ok(dmui.visible, "Download Manager window is open, as expected.");

    
    Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch).
    setBoolPref(PREF_BDM_CLOSEWHENDONE, false);

    finish();
  }
  
  waitForExplicitFinish();
  window.setTimeout(finishUp, 1000);
}
