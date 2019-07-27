






const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console-output-03.html";

let inputTests = [

  
  {
    input: "document",
    output: "HTMLDocument \u2192 " + TEST_URI,
    printOutput: "[object HTMLDocument]",
    inspectable: true,
    noClick: true,
  },

  
  {
    input: "window",
    output: "Window \u2192 " + TEST_URI,
    printOutput: "[object Window",
    inspectable: true,
    noClick: true,
  },

  
  {
    input: "document.body",
    output: "<body>",
    printOutput: "[object HTMLBodyElement]",
    inspectable: true,
    noClick: true,
  },

  
  {
    input: "document.body.dataset",
    output: "DOMStringMap {  }",
    printOutput: "[object DOMStringMap]",
    inspectable: true,
    variablesViewLabel: "DOMStringMap[0]",
  },

  
  {
    input: "document.body.classList",
    output: "DOMTokenList [  ]",
    printOutput: '""',
    inspectable: true,
    variablesViewLabel: "DOMTokenList[0]",
  },

  
  {
    input: "window.location.href",
    output: '"' + TEST_URI + '"',
    noClick: true,
  },

  
  {
    input: "window.location",
    output: "Location \u2192 " + TEST_URI,
    printOutput: TEST_URI,
    inspectable: true,
    variablesViewLabel: "Location \u2192 test-console-output-03.html",
  },

  
  {
    input: "document.body.attributes",
    output: "NamedNodeMap [  ]",
    printOutput: "[object NamedNodeMap]",
    inspectable: true,
    variablesViewLabel: "NamedNodeMap[0]",
  },

  
  {
    input: "document.styleSheets",
    output: "StyleSheetList [  ]",
    printOutput: "[object StyleSheetList",
    inspectable: true,
    variablesViewLabel: "StyleSheetList[0]",
  },

  
  {
    input: "testBodyClassName()",
    output: '<body class="test1 tezt2">',
    printOutput: "[object HTMLBodyElement]",
    inspectable: true,
    noClick: true,
  },

  
  {
    input: "testBodyID()",
    output: '<body class="test1 tezt2" id="foobarid">',
    printOutput: "[object HTMLBodyElement]",
    inspectable: true,
    noClick: true,
  },

  
  {
    input: "document.body.classList",
    output: 'DOMTokenList [ "test1", "tezt2" ]',
    printOutput: '"test1 tezt2"',
    inspectable: true,
    variablesViewLabel: "DOMTokenList[2]",
  },

  
  {
    input: "testBodyDataset()",
    output: '<body class="test1 tezt2" id="foobarid"' +
            ' data-preview="zuzu&quot;&lt;a&gt;foo">',
    printOutput: "[object HTMLBodyElement]",
    inspectable: true,
    noClick: true,
  },

  
  {
    input: "document.body.dataset",
    output: 'DOMStringMap { preview: "zuzu"<a>foo" }',
    printOutput: "[object DOMStringMap]",
    inspectable: true,
    variablesViewLabel: "DOMStringMap[1]",
  },

  
  {
    input: "document.body.attributes",
    output: 'NamedNodeMap [ class="test1 tezt2", id="foobarid", ' +
            'data-preview="zuzu&quot;&lt;a&gt;foo" ]',
    printOutput: "[object NamedNodeMap]",
    inspectable: true,
    variablesViewLabel: "NamedNodeMap[3]",
  },

  
  {
    input: "document.body.attributes[0]",
    output: 'class="test1 tezt2"',
    printOutput: "[object Attr]",
    inspectable: true,
    variablesViewLabel: 'class="test1 tezt2"',
  },
];

function test() {
  requestLongerTimeout(2);
  Task.spawn(function*() {
    const {tab} = yield loadTab(TEST_URI);
    const hud = yield openConsole(tab);
    yield checkOutputForInputs(hud, inputTests);
    inputTests = null;
  }).then(finishTest);
}
