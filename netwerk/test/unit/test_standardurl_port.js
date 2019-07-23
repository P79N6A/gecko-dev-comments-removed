var Cc = Components.classes;
var Ci = Components.interfaces;

function run_test() {
  function makeURI(aURLSpec, aCharset) {
    var ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
    return ios.newURI(aURLSpec, aCharset, null);
  }

  var httpURI = makeURI("http://foo.com");
  do_check_eq(-1, httpURI.port);

  
  httpURI.port = 80;
  do_check_eq(-1, httpURI.port);
  
  
  httpURI.port = 123;
  do_check_eq(123, httpURI.port);
  httpURI.port = 80;
  do_check_eq(-1, httpURI.port);
  do_check_false(/80/.test(httpURI.spec));

  
  httpURI.spec = "http://foo.com:81";
  do_check_eq(81, httpURI.port);
  httpURI.spec = "http://foo.com:80";
  do_check_eq(-1, httpURI.port);
  do_check_false(/80/.test(httpURI.spec));

  httpURI = makeURI("http://foo.com");
  do_check_eq(-1, httpURI.port);
  do_check_false(/80/.test(httpURI.spec));

  httpURI = makeURI("http://foo.com:80");
  do_check_eq(-1, httpURI.port);
  do_check_false(/80/.test(httpURI.spec));

  httpURI = makeURI("http://foo.com:80");
  do_check_eq(-1, httpURI.port);
  do_check_false(/80/.test(httpURI.spec));

  httpURI = makeURI("https://foo.com");
  do_check_eq(-1, httpURI.port);
  do_check_false(/443/.test(httpURI.spec));

  httpURI = makeURI("https://foo.com:443");
  do_check_eq(-1, httpURI.port);
  do_check_false(/443/.test(httpURI.spec));

  
  
  
  
}
