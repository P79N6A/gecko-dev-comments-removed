



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;


Cu.import('resource://gre/modules/CSPUtils.jsm');
Cu.import('resource://gre/modules/NetUtil.jsm');


Cu.import("resource://testing-common/httpd.js");

var httpServer = new HttpServer();

const POLICY_FROM_URI = "allow 'self'; img-src *";
const POLICY_PORT = 9000;
const POLICY_URI = "http://localhost:" + POLICY_PORT + "/policy";
const POLICY_URI_RELATIVE = "/policy";


function URI(uriString) {
  var ioService = Cc["@mozilla.org/network/io-service;1"]
                    .getService(Ci.nsIIOService);
  return ioService.newURI(uriString, null, null);
}



function do_check_in_array(arr, val, stack) {
  if (!stack)
    stack = Components.stack.caller;

  var text = val + " in [" + arr.join(",") + "]";

  for(var i in arr) {
    dump(".......... " + i + "> " + arr[i] + "\n");
    if(arr[i] == val) {
      
      ++_passedChecks;
      dump("TEST-PASS | " + stack.filename + " | [" + stack.name + " : " +
           stack.lineNumber + "] " + text + "\n");
      return;
    }
  }
  do_throw(text, stack);
}


function do_check_has_key(foo, key, stack) {
  if (!stack) 
    stack = Components.stack.caller;

  var keys = [];
  for (let k in foo) { keys.push(k); }
  var text = key + " in [" + keys.join(",") + "]";

  for (var x in foo) {
    if (x == key) {
      
      ++_passedChecks;
      dump("TEST-PASS | " + stack.filename + " | [" + stack.name + " : " +
           stack.lineNumber + "] " + text + "\n");
      return;
    }
  }
  do_throw(text, stack);
}


function do_check_equivalent(foo, bar, stack) {
  if (!stack) 
    stack = Components.stack.caller;

  var text = foo + ".equals(" + bar + ")";

  if(foo.equals && foo.equals(bar)) {
    ++_passedChecks;
      dump("TEST-PASS | " + stack.filename + " | [" + stack.name + " : " +
           stack.lineNumber + "] " + text + "\n");
      return;
  }
  do_throw(text, stack);
}

var tests = [];
function test(fcn) {
  tests.push(fcn);
}

test(
  function test_CSPHost_fromstring() {
    var h;

    h = CSPHost.fromString("*");
    do_check_neq(null, h); 

    h = CSPHost.fromString("foo.bar");
    do_check_neq(null, h); 

    h = CSPHost.fromString("*.bar");
    do_check_neq(null, h); 

    h = CSPHost.fromString("foo.*.bar");
    do_check_eq(null, h); 

    h = CSPHost.fromString("com");
    do_check_neq(null, h); 

    h = CSPHost.fromString("f00b4r.com");
    do_check_neq(null, h); 

    h = CSPHost.fromString("foo-bar.com");
    do_check_neq(null, h); 

    h = CSPHost.fromString("foo!bar.com");
    do_check_eq(null, h); 
  });

test(
  function test_CSPHost_clone() {
    h = CSPHost.fromString("*.a.b.c");
    h2 = h.clone();
    for(var i in h._segments) {
      
      do_check_eq(h._segments[i], h2._segments[i]);
    }
  });

test(
  function test_CSPHost_permits() {
    var h = CSPHost.fromString("*.b.c");
    var h2 = CSPHost.fromString("a.b.c");
    do_check_true( h.permits(h2));       
    do_check_true( h.permits("a.b.c"));  
    do_check_false(h.permits("b.c"));    
    do_check_false(h.permits("a.a.c"));  
    do_check_false(h2.permits(h));       
    do_check_false(h2.permits("b.c"));   
    do_check_true( h2.permits("a.b.c")); 
  });

test(
    function test_CSPHost_intersectWith() {
      var h = CSPHost.fromString("*.b.c");
      
      do_check_eq("*.a.b.c", h.intersectWith(CSPHost.fromString("*.a.b.c")).toString());

      
      do_check_eq(null, h.intersectWith(CSPHost.fromString("*.d.e")));
    });



