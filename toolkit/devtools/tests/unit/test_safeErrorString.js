





function run_test() {
  test_with_error();
  test_with_tricky_error();
  test_with_string();
  test_with_thrower();
  test_with_psychotic();
}

function test_with_error() {
  let s = DevToolsUtils.safeErrorString(new Error("foo bar"));
  
  do_check_true(s.includes("foo bar"));
  
  do_check_true(s.includes("test_with_error"))
  do_check_true(s.includes("test_safeErrorString.js"));
  
  do_check_true(s.includes("Line"));
  do_check_true(s.includes("column"));
}

function test_with_tricky_error() {
  let e = new Error("batman");
  e.stack = { toString: Object.create(null) };
  let s = DevToolsUtils.safeErrorString(e);
  
  do_check_true(s.includes("batman"));
}

function test_with_string() {
  let s = DevToolsUtils.safeErrorString("not really an error");
  
  do_check_true(s.includes("not really an error"));
}

function test_with_thrower() {
  let s = DevToolsUtils.safeErrorString({
    toString: () => {
      throw new Error("Muahahaha");
    }
  });
  
  do_check_eq(typeof s, "string");
}

function test_with_psychotic() {
  let s = DevToolsUtils.safeErrorString({
    toString: () => Object.create(null)
  });
  
  do_check_eq(typeof s, "string");
}
