





var i = 0;

gczeal(2);
function test() {
  if (i++ > 10000)
    return "function";
  var res = typeof (new test("1")) != 'function';
  return res ? "function" : "string";
}

test();
