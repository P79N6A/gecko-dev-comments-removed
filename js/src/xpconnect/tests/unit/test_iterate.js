




































function keys(o)
{
  for (var p in o);
}

function run_test()
{
  var o1 = { 1: 1, 2: 2, 3: 3, 4: 4 };
  var o2 = new XPCSafeJSObjectWrapper({ a: 'a', b: 'b', c: 'c' });
  var o3 = { 5: 5, 6: 6, 7: 7, 8: 8 };

  keys(o1);
  keys(o2);
  keys(o3);

  do_check_true(true);
}
