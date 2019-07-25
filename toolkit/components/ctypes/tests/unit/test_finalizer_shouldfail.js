try {
  
  
  Components.utils.import("resource://gre/modules/ctypes.jsm");
} catch(e) {
}

let acquire, dispose, null_dispose, compare, dispose_64;

function run_test()
{
  let library = open_ctypes_test_lib();

  let start = library.declare("test_finalizer_start", ctypes.default_abi,
                          ctypes.void_t,
                          ctypes.size_t);
  let stop = library.declare("test_finalizer_stop", ctypes.default_abi,
                             ctypes.void_t);
  let tester = new ResourceTester(start, stop);
  acquire = library.declare("test_finalizer_acq_size_t",
                            ctypes.default_abi,
                            ctypes.size_t,
                            ctypes.size_t);
  dispose = library.declare("test_finalizer_rel_size_t",
                            ctypes.default_abi,
                            ctypes.void_t,
                            ctypes.size_t);
  compare = library.declare("test_finalizer_cmp_size_t",
                            ctypes.default_abi,
                            ctypes.bool,
                            ctypes.size_t,
                            ctypes.size_t);

  dispose_64 = library.declare("test_finalizer_rel_int64_t",
                               ctypes.default_abi,
                               ctypes.void_t,
                               ctypes.int64_t);

  let type_afun = ctypes.FunctionType(ctypes.default_abi,
                                      ctypes.void_t,
                                      [ctypes.size_t]).ptr;

  let null_dispose_maker =
    library.declare("test_finalizer_rel_null_function",
                    ctypes.default_abi,
                    type_afun
                   );
  null_dispose = null_dispose_maker();

  tester.launch(10, test_double_dispose);
  tester.launch(10, test_finalize_bad_construction);
  tester.launch(10, test_null_dispose);
  tester.launch(10, test_pass_disposed);
  tester.launch(10, test_wrong_type);
}





function test_finalize_bad_construction() {
  
  must_throw(function() { ctypes.CDataFinalizer({}, dispose); });
  must_throw(function() { ctypes.CDataFinalizer(dispose, dispose); });

  
  must_throw(function() { ctypes.CDataFinalizer(init(0)); });

  
  must_throw(function() { ctypes.CDataFinalizer(init(0), dispose, dispose); });

  
  must_throw(function() { ctypes.CDataFinalizer(init(0), null); });

  
  must_throw(function() {
    let a;
    ctypes.CDataFinalizer(init(0), a);
  });

}




function test_double_dispose() {
  function test_one_combination(i, a, b) {
    let v = ctypes.CDataFinalizer(acquire(i), dispose);
    a(v);
    must_throw(function() { b(v); } );
  }

  let call_dispose = function(v) {
    v.dispose();
  };
  let call_forget = function(v) {
    v.forget();
  };

  test_one_combination(0, call_dispose, call_dispose);
  test_one_combination(1, call_dispose, call_forget);
  test_one_combination(2, call_forget, call_dispose);
  test_one_combination(3, call_forget, call_forget);
}





function test_null_dispose()
{
  let exception;

  exception = false;
  try {
    let v = ctypes.CDataFinalizer(acquire(0), null_dispose);
  } catch (x) {
    exception = true;
  }
  do_check_true(exception);
}





function test_pass_disposed()
{
  let exception, v;

  exception = false;
  v = ctypes.CDataFinalizer(acquire(0), dispose);
  do_check_true(compare(v, 0));
  v.forget();

  try {
    compare(v, 0);
  } catch (x) {
    exception = true;
  }
  do_check_true(exception);

  exception = false;
  v = ctypes.CDataFinalizer(acquire(0), dispose);
  do_check_true(compare(v, 0));
  v.dispose();

  try {
    compare(v, 0);
  } catch (x) {
    exception = true;
  }
  do_check_true(exception);

  exception = false;
  try {
    ctypes.int32_t(ctypes.CDataFinalizer(v, dispose));
  } catch (x) {
    exception = true;
  }
  do_check_true(exception);
}

function test_wrong_type()
{
  let int32_v = ctypes.int32_t(99);
  let exception;
  try {
    ctypes.CDataFinalizer(int32_v, dispose_64);
  } catch (x) {
    exception = x;
  }

  do_check_true(!!exception);
  do_check_eq(exception.constructor.name, "TypeError");
}
