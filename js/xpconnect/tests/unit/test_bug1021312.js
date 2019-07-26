



const Cu = Components.utils;
function run_test() {
  let sb = new Cu.Sandbox(this);
  var called = false;

  Cu.exportFunction(function(str) { do_check_true(str, "someString"); called = true; },
                    sb, { defineAs: "func" });
  
  Cu.evalInSandbox("var str = 'someString'; for (var i = 0; i < 10; ++i) str += i;", sb);
  Cu.evalInSandbox("func(str);", sb);
  do_check_true(called);
}
