







"use strict";

function run_test() {
  removeMetadata();
  updateAppInfo();
  useHttpServer();

  run_next_test();
}

add_task(function* test_defaultEngine() {
  let [engine1, engine2] = yield addTestEngines([
    { name: "Test search engine", xmlFileName: "engine.xml" },
    { name: "A second test engine", xmlFileName: "engine2.xml" },
  ]);

  let search = Services.search;

  search.defaultEngine = engine1;
  do_check_eq(search.defaultEngine, engine1);
  search.defaultEngine = engine2
  do_check_eq(search.defaultEngine, engine2);
  search.defaultEngine = engine1;
  do_check_eq(search.defaultEngine, engine1);

  
  
  
  search.moveEngine(engine2, 0);
  engine1.hidden = true;
  do_check_eq(search.defaultEngine, engine2);

  
  engine1.hidden = false;
  do_check_eq(search.defaultEngine, engine1);

  
  
  engine2.hidden = true;
  search.moveEngine(engine1, 0)
  search.defaultEngine = engine2;
  do_check_eq(search.defaultEngine, engine1);
  engine2.hidden = false;
  do_check_eq(search.defaultEngine, engine2);
});
