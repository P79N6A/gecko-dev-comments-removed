





const Cu = Components.utils;




function run_test()
{
  var sb = Cu.Sandbox("http://www.example.com");
  sb.obj = { foo: 42, __exposedProps__: { hasOwnProperty: 'r' } };
  do_check_eq(Cu.evalInSandbox('typeof obj.foo', sb), 'undefined', "COW works as expected");
  try {
    Cu.evalInSandbox('obj.hasOwnProperty', sb);
    do_check_true(false);
  } catch (e) {
    do_check_true(/privileged or cross-origin callable/i.test(e));
  }
}
