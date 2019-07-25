














































function test() {
  waitForExplicitFinish();
  openLibrary(function (win) {
    ok(true, "Library has been correctly opened");
    win.close();
    finish();
  });
}
