






add_autocomplete_test([
  "Searching for domain should autoFill it",
  "moz",
  "mozilla.org/",
  function () {
    Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", false);
    addVisits([ { url: NetUtil.newURI("http://mozilla.org/link/")
                , transition: TRANSITION_LINK }
              ]);
  }
]);

add_autocomplete_test([
  "Searching for url should autoFill it",
  "mozilla.org/li",
  "mozilla.org/link/",
  function () {
    Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", false);
    addVisits([ { url: NetUtil.newURI("http://mozilla.org/link/")
                , transition: TRANSITION_LINK }
              ]);
  }
]);



add_autocomplete_test([
  "Searching for non-typed domain should not autoFill it",
  "moz",
  "moz",
  function () {
    Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", true);
    addVisits([ { url: NetUtil.newURI("http://mozilla.org/link/")
                , transition: TRANSITION_LINK }
              ]);
  }
]);

add_autocomplete_test([
  "Searching for typed domain should autoFill it",
  "moz",
  "mozilla.org/",
  function () {
    Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", true);
    addVisits([ { url: NetUtil.newURI("http://mozilla.org/typed/")
                , transition: TRANSITION_TYPED }
              ]);
  }
]);

add_autocomplete_test([
  "Searching for non-typed url should not autoFill it",
  "mozilla.org/li",
  "mozilla.org/li",
  function () {
    Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", true);
    addVisits([ { url: NetUtil.newURI("http://mozilla.org/link/")
                , transition: TRANSITION_LINK }
              ]);
  }
]);

add_autocomplete_test([
  "Searching for typed url should autoFill it",
  "mozilla.org/li",
  "mozilla.org/link/",
  function () {
    Services.prefs.setBoolPref("browser.urlbar.autoFill.typed", true);
    addVisits([ { url: NetUtil.newURI("http://mozilla.org/link/")
                , transition: TRANSITION_TYPED }
              ]);
  }
]);
