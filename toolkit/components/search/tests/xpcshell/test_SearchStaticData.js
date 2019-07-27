






"use strict";

Cu.import("resource://gre/modules/SearchStaticData.jsm", this);

function run_test() {
  do_check_true(SearchStaticData.getAlternateDomains("www.google.com")
                                .indexOf("www.google.fr") != -1);
  do_check_true(SearchStaticData.getAlternateDomains("www.google.fr")
                                .indexOf("www.google.com") != -1);
  do_check_true(SearchStaticData.getAlternateDomains("www.google.com")
                                .every(d => d.startsWith("www.google.")));
  do_check_true(SearchStaticData.getAlternateDomains("google.com").length == 0);

  
  
  let backup = SearchStaticData.getAlternateDomains;
  SearchStaticData.getAlternateDomains = () => ["www.bing.fr"];;
  do_check_matches(SearchStaticData.getAlternateDomains("www.bing.com"), ["www.bing.fr"]);
  SearchStaticData.getAlternateDomains = backup;
}
