






add_autocomplete_test([
  "Do not autofill whitespaced entry 1",
  "mozilla.org ",
  "mozilla.org ",
  function* () {
    yield PlacesTestUtils.addVisits({
      uri: NetUtil.newURI("http://mozilla.org/link/"),
      transition: TRANSITION_TYPED
    });
  }
]);

add_autocomplete_test([
  "Do not autofill whitespaced entry 2",
  "mozilla.org/ ",
  "mozilla.org/ ",
  function* () {
    yield PlacesTestUtils.addVisits({
      uri: NetUtil.newURI("http://mozilla.org/link/"),
      transition: TRANSITION_TYPED
    });
  }
]);

add_autocomplete_test([
  "Do not autofill whitespaced entry 3",
  "mozilla.org/link ",
  "mozilla.org/link ",
  function* () {
    yield PlacesTestUtils.addVisits({
      uri: NetUtil.newURI("http://mozilla.org/link/"),
      transition: TRANSITION_TYPED
    });
  }
]);

add_autocomplete_test([
  "Do not autofill whitespaced entry 4",
  "mozilla.org/link/ ",
  "mozilla.org/link/ ",
  function* () {
    yield PlacesTestUtils.addVisits({
      uri: NetUtil.newURI("http://mozilla.org/link/"),
      transition: TRANSITION_TYPED
    });
  }
]);


add_autocomplete_test([
  "Do not autofill whitespaced entry 5",
  "moz illa ",
  "moz illa ",
  function* () {
    yield PlacesTestUtils.addVisits({
      uri: NetUtil.newURI("http://mozilla.org/link/"),
      transition: TRANSITION_TYPED
    });
  }
]);

add_autocomplete_test([
  "Do not autofill whitespaced entry 6",
  " mozilla",
  " mozilla",
  function* () {
    yield PlacesTestUtils.addVisits({
      uri: NetUtil.newURI("http://mozilla.org/link/"),
      transition: TRANSITION_TYPED
    });
  }
]);
