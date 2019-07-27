




"use strict";

const loader = new DevToolsLoader();
const require = loader.require;
const {CssLogic} = require("devtools/styleinspector/css-logic");


const TESTS = [
  { name: "simple test",
    input: "div { font-family:'Arial Black', Arial, sans-serif; }",
    expected: [
      "div {",
      "\tfont-family:'Arial Black', Arial, sans-serif;",
      "}"
    ]
  },

  { name: "whitespace before open brace",
    input: "div{}",
    expected: [
      "div {",
      "}"
    ]
  },

  { name: "minified with trailing newline",
    input: "\nbody{background:white;}div{font-size:4em;color:red}span{color:green;}\n",
    expected: [
      "",
      "body {",
      "\tbackground:white;",
      "}",
      "div {",
      "\tfont-size:4em;",
      "\tcolor:red",
      "}",
      "span {",
      "\tcolor:green;",
      "}"
    ]
  },

];

function run_test() {
  for (let test of TESTS) {
    do_print(test.name);

    
    
    let output = CssLogic.prettifyCSS(test.input);
    let expected = test.expected.join(CssLogic.LINE_SEPARATOR) +
        CssLogic.LINE_SEPARATOR;
    equal(output, expected, test.name);
  }
}
