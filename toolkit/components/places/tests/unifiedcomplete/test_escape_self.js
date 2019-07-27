








add_task(function* test_escape() {
  let uri1 = NetUtil.newURI("http://unescapeduri/");
  let uri2 = NetUtil.newURI("http://escapeduri/%40/");
  yield PlacesTestUtils.addVisits([
    { uri: uri1, title: "title" },
    { uri: uri2, title: "title" }
  ]);

  do_print("Unescaped location matches itself");
  yield check_autocomplete({
    search: "http://unescapeduri/",
    matches: [ { uri: uri1, title: "title" } ]
  });

  do_print("Escaped location matches itself");
  yield check_autocomplete({
    search: "http://escapeduri/%40/",
    matches: [ { uri: uri2, title: "title" } ]
  });

  yield cleanup();
});
