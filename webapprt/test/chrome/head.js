



Cu.import("resource://gre/modules/Services.jsm");



Services.scriptloader
        .loadSubScript("chrome://webapprt/content/mochitest-shared.js", this);

const MANIFEST_URL_BASE = Services.io.newURI(
  "http://test/webapprtChrome/webapprt/test/chrome/", null, null);











function loadWebapp(manifest, parameters, onLoad) {
  let url = Services.io.newURI(manifest, null, MANIFEST_URL_BASE);

  becomeWebapp(url.spec, parameters, function onBecome() {
    function onLoadApp() {
      gAppBrowser.removeEventListener("load", onLoadApp, true);
      onLoad();
    }
    gAppBrowser.addEventListener("load", onLoadApp, true);
    gAppBrowser.setAttribute("src", WebappRT.launchURI);
  });

  registerCleanupFunction(function() {
    
    
    let { DOMApplicationRegistry } = Cu.import("resource://gre/modules/Webapps.jsm", {});

    return new Promise(function(resolve, reject) {
      DOMApplicationRegistry.uninstall(url.spec, () => {
        
        
        gAppBrowser.setAttribute("src", "about:blank");

        resolve();
      }, reject);
    });
  });
}



let MockDownloadList = function() {
};

MockDownloadList.prototype = {
  downloads: new Set(),
  views: new Set(),

  addView: function(aView) {
    this.views.add(aView);

    for (let download of this.downloads) {
      for (let view of this.views) {
        view.onDownloadAdded(download);
      }
    }
  },

  removeView: function(aView) {
    this.views.delete(aView);
  },

  addDownload: function(aDownload) {
    this.downloads.add(aDownload);

    for (let view of this.views) {
      view.onDownloadAdded(aDownload);
    }
  },

  changeDownload: function(aDownload) {
    for (let view of this.views) {
      if (view.onDownloadChanged) {
        view.onDownloadChanged(aDownload);
      }
    }
  },

  removeDownload: function(aDownload) {
    this.downloads.delete(aDownload);

    for (let view of this.views) {
      if (view.onDownloadRemoved) {
        view.onDownloadRemoved(aDownload);
      }
    }
  },
}

function MockDownloadsModule() {
  let { DownloadView } = Cu.import("resource://webapprt/modules/DownloadView.jsm", {});

  let list = new MockDownloadList();

  let { Downloads } = Cu.import("resource://gre/modules/Downloads.jsm", {});
  let oldDownloadsGetList = Downloads.getList;

  Downloads.getList = function(aKind) {
    return new Promise(function(resolve, reject) {
      resolve(list);
    });
  };

  registerCleanupFunction(function() {
    list.removeView(DownloadView);

    Downloads.getList = oldDownloadsGetList;
  });

  
  
  
  
  DownloadView.init();

  return list;
}

function DummyDownload(aFileName) {
  this.file = Services.dirsvc.get("ProfD", Ci.nsIFile);
  this.file.append(aFileName);
  this.file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0o644);

  this.startTime = Date.now();
  this.source = {
    url: "http://mochi.test:8888//webapprtChrome/webapprt/test/chrome/download.webapp",
    isPrivate: false,
    referrer: null,
  };
  this.target = {
    path: this.file.path,
    partFilePath: this.file.path + ".part",
  }
};

DummyDownload.prototype = {
  succeeded: false,
  canceled: false,
  stopped: false,
  hasPartialData: false,
  hasProgress: false,
  progress: 0,
  currentBytes: 0,
  totalBytes: 0,
  error: null,

  
  state: 0,
  percentComplete: -1,
};

function waitDownloadListPopulation(aWin) {
  return new Promise(function(resolve, reject) {
    let disconnected = false;

    var observer = new MutationObserver(function(aMutations) {
      for each (let mutation in aMutations) {
        if (mutation.addedNodes) {
          for each (let node in mutation.addedNodes) {
            if (node.id == "downloadView") {
              observer.disconnect();
              disconnected = true;

              
              executeSoon(() => {
                resolve();
              });
            }
          }
        }
      }
    });

    observer.observe(aWin.document, {
      childList: true,
      subtree: true,
      attributes: false,
      characterData: false
    });

    registerCleanupFunction(function() {
      if (!disconnected) {
        observer.disconnect();
      }
    });
  });
}

function test_downloadList(aWin, aDownloadList) {
  let richlistbox = aWin.document.getElementById("downloadView");

  is(richlistbox.children.length, aDownloadList.length,
     "There is the correct number of richlistitems");

  for (let i = 0; i < richlistbox.children.length; i++) {
    let elm = richlistbox.children[i];

    let name = elm.getAttribute("target");

    let download = null;
    for (let d = 0; d < aDownloadList.length; d++) {
      if (aDownloadList[d].file.leafName == name) {
        download = aDownloadList[d];
        aDownloadList.splice(d, 1);
      }
    }

    if (!download) {
      ok(false, "Download item unexpected");
    } else {
      ok(true, "Download item expected");
      is(elm.getAttribute("state"), download.state, "Download state correct");
      is(elm.getAttribute("progress"), download.percentComplete,
         "Download progress correct");
    }
  }

  is(aDownloadList.length, 0,
     "All the downloads expected to be in the list were in the list");
}
