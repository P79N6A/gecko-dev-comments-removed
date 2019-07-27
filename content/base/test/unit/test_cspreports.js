



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import('resource://gre/modules/NetUtil.jsm');

var httpServer = new HttpServer();
httpServer.start(-1);
var testsToFinish = 0;

const REPORT_SERVER_PORT = httpServer.identity.primaryPort;
const REPORT_SERVER_URI = "http://localhost";
const REPORT_SERVER_PATH = "/report";





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

  
  var csp = Cc["@mozilla.org/cspcontext;1"]
              .createInstance(Ci.nsIContentSecurityPolicy);
  var policy = "default-src 'none'; " +
               "report-uri " + REPORT_SERVER_URI +
                               ":" + REPORT_SERVER_PORT +
                               "/test" + id;
  var selfuri = NetUtil.newURI(REPORT_SERVER_URI +
                               ":" + REPORT_SERVER_PORT +
                               "/foo/self");
  var selfchan = NetUtil.newChannel(selfuri);

  dump("Created test " + id + " : " + policy + "\n\n");

  
  csp.setRequestContext(selfuri, null, selfchan);

  
  
  csp.appendPolicy(policy, selfuri, useReportOnlyPolicy);

  
  var handler = makeReportHandler("/test" + id, "Test " + id, expectedJSON);
  httpServer.registerPathHandler("/test" + id, handler);

  
  callback(csp);
}

function run_test() {
  var selfuri = NetUtil.newURI(REPORT_SERVER_URI +
                               ":" + REPORT_SERVER_PORT +
                               "/foo/self");

  
  makeTest(0, {"blocked-uri": "self"}, false,
      function(csp) {
        let inlineOK = true, oReportViolation = {'value': false};
        inlineOK = csp.getAllowsInlineScript(oReportViolation);

        
        do_check_false(inlineOK);
        
        do_check_true(oReportViolation.value);

        if (oReportViolation.value) {
          
          csp.logViolationDetails(Ci.nsIContentSecurityPolicy.VIOLATION_TYPE_INLINE_SCRIPT,
                                  selfuri.asciiSpec,
                                  "script sample",
                                  0);
        }
      });

  
  makeTest(1, {"blocked-uri": "self"}, false,
      function(csp) {
        let evalOK = true, oReportViolation = {'value': false};
        evalOK = csp.getAllowsEval(oReportViolation);

        
        do_check_false(evalOK);
        
        do_check_true(oReportViolation.value);

        if (oReportViolation.value) {
          
          csp.logViolationDetails(Ci.nsIContentSecurityPolicy.VIOLATION_TYPE_EVAL,
                                  selfuri.asciiSpec,
                                  "script sample",
                                  1);
        }
      });

  makeTest(2, {"blocked-uri": "http://blocked.test/foo.js"}, false,
      function(csp) {
        
        csp.shouldLoad(Ci.nsIContentPolicy.TYPE_SCRIPT,
                      NetUtil.newURI("http://blocked.test/foo.js"),
                      null, null, null, null);
      });

  
  makeTest(3, {"blocked-uri": "self"}, true,
      function(csp) {
        let inlineOK = true, oReportViolation = {'value': false};
        inlineOK = csp.getAllowsInlineScript(oReportViolation);

        
        do_check_true(inlineOK);

        
        do_check_true(oReportViolation.value);

        if (oReportViolation.value) {
          
          csp.logViolationDetails(Ci.nsIContentSecurityPolicy.VIOLATION_TYPE_INLINE_SCRIPT,
                                  selfuri.asciiSpec,
                                  "script sample",
                                  3);
        }
      });

  
  makeTest(4, {"blocked-uri": "self"}, true,
      function(csp) {
        let evalOK = true, oReportViolation = {'value': false};
        evalOK = csp.getAllowsEval(oReportViolation);

        
        do_check_true(evalOK);
        
        do_check_true(oReportViolation.value);

        if (oReportViolation.value) {
          
          csp.logViolationDetails(Ci.nsIContentSecurityPolicy.VIOLATION_TYPE_INLINE_SCRIPT,
                                  selfuri.asciiSpec,
                                  "script sample",
                                  4);
        }
      });
}
