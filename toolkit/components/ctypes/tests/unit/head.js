try {
  
  
  Components.utils.import("resource://gre/modules/ctypes.jsm");
} catch(e) {
}

function open_ctypes_test_lib()
{
  return ctypes.open(do_get_file(ctypes.libraryName("jsctypes-test")).path);
}





function ResourceCleaner() {
  this._map = new WeakMap();
}
ResourceCleaner.prototype = {
  add: function ResourceCleaner_add(v) {
    this._map.set(v);
    return v;
  },
  cleanup: function ResourceCleaner_cleanup() {
    let keys = Components.utils.nondeterministicGetWeakMapKeys(this._map);
    keys.forEach((function cleaner(k) {
      try {
        k.dispose();
      } catch (x) {
        
        
      }
      this._map.delete(k);
    }).bind(this));
  }
};




function ResourceTester(start, stop) {
  this._start = start;
  this._stop  = stop;
}
ResourceTester.prototype = {
  launch: function(size, test, args) {
    trigger_gc();
    let cleaner = new ResourceCleaner();
    this._start(size);
    try {
      test(size, args, cleaner);
    } catch (x) {
      cleaner.cleanup();
      this._stop();
      throw x;
    }
    trigger_gc();
    cleaner.cleanup();
    this._stop();
  }
};

function structural_check_eq(a, b) {
  

  let result;
  let finished = false;
  let asource, bsource;
  try {
    asource = a.toSource();
    bsource = b.toSource();
    finished = true;
  } catch (x) {
  }
  if (finished) {
    return do_check_eq(asource, bsource);
  }

  

  try {
    structural_check_eq_aux(a, b);
    result = true;
  } catch (x) {
    dump(x);
    result = false;
  }
  do_check_true(result);
}
function structural_check_eq_aux(a, b) {
  let ak;
  try {
    ak = Object.keys(a);
  } catch (x) {
    if (a != b) {
      throw new Error("Distinct values "+a, b);
    }
    return;
  }
  ak.forEach(
    function(k) {
      let av = a[k];
      let bv = b[k];
      structural_check_eq_aux(av, bv);
    }
  );
}

function trigger_gc() {
  dump("Triggering garbage-collection");
  Components.utils.forceGC();
}

function must_throw(f) {
  let has_thrown = false;
  try {
    f();
  } catch (x) {
    has_thrown = true;
  }
  do_check_true(has_thrown);
}
