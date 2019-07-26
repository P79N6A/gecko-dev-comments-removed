





const Cu = Components.utils;




function run_test()
{
  var sb = Cu.Sandbox("http://www.example.com");
  sb.obj = { foo: 42, __exposedProps__: { hasOwnProperty: 'r' } };
  do_check_eq(Cu.evalInSandbox('typeof obj.foo', sb), 'undefined', "COW works as expected");
  do_check_true(Cu.evalInSandbox('obj.hasOwnProperty === Object.prototype.hasOwnProperty', sb),
                "Remapping happens even when the property is explicitly exposed");
  
  
  
  
}