test(
    function test_CSPSource_fromString() {
    
      
      do_check_neq(null, CSPSource.fromString("a.com", "http://abc.com"));

      
      do_check_neq(null, CSPSource.fromString("a2-c.com", "https://a.com"));

      
      do_check_neq(null, CSPSource.fromString("*.a.com", "http://abc.com"));

      
      
      do_check_eq(null, CSPSource.fromString("x.*.a.com", "http://a.com"));

      
      do_check_eq(null, CSPSource.fromString("a#2-c.com", "http://a.com"));

      

      
      do_check_neq(null, CSPSource.create("a.com:23", "http://a.com"));
      
      do_check_neq(null, CSPSource.create("https://a.com", "http://a.com"));
      
      do_check_neq(null, CSPSource.create("https://a.com:200", "http://a.com"));

      
      do_check_eq(null, CSPSource.create("http://foo.com:bar.com:23"));
      
      do_check_neq(null, CSPSource.create("data:"));
      do_check_neq(null, CSPSource.create("javascript:"));
    });

test(
    function test_CSPSource_fromString_withSelf() {
      var src;
      src = CSPSource.create("a.com", "https://foobar.com:443");
      
      do_check_true(src.permits("https://a.com:443"));
      
      do_check_false(src.permits("http://a.com"));
      
      do_check_true(src.permits("https://a.com"));
      
      src = CSPSource.create("http://a.com", "https://foobar.com:443");
      
      do_check_false(src.permits("https://a.com"));
      
      do_check_true(src.permits("http://a.com"));
      
      
      do_check_true(src.permits("http://a.com:80"));
      
      src = CSPSource.create("'self'", "https://foobar.com:443");
      
      do_check_true(src.permits("https://foobar.com:443"));
      
      do_check_false(src.permits("http://foobar.com"));
      
      do_check_true(src.permits("https://foobar.com"));
      
      do_check_false(src.permits("https://a.com"));

      src = CSPSource.create("javascript:", "https://foobar.com:443");
      
      var aUri = NetUtil.newURI("javascript:alert('foo');");
      do_check_true(src.permits(aUri));
      
      do_check_false(src.permits("https://a.com"));
      
      do_check_false(src.permits("https://foobar.com"));

    });



test(
    function test_CSPSourceList_fromString() {
      var sd = CSPSourceList.fromString("'none'");
      
      do_check_neq(null,sd);
      
      do_check_eq(0, sd._sources.length);
      do_check_true(sd.isNone());

      sd = CSPSourceList.fromString("*");
      
      do_check_eq(0, sd._sources.length);

      
      
      do_check_true(CSPSourceList.fromString("f!oo.bar").isNone());
      
      do_check_true(CSPSourceList.fromString("ht!ps://f-oo.bar").isNone());
      
      do_check_true(CSPSourceList.fromString("https://f-oo.bar:3f").isNone());
      
    });

test(
    function test_CSPSourceList_fromString_twohost() {
      var str = "foo.bar:21 https://ras.bar";
      var parsed = "http://foo.bar:21 https://ras.bar:443";
      var sd = CSPSourceList.fromString(str, URI("http://self.com:80"));
      
      do_check_neq(null,sd);
      
      do_check_eq(2, sd._sources.length);
      
      do_check_eq(parsed, sd.toString());
    });

test(
    function test_CSPSourceList_permits() {
      var nullSourceList = CSPSourceList.fromString("'none'");
      var simpleSourceList = CSPSourceList.fromString("a.com", URI("http://self.com"));
      var doubleSourceList = CSPSourceList.fromString("https://foo.com http://bar.com:88",
                                                      URI("http://self.com:88"));
      var allSourceList = CSPSourceList.fromString("*");
      var allAndMoreSourceList = CSPSourceList.fromString("* https://bar.com 'none'");

      
      do_check_false( nullSourceList.permits("http://a.com"));
      
      do_check_true( simpleSourceList.permits("http://a.com"));
      
      do_check_false( simpleSourceList.permits("http://b.com"));
      
      do_check_true( doubleSourceList.permits("http://bar.com:88"));
      
      do_check_false( doubleSourceList.permits("https://bar.com:88"));
      
      do_check_false( doubleSourceList.permits("http://bar.com:443"));
      
      do_check_false( doubleSourceList.permits("https://foo.com:88"));
      
      do_check_false( doubleSourceList.permits("http://foo.com"));

      
      do_check_true( allSourceList.permits("http://x.com:23"));
      
      do_check_true( allSourceList.permits("http://a.b.c.d.e.f.g.h.i.j.k.l.x.com"));

      
      do_check_true(allAndMoreSourceList.permits("http://a.com"));
    });

