



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;


Cu.import('resource://gre/modules/CSPUtils.jsm');
Cu.import('resource://gre/modules/NetUtil.jsm');

var httpServer = new HttpServer();
httpServer.start(-1);

const POLICY_FROM_URI = "default-src 'self'; img-src *";
const POLICY_PORT = httpServer.identity.primaryPort;
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

    h = CSPHost.fromString("{app-url-is-uid}");
    do_check_neq(null, h); 
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
    function test_CSPSource_fromString() {
    
      
      do_check_neq(null, CSPSource.fromString("a.com", undefined, "http://abc.com"));

      
      do_check_neq(null, CSPSource.fromString("a2-c.com", undefined, "https://a.com"));

      
      do_check_neq(null, CSPSource.fromString("*.a.com", undefined, "http://abc.com"));

      
      
      do_check_eq(null, CSPSource.fromString("x.*.a.com", undefined, "http://a.com"));

      
      do_check_eq(null, CSPSource.fromString("a#2-c.com", undefined, "http://a.com"));

      

      
      do_check_neq(null, CSPSource.create("a.com:23", undefined, "http://a.com"));
      
      do_check_neq(null, CSPSource.create("https://a.com", undefined, "http://a.com"));
      
      do_check_neq(null, CSPSource.create("https://a.com:200", undefined, "http://a.com"));

      
      do_check_eq(null, CSPSource.create("http://foo.com:bar.com:23"));
      
      do_check_neq(null, CSPSource.create("data:"));
      do_check_neq(null, CSPSource.create("javascript:"));

      
      do_check_neq(null, CSPSource.fromString("{app-host-is-uid}", undefined, "app://{app-host-is-uid}"));
    });

test(
    function test_CSPSource_fromString_withSelf() {
      var src;
      src = CSPSource.create("a.com", undefined, "https://foobar.com:443");
      
      do_check_true(src.permits("https://a.com:443"));
      
      do_check_false(src.permits("http://a.com"));
      
      do_check_true(src.permits("https://a.com"));

      src = CSPSource.create("http://a.com", undefined, "https://foobar.com:443");
      
      do_check_false(src.permits("https://a.com"));
      
      do_check_true(src.permits("http://a.com"));
      
      
      do_check_true(src.permits("http://a.com:80"));

      src = CSPSource.create("'self'", undefined, "https://foobar.com:443");
      
      do_check_true(src.permits("https://foobar.com:443"));
      
      do_check_false(src.permits("http://foobar.com"));
      
      do_check_true(src.permits("https://foobar.com"));
      
      do_check_false(src.permits("https://a.com"));

      src = CSPSource.create("javascript:", undefined, "https://foobar.com:443");
      
      var aUri = NetUtil.newURI("javascript:alert('foo');");
      do_check_true(src.permits(aUri));
      
      do_check_false(src.permits("https://a.com"));
      
      do_check_false(src.permits("https://foobar.com"));

      src = CSPSource.create("{app-host-is-uid}", undefined, "app://{app-host-is-uid}");
      
      do_check_false(src.permits("https://{app-host-is-uid}"));
      
      do_check_true(src.permits("app://{app-host-is-uid}"));

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
      var parsed = "http://foo.bar:21 https://ras.bar";
      var sd = CSPSourceList.fromString(str, undefined, URI("http://self.com:80"));
      
      do_check_neq(null,sd);
      
      do_check_eq(2, sd._sources.length);
      
      do_check_eq(parsed, sd.toString());
    });

test(
    function test_CSPSourceList_permits() {
      var nullSourceList = CSPSourceList.fromString("'none'");
      var simpleSourceList = CSPSourceList.fromString("a.com", undefined, URI("http://self.com"));
      var doubleSourceList = CSPSourceList.fromString("https://foo.com http://bar.com:88",
                                                      undefined,
                                                      URI("http://self.com:88"));
      var allSourceList = CSPSourceList.fromString("*");
      var allAndMoreSourceList = CSPSourceList.fromString("* https://bar.com 'none'");
      var wildcardHostSourceList = CSPSourceList.fromString("*.foo.com",
                                                            undefined, URI("http://self.com"));
      var allDoubledHostSourceList = CSPSourceList.fromString("**");
      var allGarbageHostSourceList = CSPSourceList.fromString("*a");

      
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

      
      do_check_false(allDoubledHostSourceList.permits("http://barbaz.com"));
      
      do_check_false(allGarbageHostSourceList.permits("http://barbaz.com"));

      
      do_check_true(wildcardHostSourceList.permits("http://somerandom.foo.com"));
      
      do_check_false(wildcardHostSourceList.permits("http://barbaz.com"));
    });


