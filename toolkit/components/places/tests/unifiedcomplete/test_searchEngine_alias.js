



add_task(function*() {
  
  
  Services.search.addEngineWithDetails("AliasedMozSearch", "", "doit", "",
                                       "GET", "http://s.example.com/search");


  yield check_autocomplete({
    search: "doit",
    searchParam: "enable-actions",
    matches: [ makeSearchMatch("doit") ]
  });

  yield check_autocomplete({
    search: "doit mozilla",
    searchParam: "enable-actions",
    matches: [ makeSearchMatch("doit mozilla", { engineName: "AliasedMozSearch", searchQuery: "mozilla", alias: "doit" }) ]
  });

  yield check_autocomplete({
    search: "doit mozzarella mozilla",
    searchParam: "enable-actions",
    matches: [ makeSearchMatch("doit mozzarella mozilla", { engineName: "AliasedMozSearch", searchQuery: "mozzarella mozilla", alias: "doit" }) ]
  });

  yield cleanup();
});
