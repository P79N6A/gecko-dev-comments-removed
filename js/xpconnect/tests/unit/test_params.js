



































const Cc = Components.classes;
const Ci = Components.interfaces;

function run_test() {

  
  Components.manager.autoRegister(do_get_file('../components/native/xpctest.manifest'));
  Components.manager.autoRegister(do_get_file('../components/js/xpctest.manifest'));

  
  test_component("@mozilla.org/js/xpc/test/native/Params;1");
  test_component("@mozilla.org/js/xpc/test/js/Params;1");
}

function test_component(contractid) {

  
  var o = Cc[contractid].createInstance(Ci["nsIXPCTestParams"]);

  
  var standardComparator = function(a,b) {return a == b;};
  var fuzzComparator = function(a,b) {return Math.abs(a - b) < 0.1;};
  var arrayComparator = function(a,b) {
    if (a.length != b.length)
      return false;
    for (var i = 0; i < a.length; ++i)
      if (a[i] != b[i])
        return false;
    return true;
  };

  
  
  
  
  
  function doTest(name, val1, val2, comparator) {
    if (!comparator)
      comparator = standardComparator;
    var a = val1;
    var b = {value: val2};
    var rv = o[name].call(o, a, b);
    do_check_true(comparator(rv, val2));
    do_check_true(comparator(val1, b.value));
  };

  function doIsTest(name, val1, val1Is, val2, val2Is, comparator) {
    var a = val1;
    var aIs = val1Is;
    var b = {value: val2};
    var bIs = {value: val2Is};
    var rvIs = {};
    var rv = o[name].call(o, aIs, a, bIs, b, rvIs);
    do_check_true(comparator(rv, val2));
    do_check_eq(rvIs.value, val2Is);
    do_check_true(comparator(val1, b.value));
    do_check_eq(val1Is, bIs.value);
  }

  
  
  function doTestWorkaround(name, val1) {
    var a = val1;
    var b = {value: ""};
    o[name].call(o, a, b);
    do_check_eq(val1, b.value);
  }

  
  doTest("testBoolean", true, false);
  doTest("testOctet", 4, 156);
  doTest("testShort", -456, 1299);
  doTest("testLong", 50060, -12121212);
  doTest("testLongLong", 12345, -10000000000);
  doTest("testUnsignedShort", 1532, 65000);
  doTest("testUnsignedLong", 0, 4000000000);
  doTest("testUnsignedLongLong", 215435, 3453492580348535809);
  doTest("testFloat", 4.9, -11.2, fuzzComparator);
  doTest("testDouble", -80.5, 15000.2, fuzzComparator);
  doTest("testChar", "a", "2");
  doTest("testString", "someString", "another string");
  
  doTest("testWchar", "z", "q");
  
  doTestWorkaround("testDOMString", "Beware: ☠ s");
  doTestWorkaround("testAString", "Frosty the ☃ ;-)");
  doTestWorkaround("testAUTF8String", "We deliver 〠!");
  doTestWorkaround("testACString", "Just a regular C string.");
  doTest("testJsval", {aprop: 12, bprop: "str"}, 4.22);

  
  function makeA() { return Cc["@mozilla.org/js/xpc/test/js/InterfaceA;1"].createInstance(Ci['nsIXPCTestInterfaceA']); };
  function makeB() { return Cc["@mozilla.org/js/xpc/test/js/InterfaceB;1"].createInstance(Ci['nsIXPCTestInterfaceB']); };

  
  doIsTest("testShortArray", [2, 4, 6], 3, [1, 3, 5, 7], 4, arrayComparator);
  doIsTest("testLongLongArray", [-10000000000], 1, [1, 3, 1234511234551], 3, arrayComparator);
  doIsTest("testStringArray", ["mary", "hat", "hey", "lid", "tell", "lam"], 6,
                              ["ids", "fleas", "woes", "wide", "has", "know", "!"], 7, arrayComparator);
  doIsTest("testWstringArray", ["沒有語言", "的偉大嗎?]"], 2,
                               ["we", "are", "being", "sooo", "international", "right", "now"], 7, arrayComparator);
  doIsTest("testInterfaceArray", [makeA(), makeA()], 2,
                                 [makeA(), makeA(), makeA(), makeA(), makeA(), makeA()], 6, arrayComparator);

  
  var ssTests = ["Tis not possible, I muttered", "give me back my free hardcore!", "quoth the server:", "4〠4"];
  doIsTest("testSizedString", ssTests[0], ssTests[0].length, ssTests[1], ssTests[1].length, standardComparator);
  doIsTest("testSizedWstring", ssTests[2], ssTests[2].length, ssTests[3], ssTests[3].length, standardComparator);
}
