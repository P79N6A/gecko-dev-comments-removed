




































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

  var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);

  
  
  var obs = {
    observe: function(aSubject, aTopic, aData) {
      
      ww.unregisterNotification(this);

      var win = aSubject.QueryInterface(Ci.nsIDOMEventTarget);
      win.addEventListener("DOMContentLoaded", finishUp, false);
    }
  };

  
  ww.registerNotification(obs);

  
  function finishUp() {
    var dmui = Cc["@mozilla.org/download-manager-ui;1"].
               getService(Ci.nsIDownloadManagerUI);
    ok(dmui.visible, "Download Manager window is open, as expected.");

    
    try {
      Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch).    
      clearUserPref(PREF_BDM_CLOSEWHENDONE);
    }
    catch (err) { }

    finish();
  }
  
  
  
  var key = navigator.platform.match("Linux") ? "y" : "j";
  EventUtils.synthesizeKey(key, {metaKey: true}, window.opener);

  waitForExplicitFinish();
}
