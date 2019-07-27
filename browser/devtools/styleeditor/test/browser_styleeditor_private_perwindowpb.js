






const TEST_URL = 'http://' + TEST_HOST + '/browser/browser/devtools/styleeditor/test/test_private.html';
const {LoadContextInfo} = Cu.import("resource://gre/modules/LoadContextInfo.jsm", {});
const cache = Cc["@mozilla.org/netwerk/cache-storage-service;1"]
                .getService(Ci.nsICacheStorageService);

add_task(function* () {
  info("Opening a new private window");
  let win = OpenBrowserWindow({private: true});
  yield waitForDelayedStartupFinished(win);

  info("Clearing the browser cache");
  cache.clear();

  let { ui } = yield openStyleEditorForURL(TEST_URL, win);

  is(ui.editors.length, 1, "The style editor contains one sheet.");
  let editor = ui.editors[0];

  yield editor.getSourceEditor();
  yield checkDiskCacheFor(TEST_HOST);
  win.close();

});

function checkDiskCacheFor(host)
{
  let foundPrivateData = false;
  let deferred = promise.defer();

  Visitor.prototype = {
    onCacheStorageInfo: function(num, consumption)
    {
      info("disk storage contains " + num + " entries");
    },
    onCacheEntryInfo: function(uri)
    {
      var urispec = uri.asciiSpec;
      info(urispec);
      foundPrivateData |= urispec.includes(host);
    },
    onCacheEntryVisitCompleted: function()
    {
      is(foundPrivateData, false, "web content present in disk cache");
      deferred.resolve();
    }
  };
  function Visitor() {}

  var storage = cache.diskCacheStorage(LoadContextInfo.default, false);
  storage.asyncVisitStorage(new Visitor(), true );

  return deferred.promise;
}

function waitForDelayedStartupFinished(aWindow)
{
  let deferred = promise.defer();
  Services.obs.addObserver(function observer(aSubject, aTopic) {
    if (aWindow == aSubject) {
      Services.obs.removeObserver(observer, aTopic);
      deferred.resolve();
    }
  }, "browser-delayed-startup-finished", false);

  return deferred.promise;
}
