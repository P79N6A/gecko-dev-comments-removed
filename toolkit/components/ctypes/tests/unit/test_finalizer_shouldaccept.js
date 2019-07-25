try {
  
  
  Components.utils.import("resource://gre/modules/ctypes.jsm");
} catch(e) {
}

let acquire, dispose, reset_errno, dispose_errno, acquire_ptr, dispose_ptr;

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
  reset_errno = library.declare("reset_errno",
                                ctypes.default_abi,
                                ctypes.void_t);
  dispose_errno = library.declare("test_finalizer_rel_size_t_set_errno",
                                  ctypes.default_abi,
                                  ctypes.void_t,
                                  ctypes.size_t);
  acquire_ptr = library.declare("test_finalizer_acq_int32_ptr_t",
                                ctypes.default_abi,
                                ctypes.int32_t.ptr,
                                ctypes.size_t);
  dispose_ptr = library.declare("test_finalizer_rel_int32_ptr_t",
                                ctypes.default_abi,
                                ctypes.void_t,
                                ctypes.int32_t.ptr);

  tester.launch(10, test_to_string);
  tester.launch(10, test_to_source);
  tester.launch(10, test_to_int);
  tester.launch(10, test_errno);
  tester.launch(10, test_to_pointer);
}




function test_to_string()
{
  do_print("Starting test_to_string");
  let a = ctypes.CDataFinalizer(acquire(0), dispose);
  do_check_eq(a.toString(), "0");

  a.forget();
  do_check_eq(a.toString(), "[CDataFinalizer - empty]");

  a = ctypes.CDataFinalizer(acquire(0), dispose);
  a.dispose();
  do_check_eq(a.toString(), "[CDataFinalizer - empty]");
}




function test_to_source()
{
  do_print("Starting test_to_source");
  let value = acquire(0);
  let a = ctypes.CDataFinalizer(value, dispose);
  do_check_eq(a.toSource(),
              "ctypes.CDataFinalizer("
              + ctypes.size_t(value).toSource()
              +", "
              +dispose.toSource()
              +")");
  value = null;

  a.forget();
  do_check_eq(a.toSource(), "ctypes.CDataFinalizer()");

  a = ctypes.CDataFinalizer(acquire(0), dispose);
  a.dispose();
  do_check_eq(a.toSource(), "ctypes.CDataFinalizer()");
}




function test_to_int()
{
  let value = 2;
  let wrapped, converted, finalizable;
  wrapped = ctypes.int32_t(value);
  finalizable = ctypes.CDataFinalizer(acquire(value), dispose);
  converted = ctypes.int32_t(finalizable);

  structural_check_eq(converted, wrapped);
  structural_check_eq(converted, ctypes.int32_t(finalizable.forget()));

  finalizable = ctypes.CDataFinalizer(acquire(value), dispose);
  wrapped = ctypes.int64_t(value);
  converted = ctypes.int64_t(finalizable);
  structural_check_eq(converted, wrapped);
  finalizable.dispose();
}




function test_errno(size, tc, cleanup)
{
  reset_errno();
  do_check_eq(ctypes.errno, 0);

  let finalizable = ctypes.CDataFinalizer(acquire(3), dispose_errno);
  finalizable.dispose();
  do_check_eq(ctypes.errno, 10);
  reset_errno();

  do_check_eq(ctypes.errno, 0);
  for (let i = 0; i < size; ++i) {
    finalizable = ctypes.CDataFinalizer(acquire(i), dispose_errno);
    cleanup.add(finalizable);
  }

  trigger_gc();
  do_check_eq(ctypes.errno, 0);
}




function test_to_pointer()
{
  let ptr = ctypes.int32_t(2).address();
  let finalizable = ctypes.CDataFinalizer(ptr, dispose_ptr);
  let unwrapped = ctypes.int32_t.ptr(finalizable);

  do_check_eq(""+ptr, ""+unwrapped);

  finalizable.forget(); 
}