test(
    function test_CSPSourceList_intersect() {
      
      
      
      var nullSourceList = CSPSourceList.fromString("'none'");
      var simpleSourceList = CSPSourceList.fromString("http://a.com");
      var doubleSourceList = CSPSourceList.fromString("https://foo.com http://bar.com:88");
      var singleFooSourceList = CSPSourceList.fromString("https://foo.com");
      var allSourceList = CSPSourceList.fromString("*");

      
      do_check_true(nullSourceList.intersectWith(simpleSourceList).isNone());
      
      do_check_true(nullSourceList.intersectWith(doubleSourceList).isNone());
      
      do_check_true(nullSourceList.intersectWith(allSourceList).isNone());

      
      do_check_equivalent(allSourceList.intersectWith(simpleSourceList),
                          simpleSourceList);
      
      do_check_equivalent(allSourceList.intersectWith(doubleSourceList),
                          doubleSourceList);

      
      do_check_true(simpleSourceList.intersectWith(doubleSourceList).isNone());

      
      do_check_equivalent(singleFooSourceList,
                          doubleSourceList.intersectWith(singleFooSourceList));

      

    });



test(
    function test_CSPRep_fromString() {

      
      

      var cspr;
      var cspr_allowval;
      var SD = CSPRep.SRC_DIRECTIVES;

      
      cspr = CSPRep.fromString("allow *", URI("http://self.com:80"));
      
      do_check_has_key(cspr._directives, SD.DEFAULT_SRC);

      
      
      cspr_allowval = cspr._directives[SD.DEFAULT_SRC];
      for(var d in SD) {
        
        do_check_has_key(cspr._directives, SD[d]);
        
        do_check_eq(cspr._directives[SD[d]].toString(), cspr_allowval.toString());
      }
    });


test(
    function test_CSPRep_defaultSrc() {
      var cspr, cspr_default_val, cspr_allow;
      var SD = CSPRep.SRC_DIRECTIVES;

      
      cspr = CSPRep.fromString("default-src *", URI("http://self.com:80"));
      
      do_check_has_key(cspr._directives, SD.DEFAULT_SRC);

      
      
      cspr_default_val = cspr._directives[SD.DEFAULT_SRC];
      for (var d in SD) {
        do_check_has_key(cspr._directives, SD[d]);
        
        do_check_eq(cspr._directives[SD[d]].toString(), cspr_default_val.toString());
      }

      
      
      cspr = CSPRep.fromString("default-src *", URI("http://self.com:80"));
      cspr_allow = CSPRep.fromString("allow *", URI("http://self.com:80"));

      for (var d in SD) {
        do_check_equivalent(cspr._directives[SD[d]],
                            cspr_allow._directives[SD[d]]);
      }
    });


test(
    function test_CSPRep_fromString_oneDir() {

      var cspr;
      var SD = CSPRep.SRC_DIRECTIVES;
      var DEFAULTS = [SD.STYLE_SRC, SD.MEDIA_SRC, SD.IMG_SRC, SD.FRAME_SRC];

      
      cspr = CSPRep.fromString("allow bar.com; script-src https://foo.com", 
                               URI("http://self.com"));

      for(var x in DEFAULTS) {
        
        do_check_false(cspr.permits("http://bar.com:22", DEFAULTS[x]));
        
        do_check_true(cspr.permits("http://bar.com:80", DEFAULTS[x]));
        
        do_check_false(cspr.permits("https://foo.com:400", DEFAULTS[x]));
        
        do_check_false(cspr.permits("https://foo.com", DEFAULTS[x]));
      }
      
      do_check_false(cspr.permits("http://bar.com:22", SD.SCRIPT_SRC));
      
      do_check_true(cspr.permits("https://foo.com:443", SD.SCRIPT_SRC));
    });

