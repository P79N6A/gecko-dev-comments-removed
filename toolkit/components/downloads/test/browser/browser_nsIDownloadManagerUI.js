



































let Cr = Components.results;

function test_visibility_open()
{
  var dmui = Cc["@mozilla.org/download-manager-ui;1"].
             getService(Ci.nsIDownloadManagerUI);
  is(dmui.visible, true,
     "nsIDownloadManagerUI indicates that the UI is visible");
}

function test_getAttention_opened()
{
  var dmui = Cc["@mozilla.org/download-manager-ui;1"].
             getService(Ci.nsIDownloadManagerUI);

  
  window.focus();

  dmui.getAttention();
  is(dmui.visible, true,
     "nsIDownloadManagerUI indicates that the UI is visible");
}

function test_visibility_closed(aWin)
{
  var dmui = Cc["@mozilla.org/download-manager-ui;1"].
             getService(Ci.nsIDownloadManagerUI);
  aWin.close();
  is(dmui.visible, false,
     "nsIDownloadManagerUI indicates that the UI is not visible");
}

function test_getAttention_closed()
{
  var dmui = Cc["@mozilla.org/download-manager-ui;1"].
             getService(Ci.nsIDownloadManagerUI);

  var exceptionCaught = false;
  try {
    dmui.getAttention();
  } catch (e) {
    is(e.result, Cr.NS_ERROR_UNEXPECTED,
       "Proper exception was caught");
    exceptionCaught = true;
  } finally {
    is(exceptionCaught, true,
       "Exception was caught, as expected");
  }
}

var testFuncs = [
    test_visibility_open
  , test_getAttention_opened
  , test_visibility_closed 

  , test_getAttention_closed
];

function test()
{
  var dm = Cc["@mozilla.org/download-manager;1"].
           getService(Ci.nsIDownloadManager);
  var db = dm.DBConnection;

  
  db.executeSimpleSQL("DELETE FROM moz_downloads");

  
  var win = Services.wm.getMostRecentWindow("Download:Manager");
  if (win)
    win.close();

  
  Cc["@mozilla.org/download-manager-ui;1"].
  getService(Ci.nsIDownloadManagerUI).show();

  
  function finishUp() {
    var win = Services.wm.getMostRecentWindow("Download:Manager");

    
    for each (var t in testFuncs)
      t(win);

    finish();
  }
  
  waitForExplicitFinish();
  window.setTimeout(finishUp, 1000);
}
