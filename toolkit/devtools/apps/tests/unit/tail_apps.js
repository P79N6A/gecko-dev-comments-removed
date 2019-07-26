if (gClient) {
  
  gClient.close(function () {
    run_next_test();
  });
}

Services.prefs.setBoolPref("devtools.debugger.enable-content-actors", originalPrefValue);
