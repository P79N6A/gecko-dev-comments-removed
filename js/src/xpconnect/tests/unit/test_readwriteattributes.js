




































var gInterface = Components.interfaces["nsIXPCTestObjectReadWrite"];

function run_test() {

  
  Components.manager.autoRegister(do_get_file('../components/native/xpctest.manifest'));
  Components.manager.autoRegister(do_get_file('../components/js/xpctest.manifest'));

  
  test_component("@mozilla.org/js/xpc/test/native/ObjectReadWrite;1");
  test_component("@mozilla.org/js/xpc/test/js/ObjectReadWrite;1");

}

function test_component(contractid) {

  
  var o = Components.classes[contractid].createInstance(gInterface);

  
  do_check_eq("XPConnect Read-Writable String", o.stringProperty);
  do_check_eq(true, o.booleanProperty);
  do_check_eq(32767, o.shortProperty);
  do_check_eq(2147483647, o.longProperty);
  do_check_true(5.25 < o.floatProperty && 5.75 > o.floatProperty);
  do_check_eq("X", o.charProperty);

  
  o.stringProperty = "another string";
  o.booleanProperty = false;
  o.shortProperty = -12345;
  o.longProperty = 1234567890;
  o.floatProperty = 10.2;
  o.charProperty = "Z";

  
  do_check_eq("another string", o.stringProperty);
  do_check_eq(false, o.booleanProperty);
  do_check_eq(-12345, o.shortProperty);
  do_check_eq(1234567890, o.longProperty);
  do_check_true(10.15 < o.floatProperty && 10.25 > o.floatProperty);
  do_check_eq("Z", o.charProperty);

  

  function SetAndTestBooleanProperty(newValue, expectedValue) {
    o.booleanProperty = newValue;
    do_check_eq(expectedValue, o.booleanProperty);
  };
  SetAndTestBooleanProperty(false, false);
  SetAndTestBooleanProperty(1, true);
  SetAndTestBooleanProperty(null, false);
  SetAndTestBooleanProperty("A", true);
  SetAndTestBooleanProperty(undefined, false);
  SetAndTestBooleanProperty([], true);
  SetAndTestBooleanProperty({}, true);
}