test(
    function test_CSPRep_fromString_twodir() {
      var cspr;
      var SD = CSPRep.SRC_DIRECTIVES;
      var DEFAULTS = [SD.STYLE_SRC, SD.MEDIA_SRC, SD.FRAME_SRC];

      
      var polstr = "allow allow.com; "
                  + "script-src https://foo.com; "
                  + "img-src bar.com:*";
      cspr = CSPRep.fromString(polstr, URI("http://self.com"));

      for(var x in DEFAULTS) {
        do_check_true(cspr.permits("http://allow.com", DEFAULTS[x]));
        
        do_check_false(cspr.permits("https://foo.com:400", DEFAULTS[x]));
        
        do_check_false(cspr.permits("http://bar.com:400", DEFAULTS[x]));
        
      }
      
      do_check_false(cspr.permits("http://allow.com:22", SD.IMG_SRC));
      
      do_check_false(cspr.permits("https://foo.com:400", SD.IMG_SRC));
      
      do_check_true(cspr.permits("http://bar.com:88", SD.IMG_SRC));

      
      do_check_false(cspr.permits("http://allow.com:22", SD.SCRIPT_SRC));
      
      do_check_true(cspr.permits("https://foo.com:443", SD.SCRIPT_SRC));
      
      do_check_false(cspr.permits("http://bar.com:400", SD.SCRIPT_SRC));
    });

test(function test_CSPRep_fromString_withself() {
      var cspr;
      var SD = CSPRep.SRC_DIRECTIVES;
      var self = "https://self.com:34";

      
      cspr = CSPRep.fromString("allow 'self'; script-src 'self' https://*:*",
                              URI(self));
      
      do_check_false(cspr.permits("https://foo.com:400", SD.IMG_SRC));
      
      do_check_true(cspr.permits(self, SD.IMG_SRC));
      
      do_check_false(cspr.permits("http://evil.com", SD.SCRIPT_SRC));
      
      do_check_true(cspr.permits(self, SD.SCRIPT_SRC));
      
      do_check_true(cspr.permits("https://evil.com:100", SD.SCRIPT_SRC));
     });



test(function test_FrameAncestor_defaults() {
      var cspr;
      var SD = CSPRep.SRC_DIRECTIVES;
      var self = "http://self.com:34";

      cspr = CSPRep.fromString("allow 'none'", URI(self));

      
      do_check_true(cspr.permits("https://foo.com:400", SD.FRAME_ANCESTORS));
      do_check_true(cspr.permits("http://self.com:34", SD.FRAME_ANCESTORS));
      do_check_true(cspr.permits("https://self.com:34", SD.FRAME_ANCESTORS));
      do_check_true(cspr.permits("http://self.com", SD.FRAME_ANCESTORS));
      do_check_true(cspr.permits("http://subd.self.com:34", SD.FRAME_ANCESTORS));

      cspr = CSPRep.fromString("allow 'none'; frame-ancestors 'self'", URI(self));

      
      do_check_true(cspr.permits("http://self.com:34", SD.FRAME_ANCESTORS));
      do_check_false(cspr.permits("https://foo.com:400", SD.FRAME_ANCESTORS));
      do_check_false(cspr.permits("https://self.com:34", SD.FRAME_ANCESTORS));
      do_check_false(cspr.permits("http://self.com", SD.FRAME_ANCESTORS));
      do_check_false(cspr.permits("http://subd.self.com:34", SD.FRAME_ANCESTORS));
     });

