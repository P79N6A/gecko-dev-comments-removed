


"use strict";




const TEST_CASES = [
  {
    input: '(new DOMParser()).parseFromString("<a />", "text/html")',
    output: "HTMLDocument",
    inspectable: true,
  },
  {
    input: '(new DOMParser()).parseFromString("<a />", "application/xml")',
    output: "XMLDocument",
    inspectable: true,
  },
  {
    input: '(new DOMParser()).parseFromString("<svg></svg>", "image/svg+xml")',
    output: "SVGDocument",
    inspectable: true,
  },
];

const TEST_URI = "data:text/html;charset=utf8," +
  "browser_webconsole_inspect-parsed-documents.js";
let test = asyncTest(function* () {
    let {tab} = yield loadTab(TEST_URI);
    let hud = yield openConsole(tab);
    yield checkOutputForInputs(hud, TEST_CASES);
});
