






const TEST_URI = "data:text/html;charset=utf8,test for console output - 01";

let dateNow = Date.now();
let {DebuggerServer} = Cu.import("resource://gre/modules/devtools/dbg-server.jsm", {});

let LONG_STRING_LENGTH = DebuggerServer.LONG_STRING_LENGTH;
let LONG_STRING_INITIAL_LENGTH = DebuggerServer.LONG_STRING_INITIAL_LENGTH;
DebuggerServer.LONG_STRING_LENGTH = 100;
DebuggerServer.LONG_STRING_INITIAL_LENGTH = 50;

let longString = (new Array(DebuggerServer.LONG_STRING_LENGTH + 4)).join("a");
let initialString = longString.substring(0, DebuggerServer.LONG_STRING_INITIAL_LENGTH);

let inputTests = [
  
  {
    input: "'hello \\nfrom \\rthe \\\"string world!'",
    output: "\"hello \\nfrom \\rthe \\\"string world!\"",
  },

  
  {
    
    input: "'\xFA\u1E47\u0129\xE7\xF6d\xEA \u021B\u0115\u0219\u0165'",
    output: "\"\\xFA\\u1E47\\u0129\\xE7\\xF6d\\xEA \\u021B\\u0115\\u0219\\u0165\"",
  },

  
  {
    input: "'" + longString + "'",
    output: '"' + initialString + "\"[\u2026]",
    printOutput: initialString,
  },

  
  {
    input: "''",
    output: '""',
    printOutput: '""',
  },

  
  {
    input: "0",
    output: "0",
  },

  
  {
    input: "'0'",
    output: '"0"',
  },

  
  {
    input: "42",
    output: "42",
  },

  
  {
    input: "'42'",
    output: '"42"',
  },

  
  {
    input: "/foobar/",
    output: "/foobar/",
    inspectable: true,
  },

  
  {
    input: "/foo?b*\\s\"ar/igym",
    output: "/foo?b*\\s\"ar/gimy",
    printOutput: "/foo?b*\\\\s\\\"ar/gimy",
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
    input: "false",
    output: "false",
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
];

longString = initialString = null;

function test() {
  registerCleanupFunction(() => {
    DebuggerServer.LONG_STRING_LENGTH = LONG_STRING_LENGTH;
    DebuggerServer.LONG_STRING_INITIAL_LENGTH = LONG_STRING_INITIAL_LENGTH;
  });

  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole().then((hud) => {
      return checkOutputForInputs(hud, inputTests);
    }).then(finishTest);
  }, true);
}
