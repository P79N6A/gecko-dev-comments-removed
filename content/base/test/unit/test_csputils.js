



































Components.utils.import('resource://gre/modules/CSPUtils.jsm');


do_load_httpd_js();

var httpServer = new nsHttpServer();

const POLICY_FROM_URI = "allow 'self'; img-src *";
const POLICY_PORT = 9000;
const POLICY_URI = "http://localhost:" + POLICY_PORT + "/policy";


function do_check_has_key(foo, key, stack) {
  if (!stack) 
    stack = Components.stack.caller;

  var keys = [];
  for(let k in keys) { keys.push(k); }
  var text = key + " in [" + keys.join(",") + "]";

  for(var x in foo) {
    if(x == key) {
      
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
    
      
      do_check_neq(null, CSPSource.fromString("a.com"));

      
      do_check_neq(null, CSPSource.fromString("a2-c.com"));

      
      do_check_neq(null, CSPSource.fromString("*.a.com"));

      
      
      do_check_eq(null, CSPSource.fromString("x.*.a.com"));

      
      do_check_eq(null, CSPSource.fromString("a#2-c.com"));
      

      
      do_check_neq(null, CSPSource.create("a.com:23"));
      
      do_check_neq(null, CSPSource.create("https://a.com"));
      
      do_check_neq(null, CSPSource.create("https://a.com:200"));
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
      var parsed = "foo.bar:21 https://ras.bar";
      var sd = CSPSourceList.fromString(str, "http://self.com:80");
      
      do_check_neq(null,sd);
      
      do_check_eq(2, sd._sources.length);
      
      do_check_eq(parsed, sd.toString());
    });

test(
    function test_CSPSourceList_permits() {
      var nullSourceList = CSPSourceList.fromString("'none'");
      var simpleSourceList = CSPSourceList.fromString("a.com", "http://self.com");
      var doubleSourceList = CSPSourceList.fromString("https://foo.com http://bar.com:88",
                                                      "http://self.com:88");
      var allSourceList = CSPSourceList.fromString("*");

      
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

    });

test(
    function test_CSPSourceList_intersect() {
      
      
      
      var nullSourceList = CSPSourceList.fromString("'none'");
      var simpleSourceList = CSPSourceList.fromString("a.com");
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

      
      cspr = CSPRep.fromString("allow *", "http://self.com:80");
      
      do_check_has_key(cspr._directives, CSPRep.SRC_DIRECTIVES.ALLOW);

      
      
      var SD = CSPRep.SRC_DIRECTIVES;
      cspr_allowval = cspr._directives[SD.ALLOW];
      for(var d in CSPRep.SRC_DIRECTIVES) {
        
        do_check_has_key(cspr._directives, SD[d]);
        
        do_check_eq(cspr._directives[SD[d]].toString(), cspr_allowval.toString());
      }
    });


test(
    function test_CSPRep_fromString_oneDir() {

      var cspr;
      var SD = CSPRep.SRC_DIRECTIVES;
      var DEFAULTS = [SD.STYLE_SRC, SD.MEDIA_SRC, SD.IMG_SRC,
                      SD.FRAME_ANCESTORS, SD.FRAME_SRC];

      
      cspr = CSPRep.fromString("allow bar.com; script-src https://foo.com", 
                               "http://self.com");

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
      var DEFAULTS = [SD.STYLE_SRC, SD.MEDIA_SRC, SD.FRAME_ANCESTORS, SD.FRAME_SRC];

      
      var polstr = "allow allow.com; "
                  + "script-src https://foo.com; "
                  + "img-src bar.com:*";
      cspr = CSPRep.fromString(polstr, "http://self.com");

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
                              self);
      
      do_check_false(cspr.permits("https://foo.com:400", SD.IMG_SRC));
      
      CSPdebug(cspr);
      do_check_true(cspr.permits(self, SD.IMG_SRC));
      
      do_check_false(cspr.permits("http://evil.com", SD.SCRIPT_SRC));
      
      do_check_true(cspr.permits(self, SD.SCRIPT_SRC));
      
      do_check_true(cspr.permits("https://evil.com:100", SD.SCRIPT_SRC));
     });


test(function test_CSPRep_fromPolicyURI() {
        var cspr;
        var SD = CSPRep.SRC_DIRECTIVES;
        var self = "http://localhost:" + POLICY_PORT;

        cspr = CSPRep.fromString("policy-uri " + POLICY_URI, self);
        cspr_static = CSPRep.fromString(POLICY_FROM_URI, self);

        
        do_check_neq(null,cspr);

        
        for(var i in SD) {
          
          do_check_equivalent(cspr._directives[SD[i]],
                              cspr_static._directives[SD[i]]);
        }
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



