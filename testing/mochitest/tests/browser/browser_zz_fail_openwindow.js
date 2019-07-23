function test() {
  waitForExplicitFinish();
  function done() {
    ok(true, "timeout ran");
    finish();
  }

  ok(OpenBrowserWindow(), "opened browser window");
  

  setTimeout(done, 10000);
}
