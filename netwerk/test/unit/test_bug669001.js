do_load_httpd_js();

var httpServer = null;
var path = "/bug699001";
var URI = "http://localhost:4444" + path;

function make_channel(url) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  return ios.newChannel(url, "", null);
}

var fetched;









var tests = [
{
  prepare: function() { },
  test: function(response) {
    do_check_true(fetched);
  }
},
{
  prepare: function() { },
  test: function(response) {
    do_check_false(fetched);
  }
},
{
  prepare: function() {
    setUA("A different User Agent");
  },
  test: function(response) {
    do_check_true(fetched);
  }
},
{
  prepare: function() { },
  test: function(response) {
    do_check_false(fetched);
  }
},
{
  prepare: function() {
    setUA("And another User Agent");
  },
  test: function(response) {
    do_check_true(fetched);
  }
},
{
  prepare: function() { },
  test: function(response) {
    do_check_false(fetched);
  }
}
];

function handler(metadata, response)
{
  if (metadata.hasHeader("If-None-Match")) {
    response.setStatusLine(metadata.httpVersion, 304, "Not modified");
  }
  else {
    response.setStatusLine(metadata.httpVersion, 200, "OK");
    response.setHeader("Content-Type", "text/plain");

    var body = "body";
    response.bodyOutputStream.write(body, body.length);
  }

  fetched = true;

  response.setHeader("Expires", getDateString(+1));
  response.setHeader("Cache-Control", "private");
  response.setHeader("Vary", "User-Agent");
  response.setHeader("ETag", "1234");
}

function run_test()
{
  httpServer = new nsHttpServer();
  httpServer.registerPathHandler(path, handler);
  httpServer.start(4444);

  do_test_pending();

  nextTest();
}

function nextTest()
{
  fetched = false;
  tests[0].prepare();

  dump("Testing with User-Agent: " + getUA() + "\n");
  var chan = make_channel(URI);

  
  
  do_execute_soon(function() {
    chan.asyncOpen(new ChannelListener(checkAndShiftTest, null), null);
  });
}

function checkAndShiftTest(request, response)
{
  tests[0].test(response);

  tests.shift();
  if (tests.length == 0) {
    httpServer.stop(tearDown);
    return;
  }

  nextTest();
}

function tearDown()
{
  setUA("");
  do_test_finished();
}



function getUA()
{
  var httphandler = Cc["@mozilla.org/network/protocol;1?name=http"].
                 getService(Ci.nsIHttpProtocolHandler);
  return httphandler.userAgent;
}

function setUA(value)
{
  var prefs = Cc["@mozilla.org/preferences-service;1"].
                 getService(Ci.nsIPrefBranch);
  prefs.setCharPref("general.useragent.override", value);
}

function getDateString(yearDelta) {
  var months = [ 'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug',
          'Sep', 'Oct', 'Nov', 'Dec' ];
  var days = [ 'Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat' ];

  var d = new Date();
  return days[d.getUTCDay()] + ", " + d.getUTCDate() + " "
          + months[d.getUTCMonth()] + " " + (d.getUTCFullYear() + yearDelta)
          + " " + d.getUTCHours() + ":" + d.getUTCMinutes() + ":"
          + d.getUTCSeconds() + " UTC";
}
