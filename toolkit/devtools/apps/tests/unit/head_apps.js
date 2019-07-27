


const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;
const CC = Components.Constructor;

Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");

let gClient, gActor;

function connect(onDone) {
  
  DebuggerServer.init(function () { return true; });
  
  
  DebuggerServer.addBrowserActors();

  
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function onConnect() {
    gClient.listTabs(function onListTabs(aResponse) {
      gActor = aResponse.webappsActor;
      onDone();
    });
  });
}

function webappActorRequest(request, onResponse) {
  if (!gActor) {
    connect(webappActorRequest.bind(null, request, onResponse));
    return;
  }

  request.to = gActor;
  gClient.request(request, onResponse);
}


function installTestApp(zipName, appId, onDone) {
  
  let zip = do_get_file("data/" + zipName);
  let appDir = FileUtils.getDir("TmpD", ["b2g", appId], true, true);
  zip.copyTo(appDir, "application.zip");

  let request = {type: "install", appId: appId};
  webappActorRequest(request, function (aResponse) {
    do_check_eq(aResponse.appId, appId);
    if ("error" in aResponse) {
      do_throw("Error: " + aResponse.error);
    }
    if ("message" in aResponse) {
      do_throw("Error message: " + aResponse.message);
    }
    do_check_false("error" in aResponse);

    onDone();
  });
};

function setup() {
  
  
  do_get_profile();

  
  
  do_get_webappsdir();

  
  Components.utils.import("resource://testing-common/AppInfo.jsm");
  updateAppInfo();

  
  
  Components.utils.import('resource://gre/modules/Webapps.jsm');
  DOMApplicationRegistry.allAppsLaunchable = true;

  
  Cu.import("resource://gre/modules/WebappOSUtils.jsm");
  WebappOSUtils.getPackagePath = function(aApp) {
    return aApp.basePath + "/" + aApp.id;
  }
}

function do_get_webappsdir() {
  var webappsDir = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
  webappsDir.append("test_webapps");
  if (!webappsDir.exists())
    webappsDir.create(Ci.nsIFile.DIRECTORY_TYPE, parseInt("755", 8));

  var coreAppsDir = Services.dirsvc.get("ProfD", Ci.nsILocalFile);
  coreAppsDir.append("test_coreapps");
  if (!coreAppsDir.exists())
    coreAppsDir.create(Ci.nsIFile.DIRECTORY_TYPE, parseInt("755", 8));

  
  
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


