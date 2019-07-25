
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import('resource://gre/modules/CSPUtils.jsm');
Cu.import("resource://testing-common/httpd.js");

var httpserv = null;

const POLICY_FROM_URI = "allow 'self'; img-src *";
const POLICY_PORT = 9000;
const POLICY_URI = "http://localhost:" + POLICY_PORT + "/policy";
const POLICY_URI_RELATIVE = "/policy";
const DOCUMENT_URI = "http://localhost:" + POLICY_PORT + "/document";
const CSP_DOC_BODY = "CSP doc content";
const SD = CSPRep.SRC_DIRECTIVES;


var TESTS = [];


function mkuri(foo) {
  return Cc["@mozilla.org/network/io-service;1"]
           .getService(Ci.nsIIOService)
           .newURI(foo, null, null);
}


function do_check_equivalent(foo, bar, stack) {
  if (!stack)
    stack = Components.stack.caller;

  var text = foo + ".equals(" + bar + ")";

  if (foo.equals && foo.equals(bar)) {
    dump("TEST-PASS | " + stack.filename + " | [" + stack.name + " : " +
         stack.lineNumber + "] " + text + "\n");
    return;
  }
  do_throw(text, stack);
}

function listener(csp, cspr_static) {
  this.buffer = "";
  this._csp = csp;
  this._cspr_static = cspr_static;
}

listener.prototype = {
  onStartRequest: function (request, ctx) {
  },

  onDataAvailable: function (request, ctx, stream, offset, count) {
    var sInputStream = Cc["@mozilla.org/scriptableinputstream;1"]
      .createInstance(Ci.nsIScriptableInputStream);
    sInputStream.init(stream);
    this.buffer = this.buffer.concat(sInputStream.read(count));
  },

  onStopRequest: function (request, ctx, status) {
    
    
    if (this.buffer == CSP_DOC_BODY) {

      
      
      
      
      let cspr_str = this._csp.policy;
      let cspr = CSPRep.fromString(cspr_str, mkuri(DOCUMENT_URI));

      
      
      
      let cspr_static_str = this._cspr_static.toString();
      let cspr_static_reparse = CSPRep.fromString(cspr_static_str, mkuri(DOCUMENT_URI));

      
      do_check_neq(null, cspr);
      do_check_true(cspr.equals(cspr_static_reparse));

      
      if (TESTS.length == 0) {
        httpserv.stop(do_test_finished);
      } else {
        do_test_finished();
        (TESTS.shift())();
      }
    }
  }
};

function run_test() {
  httpserv = new HttpServer();
  httpserv.registerPathHandler("/document", csp_doc_response);
  httpserv.registerPathHandler("/policy", csp_policy_response);
  httpserv.start(POLICY_PORT);
  TESTS = [ test_CSPRep_fromPolicyURI, test_CSPRep_fromRelativePolicyURI ];

  
  
  (TESTS.shift())();
}

function makeChan(url) {
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  var chan = ios.newChannel(url, null, null).QueryInterface(Ci.nsIHttpChannel);
  return chan;
}

function csp_doc_response(metadata, response) {
  response.setStatusLine(metadata.httpVersion, 200, "OK");
  response.setHeader("Content-Type", "text/html", false);
  response.bodyOutputStream.write(CSP_DOC_BODY, CSP_DOC_BODY.length);
}

function csp_policy_response(metadata, response) {
  response.setStatusLine(metadata.httpVersion, 200, "OK");
  response.setHeader("Content-Type", "text/csp", false);
  response.bodyOutputStream.write(POLICY_FROM_URI, POLICY_FROM_URI.length);
}


function test_CSPRep_fromPolicyURI() {
  do_test_pending();
  let csp = Cc["@mozilla.org/contentsecuritypolicy;1"]
              .createInstance(Ci.nsIContentSecurityPolicy);
  
  
  
  let cspr_static = CSPRep.fromString(POLICY_FROM_URI, mkuri(DOCUMENT_URI));

  
  var docChan = makeChan(DOCUMENT_URI);
  docChan.asyncOpen(new listener(csp, cspr_static), null);

  
  
  
  CSPRep.fromString("policy-uri " + POLICY_URI,
                    mkuri(DOCUMENT_URI), docChan, csp);
}

function test_CSPRep_fromRelativePolicyURI() {
  do_test_pending();
  let csp = Cc["@mozilla.org/contentsecuritypolicy;1"]
              .createInstance(Ci.nsIContentSecurityPolicy);
  
  
  
  let cspr_static = CSPRep.fromString(POLICY_FROM_URI, mkuri(DOCUMENT_URI));

  
  var docChan = makeChan(DOCUMENT_URI);
  docChan.asyncOpen(new listener(csp, cspr_static), null);

  
  
  
  CSPRep.fromString("policy-uri " + POLICY_URI_RELATIVE,
                    mkuri(DOCUMENT_URI), docChan, csp);
}
