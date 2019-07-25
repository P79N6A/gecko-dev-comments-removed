





































function run_test()
{
  var obj = { num: 5, str: 'foo' };
  var weak = Components.utils.getWeakReference(obj);

  do_check_true(weak.get() === obj);
  do_check_true(weak.get().num == 5);
  do_check_true(weak.get().str == 'foo');

  
  Components.utils.forceGC();

  
  do_check_true(weak.get() === obj);
  do_check_true(weak.get().num == 5);
  do_check_true(weak.get().str == 'foo');

  
  
  
  
  obj = { num: 6, str: 'foo2' };
  var weak2 = Components.utils.getWeakReference(obj);
  do_check_true(weak2.get() === obj);

  Components.utils.forceGC();

  
  
  do_check_true(weak.get() === null);
}
