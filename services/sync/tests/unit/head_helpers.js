


Cu.import("resource://services-common/async.js");
Cu.import("resource://testing-common/services/common/utils.js");

let provider = {
  getFile: function(prop, persistent) {
    persistent.value = true;
    switch (prop) {
      case "ExtPrefDL":
        return [Services.dirsvc.get("CurProcD", Ci.nsIFile)];
      default:
        throw Cr.NS_ERROR_FAILURE;
    }
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDirectoryServiceProvider])
};
Services.dirsvc.QueryInterface(Ci.nsIDirectoryService).registerProvider(provider);


let gGlobalScope = this;

function ExtensionsTestPath(path) {
  if (path[0] != "/") {
    throw Error("Path must begin with '/': " + path);
  }

  return "../../../../toolkit/mozapps/extensions/test/xpcshell" + path;
}








function loadAddonTestFunctions() {
  const path = ExtensionsTestPath("/head_addons.js");
  let file = do_get_file(path);
  let uri = Services.io.newFileURI(file);
  Services.scriptloader.loadSubScript(uri.spec, gGlobalScope);
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");
}

function getAddonInstall(name) {
  let f = do_get_file(ExtensionsTestPath("/addons/" + name + ".xpi"));
  let cb = Async.makeSyncCallback();
  AddonManager.getInstallForFile(f, cb);

  return Async.waitForSyncCallback(cb);
}










function getAddonFromAddonManagerByID(id) {
   let cb = Async.makeSyncCallback();
   AddonManager.getAddonByID(id, cb);
   return Async.waitForSyncCallback(cb);
}






function installAddonFromInstall(install) {
  let cb = Async.makeSyncCallback();
  let listener = {onInstallEnded: cb};
  AddonManager.addInstallListener(listener);
  install.install();
  Async.waitForSyncCallback(cb);
  AddonManager.removeAddonListener(listener);

  do_check_neq(null, install.addon);
  do_check_neq(null, install.addon.syncGUID);

  return install.addon;
}








function installAddon(name) {
  let install = getAddonInstall(name);
  do_check_neq(null, install);
  return installAddonFromInstall(install);
}







function uninstallAddon(addon) {
  let cb = Async.makeSyncCallback();
  let listener = {onUninstalled: function(uninstalled) {
    if (uninstalled.id == addon.id) {
      AddonManager.removeAddonListener(listener);
      cb(uninstalled);
    }
  }};

  AddonManager.addAddonListener(listener);
  addon.uninstall();
  Async.waitForSyncCallback(cb);
}

function generateNewKeys(collectionKeys, collections=null) {
  let wbo = collectionKeys.generateNewKeysWBO(collections);
  let modified = new_timestamp();
  collectionKeys.setContents(wbo.cleartext, modified);
}





function mockShouldSkipWindow (win) {
  return win.closed ||
         win.mockIsPrivate;
}

function mockGetTabState (tab) {
  return tab;
}

function mockGetWindowEnumerator(url, numWindows, numTabs, indexes, moreURLs) {
  let elements = [];

  function url2entry(url) {
    return {
      url: ((typeof url == "function") ? url() : url),
      title: "title"
    };
  }

  for (let w = 0; w < numWindows; ++w) {
    let tabs = [];
    let win = {
      closed: false,
      mockIsPrivate: false,
      gBrowser: {
        tabs: tabs,
      },
    };
    elements.push(win);

    for (let t = 0; t < numTabs; ++t) {
      tabs.push(TestingUtils.deepCopy({
        index: indexes ? indexes() : 1,
        entries: (moreURLs ? [url].concat(moreURLs()) : [url]).map(url2entry),
        attributes: {
          image: "image"
        },
        lastAccessed: 1499
      }));
    }
  }

  
  elements.push({
    closed: true,
    mockIsPrivate: false,
    gBrowser: {
      tabs: [],
    },
  });
 
  elements.push({
    closed: false,
    mockIsPrivate: true,
    gBrowser: {
      tabs: [],
    },
  });

  return {
    hasMoreElements: function () {
      return elements.length;
    },
    getNext: function () {
      return elements.shift();
    },
  };
}


function do_check_array_eq(a1, a2) {
  do_check_eq(a1.length, a2.length);
  for (let i = 0; i < a1.length; ++i) {
    do_check_eq(a1[i], a2[i]);
  }
}
