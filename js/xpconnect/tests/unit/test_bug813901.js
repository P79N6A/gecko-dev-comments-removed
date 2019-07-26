





const Cu = Components.utils;



function checkThrows(expression, sb, regexp) {
  var result = Cu.evalInSandbox('(function() { try { ' + expression + '; return "allowed"; } catch (e) { return e.toString(); }})();', sb);
  dump('result: ' + result + '\n\n\n');
  do_check_true(!!regexp.exec(result));
}

function run_test() {

  var sb = new Cu.Sandbox('http://www.example.org');
  sb.obj = {foo: 2};
  checkThrows('obj.foo = 3;', sb, /denied/);
  Cu.evalInSandbox("var p = {__exposedProps__: {foo: 'rw'}};", sb);
  sb.obj.__proto__ = sb.p;
  checkThrows('obj.foo = 4;', sb, /__exposedProps__/);
}
