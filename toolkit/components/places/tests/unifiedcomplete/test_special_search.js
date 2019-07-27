











function setSuggestPrefsToFalse() {
  Services.prefs.setBoolPref("browser.urlbar.suggest.history", false);
  Services.prefs.setBoolPref("browser.urlbar.suggest.history.onlyTyped", false);
  Services.prefs.setBoolPref("browser.urlbar.suggest.bookmark", false);
}

add_task(function* test_special_searches() {
  let uri1 = NetUtil.newURI("http://url/");
  let uri2 = NetUtil.newURI("http://url/2");
  let uri3 = NetUtil.newURI("http://foo.bar/");
  let uri4 = NetUtil.newURI("http://foo.bar/2");
  let uri5 = NetUtil.newURI("http://url/star");
  let uri6 = NetUtil.newURI("http://url/star/2");
  let uri7 = NetUtil.newURI("http://foo.bar/star");
  let uri8 = NetUtil.newURI("http://foo.bar/star/2");
  let uri9 = NetUtil.newURI("http://url/tag");
  let uri10 = NetUtil.newURI("http://url/tag/2");
  let uri11 = NetUtil.newURI("http://foo.bar/tag");
  let uri12 = NetUtil.newURI("http://foo.bar/tag/2");
  yield PlacesTestUtils.addVisits([
    { uri: uri1, title: "title", transition: TRANSITION_TYPED },
    { uri: uri2, title: "foo.bar" },
    { uri: uri3, title: "title" },
    { uri: uri4, title: "foo.bar", transition: TRANSITION_TYPED },
    { uri: uri6, title: "foo.bar" },
    { uri: uri11, title: "title", transition: TRANSITION_TYPED }
  ]);
  addBookmark( { uri: uri5, title: "title" } );
  addBookmark( { uri: uri6, title: "foo.bar" } );
  addBookmark( { uri: uri7, title: "title" } );
  addBookmark( { uri: uri8, title: "foo.bar" } );
  addBookmark( { uri: uri9, title: "title", tags: [ "foo.bar" ] } );
  addBookmark( { uri: uri10, title: "foo.bar", tags: [ "foo.bar" ] } );
  addBookmark( { uri: uri11, title: "title", tags: [ "foo.bar" ] } );
  addBookmark( { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ] } );

  
  do_print("History restrict");
  yield check_autocomplete({
    search: "^",
    matches: [ { uri: uri1, title: "title" },
               { uri: uri2, title: "foo.bar" },
               { uri: uri3, title: "title" },
               { uri: uri4, title: "foo.bar" },
               { uri: uri6, title: "foo.bar" },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("Star restrict");
  yield check_autocomplete({
    search: "*",
    matches: [ { uri: uri5, title: "title", style: [ "bookmark" ] },
               { uri: uri6, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri7, title: "title", style: [ "bookmark" ] },
               { uri: uri8, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri9, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri10, title: "foo.bar", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar"], style: [ "bookmark-tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] } ]
  });

  do_print("Tag restrict");
  yield check_autocomplete({
    search: "+",
    matches: [ { uri: uri9, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri10, title: "foo.bar", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  
  do_print("Special as first word");
  yield check_autocomplete({
    search: "^ foo bar",
    matches: [ { uri: uri2, title: "foo.bar" },
               { uri: uri3, title: "title" },
               { uri: uri4, title: "foo.bar" },
               { uri: uri6, title: "foo.bar" },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("Special as middle word");
  yield check_autocomplete({
    search: "foo ^ bar",
    matches: [ { uri: uri2, title: "foo.bar" },
               { uri: uri3, title: "title" },
               { uri: uri4, title: "foo.bar" },
               { uri: uri6, title: "foo.bar" },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("Special as last word");
  yield check_autocomplete({
    search: "foo bar ^",
    matches: [ { uri: uri2, title: "foo.bar" },
               { uri: uri3, title: "title" },
               { uri: uri4, title: "foo.bar" },
               { uri: uri6, title: "foo.bar" },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  
  do_print("foo ^ -> history");
  yield check_autocomplete({
    search: "foo ^",
    matches: [ { uri: uri2, title: "foo.bar" },
               { uri: uri3, title: "title" },
               { uri: uri4, title: "foo.bar" },
               { uri: uri6, title: "foo.bar" },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo | -> history (change pref)");
  changeRestrict("history", "|");
  yield check_autocomplete({
    search: "foo |",
    matches: [ { uri: uri2, title: "foo.bar" },
               { uri: uri3, title: "title" },
               { uri: uri4, title: "foo.bar" },
               { uri: uri6, title: "foo.bar" },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo * -> is star");
  resetRestrict("history");
  yield check_autocomplete({
    search: "foo *",
    matches: [ { uri: uri6, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri7, title: "title", style: [ "bookmark" ] },
               { uri: uri8, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri9, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri10, title: "foo.bar", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] } ]
  });

  do_print("foo | -> is star (change pref)");
  changeRestrict("bookmark", "|");
  yield check_autocomplete({
    search: "foo |",
    matches: [ { uri: uri6, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri7, title: "title", style: [ "bookmark" ] },
               { uri: uri8, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri9, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri10, title: "foo.bar", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] } ]
  });

  do_print("foo # -> in title");
  resetRestrict("bookmark");
  yield check_autocomplete({
    search: "foo #",
    matches: [ { uri: uri2, title: "foo.bar" },
               { uri: uri4, title: "foo.bar" },
               { uri: uri6, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri8, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri9, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri10, title: "foo.bar", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo | -> in title (change pref)");
  changeRestrict("title", "|");
  yield check_autocomplete({
    search: "foo |",
    matches: [ { uri: uri2, title: "foo.bar" },
               { uri: uri4, title: "foo.bar" },
               { uri: uri6, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri8, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri9, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri10, title: "foo.bar", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo @ -> in url");
  resetRestrict("title");
  yield check_autocomplete({
    search: "foo @",
    matches: [ { uri: uri3, title: "title" },
               { uri: uri4, title: "foo.bar" },
               { uri: uri7, title: "title", style: [ "bookmark" ] },
               { uri: uri8, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo | -> in url (change pref)");
  changeRestrict("url", "|");
  yield check_autocomplete({
    search: "foo |",
    matches: [ { uri: uri3, title: "title" },
               { uri: uri4, title: "foo.bar" },
               { uri: uri7, title: "title", style: [ "bookmark" ] },
               { uri: uri8, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo + -> is tag");
  resetRestrict("url");
  yield check_autocomplete({
    search: "foo +",
    matches: [ { uri: uri9, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri10, title: "foo.bar", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo | -> is tag (change pref)");
  changeRestrict("tag", "|");
  yield check_autocomplete({
    search: "foo |",
    matches: [ { uri: uri9, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri10, title: "foo.bar", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo ~ -> is typed");
  resetRestrict("tag");
  yield check_autocomplete({
    search: "foo ~",
    matches: [ { uri: uri4, title: "foo.bar" },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo | -> is typed (change pref)");
  changeRestrict("typed", "|");
  yield check_autocomplete({
    search: "foo |",
    matches: [ { uri: uri4, title: "foo.bar" },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  
  do_print("foo ^ * -> history, is star");
  resetRestrict("typed");
  yield check_autocomplete({
    search: "foo ^ *",
    matches: [ { uri: uri6, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] } ]
  });

  do_print("foo ^ # -> history, in title");
  yield check_autocomplete({
    search: "foo ^ #",
    matches: [ { uri: uri2, title: "foo.bar" },
               { uri: uri4, title: "foo.bar" },
               { uri: uri6, title: "foo.bar" },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo ^ @ -> history, in url");
  yield check_autocomplete({
    search: "foo ^ @",
    matches: [ { uri: uri3, title: "title" },
               { uri: uri4, title: "foo.bar" },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo ^ + -> history, is tag");
  yield check_autocomplete({
    search: "foo ^ +",
    matches: [ { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo ^ ~ -> history, is typed");
  yield check_autocomplete({
    search: "foo ^ ~",
    matches: [ { uri: uri4, title: "foo.bar" },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo * # -> is star, in title");
  yield check_autocomplete({
    search: "foo * #",
    matches: [ { uri: uri6, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri8, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri9, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri10, title: "foo.bar", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] } ]
  });

  do_print("foo * @ -> is star, in url");
  yield check_autocomplete({
    search: "foo * @",
    matches: [ { uri: uri7, title: "title", style: [ "bookmark" ] },
               { uri: uri8, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] } ]
  });

  do_print("foo * + -> same as +");
  yield check_autocomplete({
    search: "foo * +",
    matches: [ { uri: uri9, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri10, title: "foo.bar", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] } ]
  });

  do_print("foo * ~ -> is star, is typed");
  yield check_autocomplete({
    search: "foo * ~",
    matches: [ { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] } ]
  });

  do_print("foo # @ -> in title, in url");
  yield check_autocomplete({
    search: "foo # @",
    matches: [ { uri: uri4, title: "foo.bar" },
               { uri: uri8, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo # + -> in title, is tag");
  yield check_autocomplete({
    search: "foo # +",
    matches: [ { uri: uri9, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri10, title: "foo.bar", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo # ~ -> in title, is typed");
  yield check_autocomplete({
    search: "foo # ~",
    matches: [ { uri: uri4, title: "foo.bar" },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo @ + -> in url, is tag");
  yield check_autocomplete({
    search: "foo @ +",
    matches: [ { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo @ ~ -> in url, is typed");
  yield check_autocomplete({
    search: "foo @ ~",
    matches: [ { uri: uri4, title: "foo.bar" },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  do_print("foo + ~ -> is tag, is typed");
  yield check_autocomplete({
    search: "foo + ~",
    matches: [ { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "tag" ] } ]
  });

  
  
  Services.prefs.setBoolPref("browser.urlbar.autoFill", false);

  
  do_print("foo -> default history");
  setSuggestPrefsToFalse();
  Services.prefs.setBoolPref("browser.urlbar.suggest.history", true);
  yield check_autocomplete({
    search: "foo",
    matches: [ { uri: uri2, title: "foo.bar" },
               { uri: uri3, title: "title" },
               { uri: uri4, title: "foo.bar" },
               { uri: uri6, title: "foo.bar" },
               { uri: uri11, title: "title", tags: ["foo.bar"], style: [ "tag" ] } ]
  });

  do_print("foo -> default history, is star");
  setSuggestPrefsToFalse();
  Services.prefs.setBoolPref("browser.urlbar.suggest.history", true);
  Services.prefs.setBoolPref("browser.urlbar.suggest.bookmark", true);
  yield check_autocomplete({
    search: "foo",
    matches: [ { uri: uri2, title: "foo.bar" },
               { uri: uri3, title: "title" },
               { uri: uri4, title: "foo.bar" },
               { uri: uri6, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri7, title: "title", style: [ "bookmark" ] },
               { uri: uri8, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri9, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri10, title: "foo.bar", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar"], style: [ "bookmark-tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] } ]
  });

  do_print("foo -> default history, is star, is typed");
  setSuggestPrefsToFalse();
  Services.prefs.setBoolPref("browser.urlbar.suggest.history", true);
  Services.prefs.setBoolPref("browser.urlbar.suggest.history.onlyTyped", true);
  Services.prefs.setBoolPref("browser.urlbar.suggest.bookmark", true);
  yield check_autocomplete({
    search: "foo",
    matches: [ { uri: uri4, title: "foo.bar" },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] } ]
  });

  do_print("foo -> is star");
  setSuggestPrefsToFalse();
  Services.prefs.setBoolPref("browser.urlbar.suggest.history", false);
  Services.prefs.setBoolPref("browser.urlbar.suggest.bookmark", true);
  yield check_autocomplete({
    search: "foo",
    matches: [ { uri: uri6, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri7, title: "title", style: [ "bookmark" ] },
               { uri: uri8, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri9, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri10, title: "foo.bar", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] } ]
  });

  do_print("foo -> is star, is typed");
  setSuggestPrefsToFalse();
  
  Services.prefs.setBoolPref("browser.urlbar.suggest.history.onlyTyped", true);
  Services.prefs.setBoolPref("browser.urlbar.suggest.bookmark", true);
  yield check_autocomplete({
    search: "foo",
    matches: [ { uri: uri6, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri7, title: "title", style: [ "bookmark" ] },
               { uri: uri8, title: "foo.bar", style: [ "bookmark" ] },
               { uri: uri9, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri10, title: "foo.bar", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri11, title: "title", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] },
               { uri: uri12, title: "foo.bar", tags: [ "foo.bar" ], style: [ "bookmark-tag" ] }  ]
  });

  yield cleanup();
});
