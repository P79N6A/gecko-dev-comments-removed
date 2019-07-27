






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
}
