



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import('resource://gre/modules/CSPUtils.jsm');
Cu.import('resource://gre/modules/NetUtil.jsm');


Cu.import("resource://testing-common/httpd.js");

const REPORT_SERVER_PORT = 9000;
const REPORT_SERVER_URI = "http://localhost";
const REPORT_SERVER_PATH = "/report";

var httpServer = null;
var testsToFinish = 0;





function makeReportHandler(testpath, message, expectedJSON) {
  return function(request, response) {
    
    if (request.method !== "POST") {
      do_throw("violation report should be a POST request");
      return;
    }

    
    var reportObj = JSON.parse(
          NetUtil.readInputStreamToString(
            request.bodyInputStream,
            request.bodyInputStream.available()));

    dump("GOT REPORT:\n" + JSON.stringify(reportObj) + "\n");
    dump("TESTPATH:    " + testpath + "\n");
    dump("EXPECTED:  \n" + JSON.stringify(expectedJSON) + "\n\n");

    for (var i in expectedJSON)
      do_check_eq(expectedJSON[i], reportObj['csp-report'][i]);

    testsToFinish--;
    httpServer.registerPathHandler(testpath, null);
    if (testsToFinish < 1)
      httpServer.stop(do_test_finished);
    else
      do_test_finished();
  };
}






function makeTest(id, expectedJSON, useReportOnlyPolicy, callback) {
  testsToFinish++;
  do_test_pending();

  
  var csp = Cc["@mozilla.org/contentsecuritypolicy;1"]
              .createInstance(Ci.nsIContentSecurityPolicy);
  var policy = "allow 'none'; " +
               "report-uri " + REPORT_SERVER_URI +
                               ":" + REPORT_SERVER_PORT +
                               "/test" + id;
  var selfuri = NetUtil.newURI(REPORT_SERVER_URI +
                               ":" + REPORT_SERVER_PORT +
                               "/foo/self");
  var selfchan = NetUtil.newChannel(selfuri);

  dump("Created test " + id + " : " + policy + "\n\n");

  
  csp.scanRequestData(selfchan);

  
  csp.refinePolicy(policy, selfuri, false);

  
  if (useReportOnlyPolicy) csp.reportOnlyMode = true;

  
  var handler = makeReportHandler("/test" + id, "Test " + id, expectedJSON);
  httpServer.registerPathHandler("/test" + id, handler);

  
  callback(csp);
}

function run_test() {
  var selfuri = NetUtil.newURI(REPORT_SERVER_URI +
                               ":" + REPORT_SERVER_PORT +
                               "/foo/self");

  httpServer = new HttpServer();
  httpServer.start(REPORT_SERVER_PORT);

  
  makeTest(0, {"blocked-uri": "self"}, false,
      function(csp) {
        let inlineOK = true, oReportViolation = {};
        inlineOK = csp.getAllowsInlineScript(oReportViolation);

        
        do_check_false(inlineOK);
        
        do_check_true(oReportViolation.value);

        
        csp.logViolationDetails(Ci.nsIContentSecurityPolicy.VIOLATION_TYPE_INLINE_SCRIPT,
                                selfuri.asciiSpec,
                                "script sample",
                                0);
      });

  
  makeTest(1, {"blocked-uri": "self"}, false,
      function(csp) {
        let evalOK = true, oReportViolation = {};
        evalOK = csp.getAllowsEval(oReportViolation);

        
        do_check_false(evalOK);
        
        do_check_true(oReportViolation.value);

        
        csp.logViolationDetails(Ci.nsIContentSecurityPolicy.VIOLATION_TYPE_INLINE_SCRIPT,
                                selfuri.asciiSpec,
                                "script sample",
                                1);
      });

  makeTest(2, {"blocked-uri": "http://blocked.test/foo.js"}, false,
      function(csp) {
        
        csp.shouldLoad(Ci.nsIContentPolicy.TYPE_SCRIPT,
                      NetUtil.newURI("http://blocked.test/foo.js"),
                      null, null, null, null);
      });

  
  makeTest(3, {"blocked-uri": "self"}, true,
      function(csp) {
        let inlineOK = true, oReportViolation = {};
        inlineOK = csp.getAllowsInlineScript(oReportViolation);

        
        do_check_true(inlineOK);
        
        do_check_true(oReportViolation.value);

        
        csp.logViolationDetails(Ci.nsIContentSecurityPolicy.VIOLATION_TYPE_INLINE_SCRIPT,
                                selfuri.asciiSpec,
                                "script sample",
                                3);
      });

  
  makeTest(4, {"blocked-uri": "self"}, true,
      function(csp) {
        let evalOK = true, oReportViolation = {};
        evalOK = csp.getAllowsEval(oReportViolation);

        
        do_check_true(evalOK);
        
        do_check_true(oReportViolation.value);

        
        csp.logViolationDetails(Ci.nsIContentSecurityPolicy.VIOLATION_TYPE_INLINE_SCRIPT,
                                selfuri.asciiSpec,
                                "script sample",
                                4);
      });
}
