







"use strict";

const TEST_URI = "data:text/html;charset=utf8,Bug 1005909 - Clickable URLS";

let inputTests = [

  
  {
    input: "'http://example.com'",
    output: "http://example.com",
    expectedTab: "http://example.com/",
  },

  
  {
    input: "'https://example.com'",
    output: "https://example.com",
    expectedTab: "https://example.com/",
  },

  
  {
    input: "'https://example.com:443'",
    output: "https://example.com:443",
    expectedTab: "https://example.com/",
  },

  
  {
    input: "'http://example.com/foo'",
    output: "http://example.com/foo",
    expectedTab: "http://example.com/foo",
  },

  
  {
    input: "'foo http://example.com bar'",
    output: "foo http://example.com bar",
    expectedTab: "http://example.com/",
  },

  
  {
    input: "'foo\\nhttp://example.com\\nbar'",
    output: "foo\nhttp://example.com\nbar",
    expectedTab: "http://example.com/",
  },

  
  {
    input: "'http://example.com http://example.com'",
    output: "http://example.com http://example.com",
    expectedTab: "http://example.com/",
  },

  
  {
    input: "'example.com'",
    output: "example.com",
  },

  
  {
    input: "'foo://example.com'",
    output: "foo://example.com",
  },

];

function test() {
  Task.spawn(function*() {
    let {tab} = yield loadTab(TEST_URI);
    let hud = yield openConsole(tab);
    yield checkOutputForInputs(hud, inputTests);
    inputTests = null;
  }).then(finishTest);
}
