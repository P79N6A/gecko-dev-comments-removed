





function test() {
  waitForExplicitFinish();

  
  forgetClosedWindows();

  
  
  whenNewWindowLoaded({private: true}, function (win) {
    info("The private window got loaded");
    win.addEventListener("SSWindowClosing", function onclosing() {
      win.removeEventListener("SSWindowClosing", onclosing, false);
      executeSoon(function () {
        is (ss.getClosedWindowCount(), 0,
            "The private window should not have been stored");
        finish();
      });
    }, false);
    win.close();
  });
}
