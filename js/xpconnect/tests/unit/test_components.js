const Cu = Components.utils;

function run_test() {
  var sb1 = Cu.Sandbox("http://www.blah.com");
  var sb2 = Cu.Sandbox("http://www.blah.com");
  var sb3 = Cu.Sandbox(this);
  var sb4 = Cu.Sandbox("http://www.other.com");
  var rv;

  
  
  
  [sb1, sb2, sb4].forEach(function(x) { x.Components = Cu.getComponentsForScope(x); });

  
  sb1.C = Components;
  checkThrows("C.utils", sb1);
  checkThrows("C.classes", sb1);

  
  do_check_eq(Cu.evalInSandbox("typeof Components.interfaces", sb1), 'object');
  do_check_eq(Cu.evalInSandbox("typeof Components.utils", sb1), 'undefined');
  do_check_eq(Cu.evalInSandbox("typeof Components.classes", sb1), 'undefined');

  
  var C2 = Cu.evalInSandbox("Components", sb2);
  var whitelist = ['interfaces', 'interfacesByID', 'results', 'isSuccessCode', 'QueryInterface'];
  for (var prop in Components) {
    do_print("Checking " + prop);
    do_check_eq((prop in C2), whitelist.indexOf(prop) != -1);
  }

  
  sb1.C2 = C2;
  do_check_eq(Cu.evalInSandbox("typeof C2.interfaces", sb1), 'object');
  do_check_eq(Cu.evalInSandbox("typeof C2.utils", sb1), 'undefined');
  do_check_eq(Cu.evalInSandbox("typeof C2.classes", sb1), 'undefined');

  
  sb3.C = Components;
  rv = Cu.evalInSandbox("C.utils", sb3);
  do_check_eq(rv, Cu);

  
  sb4.C2 = C2;
  checkThrows("C2.interfaces", sb4);
  checkThrows("C2.utils", sb4);
  checkThrows("C2.classes", sb4);
}

function checkThrows(expression, sb) {
  var result = Cu.evalInSandbox('(function() { try { ' + expression + '; return "allowed"; } catch (e) { return e.toString(); }})();', sb);
  do_check_true(!!/denied/.exec(result));
}
