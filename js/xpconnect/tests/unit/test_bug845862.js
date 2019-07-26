const Cu = Components.utils;

function run_test() {
  
  
  
  var sb = new Cu.Sandbox('http://www.example.com', {wantXrays: false});
  Cu.evalInSandbox("this.foo = {}; Object.defineProperty(foo, 'bar', {get: function() {return {};}});", sb);
  do_check_true(sb.foo != XPCNativeWrapper(sb.foo), "sb.foo is waived");
  var desc = Object.getOwnPropertyDescriptor(sb.foo, 'bar');
  var b = desc.get();
  do_check_true(b != XPCNativeWrapper(b), "results from accessor descriptors are waived");
}
