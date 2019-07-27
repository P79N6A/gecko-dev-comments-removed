















let katakana = ["\u30a8", "\u30c9"]; 
let ideograph = ["\u4efb", "\u5929", "\u5802"]; 

add_task(function* test_escape() {
  Services.prefs.setBoolPref("browser.urlbar.autoFill.searchEngines", false);

  let uri1 = NetUtil.newURI("http://matchme/");
  let uri2 = NetUtil.newURI("http://dontmatchme/");
  let uri3 = NetUtil.newURI("http://title/1");
  let uri4 = NetUtil.newURI("http://title/2");
  let uri5 = NetUtil.newURI("http://tag/1");
  let uri6 = NetUtil.newURI("http://tag/2");
  let uri7 = NetUtil.newURI("http://crazytitle/");
  let uri8 = NetUtil.newURI("http://katakana/");
  let uri9 = NetUtil.newURI("http://ideograph/");
  let uri10 = NetUtil.newURI("http://camel/pleaseMatchMe/");

  yield promiseAddVisits([ { uri: uri1, title: "title1" },
                           { uri: uri2, title: "title1" },
                           { uri: uri3, title: "matchme2" },
                           { uri: uri4, title: "dontmatchme3" },
                           { uri: uri5, title: "title1" },
                           { uri: uri6, title: "title1" },
                           { uri: uri7, title: "!@#$%^&*()_+{}|:<>?word" },
                           { uri: uri8, title: katakana.join("") },
                           { uri: uri9, title: ideograph.join("") },
                           { uri: uri10, title: "title1" } ]);
  addBookmark( { uri: uri5, title: "title1", tags: [ "matchme2" ] } );
  addBookmark( { uri: uri6, title: "title1", tags: [ "dontmatchme3" ] } );

  
  Services.prefs.setIntPref("browser.urlbar.matchBehavior", 2);

  do_log_info("Match 'match' at the beginning or after / or on a CamelCase");
  yield check_autocomplete({
    search: "match",
    matches: [ { uri: uri1, title: "title1" },
               { uri: uri3, title: "matchme2" },
               { uri: uri5, title: "title1", tags: [ "matchme2" ] },
               { uri: uri10, title: "title1" } ]
  });

  do_log_info("Match 'dont' at the beginning or after /");
  yield check_autocomplete({
    search: "dont",
    matches: [ { uri: uri2, title: "title1" },
               { uri: uri4, title: "dontmatchme3" },
               { uri: uri6, title: "title1", tags: [ "dontmatchme3" ] } ]
  });

  do_log_info("Match 'match' at the beginning or after / or on a CamelCase");
  yield check_autocomplete({
    search: "2",
    matches: [ { uri: uri3, title: "matchme2" },
               { uri: uri4, title: "dontmatchme3" },
               { uri: uri5, title: "title1", tags: [ "matchme2" ] },
               { uri: uri6, title: "title1", tags: [ "dontmatchme3" ] } ]
  });

  do_log_info("Match 't' at the beginning or after /");
  yield check_autocomplete({
    search: "t",
    matches: [ { uri: uri1, title: "title1" },
               { uri: uri2, title: "title1" },
               { uri: uri3, title: "matchme2" },
               { uri: uri4, title: "dontmatchme3" },
               { uri: uri5, title: "title1", tags: [ "matchme2" ] },
               { uri: uri6, title: "title1", tags: [ "dontmatchme3" ] },
               { uri: uri10, title: "title1" } ]
  });

  do_log_info("Match 'word' after many consecutive word boundaries");
  yield check_autocomplete({
    search: "word",
    matches: [ { uri: uri7, title: "!@#$%^&*()_+{}|:<>?word" } ]
  });

  do_log_info("Match a word boundary '/' for everything");
  yield check_autocomplete({
    search: "/",
    matches: [ { uri: uri1, title: "title1" },
               { uri: uri2, title: "title1" },
               { uri: uri3, title: "matchme2" },
               { uri: uri4, title: "dontmatchme3" },
               { uri: uri5, title: "title1", tags: [ "matchme2" ] },
               { uri: uri6, title: "title1", tags: [ "dontmatchme3" ] },
               { uri: uri7, title: "!@#$%^&*()_+{}|:<>?word" },
               { uri: uri8, title: katakana.join("") },
               { uri: uri9, title: ideograph.join("") },
               { uri: uri10, title: "title1" } ]
  });

  do_log_info("Match word boundaries '()_+' that are among word boundaries");
  yield check_autocomplete({
    search: "()_+",
    matches: [ { uri: uri7, title: "!@#$%^&*()_+{}|:<>?word" } ]
  });

  do_log_info("Katakana characters form a string, so match the beginning");
  yield check_autocomplete({
    search: katakana[0],
    matches: [ { uri: uri8, title: katakana.join("") } ]
  });








 do_log_info("Ideographs are treated as words so 'nin' is one word");
  yield check_autocomplete({
    search: ideograph[0],
    matches: [ { uri: uri9, title: ideograph.join("") } ]
  });

 do_log_info("Ideographs are treated as words so 'ten' is another word");
  yield check_autocomplete({
    search: ideograph[1],
    matches: [ { uri: uri9, title: ideograph.join("") } ]
  });

 do_log_info("Ideographs are treated as words so 'do' is yet another word");
  yield check_autocomplete({
    search: ideograph[2],
    matches: [ { uri: uri9, title: ideograph.join("") } ]
  });

 do_log_info("Extra negative assert that we don't match in the middle");
  yield check_autocomplete({
    search: "ch",
    matches: [ ]
  });

 do_log_info("Don't match one character after a camel-case word boundary (bug 429498)");
  yield check_autocomplete({
    search: "atch",
    matches: [ ]
  });

  
  Services.prefs.setIntPref("browser.urlbar.matchBehavior", 1);

  yield check_autocomplete({
    search: "tch",
    matches: [ { uri: uri1, title: "title1" },
               { uri: uri2, title: "title1" },
               { uri: uri3, title: "matchme2" },
               { uri: uri4, title: "dontmatchme3" },
               { uri: uri5, title: "title1", tags: [ "matchme2" ] },
               { uri: uri6, title: "title1", tags: [ "dontmatchme3" ] },
               { uri: uri10, title: "title1" } ]
  });

  yield cleanup();
});
