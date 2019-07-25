Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/resource.js");
Cu.import("resource://weave/auth.js");
Cu.import("resource://weave/identity.js");

let logger;
let Httpd = {};
Cu.import("resource://harness/modules/httpd.js", Httpd);

function server_open(metadata, response) {
  let body = "This path exists";
  response.setStatusLine(metadata.httpVersion, 200, "OK");
  response.bodyOutputStream.write(body, body.length);
}

function server_protected(metadata, response) {
  let body;

  
  if (metadata.hasHeader("Authorization") &&
      metadata.getHeader("Authorization") == "Basic Z3Vlc3Q6Z3Vlc3Q=") {
    body = "This path exists and is protected";
    response.setStatusLine(metadata.httpVersion, 200, "OK, authorized");
    response.setHeader("WWW-Authenticate", 'Basic realm="secret"', false);

  } else {
    body = "This path exists and is protected - failed";
    response.setStatusLine(metadata.httpVersion, 401, "Unauthorized");
    response.setHeader("WWW-Authenticate", 'Basic realm="secret"', false);
  }

  response.bodyOutputStream.write(body, body.length);
}

function server_404(metadata, response) {
  let body = "File not found";
  response.setStatusLine(metadata.httpVersion, 404, "Not Found");
  response.bodyOutputStream.write(body, body.length);
}

function run_test() {
  logger = Log4Moz.repository.getLogger('Test');
  Log4Moz.repository.rootLogger.addAppender(new Log4Moz.DumpAppender());

  let server = new Httpd.nsHttpServer();
  server.registerPathHandler("/open", server_open);
  server.registerPathHandler("/protected", server_protected);
  server.registerPathHandler("/404", server_404);
  server.start(8080);

  Utils.prefs.setIntPref("network.numRetries", 1); 

  

  let res = new Resource("http://localhost:8080/open");
  let content = res.get();
  do_check_eq(content, "This path exists");
  do_check_eq(res.lastChannel.responseStatus, 200);

  
  let res2 = new Resource("http://localhost:8080/protected");
  try {
    content = res2.get();
    do_check_true(false); 
  } catch (e) {}

  

  let auth = new BasicAuthenticator(new Identity("secret", "guest", "guest"));
  let res3 = new Resource("http://localhost:8080/protected");
  res3.authenticator = auth;
  content = res3.get();
  do_check_eq(content, "This path exists and is protected");
  do_check_eq(res3.lastChannel.responseStatus, 200);

  

  let res4 = new Resource("http://localhost:8080/404");
  try {
    let content = res4.get();
    do_check_true(false); 
  } catch (e) {}
  do_check_eq(res4.lastChannel.responseStatusText, "Not Found");
  do_check_eq(res4.lastChannel.responseStatus, 404);

  
  
  
  

  server.stop();
}
