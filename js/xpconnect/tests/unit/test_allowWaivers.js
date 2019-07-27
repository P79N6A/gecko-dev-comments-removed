const Cu = Components.utils;
function checkWaivers(from, allowed) {
  var sb = new Cu.Sandbox('http://example.com');
  from.test = sb.eval('var o = {prop: 2, f: function() {return 42;}}; o');

  
  do_check_eq(from.eval('test.prop'), 2);
  do_check_eq(from.eval('test.f'), undefined);

  
  do_check_eq(from.eval('!!test.wrappedJSObject'), allowed);
  do_check_eq(from.eval('XPCNativeWrapper.unwrap(test) !== test'), allowed);

  
  
  var friend = new Cu.Sandbox(Cu.getObjectPrincipal(from));
  friend.test = from.test;
  friend.eval('var waived = test.wrappedJSObject;');
  do_check_true(friend.eval('waived.f()'), 42);
  friend.from = from;
  friend.eval('from.waived = waived');
  do_check_eq(from.eval('!!waived.f'), allowed);
}

function run_test() {
  checkWaivers(new Cu.Sandbox('http://example.com'), true);
  checkWaivers(new Cu.Sandbox('http://example.com', {allowWaivers: false}), false);
  checkWaivers(new Cu.Sandbox(['http://example.com']), true);
  checkWaivers(new Cu.Sandbox(['http://example.com'], {allowWaivers: false}), false);
}
