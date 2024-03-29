



add_autocomplete_test([
  "Searching for host match without slash should match host",
  "file",
  "file.org/",
  function* () {
    yield PlacesTestUtils.addVisits({
      uri: NetUtil.newURI("http://file.org/test/"),
      transition: TRANSITION_TYPED
    }, {
      uri: NetUtil.newURI("file:///c:/test.html"),
      transition: TRANSITION_TYPED
    });
  },
]);

add_autocomplete_test([
  "Searching match with slash at the end should do nothing",
  "file.org/",
  "file.org/",
  function* () {
    yield PlacesTestUtils.addVisits({
      uri: NetUtil.newURI("http://file.org/test/"),
      transition: TRANSITION_TYPED
    }, {
      uri: NetUtil.newURI("file:///c:/test.html"),
      transition: TRANSITION_TYPED
    });
  },
]);

add_autocomplete_test([
  "Searching match with slash in the middle should match url",
  "file.org/t",
  "file.org/test/",
  function* () {
    yield PlacesTestUtils.addVisits({
      uri: NetUtil.newURI("http://file.org/test/"),
      transition: TRANSITION_TYPED
    }, {
      uri: NetUtil.newURI("file:///c:/test.html"),
      transition: TRANSITION_TYPED
    });
  },
]);

add_autocomplete_test([
  "Searching for non-host match without slash should not match url",
  "file",
  "file",
  function* () {
    yield PlacesTestUtils.addVisits({
      uri: NetUtil.newURI("file:///c:/test.html"),
      transition: TRANSITION_TYPED
    });
  },
]);
