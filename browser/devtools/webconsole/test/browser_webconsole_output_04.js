






const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console-output-04.html";

let inputTests = [
  
  {
    input: "testTextNode()",
    output: '#text "hello world!"',
    printOutput: "[object Text]",
    inspectable: true,
    noClick: true,
  },

  
  {
    input: "testCommentNode()",
    output: /<!--\s+- Any copyright /,
    printOutput: "[object Comment]",
    inspectable: true,
    noClick: true,
  },

  
  {
    input: "testDocumentFragment()",
    output: 'DocumentFragment [ <div#foo1.bar>, <div#foo3> ]',
    printOutput: "[object DocumentFragment]",
    inspectable: true,
    variablesViewLabel: "DocumentFragment[2]",
  },

  
  {
    input: "testError()",
    output: "TypeError: window.foobar is not a function\n" +
            "Stack trace:\n" +
            "testError@" + TEST_URI + ":44",
    printOutput: '"TypeError: window.foobar is not a function"',
    inspectable: true,
    variablesViewLabel: "TypeError",
  },

  
  {
    input: "testDOMException()",
    output: 'DOMException [SyntaxError: "An invalid or illegal string was specified"',
    printOutput: '[object XrayWrapper [object DOMException]]"',
    inspectable: true,
    variablesViewLabel: "SyntaxError",
  },

  
  {
    input: "testCSSStyleDeclaration()",
    output: 'CSS2Properties { color: "green", font-size: "2em" }',
    printOutput: "[object CSS2Properties]",
    inspectable: true,
    noClick: true,
  },

  
  {
    input: "testStyleSheetList()",
    output: "StyleSheetList [ CSSStyleSheet ]",
    printOutput: "[object StyleSheetList",
    inspectable: true,
    variablesViewLabel: "StyleSheetList[1]",
  },

  
  {
    input: "document.styleSheets[0]",
    output: "CSSStyleSheet",
    printOutput: "[object CSSStyleSheet]",
    inspectable: true,
  },

  
  {
    input: "document.styleSheets[0].cssRules",
    output: "CSSRuleList [ CSSStyleRule, CSSMediaRule ]",
    printOutput: "[object CSSRuleList",
    inspectable: true,
    variablesViewLabel: "CSSRuleList[2]",
  },

  
  {
    input: "document.styleSheets[0].cssRules[0]",
    output: 'CSSStyleRule "p, div"',
    printOutput: "[object CSSStyleRule",
    inspectable: true,
    variablesViewLabel: "CSSStyleRule",
  },

  
  {
    input: "document.styleSheets[0].cssRules[1]",
    output: 'CSSMediaRule "print"',
    printOutput: "[object CSSMediaRule",
    inspectable: true,
    variablesViewLabel: "CSSMediaRule",
  },
];

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole().then((hud) => {
      return checkOutputForInputs(hud, inputTests);
    }).then(finishTest);
  }, true);
}
