






"use strict";

function run_test() {
  removeMetadata();
  updateAppInfo();
  useHttpServer();

  run_next_test();
}

add_task(function* test_rel_searchform() {
  let engineNames = [
    "engine-rel-searchform.xml",
    "engine-rel-searchform-post.xml",
  ];

  
  
  
  
  
  let items = [for (e of engineNames) { name: e, xmlFileName: e }];
  for (let engine of yield addTestEngines(items)) {
    do_check_eq(engine.searchForm, "http://" + engine.name + "/?search");
  }
});
