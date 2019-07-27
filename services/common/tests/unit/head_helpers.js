



Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://testing-common/services/common/logging.js");

let btoa = Cu.import("resource://gre/modules/Log.jsm").btoa;
let atob = Cu.import("resource://gre/modules/Log.jsm").atob;

function do_check_empty(obj) {
  do_check_attribute_count(obj, 0);
}

function do_check_attribute_count(obj, c) {
  do_check_eq(c, Object.keys(obj).length);
}

function do_check_throws(aFunc, aResult, aStack) {
  if (!aStack) {
    try {
      
      aStack = Components.stack.caller;
    } catch (e) {}
  }

  try {
    aFunc();
  } catch (e) {
    do_check_eq(e.result, aResult, aStack);
    return;
  }
  do_throw("Expected result " + aResult + ", none thrown.", aStack);
}











function do_check_throws_message(aFunc, aResult) {
  try {
    aFunc();
  } catch (e) {
    do_check_eq(e.message, aResult);
    return;
  }
  do_throw("Expected an error, none thrown.");
}










let _ = function(some, debug, text, to) {
  print(Array.slice(arguments).join(" "));
};

function httpd_setup (handlers, port=-1) {
  let server = new HttpServer();
  for (let path in handlers) {
    server.registerPathHandler(path, handlers[path]);
  }
  try {
    server.start(port);
  } catch (ex) {
    _("==========================================");
    _("Got exception starting HTTP server on port " + port);
    _("Error: " + CommonUtils.exceptionStr(ex));
    _("Is there a process already listening on port " + port + "?");
    _("==========================================");
    do_throw(ex);
  }

  
  let i = server.identity;
  server.baseURI = i.primaryScheme + "://" + i.primaryHost + ":" + i.primaryPort;

  return server;
}

function httpd_handler(statusCode, status, body) {
  return function handler(request, response) {
    _("Processing request");
    
    request.body = readBytesFromInputStream(request.bodyInputStream);
    handler.request = request;

    response.setStatusLine(request.httpVersion, statusCode, status);
    if (body) {
      response.bodyOutputStream.write(body, body.length);
    }
  };
}





function readBytesFromInputStream(inputStream, count) {
  return CommonUtils.readBytesFromInputStream(inputStream, count);
}




function ensureThrows(func) {
  return function() {
    try {
      func.apply(this, arguments);
    } catch (ex) {
      do_throw(ex);
    }
  };
}








let PACSystemSettings = {
  CID: Components.ID("{5645d2c1-d6d8-4091-b117-fe7ee4027db7}"),
  contractID: "@mozilla.org/system-proxy-settings;1",

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIFactory,
                                         Ci.nsISystemProxySettings]),

  createInstance: function createInstance(outer, iid) {
    if (outer) {
      throw Cr.NS_ERROR_NO_AGGREGATION;
    }
    return this.QueryInterface(iid);
  },

  lockFactory: function lockFactory(lock) {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  },

  
  
  mainThreadOnly: true,
  PACURI: null,
  getProxyForURI: function getProxyForURI(aURI) {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  }
};

function installFakePAC() {
  _("Installing fake PAC.");
  Cm.nsIComponentRegistrar
    .registerFactory(PACSystemSettings.CID,
                     "Fake system proxy-settings",
                     PACSystemSettings.contractID,
                     PACSystemSettings);
}

function uninstallFakePAC() {
  _("Uninstalling fake PAC.");
  let CID = PACSystemSettings.CID;
  Cm.nsIComponentRegistrar.unregisterFactory(CID, PACSystemSettings);
}





function ensureStartOverKeepsIdentity() {
  Cu.import("resource://gre/modules/Services.jsm");
  Services.prefs.setBoolPref("services.sync-testing.startOverKeepIdentity", true);
  do_register_cleanup(function() {
    Services.prefs.clearUserPref("services.sync-testing.startOverKeepIdentity");
  });
}
ensureStartOverKeepsIdentity();
