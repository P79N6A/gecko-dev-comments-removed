


const URL = "about:robots";

function test() {
  let win;

  let listener = {
    onLocationChange: (webProgress, request, uri, flags) => {
      ok(webProgress.isTopLevel, "Received onLocationChange from top frame");
      is(uri.spec, URL, "Received onLocationChange for correct URL");
      finish();
    }
  };

  waitForExplicitFinish();

  
  registerCleanupFunction(() => {
    win.gBrowser.removeProgressListener(listener);
    win.close();
  });

  
  whenNewWindowOpened(w => win = w);

  
  openLinkIn(URL, "window", {});

  
  
  (function tryAddProgressListener() {
    executeSoon(() => {
      try {
        win.gBrowser.addProgressListener(listener);
      } catch (e) {
        
        tryAddProgressListener();
      }
    });
  })();
}

function whenNewWindowOpened(cb) {
  Services.obs.addObserver(function obs(win) {
    Services.obs.removeObserver(obs, "domwindowopened");
    cb(win);
  }, "domwindowopened", false);
}