test(function test_CSP_ReportURI_parsing() {
      var cspr;
      var SD = CSPRep.SRC_DIRECTIVES;
      var self = "http://self.com:34";
      var parsedURIs = [];

      var uri_valid_absolute = self + "/report.py";
      var uri_invalid_host_absolute = "http://foo.org:34/report.py";
      var uri_valid_relative = "/report.py";
      var uri_valid_relative_expanded = self + uri_valid_relative;
      var uri_valid_relative2 = "foo/bar/report.py";
      var uri_valid_relative2_expanded = self + "/" + uri_valid_relative2;
      var uri_invalid_relative = "javascript:alert(1)";

      cspr = CSPRep.fromString("allow *; report-uri " + uri_valid_absolute, URI(self));
      parsedURIs = cspr.getReportURIs().split(/\s+/);
      do_check_in_array(parsedURIs, uri_valid_absolute);
      do_check_eq(parsedURIs.length, 1);

      cspr = CSPRep.fromString("allow *; report-uri " + uri_invalid_host_absolute, URI(self));
      parsedURIs = cspr.getReportURIs().split(/\s+/);
      do_check_in_array(parsedURIs, "");
      do_check_eq(parsedURIs.length, 1); 

      cspr = CSPRep.fromString("allow *; report-uri " + uri_invalid_relative, URI(self));
      parsedURIs = cspr.getReportURIs().split(/\s+/);
      do_check_in_array(parsedURIs, "");
      do_check_eq(parsedURIs.length, 1);

      cspr = CSPRep.fromString("allow *; report-uri " + uri_valid_relative, URI(self));
      parsedURIs = cspr.getReportURIs().split(/\s+/);
      do_check_in_array(parsedURIs, uri_valid_relative_expanded);
      do_check_eq(parsedURIs.length, 1);

      cspr = CSPRep.fromString("allow *; report-uri " + uri_valid_relative2, URI(self));
      parsedURIs = cspr.getReportURIs().split(/\s+/);
      dump(parsedURIs.length);
      do_check_in_array(parsedURIs, uri_valid_relative2_expanded);
      do_check_eq(parsedURIs.length, 1);

      
      cspr = CSPRep.fromString("allow *; report-uri " +
                               uri_valid_relative2 + " " +
                               uri_valid_absolute, URI(self));
      parsedURIs = cspr.getReportURIs().split(/\s+/);
      do_check_in_array(parsedURIs, uri_valid_relative2_expanded);
      do_check_in_array(parsedURIs, uri_valid_absolute);
      do_check_eq(parsedURIs.length, 2);

      cspr = CSPRep.fromString("allow *; report-uri " +
                               uri_valid_relative2 + " " +
                               uri_invalid_host_absolute + " " +
                               uri_valid_absolute, URI(self));
      parsedURIs = cspr.getReportURIs().split(/\s+/);
      do_check_in_array(parsedURIs, uri_valid_relative2_expanded);
      do_check_in_array(parsedURIs, uri_valid_absolute);
      do_check_eq(parsedURIs.length, 2);
    });

test(
    function test_bug672961_withNonstandardSelfPort() {
      














      var src;
      src = CSPSource.create("a.com", "https://foobar.com:4443");
      
      do_check_false(src.permits("http://a.com"));
      
      do_check_true(src.permits("https://a.com"));
      
      do_check_true(src.permits("https://a.com:443"));
      
      src = CSPSource.create("http://a.com", "https://foobar.com:4443");
      
      do_check_false(src.permits("https://a.com"));
      
      do_check_true(src.permits("http://a.com"));
      
      do_check_true(src.permits("http://a.com:80"));
      
      src = CSPSource.create("'self'", "https://foobar.com:4443");
      
      do_check_true(src.permits("https://foobar.com:4443"));
      do_check_false(src.permits("https://foobar.com"));
      do_check_false(src.permits("https://foobar.com:443"));

      
      do_check_false(src.permits("http://foobar.com:4443"));
      do_check_false(src.permits("http://foobar.com"));

    });

test(
    function test_bug634773_noneAndStarAreDifferent() {
      





      var p_none = CSPSourceList.fromString("'none'", "http://foo.com", false);
      var p_all = CSPSourceList.fromString("*", "http://foo.com", false);
      var p_one = CSPSourceList.fromString("bar.com", "http://foo.com", false);

      do_check_false(p_none.equals(p_all));
      do_check_false(p_none.equals(p_one));
      do_check_false(p_all.equals(p_none));
      do_check_false(p_all.equals(p_one));

      do_check_true(p_all.permits("http://bar.com"));
      do_check_true(p_one.permits("http://bar.com"));
      do_check_false(p_none.permits("http://bar.com"));
    });




























function run_test() {
  function policyresponder(request,response) {
    response.setStatusLine(request.httpVersion, 200, "OK");
    response.setHeader("Content-Type", "text/csp", false);
    response.bodyOutputStream.write(POLICY_FROM_URI, POLICY_FROM_URI.length);
  }
  
  httpServer.registerPathHandler("/policy", policyresponder);
  httpServer.start(POLICY_PORT);

  for(let i in tests) {
    tests[i]();
  }

  
  httpServer.stop(function() { });
  do_test_finished();
}



