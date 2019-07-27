
















function assert_equals(a, b, msg) {
  let text = msg + ": " + _wrap_with_quotes_if_necessary(a) +
                 " == " + _wrap_with_quotes_if_necessary(b);
  do_report_result(a == b, text, Components.stack.caller, false);
}

function assert_not_equals(a, b, msg) {
  let text = msg + ": " + _wrap_with_quotes_if_necessary(a) +
                 " != " + _wrap_with_quotes_if_necessary(b);
  do_report_result(a != b, text, Components.stack.caller, false);
}

function assert_array_equals(a, b, msg) {
  do_report_result(a.length == b.length,
                   msg + ": (length) " + a.length + " == " + b.length,
                   Components.stack.caller, false);
  for (let i = 0; i < a.length; ++i) {
    if (a[i] !== b[i]) {
        do_report_result(false,
                         msg + ": [" + i + "] " +
                               _wrap_with_quotes_if_necessary(a[i]) +
                               " === " +
                               _wrap_with_quotes_if_necessary(b[i]),
                         Components.stack.caller, false);
    }
  }
  
  do_report_result(true, msg + ": all array elements equal",
                   Components.stack.caller, false);
}

function assert_true(cond, msg) {
  do_report_result(!!cond, msg + ": " + uneval(cond),
                   Components.stack.caller, false);
}

function assert_throws(ex, func) {
  if (!('name' in ex))
    do_throw("first argument to assert_throws must be of the form " +
             "{'name': something}");

  let msg = "expected to catch an exception named " + ex.name;

  try {
    func();
  } catch (e) {
    if ('name' in e)
      do_report_result(e.name == ex.name,
                       msg + ", got " + e.name,
                       Components.stack.caller, false);
    else
      do_report_result(false,
                       msg + ", got " + legible_exception(ex),
                       Components.stack.caller, false);

    return;
  }

  
  
  do_report_result(false, msg + ", but returned normally",
                   Components.stack.caller, false);
}

let tests = [];

function test(func, msg) {
  tests.push({msg: msg, func: func,
              filename: Components.stack.caller.filename });
}

function run_test() {
  tests.forEach(function(t) {
    do_print("test group: " + t.msg,
             {source_file: t.filename});
    t.func();
  });
};
