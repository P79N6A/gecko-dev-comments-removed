






const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console-output-02.html";

let inputTests = [
  
  {
    input: "document.getElementById",
    output: "function getElementById()",
    printOutput: "function getElementById() {\\n    [native code]\\n}",
    inspectable: true,
    variablesViewLabel: "getElementById()",
  },

  
  {
    input: "(function() { return 42; })",
    output: "function ()",
    printOutput: "function () { return 42; }",
    inspectable: true,
  },

  
  {
    input: "window.testfn1",
    output: "function testfn1()",
    printOutput: "function testfn1() { return 42; }",
    inspectable: true,
    variablesViewLabel: "testfn1()",
  },

  
  {
    input: "testobj1.testfn2",
    output: "function testobj1.testfn2()",
    printOutput: "function () { return 42; }",
    inspectable: true,
    variablesViewLabel: "testobj1.testfn2()",
  },

  
  {
    input: "window.testfn3",
    output: "function testfn3DisplayName()",
    printOutput: "function testfn3() { return 42; }",
    inspectable: true,
    variablesViewLabel: "testfn3DisplayName()",
  },

  
  {
    input: "window.array1",
    output: '[1, 2, 3, "a", "b", "c", "4", "5"]',
    printOutput: "1,2,3,a,b,c,4,5",
    inspectable: true,
    variablesViewLabel: "Array[8]",
  },

  
  {
    input: "window.array2",
    output: '["a", HTMLDocument \u2192 test-console-output-02.html, <body>, ' +
            "DOMStringMap[0], DOMTokenList[0]]",
    printOutput: '"a,[object HTMLDocument],[object HTMLBodyElement],' +
                 '[object DOMStringMap],"',
    inspectable: true,
    variablesViewLabel: "Array[5]",
  },

  
  {
    input: "window.array3",
    output: '[1, Window \u2192 test-console-output-02.html, null, "a", "b", ' +
            'undefined, false, "", -Infinity, testfn3DisplayName(), 3 more\u2026]',
    printOutput: '"1,[object Window],,a,b,,false,,-Infinity,' +
                 'function testfn3() { return 42; },[object Object],foo,bar"',
    inspectable: true,
    variablesViewLabel: "Array[13]",
  },

  
  {
    input: "window.array4",
    output: '[,,,,, "test", Array[7]]',
    printOutput: '",,,,,test,"',
    inspectable: true,
    variablesViewLabel: "Array[7]",
  },

  
  {
    input: "window.typedarray1",
    output: 'Int32Array [1, 287, 8651, 40983, 8754]',
    printOutput: "[object Int32Array]",
    inspectable: true,
    variablesViewLabel: "Int32Array[5]",
  },

  
  {
    input: "window.set1",
    output: 'Set [1, 2, null, Array[13], "a", "b", undefined, <head>, Set[9]]',
    printOutput: "[object Set]",
    inspectable: true,
    variablesViewLabel: "Set[9]",
  },

  
  {
    input: "window.testobj2",
    output: '{a: "b", c: "d", e: 1, f: "2", foo: Object, bar: Object, ' +
            "getterTest: Getter}",
    printOutput: "[object Object]",
    inspectable: true,
    variablesViewLabel: "Object",
  },

  
  {
    input: "window.testobj3",
    output: '{a: "b", c: "d", e: 1, f: "2", g: true, h: null, i: undefined, ' +
            'j: "", k: StyleSheetList[0], l: NodeList[5], 2 more\u2026}',
    printOutput: "[object Object]",
    inspectable: true,
    variablesViewLabel: "Object",
  },

  
  {
    input: "window.testobj4",
    output: '{a: "b", c: "d", 1 more\u2026}',
    printOutput: "[object Object]",
    inspectable: true,
    variablesViewLabel: "Object",
  },

  
  {
    input: "window.map1",
    output: 'Map {a: "b", HTMLCollection[2]: Object, Map[3]: Set[9]}',
    printOutput: "[object Map]",
    inspectable: true,
    variablesViewLabel: "Map[3]",
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
