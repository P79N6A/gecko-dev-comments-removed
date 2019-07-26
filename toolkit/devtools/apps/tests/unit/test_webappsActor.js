


Components.utils.import("resource://gre/modules/devtools/dbg-server.jsm");
Components.utils.import("resource://gre/modules/devtools/dbg-client.jsm");

Components.utils.import("resource://gre/modules/FileUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;
const Cr = Components.results;

let gClient, gActor;
let gAppId = "actor-test";

add_test(function testSetup() {
  
  DebuggerServer.init(function () { return true; });
  
  
  DebuggerServer.addBrowserActors();

  
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function onConnect() {
    gClient.listTabs(function onListTabs(aResponse) {
      gActor = aResponse.webappsActor;
      run_next_test();
    });
  });
});

add_test(function testLaunchInexistantApp() {
  let request = {to: gActor, type: "launch", manifestURL: "http://foo.com"};
  gClient.request(request, function (aResponse) {
    do_check_eq(aResponse.error, "NO_SUCH_APP");
    run_next_test();
  });
});

add_test(function testCloseInexistantApp() {
  let request = {to: gActor, type: "close", manifestURL: "http://foo.com"};
  gClient.request(request, function (aResponse) {
    do_check_eq(aResponse.error, "missingParameter");
    do_check_eq(aResponse.message, "No application for http://foo.com");
    run_next_test();
  });
});


add_test(function testInstallPackaged() {
  
  let zip = do_get_file("data/app.zip");
  let appDir = FileUtils.getDir("TmpD", ["b2g", gAppId], true, true);
  zip.copyTo(appDir, "application.zip");

  let request = {to: gActor, type: "install", appId: gAppId};
  gClient.request(request, function (aResponse) {
    do_check_eq(aResponse.appId, gAppId);
  });

  
  
  gClient.addListener("webappsEvent", function (aState, aType, aPacket) {
    do_check_eq(aType.appId, gAppId);
    if ("error" in aType) {
      do_print("Error: " + aType.error);
    }
    if ("message" in aType) {
      do_print("Error message: " + aType.message);
    }
    do_check_eq("error" in aType, false);

    run_next_test();
  });
});



add_test(function testGetAll() {
  let request = {to: gActor, type: "getAll"};
  gClient.request(request, function (aResponse) {
    do_check_true("apps" in aResponse);
    let apps = aResponse.apps;
    do_check_true(apps.length > 0);
    for (let i = 0; i < apps.length; i++) {
      let app = apps[i];
      if (app.id == gAppId) {
        do_check_eq(app.name, "Test app");
        do_check_eq(app.manifest.description, "Testing webapps actor");
        do_check_eq(app.manifest.launch_path, "/index.html");
        do_check_eq(app.origin, "app://" + gAppId);
        do_check_eq(app.installOrigin, app.origin);
        do_check_eq(app.manifestURL, app.origin + "/manifest.webapp");
        run_next_test();
        return;
      }
    }
    do_throw("Unable to find the test app by its id");
  });
});

add_test(function testLaunchApp() {
  let manifestURL = "app://" + gAppId + "/manifest.webapp";
  let startPoint = "/index.html";
  let request = {
    to: gActor, type: "launch",
    manifestURL: manifestURL,
    startPoint: startPoint
  };
  Services.obs.addObserver(function observer(subject, topic, data) {
    Services.obs.removeObserver(observer, topic);
    let json = JSON.parse(data);
    do_check_eq(json.manifestURL, manifestURL);
    do_check_eq(json.startPoint, startPoint);
    run_next_test();
  }, "webapps-launch", false);

  gClient.request(request, function (aResponse) {
    do_check_false("error" in aResponse);
  });
});

add_test(function testCloseApp() {
  let manifestURL = "app://" + gAppId + "/manifest.webapp";
  let request = {
    to: gActor, type: "close",
    manifestURL: manifestURL
  };
  Services.obs.addObserver(function observer(subject, topic, data) {
    Services.obs.removeObserver(observer, topic);
    let json = JSON.parse(data);
    do_check_eq(json.manifestURL, manifestURL);
    run_next_test();
  }, "webapps-close", false);

  gClient.request(request, function (aResponse) {
    do_check_false("error" in aResponse);
  });
});

add_test(function testUninstall() {
  let origin = "app://" + gAppId;
  let manifestURL = origin + "/manifest.webapp";
  let request = {
    to: gActor, type: "uninstall",
    manifestURL: manifestURL
  };

  let gotFirstUninstallEvent = false;
  Services.obs.addObserver(function observer(subject, topic, data) {
    Services.obs.removeObserver(observer, topic);
    let json = JSON.parse(data);
    do_check_eq(json.manifestURL, manifestURL);
    do_check_eq(json.origin, origin);
    gotFirstUninstallEvent = true;
  }, "webapps-uninstall", false);

  Services.obs.addObserver(function observer(subject, topic, data) {
    Services.obs.removeObserver(observer, topic);
    let json = JSON.parse(data);
    do_check_eq(json.manifestURL, manifestURL);
    do_check_eq(json.origin, origin);
    do_check_eq(json.id, gAppId);
    do_check_true(gotFirstUninstallEvent);
    run_next_test();
  }, "webapps-sync-uninstall", false);

  gClient.request(request, function (aResponse) {
    do_check_false("error" in aResponse);
  });
});


add_test(function testTearDown() {
  gClient.close(function () {
    run_next_test();
  });
});

function run_test() {
  
  
  do_get_profile();

  
  
  do_get_webappsdir();

  
  Components.utils.import("resource://testing-common/AppInfo.jsm");
  updateAppInfo();

  
  
  Components.utils.import('resource://gre/modules/Webapps.jsm');
  DOMApplicationRegistry.allAppsLaunchable = true;

  run_next_test();
}

function do_get_webappsdir() {
  var webappsDir = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
  webappsDir.append("test_webapps");
  if (!webappsDir.exists())
    webappsDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);

  var coreAppsDir = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
  coreAppsDir.append("test_coreapps");
  if (!coreAppsDir.exists())
    coreAppsDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);

  
  
  var provider = {
    getFile: function(prop, persistent) {
      persistent.value = true;
      if (prop == "webappsDir") {
        return webappsDir.clone();
      }
      else if (prop == "coreAppsDir") {
        return coreAppsDir.clone();
      }
      throw Cr.NS_ERROR_FAILURE;
    },
    QueryInterface: function(iid) {
      if (iid.equals(Ci.nsIDirectoryServiceProvider) ||
          iid.equals(Ci.nsISupports)) {
        return this;
      }
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
  };
  Services.dirsvc.QueryInterface(Ci.nsIDirectoryService).registerProvider(provider);
}


