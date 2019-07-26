



const { getRulesForLocale } = require("sdk/l10n/plural-rules");




function map(test, f, n, form) {
  test.assertEqual(f(n), form, n + " maps to '" + form + "'");
}

exports.testFrench = function(test) {
  let f = getRulesForLocale("fr");
  map(test, f, -1, "other");
  map(test, f, 0, "one");
  map(test, f, 1, "one");
  map(test, f, 1.5, "one");
  map(test, f, 2, "other");
  map(test, f, 100, "other");
}

exports.testEnglish = function(test) {
  let f = getRulesForLocale("en");
  map(test, f, -1, "other");
  map(test, f, 0, "other");
  map(test, f, 1, "one");
  map(test, f, 1.5, "other");
  map(test, f, 2, "other");
  map(test, f, 100, "other");
}

exports.testArabic = function(test) {
  let f = getRulesForLocale("ar");
  map(test, f, -1, "other");
  map(test, f, 0, "zero");
  map(test, f, 0.5, "other");

  map(test, f, 1, "one");
  map(test, f, 1.5, "other");

  map(test, f, 2, "two");
  map(test, f, 2.5, "other");

  map(test, f, 3, "few");
  map(test, f, 3.5, "few"); 
                            
  map(test, f, 5, "few");
  map(test, f, 10, "few");
  map(test, f, 103, "few");
  map(test, f, 105, "few");
  map(test, f, 110, "few");
  map(test, f, 203, "few");
  map(test, f, 205, "few");
  map(test, f, 210, "few");

  map(test, f, 11, "many");
  map(test, f, 50, "many");
  map(test, f, 99, "many");
  map(test, f, 111, "many");
  map(test, f, 150, "many");
  map(test, f, 199, "many");

  map(test, f, 100, "other");
  map(test, f, 101, "other");
  map(test, f, 102, "other");
  map(test, f, 200, "other");
  map(test, f, 201, "other");
  map(test, f, 202, "other");
}

exports.testJapanese = function(test) {
  
  let f = getRulesForLocale("ja");
  map(test, f, -1, "other");
  map(test, f, 0, "other");
  map(test, f, 1, "other");
  map(test, f, 1.5, "other");
  map(test, f, 2, "other");
  map(test, f, 100, "other");
}
