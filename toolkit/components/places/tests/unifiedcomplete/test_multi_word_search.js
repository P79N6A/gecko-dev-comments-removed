













add_task(function* test_match_beginning() {
  let uri1 = NetUtil.newURI("http://a.b.c/d-e_f/h/t/p");
  let uri2 = NetUtil.newURI("http://d.e.f/g-h_i/h/t/p");
  let uri3 = NetUtil.newURI("http://g.h.i/j-k_l/h/t/p");
  let uri4 = NetUtil.newURI("http://j.k.l/m-n_o/h/t/p");
  yield PlacesTestUtils.addVisits([
    { uri: uri1, title: "f(o)o b<a>r" },
    { uri: uri2, title: "b(a)r b<a>z" },
    { uri: uri3, title: "f(o)o b<a>r" },
    { uri: uri4, title: "f(o)o b<a>r" }
  ]);
  addBookmark({ uri: uri3, title: "f(o)o b<a>r" });
  addBookmark({ uri: uri4, title: "b(a)r b<a>z" });

  do_print("Match 2 terms all in url");
  yield check_autocomplete({
    search: "c d",
    matches: [ { uri: uri1, title: "f(o)o b<a>r" } ]
  });

  do_print("Match 1 term in url and 1 term in title");
  yield check_autocomplete({
    search: "b e",
    matches: [ { uri: uri1, title: "f(o)o b<a>r" },
               { uri: uri2, title: "b(a)r b<a>z" } ]
  });

  do_print("Match 3 terms all in title; display bookmark title if matched");
  yield check_autocomplete({
    search: "b a z",
    matches: [ { uri: uri2, title: "b(a)r b<a>z" },
               { uri: uri4, title: "b(a)r b<a>z", style: [ "bookmark" ] } ]
  });

  do_print("Match 2 terms in url and 1 in title; make sure bookmark title is used for search");
  yield check_autocomplete({
    search: "k f t",
    matches: [ { uri: uri3, title: "f(o)o b<a>r", style: [ "bookmark" ] } ]
  });

  do_print("Match 3 terms in url and 1 in title");
  yield check_autocomplete({
    search: "d i g z",
    matches: [ { uri: uri2, title: "b(a)r b<a>z" } ]
  });

  do_print("Match nothing");
  yield check_autocomplete({
    search: "m o z i",
    matches: [ ]
  });

  yield cleanup();
});
