




"use strict";

function test() {
  let testcases = [
    ["example", {}, "example"],
    ["example", {ctrlKey: true}, "http://www.example.com/"],
    ["example.org", {ctrlKey: true}, "example.org"],
    ["example", {shiftKey: true}, "http://www.example.net/"],
    ["example", {shiftKey: true, ctrlKey: true}, "http://www.example.org/"],
    ["  example  ", {ctrlKey: true}, "http://www.example.com/"],
    [" example/a ", {ctrlKey: true}, "http://www.example.com/a"]
  ];
  for (let [input, modifiers, result] of testcases) {
    is(BrowserUI._canonizeURL(input, modifiers), result, input + " -> " + result);
  }
}
