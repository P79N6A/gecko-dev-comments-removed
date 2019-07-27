




const {parseDeclarations} = devtools.require("devtools/styleinspector/css-parsing-utils");

const TEST_DATA = [
  
  {
    input: "p:v;",
    expected: [{name: "p", value: "v", priority: ""}]
  },
  
  {
    input: "this:is;a:test;",
    expected: [
      {name: "this", value: "is", priority: ""},
      {name: "a", value: "test", priority: ""}
    ]
  },
  
  {
    input: "name:value;",
    expected: [{name: "name", value: "value", priority: ""}]
  },
  
  {
    input: "name:value",
    expected: [{name: "name", value: "value", priority: ""}]
  },
  
  {
    input: "p1 : v1 ; \t\t  \n p2:v2;   \n\n\n\n\t  p3    :   v3;",
    expected: [
      {name: "p1", value: "v1", priority: ""},
      {name: "p2", value: "v2", priority: ""},
      {name: "p3", value: "v3", priority: ""},
    ]
  },
  
  {
    input: "p1: v1; p2: v2 !important;",
    expected: [
      {name: "p1", value: "v1", priority: ""},
      {name: "p2", value: "v2", priority: "important"}
    ]
  },
  
  {
    input: "p1: v1 !important; p2: v2",
    expected: [
      {name: "p1", value: "v1", priority: "important"},
      {name: "p2", value: "v2", priority: ""}
    ]
  },
  
  {
    input: "p1: v1 !  important; p2: v2 ! important;",
    expected: [
      {name: "p1", value: "v1", priority: "important"},
      {name: "p2", value: "v2", priority: "important"}
    ]
  },
  
  {
    input: "p1: v1 important;",
    expected: [
      {name: "p1", value: "v1 important", priority: ""}
    ]
  },
  
  {
    input: "background-image: url(../../relative/image.png)",
    expected: [{name: "background-image", value: "url(\"../../relative/image.png\")", priority: ""}]
  },
  {
    input: "background-image: url(http://site.com/test.png)",
    expected: [{name: "background-image", value: "url(\"http://site.com/test.png\")", priority: ""}]
  },
  {
    input: "background-image: url(wow.gif)",
    expected: [{name: "background-image", value: "url(\"wow.gif\")", priority: ""}]
  },
  
  {
    input: "background: red url(\"http://site.com/image{}:;.png?id=4#wat\") repeat top right",
    expected: [
      {name: "background", value: "red url(\"http://site.com/image{}:;.png?id=4#wat\") repeat top right", priority: ""}
    ]
  },
  
  {input: "", expected: []},
  
  {input: "       \n \n   \n   \n \t  \t\t\t  ", expected: []},
  
  {input: null, throws: true},
  
  {input: undefined, throws: true},
  
  {
    input: "content: \";color:red;}selector{color:yellow;\"",
    expected: [
      {name: "content", value: "\";color:red;}selector{color:yellow;\"", priority: ""}
    ]
  },
  
  
  {
    input: "body {color:red;} p {color: blue;}",
    expected: [
      {name: "body {color", value: "red", priority: ""},
      {name: "} p {color", value: "blue", priority: ""},
      {name: "}", value: "", priority: ""}
    ]
  },
  
  {
    input: "color :red : font : arial;",
    expected : [
      {name: "color", value: "red : font : arial", priority: ""}
    ]
  },
  {input: "background: red;;;;;", expected: [{name: "background", value: "red", priority: ""}]},
  {input: "background:;", expected: [{name: "background", value: "", priority: ""}]},
  {input: ";;;;;", expected: []},
  {input: ":;:;", expected: []},
  
  {input: "color", expected: [
    {name: "color", value: "", priority: ""}
  ]},
  
  {input: "color:blue;font", expected: [
    {name: "color", value: "blue", priority: ""},
    {name: "font", value: "", priority: ""}
  ]},
  
  {input: "color:blue;font:", expected: [
    {name: "color", value: "blue", priority: ""},
    {name: "font", value: "", priority: ""}
  ]},
  
  {input: "Arial;color:blue;", expected: [
    {name: "", value: "Arial", priority: ""},
    {name: "color", value: "blue", priority: ""}
  ]},
  
  {input: "color: #333", expected: [{name: "color", value: "#333", priority: ""}]},
  {input: "color: #456789", expected: [{name: "color", value: "#456789", priority: ""}]},
  {input: "wat: #XYZ", expected: [{name: "wat", value: "#XYZ", priority: ""}]},
  
  {input: "content: \"this is a 'string'\"", expected: [{name: "content", value: "\"this is a 'string'\"", priority: ""}]},
  {input: 'content: "this is a \\"string\\""', expected: [{name: "content", value: '\'this is a "string"\'', priority: ""}]},
  {input: "content: 'this is a \"string\"'", expected: [{name: "content", value: '\'this is a "string"\'', priority: ""}]},
  {input: "content: 'this is a \\'string\\'", expected: [{name: "content", value: '"this is a \'string\'"', priority: ""}]},
  {input: "content: 'this \\' is a \" really strange string'", expected: [{name: "content", value: '"this \' is a \" really strange string"', priority: ""}]},
  {
    input: "content: \"a not s\\\
          o very long title\"",
    expected: [
      {name: "content", value: '"a not s\
          o very long title"', priority: ""}
    ]
  }
];

function run_test() {
  for (let test of TEST_DATA) {
    do_print("Test input string " + test.input);
    let output;
    try {
      output = parseDeclarations(test.input);
    } catch (e) {
      do_print("parseDeclarations threw an exception with the given input string");
      if (test.throws) {
        do_print("Exception expected");
        do_check_true(true);
      } else {
        do_print("Exception unexpected\n" + e);
        do_check_true(false);
      }
    }
    if (output) {
      assertOutput(output, test.expected);
    }
  }
}

function assertOutput(actual, expected) {
  if (actual.length === expected.length) {
    for (let i = 0; i < expected.length; i ++) {
      do_check_true(!!actual[i]);
      do_print("Check that the output item has the expected name, value and priority");
      do_check_eq(expected[i].name, actual[i].name);
      do_check_eq(expected[i].value, actual[i].value);
      do_check_eq(expected[i].priority, actual[i].priority);
    }
  } else {
    for (let prop of actual) {
      do_print("Actual output contained: {name: "+prop.name+", value: "+prop.value+", priority: "+prop.priority+"}");
    }
    do_check_eq(actual.length, expected.length);
  }
}
