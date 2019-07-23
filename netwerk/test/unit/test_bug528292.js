do_load_httpd_js();

const sentCookieVal     = "foo=bar";
const responseBody      = "response body";
const baseURL           = "http://localhost:4444";
const preRedirectPath   = "/528292/pre-redirect";
const preRedirectURL    = baseURL + preRedirectPath;
const postRedirectPath  = "/528292/post-redirect";
const postRedirectURL   = baseURL + postRedirectPath;
var   httpServer        = null;
var   receivedCookieVal = null;

function preRedirectHandler(metadata, response)
{
  response.setStatusLine(metadata.httpVersion, 302, "Found");
  response.setHeader("Location", postRedirectURL, false);
  return;
}

function postRedirectHandler(metadata, response)
{
  receivedCookieVal = metadata.getHeader("Cookie");
  response.setHeader("Content-Type", "text/plain");
  response.bodyOutputStream.write(responseBody, responseBody.length);
}

function run_test()
{
  
  httpServer = new nsHttpServer();
  httpServer.registerPathHandler(preRedirectPath, preRedirectHandler);
  httpServer.registerPathHandler(postRedirectPath, postRedirectHandler);
  httpServer.start(4444);

  
  Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch).
    setIntPref("network.cookie.cookieBehavior", 1);

  var ioService = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);

  
  
  
  
  var chan = ioService.newChannel(preRedirectURL, "", null).
             QueryInterface(Ci.nsIHttpChannel).
             QueryInterface(Ci.nsIHttpChannelInternal);
  chan.forceAllowThirdPartyCookie = true;

  
  
  
  var postRedirectURI = ioService.newURI(postRedirectURL, "", null);
  Cc["@mozilla.org/cookieService;1"].getService(Ci.nsICookieService).
    setCookieString(postRedirectURI, null, sentCookieVal, chan);

  
  chan.asyncOpen(new ChannelListener(finish_test, null), null);
  do_test_pending();
}

function finish_test(event)
{
  do_check_eq(receivedCookieVal, sentCookieVal);
  httpServer.stop(do_test_finished);
}
