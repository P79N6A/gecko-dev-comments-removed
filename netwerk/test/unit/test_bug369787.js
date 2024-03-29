Cu.import("resource://testing-common/httpd.js");
Cu.import("resource://gre/modules/Services.jsm");

const BUGID = "369787";
var server = null;
var channel = null;

function change_content_type() {
  var origType = channel.contentType;
  const newType = "x-foo/x-bar";
  channel.contentType = newType;
  do_check_eq(channel.contentType, newType);
  channel.contentType = origType;
  do_check_eq(channel.contentType, origType);
}

function TestListener() {
}
TestListener.prototype.onStartRequest = function(request, context) {
  try {
    
    channel = request.QueryInterface(Components.interfaces.nsIChannel);

    change_content_type();
  } catch (ex) {
    print(ex);
    throw ex;
  }
}
TestListener.prototype.onStopRequest = function(request, context, status) {
  try {
    change_content_type();
  } catch (ex) {
    print(ex);
    
  }

  do_timeout(0, after_channel_closed);
}

function after_channel_closed() {
  try {
    change_content_type();
  } finally {
    server.stop(do_test_finished);
  }
}

function run_test() {
  
  server = new HttpServer();

  server.registerPathHandler("/bug" + BUGID, bug369787);

  server.start(-1);

  
  channel =
      Components.classes["@mozilla.org/network/io-service;1"].
      getService(Components.interfaces.nsIIOService).
      newChannel2("http://localhost:" +
                  server.identity.primaryPort + "/bug" + BUGID,
                  null,
                  null,
                  null,      
                  Services.scriptSecurityManager.getSystemPrincipal(),
                  null,      
                  Ci.nsILoadInfo.SEC_NORMAL,
                  Ci.nsIContentPolicy.TYPE_OTHER);

  channel.QueryInterface(Components.interfaces.nsIHttpChannel);
  channel.asyncOpen(new TestListener(), null);

  do_test_pending();
}


function bug369787(metadata, response) {
  
}
