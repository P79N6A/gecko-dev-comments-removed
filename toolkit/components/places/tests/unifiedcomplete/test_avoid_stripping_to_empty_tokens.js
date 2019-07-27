



add_task(function* test_protocol_trimming() {
  for (let prot of ["http", "https", "ftp"]) {
    let visit = {
      
      uri: NetUtil.newURI(prot + "://www.mozilla.org/test/?q=" + prot + encodeURIComponent("://") + "www.foo"),
      title: "Test title",
      transition: TRANSITION_TYPED
    };
    yield PlacesTestUtils.addVisits(visit);
    let matches = [{uri: visit.uri, title: visit.title}];

    let inputs = [
      prot + "://",
      prot + ":// ",
      prot + ":// mo",
      prot + "://mo te",
      prot + "://www.",
      prot + "://www. ",
      prot + "://www. mo",
      prot + "://www.mo te",
      "www.",
      "www. ",
      "www. mo",
      "www.mo te"
    ];
    for (let input of inputs) {
      do_print("Searching for: " + input);
      yield check_autocomplete({
        search: input,
        matches: matches
      });
    }

    yield cleanup();
  }
});

