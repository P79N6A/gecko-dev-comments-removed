





add_autocomplete_test([
  "Searching for zero frecency domain should not autoFill it",
  "moz",
  "moz",
  function () {
    Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", false);
    PlacesTestUtils.addVisits({
      uri: NetUtil.newURI("http://mozilla.org/framed_link/"),
      transition: TRANSITION_FRAMED_LINK
    });
  }
]);

add_autocomplete_test([
  "Searching for zero frecency url should not autoFill it",
  "mozilla.org/f",
  "mozilla.org/f",
  function () {
    Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", false);
    PlacesTestUtils.addVisits({
      uri: NetUtil.newURI("http://mozilla.org/framed_link/"),
      transition: TRANSITION_FRAMED_LINK
    });
  }
]);
