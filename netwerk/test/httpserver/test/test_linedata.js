







function run_test()
{
  var data = new LineData();
  data.appendBytes(["a".charCodeAt(0), CR]);

  var out = { value: "" };
  do_check_false(data.readLine(out));

  data.appendBytes([LF]);
  do_check_true(data.readLine(out));
  do_check_eq(out.value, "a");
}
