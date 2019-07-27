






const TEST_URI = "data:text/html;charset=utf8,test for console output - 05";

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
];

function test() {
  Task.spawn(function*() {
    let {tab} = yield loadTab(TEST_URI);
    let hud = yield openConsole(tab);
    return checkOutputForInputs(hud, inputTests);
  }).then(finishUp);
}

function finishUp() {
  inputTests = null;
  finishTest();
}
