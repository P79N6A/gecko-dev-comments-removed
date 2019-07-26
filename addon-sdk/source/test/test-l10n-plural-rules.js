



const { getRulesForLocale } = require("sdk/l10n/plural-rules");




function map(assert, f, n, form) {
  assert.equal(f(n), form, n + " maps to '" + form + "'");
}

exports.testFrench = function(assert) {
  let f = getRulesForLocale("fr");
  map(assert, f, -1, "other");
  map(assert, f, 0, "one");
  map(assert, f, 1, "one");
  map(assert, f, 1.5, "one");
  map(assert, f, 2, "other");
  map(assert, f, 100, "other");
}

exports.testEnglish = function(assert) {
  let f = getRulesForLocale("en");
  map(assert, f, -1, "other");
  map(assert, f, 0, "other");
  map(assert, f, 1, "one");
  map(assert, f, 1.5, "other");
  map(assert, f, 2, "other");
  map(assert, f, 100, "other");
}

exports.testArabic = function(assert) {
  let f = getRulesForLocale("ar");
  map(assert, f, -1, "other");
  map(assert, f, 0, "zero");
  map(assert, f, 0.5, "other");

  map(assert, f, 1, "one");
  map(assert, f, 1.5, "other");

  map(assert, f, 2, "two");
  map(assert, f, 2.5, "other");

  map(assert, f, 3, "few");
  map(assert, f, 3.5, "few"); 
                            
  map(assert, f, 5, "few");
  map(assert, f, 10, "few");
  map(assert, f, 103, "few");
  map(assert, f, 105, "few");
  map(assert, f, 110, "few");
  map(assert, f, 203, "few");
  map(assert, f, 205, "few");
  map(assert, f, 210, "few");

  map(assert, f, 11, "many");
  map(assert, f, 50, "many");
  map(assert, f, 99, "many");
  map(assert, f, 111, "many");
  map(assert, f, 150, "many");
  map(assert, f, 199, "many");

  map(assert, f, 100, "other");
  map(assert, f, 101, "other");
  map(assert, f, 102, "other");
  map(assert, f, 200, "other");
  map(assert, f, 201, "other");
  map(assert, f, 202, "other");
}

exports.testJapanese = function(assert) {
  
  let f = getRulesForLocale("ja");
  map(assert, f, -1, "other");
  map(assert, f, 0, "other");
  map(assert, f, 1, "other");
  map(assert, f, 1.5, "other");
  map(assert, f, 2, "other");
  map(assert, f, 100, "other");
}

require('sdk/test').run(exports);