test(
    function test_CSPRep_fromString() {

      var cspr;
      var cspr_allowval;
      var SD = CSPRep.SRC_DIRECTIVES;
      var DEFAULTS = [SD.STYLE_SRC, SD.MEDIA_SRC, SD.IMG_SRC, SD.SCRIPT_SRC, SD.FONT_SRC,
                      SD.OBJECT_SRC, SD.FRAME_SRC, SD.CONNECT_SRC];

      
      cspr = CSPRep.fromString("default-src *", URI("http://self.com:80"));
      
      do_check_has_key(cspr._directives, SD.DEFAULT_SRC);

      for(var x in DEFAULTS) {
        
        
        do_check_true(cspr.permits("http://bar.com", DEFAULTS[x]));
      }
    });


test(
    function test_CSPRep_fromString_oneDir() {

      var cspr;
      var SD = CSPRep.SRC_DIRECTIVES;
      var DEFAULTS = [SD.STYLE_SRC, SD.MEDIA_SRC, SD.IMG_SRC,
                      SD.FRAME_SRC, SD.CONNECT_SRC];

      
      cspr = CSPRep.fromString("default-src bar.com; script-src https://foo.com",
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
    function test_CSPRep_fromString_twoDir() {
      var cspr;

      var SD = CSPRep.SRC_DIRECTIVES;

      var DEFAULTS = [SD.STYLE_SRC, SD.MEDIA_SRC, SD.FRAME_SRC,
                      SD.CONNECT_SRC];

      
      var polstr = "default-src allow.com; " +
                   "script-src https://foo.com; " +
                   "img-src bar.com:*";
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
      var self = "https://self.com:34";
      var SD = CSPRep.SRC_DIRECTIVES;

      
      cspr = CSPRep.fromString("default-src 'self'; script-src 'self' https://*:*",
                               URI(self));
      
      do_check_false(cspr.permits("https://foo.com:400", SD.IMG_SRC));
      
      do_check_true(cspr.permits(self, SD.IMG_SRC));
      
      do_check_false(cspr.permits("http://evil.com", SD.SCRIPT_SRC));
      
      do_check_true(cspr.permits(self, SD.SCRIPT_SRC));
      
      do_check_true(cspr.permits("https://evil.com:100", SD.SCRIPT_SRC));
     });




test(function test_FrameAncestor_defaults() {
      var cspr;
      var self = "http://self.com:34";
      var SD = CSPRep.SRC_DIRECTIVES;

      cspr = CSPRep.fromString("default-src 'none'", URI(self));

      
      do_check_true(cspr.permits("https://foo.com:400", SD.FRAME_ANCESTORS));
      do_check_true(cspr.permits("http://self.com:34", SD.FRAME_ANCESTORS));
      do_check_true(cspr.permits("https://self.com:34", SD.FRAME_ANCESTORS));
      do_check_true(cspr.permits("http://self.com", SD.FRAME_ANCESTORS));
      do_check_true(cspr.permits("http://subd.self.com:34", SD.FRAME_ANCESTORS));

      cspr = CSPRep.fromString("default-src 'none'; frame-ancestors 'self'", URI(self));

      
      do_check_true(cspr.permits("http://self.com:34", SD.FRAME_ANCESTORS));
      do_check_false(cspr.permits("https://foo.com:400", SD.FRAME_ANCESTORS));
      do_check_false(cspr.permits("https://self.com:34", SD.FRAME_ANCESTORS));
      do_check_false(cspr.permits("http://self.com", SD.FRAME_ANCESTORS));
      do_check_false(cspr.permits("http://subd.self.com:34", SD.FRAME_ANCESTORS));
     });


test(function test_FrameAncestor_TLD_defaultPorts() {
      var cspr;
      var SD = CSPRep.SRC_DIRECTIVES;
      var self = "http://self"; 

      cspr = CSPRep.fromString("default-src 'self'; frame-ancestors 'self' http://foo:80 bar:80 http://three", URI(self));

      
      do_check_true(cspr.permits("http://self", SD.FRAME_ANCESTORS));
      do_check_true(cspr.permits("http://self:80", SD.FRAME_ANCESTORS));
      do_check_true(cspr.permits("http://foo", SD.FRAME_ANCESTORS));
      do_check_true(cspr.permits("http://foo:80", SD.FRAME_ANCESTORS));
      do_check_true(cspr.permits("http://bar", SD.FRAME_ANCESTORS));
      do_check_true(cspr.permits("http://three:80", SD.FRAME_ANCESTORS));

      do_check_false(cspr.permits("https://foo:400", SD.FRAME_ANCESTORS));
      do_check_false(cspr.permits("https://self:34", SD.FRAME_ANCESTORS));
      do_check_false(cspr.permits("https://bar", SD.FRAME_ANCESTORS));
      do_check_false(cspr.permits("http://three:81", SD.FRAME_ANCESTORS));
      do_check_false(cspr.permits("https://three:81", SD.FRAME_ANCESTORS));
     });

test(function test_FrameAncestor_ignores_userpass_bug779918() {
      var cspr;
      var SD = CSPRep.SRC_DIRECTIVES;
      var self = "http://self.com/bar";
      var testPolicy = "default-src 'self'; frame-ancestors 'self'";

      cspr = CSPRep.fromString(testPolicy, URI(self));

      
      do_check_true(cspr.permits(URI("http://username:password@self.com/foo"), SD.FRAME_ANCESTORS));
      do_check_true(cspr.permits(URI("http://other:pass1@self.com/foo"), SD.FRAME_ANCESTORS));
      do_check_true(cspr.permits(URI("http://self.com:80/foo"), SD.FRAME_ANCESTORS));
      do_check_true(cspr.permits(URI("http://self.com/foo"), SD.FRAME_ANCESTORS));

      
      
      
      function testPermits(aChildUri, aParentUri, aContentType) {
        let cspObj = Cc["@mozilla.org/contentsecuritypolicy;1"]
                       .createInstance(Ci.nsIContentSecurityPolicy);
        cspObj.appendPolicy(testPolicy, aChildUri, false);
        let docshellparent = Cc["@mozilla.org/docshell;1"]
                               .createInstance(Ci.nsIDocShell);
        let docshellchild  = Cc["@mozilla.org/docshell;1"]
                               .createInstance(Ci.nsIDocShell);
        docshellparent.setCurrentURI(aParentUri);
        docshellchild.setCurrentURI(aChildUri);
        docshellparent.addChild(docshellchild);
        return cspObj.permitsAncestry(docshellchild);
      };

      
      do_check_true(testPermits(URI("http://username:password@self.com/foo"),
                                URI("http://self.com/bar")));
      do_check_true(testPermits(URI("http://user1:pass1@self.com/foo"),
                                URI("http://self.com/bar")));
      do_check_true(testPermits(URI("http://self.com/foo"),
                                URI("http://self.com/bar")));

      
      do_check_true(testPermits(URI("http://username:password@self.com/foo"),
                                URI("http://username:password@self.com/bar")));
      do_check_true(testPermits(URI("http://user1:pass1@self.com/foo"),
                                URI("http://username:password@self.com/bar")));
      do_check_true(testPermits(URI("http://self.com/foo"),
                                URI("http://username:password@self.com/bar")));
     });

test(function test_CSP_ReportURI_parsing() {
      var cspr;
      var SD = CSPRep.SRC_DIRECTIVES;
      var self = "http://self.com:34";
      var parsedURIs = [];

      var uri_valid_absolute = self + "/report.py";
      var uri_other_host_absolute = "http://foo.org:34/report.py";
      var uri_valid_relative = "/report.py";
      var uri_valid_relative_expanded = self + uri_valid_relative;
      var uri_valid_relative2 = "foo/bar/report.py";
      var uri_valid_relative2_expanded = self + "/" + uri_valid_relative2;
      var uri_invalid_relative = "javascript:alert(1)";
      var uri_other_scheme_absolute = "https://self.com/report.py";
      var uri_other_scheme_and_host_absolute = "https://foo.com/report.py";

      cspr = CSPRep.fromString("default-src *; report-uri " + uri_valid_absolute, URI(self));
      parsedURIs = cspr.getReportURIs().split(/\s+/);
      do_check_in_array(parsedURIs, uri_valid_absolute);
      do_check_eq(parsedURIs.length, 1);

      cspr = CSPRep.fromString("default-src *; report-uri " + uri_other_host_absolute, URI(self));
      parsedURIs = cspr.getReportURIs().split(/\s+/);
      do_check_in_array(parsedURIs, uri_other_host_absolute);
      do_check_eq(parsedURIs.length, 1); 

      cspr = CSPRep.fromString("default-src *; report-uri " + uri_invalid_relative, URI(self));
      parsedURIs = cspr.getReportURIs().split(/\s+/);
      do_check_in_array(parsedURIs, "");
      do_check_eq(parsedURIs.length, 1);

      cspr = CSPRep.fromString("default-src *; report-uri " + uri_valid_relative, URI(self));
      parsedURIs = cspr.getReportURIs().split(/\s+/);
      do_check_in_array(parsedURIs, uri_valid_relative_expanded);
      do_check_eq(parsedURIs.length, 1);

      cspr = CSPRep.fromString("default-src *; report-uri " + uri_valid_relative2, URI(self));
      parsedURIs = cspr.getReportURIs().split(/\s+/);
      dump(parsedURIs.length);
      do_check_in_array(parsedURIs, uri_valid_relative2_expanded);
      do_check_eq(parsedURIs.length, 1);

      
      cspr = CSPRep.fromString("default-src *; report-uri " + uri_other_scheme_absolute, URI(self));
      parsedURIs = cspr.getReportURIs().split(/\s+/);
      dump(parsedURIs.length);
      do_check_in_array(parsedURIs, uri_other_scheme_absolute);
      do_check_eq(parsedURIs.length, 1);

      
      cspr = CSPRep.fromString("default-src *; report-uri " + uri_other_scheme_and_host_absolute, URI(self));
      parsedURIs = cspr.getReportURIs().split(/\s+/);
      dump(parsedURIs.length);
      do_check_in_array(parsedURIs, uri_other_scheme_and_host_absolute);
      do_check_eq(parsedURIs.length, 1);

      
      cspr = CSPRep.fromString("default-src *; report-uri " +
                               uri_valid_relative2 + " " +
                               uri_valid_absolute, URI(self));
      parsedURIs = cspr.getReportURIs().split(/\s+/);
      do_check_in_array(parsedURIs, uri_valid_relative2_expanded);
      do_check_in_array(parsedURIs, uri_valid_absolute);
      do_check_eq(parsedURIs.length, 2);

      cspr = CSPRep.fromString("default-src *; report-uri " +
                               uri_valid_relative2 + " " +
                               uri_other_host_absolute + " " +
                               uri_valid_absolute, URI(self));
      parsedURIs = cspr.getReportURIs().split(/\s+/);
      do_check_in_array(parsedURIs, uri_valid_relative2_expanded);
      do_check_in_array(parsedURIs, uri_other_host_absolute);
      do_check_in_array(parsedURIs, uri_valid_absolute);
      do_check_eq(parsedURIs.length, 3);
    });

test(
     function test_bug634778_duplicateDirective_Detection() {
      var cspr;
      var SD = CSPRep.SRC_DIRECTIVES;
      var self = "http://self.com:34";
      var firstDomain = "http://first.com";
      var secondDomain = "http://second.com";
      var thirdDomain = "http://third.com";

      
      
      
      cspr = CSPRep.fromString("default-src " + self + "; default-src " +
                              firstDomain, URI(self));
      do_check_true(cspr.permits(self, SD.DEFAULT_SRC));
      do_check_false(cspr.permits(firstDomain, SD.DEFAULT_SRC));

      
      cspr = CSPRep.fromString("default-src *; report-uri " + self + "/report.py; report-uri "
                              + firstDomain + "/report.py", URI(self));
      parsedURIs = cspr.getReportURIs().split(/\s+/);
      do_check_in_array(parsedURIs, self + "/report.py");
      do_check_eq(parsedURIs.length, 1);

      
      cspr = CSPRep.fromString("img-src " + firstDomain + "; default-src " + self
                               + "; img-src " + secondDomain, URI(self));
      do_check_true(cspr.permits(firstDomain, SD.IMG_SRC));
      do_check_false(cspr.permits(secondDomain, SD.IMG_SRC));
      do_check_true(cspr.permits(self, SD.DEFAULT_SRC));

      
      cspr = CSPRep.fromString("img-src " + firstDomain + "; default-src " + self
                              + "; img-src " + secondDomain, URI(self));
      do_check_true(cspr.permits(firstDomain, SD.IMG_SRC));
      do_check_false(cspr.permits(secondDomain, SD.IMG_SRC));

      
      cspr = CSPRep.fromString("default-src " + self + "; img-src " + firstDomain
                              + "; img-src " + secondDomain, URI(self));
      do_check_true(cspr.permits(firstDomain, SD.IMG_SRC));
      do_check_false(cspr.permits(secondDomain, SD.IMG_SRC));

      
      cspr = CSPRep.fromString("default-src " + self + "; img-src " + firstDomain
                              + "; img-src " + secondDomain + "; img-src "
                              + thirdDomain, URI(self));
      do_check_true(cspr.permits(firstDomain, SD.IMG_SRC));
      do_check_false(cspr.permits(secondDomain, SD.IMG_SRC));
      do_check_false(cspr.permits(thirdDomain, SD.IMG_SRC));

      
      cspr = CSPRep.fromString("default-src " + self + "; style-src "
                               + firstDomain + "; media-src " + firstDomain
                               + "; media-src " + secondDomain + "; style-src "
                               + thirdDomain, URI(self));
      do_check_true(cspr.permits(self, SD.DEFAULT_SRC));
      do_check_true(cspr.permits(firstDomain, SD.STYLE_SRC));
      do_check_true(cspr.permits(firstDomain, SD.MEDIA_SRC));
      do_check_false(cspr.permits(secondDomain, SD.MEDIA_SRC));
      do_check_false(cspr.permits(thirdDomain, SD.STYLE_SRC));
    });

test(
    function test_bug672961_withNonstandardSelfPort() {
      














      var src;
      src = CSPSource.create("a.com", undefined, "https://foobar.com:4443");
      
      do_check_false(src.permits("http://a.com"));
      
      do_check_true(src.permits("https://a.com"));
      
      do_check_true(src.permits("https://a.com:443"));

      src = CSPSource.create("http://a.com", undefined, "https://foobar.com:4443");
      
      do_check_false(src.permits("https://a.com"));
      
      do_check_true(src.permits("http://a.com"));
      
      do_check_true(src.permits("http://a.com:80"));

      src = CSPSource.create("'self'", undefined, "https://foobar.com:4443");
      
      do_check_true(src.permits("https://foobar.com:4443"));
      do_check_false(src.permits("https://foobar.com"));
      do_check_false(src.permits("https://foobar.com:443"));

      
      do_check_false(src.permits("http://foobar.com:4443"));
      do_check_false(src.permits("http://foobar.com"));

    });

test(
    function test_bug634773_noneAndStarAreDifferent() {
      





      var p_none = CSPSourceList.fromString("'none'", undefined, "http://foo.com", false);
      var p_all = CSPSourceList.fromString("*", undefined, "http://foo.com", false);
      var p_one = CSPSourceList.fromString("bar.com", undefined, "http://foo.com", false);

      do_check_false(p_none.equals(p_all));
      do_check_false(p_none.equals(p_one));
      do_check_false(p_all.equals(p_none));
      do_check_false(p_all.equals(p_one));

      do_check_true(p_all.permits("http://bar.com"));
      do_check_true(p_one.permits("http://bar.com"));
      do_check_false(p_none.permits("http://bar.com"));
    });


test(
    function test_bug764937_defaultSrcMissing() {
      var cspObj = Cc["@mozilla.org/contentsecuritypolicy;1"]
                     .createInstance(Ci.nsIContentSecurityPolicy);
      var selfURI = URI("http://self.com/");

      function testPermits(cspObj, aUri, aContentType) {
        return cspObj.shouldLoad(aContentType, aUri, null, null, null, null)
               == Ci.nsIContentPolicy.ACCEPT;
      };

      const policy = "script-src 'self'";
      cspObj.appendPolicy(policy, selfURI, false);

      
      
      
      do_check_true(testPermits(cspObj,
                                URI("http://bar.com/foo.png"),
                                Ci.nsIContentPolicy.TYPE_IMAGE));
      do_check_true(testPermits(cspObj,
                                URI("http://self.com/foo.png"),
                                Ci.nsIContentPolicy.TYPE_IMAGE));
      do_check_true(testPermits(cspObj,
                                URI("http://self.com/foo.js"),
                                Ci.nsIContentPolicy.TYPE_SCRIPT));
      do_check_false(testPermits(cspObj,
                                 URI("http://bar.com/foo.js"),
                                 Ci.nsIContentPolicy.TYPE_SCRIPT));

    });

test(function test_equals_does_case_insensitive_comparison() {
      
      
      
      

      
      var upperCaseHost = "http://FOO.COM";
      var lowerCaseHost = "http://foo.com";
      var src1 = CSPSource.fromString(lowerCaseHost);
      var src2 = CSPSource.fromString(lowerCaseHost);
      do_check_true(src1.equals(src2))
      var src3 = CSPSource.fromString(upperCaseHost);
      do_check_true(src1.equals(src3))

      
      var upperCaseScheme = "HTTP";
      var lowerCaseScheme = "http";
      src1 = CSPHost.fromString(lowerCaseScheme);
      src2 = CSPHost.fromString(lowerCaseScheme);
      do_check_true(src1.equals(src2));
      src3 = CSPHost.fromString(upperCaseScheme);
      do_check_true(src1.equals(src3));

      
      var upperCaseKeywords = "'SELF'";
      var lowerCaseKeywords = "'self'";
      src1 = CSPSourceList.fromString(lowerCaseKeywords);
      src2 = CSPSourceList.fromString(lowerCaseKeywords);
      do_check_true(src1.equals(src2))
      src3 = CSPSourceList.fromString(upperCaseKeywords);
      do_check_true(src1.equals(src3))

  });

test(function test_csp_permits_case_insensitive() {
      var cspr;
      var SD = CSPRep.SRC_DIRECTIVES;

      
      var selfHost = "http://self.com";
      var testPolicy1 = "DEFAULT-src 'self';";
      cspr = CSPRep.fromString(testPolicy1, URI(selfHost));
      do_check_true(cspr.permits(URI("http://self.com"), SD.DEFAULT_SRC));

      
      var testPolicy2 = "default-src 'self' http://FOO.COM";
      cspr = CSPRep.fromString(testPolicy2, URI(selfHost));
      do_check_true(cspr.permits(URI("http://foo.com"), SD.DEFAULT_SRC));

      
      var testPolicy3 = "default-src 'self' HTTP://foo.com";
      cspr = CSPRep.fromString(testPolicy3, URI(selfHost));
      do_check_true(cspr.permits(URI("http://foo.com"), SD.DEFAULT_SRC));

      
      var testPolicy4 = "default-src 'NONE'";
      cspr = CSPRep.fromString(testPolicy4, URI(selfHost));
      do_check_false(cspr.permits(URI("http://foo.com"), SD.DEFAULT_SRC));
  });


























function run_test() {
  function policyresponder(request,response) {
    response.setStatusLine(request.httpVersion, 200, "OK");
    response.setHeader("Content-Type", "text/csp", false);
    response.bodyOutputStream.write(POLICY_FROM_URI, POLICY_FROM_URI.length);
  }
  
  httpServer.registerPathHandler("/policy", policyresponder);

  for(let i in tests) {
    add_task(tests[i]);
  }

  do_register_cleanup(function () {
    
    httpServer.stop(function() { });
  });

  run_next_test();
}



