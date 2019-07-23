


























































function PROT_Reporter() {
  this.debugZone = "reporter";
  this.prefs_ = new G_Preferences();
}









PROT_Reporter.prototype.report = function(subject, data) {
  
  if (!this.prefs_.getPref(kPhishWardenRemoteLookups, false))
    return;
  
  var url = gDataProvider.getReportURL();

  
  
  if (!url)
    return;

  url += "evts=" + encodeURIComponent(subject)
         + "&evtd=" + encodeURIComponent(data);
  G_Debug(this, "Sending report: " + url);
  (new PROT_XMLFetcher(true )).get(url, null );
}
