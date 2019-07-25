function test() {
  waitForExplicitFinish();
  var startup_info = Components.classes["@mozilla.org/toolkit/app-startup;1"].getService(Components.interfaces.nsIAppStartup_MOZILLA_2_0).getStartupInfo();
  
  is(startup_info.process <= startup_info.main, true, "process created before main is run " + uneval(startup_info));

  
  if (startup_info.firstPaint)
    is(startup_info.main <= startup_info.firstPaint, true, "main ran before first paint " + uneval(startup_info));

  is(startup_info.main < startup_info.sessionRestored, true, "Session restored after main " + uneval(startup_info));
  finish();
}
