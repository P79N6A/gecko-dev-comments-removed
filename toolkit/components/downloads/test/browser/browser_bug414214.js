




































function test()
{
  const PREF_BDM_CLOSEWHENDONE = "browser.download.manager.closeWhenDone";
  var dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  var db = dm.DBConnection;

  
  db.executeSimpleSQL("DELETE FROM moz_downloads");

  
  var win = Services.wm.getMostRecentWindow("Download:Manager");
  if (win)
    win.close();

  
  Services.prefs.setBoolPref(PREF_BDM_CLOSEWHENDONE, true);

  
  
  Services.ww.registerNotification(function (aSubject, aTopic, aData) {
    Services.ww.unregisterNotification(arguments.callee);

    var win = aSubject.QueryInterface(Ci.nsIDOMEventTarget);
    win.addEventListener("DOMContentLoaded", finishUp, false);
  });

  
  function finishUp() {
    var dmui = Cc["@mozilla.org/download-manager-ui;1"].
               getService(Ci.nsIDownloadManagerUI);
    ok(dmui.visible, "Download Manager window is open, as expected.");

    
    try {
      Services.prefs.clearUserPref(PREF_BDM_CLOSEWHENDONE);
    }
    catch (err) { }

    finish();
  }
  
  
  
  var key = navigator.platform.match("Linux") ? "y" : "j";
  EventUtils.synthesizeKey(key, {metaKey: true}, window.opener);

  waitForExplicitFinish();
}
