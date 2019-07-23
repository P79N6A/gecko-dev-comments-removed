






































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
         getService(Ci.nsINavHistoryService);
var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
         getService(Ci.nsINavBookmarksService);









function callback(aService)
{
  this.callCount = 0;
  this.service = aService;
}
callback.prototype = {
  
  

  runBatched: function(aUserData)
  {
    this.callCount++;

    if (this.callCount == 1) {
      
      this.service.runInBatchMode(this, null);
      return;
    }

    do_check_eq(this.callCount, 2);
    do_test_finished();
  },

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsINavHistoryBatchCallback])
};

function run_test() {
  
  do_test_pending();
  hs.runInBatchMode(new callback(hs), null);

  
  do_test_pending();
  bs.runInBatchMode(new callback(bs), null);
}
