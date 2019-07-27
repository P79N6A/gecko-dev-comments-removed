



add_task(function*() {
  
  
  Services.search.addEngineWithDetails("AliasedMozSearch", "", "doit", "",
                                       "GET", "http://s.example.com/search");

  do_print("search engine");
  yield check_autocomplete({
    search: "mozilla",
    searchParam: "enable-actions",
    matches: [ makeSearchMatch("mozilla") ]
  });

  do_print("search engine, uri-like input");
  yield check_autocomplete({
    search: "http:///",
    searchParam: "enable-actions",
    matches: [ makeSearchMatch("http:///") ]
  });

  do_print("search engine, multiple words");
  yield check_autocomplete({
    search: "mozzarella cheese",
    searchParam: "enable-actions",
    matches: [ makeSearchMatch("mozzarella cheese") ]
  });

  do_print("search engine, after current engine has changed");
  Services.search.addEngineWithDetails("MozSearch2", "", "", "", "GET",
                                       "http://s.example.com/search2");
  engine = Services.search.getEngineByName("MozSearch2");
  notEqual(Services.search.currentEngine, engine, "New engine shouldn't be the current engine yet");
  Services.search.currentEngine = engine;
  yield check_autocomplete({
    search: "mozilla",
    searchParam: "enable-actions",
    matches: [ makeSearchMatch("mozilla", { engineName: "MozSearch2" }) ]
  });

  yield cleanup();
});
