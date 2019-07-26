



































const Cc = Components.classes;
const Ci = Components.interfaces;

function run_test() {

  
  Components.manager.autoRegister(do_get_file('../components/js/xpctest.manifest'));

  
  var ifs = {
    a: Ci['nsIXPCTestInterfaceA'],
    b: Ci['nsIXPCTestInterfaceB'],
    c: Ci['nsIXPCTestInterfaceC']
  };

  
  var cls = Cc["@mozilla.org/js/xpc/test/js/TestInterfaceAll;1"];

  
  for (i = 0; i < 2; ++i)
    play_with_tearoffs(ifs, cls);
}

function play_with_tearoffs(ifs, cls) {

  
  var instances = [];
  for (var i = 0; i < 300; ++i)
    instances.push(cls.createInstance(ifs.b));

  
  gc();

  
  instances.forEach(function(v, i, a) { v.QueryInterface(ifs.a); });

  
  instances.forEach(function(v, i, a) { v.QueryInterface(ifs.c); });

  
  do_check_true('name' in instances[10], 'Have the prop from A/B');
  do_check_true('someInteger' in instances[10], 'Have the prop from C');

  
  var aTearOffs = instances.map(function(v, i, a) { return v.nsIXPCTestInterfaceA; } );
  var bTearOffs = instances.map(function(v, i, a) { return v.nsIXPCTestInterfaceB; } );

  
  do_check_true('name' in aTearOffs[1], 'Have the prop from A');
  do_check_true(!('someInteger' in aTearOffs[1]), 'Dont have the prop from C');

  
  gc();

  
  for (var i = 0; i < instances.length; ++i)
    if (i % 2 == 0)
        instances[i] = null;

  
  gc();

  
  for (var i = 0; i < aTearOffs.length; ++i)
    if (i % 3 == 0)
        aTearOffs[i] = null;

  
  gc();

  
  for (var i = 0; i < bTearOffs.length; ++i)
    if (i % 5 == 0)
        bTearOffs[i] = null;

  
  gc();

  
  bTearOffs = 0;

  
  gc();

  
  var cTearOffs = instances.map(function(v, i, a) { return v ? v.nsIXPCTestInterfaceC : null; } );

  
  do_check_true(!('name' in cTearOffs[1]), 'Dont have the prop from A');
  do_check_true('someInteger' in cTearOffs[1], 'have the prop from C');

  
  aTearOffs = null;

  
  gc();

  
  instances = null;
  gc();

  
  do_check_true(true, "Got all the way through without crashing!");
}
