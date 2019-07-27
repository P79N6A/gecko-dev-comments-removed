


 "use strict";

const { makeFilters } = require("sdk/deprecated/unit-test-finder");

const testFiles = [ "test-clipboard", "test-timers" ]
const testMethods = [ "test set clipboard", "test setTimeout" ]

exports["test makeFilters no options"] = (assert) => {
  let { fileFilter, testFilter } = makeFilters();
  testFiles.forEach(f => assert.ok(fileFilter(f), "using no options on filename " + f + " works"));
  testMethods.forEach(m => assert.ok(testFilter(m), "using no options on method name " + m + " works"));
}

exports["test makeFilters no filter"] = (assert) => {
  let { fileFilter, testFilter } = makeFilters({});
  testFiles.forEach(f => assert.ok(fileFilter(f), "using no options on filename " + f + " works"));
  testMethods.forEach(m => assert.ok(testFilter(m), "using no options on method name " + m + " works"));
}

exports["test makeFilters no method filter"] = (assert) => {
  let { fileFilter, testFilter } = makeFilters({ filter: "i" });
  testFiles.forEach(f => assert.ok(fileFilter(f), "using filter 'i' on filename " + f + " works"));
  testMethods.forEach(m => assert.ok(testFilter(m), "using filter 'i' on method name " + m + " works"));

  let { fileFilter, testFilter } = makeFilters({ filter: "i:" });
  testFiles.forEach(f => assert.ok(fileFilter(f), "using filter 'i:' on filename " + f + " works"));
  testMethods.forEach(m => assert.ok(testFilter(m), "using filter 'i:' on method name " + m + " works"));

  let { fileFilter, testFilter } = makeFilters({ filter: "z:" });
  testFiles.forEach(f => assert.ok(!fileFilter(f), "using filter 'z:' on filename " + f + " dnw"));
  testMethods.forEach(m => assert.ok(testFilter(m), "using filter 'z:' on method name " + m + " works"));
}

exports["test makeFilters no file filter"] = (assert) => {
  let { fileFilter, testFilter } = makeFilters({ filter: ":i" });
  testFiles.forEach(f => assert.ok(fileFilter(f), "using filter ':i' on filename " + f + " works"));
  testMethods.forEach(m => assert.ok(testFilter(m), "using filter ':i' on method name " + m + " works"));

  let { fileFilter, testFilter } = makeFilters({ filter: ":z" });
  testFiles.forEach(f => assert.ok(fileFilter(f), "using filter ':z' on filename " + f + " works"));
  testMethods.forEach(m => assert.ok(!testFilter(m), "using filter ':z' on method name " + m + " dnw"));
}

exports["test makeFilters both filters"] = (assert) => {
  let { fileFilter, testFilter } = makeFilters({ filter: "i:i" });
  testFiles.forEach(f => assert.ok(fileFilter(f), "using filter 'i:i' on filename " + f + " works"));
  testMethods.forEach(m => assert.ok(testFilter(m), "using filter 'i:i' on method name " + m + " works"));

  let { fileFilter, testFilter } = makeFilters({ filter: "z:z" });
  testFiles.forEach(f => assert.ok(!fileFilter(f), "using filter 'z:z' on filename " + f + " dnw"));
  testMethods.forEach(m => assert.ok(!testFilter(m), "using filter 'z:z' on method name " + m + " dnw"));
}

require("sdk/test").run(exports);
