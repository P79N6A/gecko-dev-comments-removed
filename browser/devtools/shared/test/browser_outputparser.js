


"use strict";

let {Services} = Cu.import("resource://gre/modules/Services.jsm", {});
let {Loader} = Cu.import("resource://gre/modules/commonjs/toolkit/loader.js",
                         {});
let {OutputParser} = devtools.require("devtools/output-parser");

add_task(function*() {
  yield promiseTab("about:blank");
  yield performTest();
  gBrowser.removeCurrentTab();
});

function* performTest() {
  let [host, , doc] = yield createHost("bottom", "data:text/html," +
    "<h1>browser_outputParser.js</h1><div></div>");

  let parser = new OutputParser();
  testParseCssProperty(doc, parser);
  testParseCssVar(doc, parser);

  host.destroy();
}


let COLOR_TEST_CLASS = "test-class";














function makeColorTest(name, value, segments) {
  let result = {
    name,
    value,
    expected: ""
  };

  for (let segment of segments) {
    if (typeof (segment) === "string") {
      result.expected += segment;
    } else {
      result.expected += "<span data-color=\"" + segment.value + "\">" +
        "<span style=\"background-color:" + segment.name +
        "\" class=\"" + COLOR_TEST_CLASS + "\"></span><span>" +
        segment.value + "</span></span>";
    }
  }

  result.desc = "Testing " + name + ": " + value;

  return result;
}

function testParseCssProperty(doc, parser) {
  let tests = [
    makeColorTest("border", "1px solid red",
                  ["1px solid ", {name: "red", value: "#F00"}]),

    makeColorTest("background-image",
                  "linear-gradient(to right, #F60 10%, rgba(0,0,0,1))",
                  ["linear-gradient(to right, ", {name: "#F60", value: "#F60"},
                   " 10%, ", {name: "rgba(0,0,0,1)", value: "#000"},
                   ")"]),

    
    makeColorTest("font-family", "arial black", ["arial black"]),

    makeColorTest("box-shadow", "0 0 1em red",
                  ["0 0 1em ", {name: "red", value: "#F00"}]),

    makeColorTest("box-shadow",
                  "0 0 1em red, 2px 2px 0 0 rgba(0,0,0,.5)",
                  ["0 0 1em ", {name: "red", value: "#F00"},
                   ", 2px 2px 0 0 ",
                   {name: "rgba(0,0,0,.5)", value: "rgba(0,0,0,.5)"}]),

    makeColorTest("content", "\"red\"", ["\"red\""]),

    
    makeColorTest("hellothere", "'red'", ["'red'"]),

    
    
    
    
    
    
    

  ];

  let target = doc.querySelector("div");
  ok(target, "captain, we have the div");

  for (let test of tests) {
    info(test.desc);

    let frag = parser.parseCssProperty(test.name, test.value, {
      colorSwatchClass: COLOR_TEST_CLASS
    });

    target.appendChild(frag);

    is(target.innerHTML, test.expected,
       "CSS property correctly parsed for " + test.name + ": " + test.value);

    target.innerHTML = "";
  }
}

function testParseCssVar(doc, parser) {
  let frag = parser.parseCssProperty("color", "var(--some-kind-of-green)", {
    colorSwatchClass: "test-colorswatch"
  });

  let target = doc.querySelector("div");
  ok(target, "captain, we have the div");
  target.appendChild(frag);

  is(target.innerHTML, "var(--some-kind-of-green)",
     "CSS property correctly parsed");

  target.innerHTML = "";
}
