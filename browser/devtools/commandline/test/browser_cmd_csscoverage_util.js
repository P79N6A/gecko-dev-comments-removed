




const csscoverage = require("devtools/server/actors/csscoverage");

let test = asyncTest(function*() {
  testDeconstructRuleId();
});

function testDeconstructRuleId() {
  
  let rule = csscoverage.deconstructRuleId("http://thing/blah|10|20");
  is(rule.url, "http://thing/blah", "1 url");
  is(rule.line, 10, "1 line");
  is(rule.column, 20, "1 column");

  
  rule = csscoverage.deconstructRuleId("http://thing/blah?q=a|b|11|22");
  is(rule.url, "http://thing/blah?q=a|b", "2 url");
  is(rule.line, 11, "2 line");
  is(rule.column, 22, "2 column");
}
