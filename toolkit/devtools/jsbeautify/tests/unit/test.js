




"use strict";

function run_test() {
  var sanityTest = new SanityTest();
  var results = run_beautifier_tests(sanityTest,
                                     Urlencoded,
                                     beautify.js,
                                     beautify.html,
                                     beautify.css);

  for (let [test_name, parameters, expected_value, result] of sanityTest.successes) {
    equal(result, expected_value, "actual result matches expected");
  }

  for (let [test_name, parameters, expected_value, result] of sanityTest.failures) {
    equal(result, expected_value, "actual result matches expected");
  }
}
