





































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

  
  obj = null;
  Components.utils.forceGC();

  
  
  do_check_true(weak.get() === null);
}
