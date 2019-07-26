






add_task(function* run_next_throws() {
  let err = null;
  try {
    run_next_test();
  } catch (e) {
    err = e;
    info("run_next_test threw " + err);
  }
  ok(err, "run_next_test() should throw an error inside an add_task test");
});
