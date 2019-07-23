



































function test()
{
  
  let wm = Cc["@mozilla.org/appshell/window-mediator;1"].
           getService(Ci.nsIWindowMediator);
  let win = wm.getMostRecentWindow("Download:Manager");
  if (win) win.close();

  
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  ww.registerNotification({
    observe: function(aSubject, aTopic, aData) {
      ww.unregisterNotification(this);
      aSubject.QueryInterface(Ci.nsIDOMEventTarget).
      addEventListener("DOMContentLoaded", doTest, false);
    }
  });

  
  let doTest  = function() setTimeout(function() {
    win = wm.getMostRecentWindow("Download:Manager");

    
    let search = win.document.getElementById("searchbox");
    search.focus();
    search.value = "download manager escape test";

    
    let sendEscape = function() EventUtils.synthesizeKey("VK_ESCAPE", {}, win);

    
    sendEscape();
    is(win.closed, false,
      "Escape doesn't close the window when in the search box");
    is(search.value, "", "Escape correctly emptied the search box");

    
    sendEscape();
    is(win.closed, true,
      "Previous escape moved focus to list and now escape closed the window");

    finish();
  }, 0);
 
  
  Cc["@mozilla.org/download-manager-ui;1"].
  getService(Ci.nsIDownloadManagerUI).show();

  waitForExplicitFinish();
}
