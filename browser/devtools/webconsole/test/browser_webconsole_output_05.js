






"use strict";

const TEST_URI = "data:text/html;charset=utf8,test for console output - 05";
const ELLIPSIS = Services.prefs.getComplexValue("intl.ellipsis",
  Ci.nsIPrefLocalizedString).data;

let dateNow = Date.now();

let inputTests = [
  
  {
    input: "/foo?b*\\s\"ar/igym",
    output: "/foo?b*\\s\"ar/gimy",
    printOutput: "/foo?b*\\s\"ar/gimy",
    inspectable: true,
  },

  
  {
    input: "null",
    output: "null",
  },

  
  {
    input: "undefined",
    output: "undefined",
  },

  
  {
    input: "true",
    output: "true",
  },

  
  {
    input: "new Boolean(false)",
    output: "false",
    inspectable: true,
  },

  
  {
    input: "new Date(" + dateNow + ")",
    output: "Date " + (new Date(dateNow)).toISOString(),
    printOutput: (new Date(dateNow)).toString(),
    inspectable: true,
  },

  
  {
    input: "new Date('test')",
    output: "Invalid Date",
    printOutput: "Invalid Date",
    inspectable: true,
    variablesViewLabel: "Invalid Date",
  },

  
  {
    input: "Date.prototype",
    output: /Object \{.*\}/,
    printOutput: "Invalid Date",
    inspectable: true,
    variablesViewLabel: "Object",
  },

  
  {
    input: "new Number(43)",
    output: "43",
    inspectable: true,
  },

  
  {
    input: "new String('hello')",
    output: 'String [ "h", "e", "l", "l", "o" ]',
    printOutput: "hello",
    inspectable: true,
    variablesViewLabel: "String[5]"
  },

  
  {
    
    
    input: "new Promise(function () {})",
    output: 'Promise { <state>: "pending" }',
    printOutput: "[object Promise]",
    inspectable: true,
    variablesViewLabel: "Promise"
  },

  
  {
    input: "(function () { var p = new Promise(function () {}); " +
           "p.foo = 1; return p; }())",
    output: 'Promise { <state>: "pending", foo: 1 }',
    printOutput: "[object Promise]",
    inspectable: true,
    variablesViewLabel: "Promise"
  },

  
  {
    input: "new Object({1: 'this\\nis\\nsupposed\\nto\\nbe\\na\\nvery" +
           "\\nlong\\nstring\\n,shown\\non\\na\\nsingle\\nline', " +
           "2: 'a shorter string', 3: 100})",
    output: 'Object { 1: "this is supposed to be a very long ' + ELLIPSIS +
            '", 2: "a shorter string", 3: 100 }',
    printOutput: "[object Object]",
    inspectable: false,
  }
];

function test() {
  requestLongerTimeout(2);
  Task.spawn(function*() {
    let {tab} = yield loadTab(TEST_URI);
    let hud = yield openConsole(tab);
    return checkOutputForInputs(hud, inputTests);
  }).then(finishUp);
}

function finishUp() {
  inputTests = dateNow = null;
  finishTest();
}
