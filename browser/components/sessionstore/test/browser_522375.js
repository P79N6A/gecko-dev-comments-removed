function test() {
  var startup_info = Components.classes["@mozilla.org/toolkit/app-startup;1"].getService(Components.interfaces.nsIAppStartup).getStartupInfo();
  

  
  
  var telemetry = Cc["@mozilla.org/base/telemetry;1"].getService(Ci.nsITelemetry);
  var snapshot = telemetry.getHistogramById("STARTUP_MEASUREMENT_ERRORS")
                          .snapshot();

  if (snapshot.counts[0] == 0)
    ok(startup_info.process <= startup_info.main, "process created before main is run " + uneval(startup_info));
  else
    todo(false, "An error occurred while recording the process creation timestamp, skipping this test");

  
  if (startup_info.firstPaint)
    ok(startup_info.main <= startup_info.firstPaint, "main ran before first paint " + uneval(startup_info));

  ok(startup_info.main < startup_info.sessionRestored, "Session restored after main " + uneval(startup_info));
}
