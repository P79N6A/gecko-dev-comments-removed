


"use strict";











let TEST_URL = "data:text/html,<div>markup-view attributes addition test</div>";
let TEST_DATA = [{
  desc: "Add an attribute value without closing \"",
  text: 'style="display: block;',
  expectedAttributes: {
    style: "display: block;"
  }
}, {
  desc: "Add an attribute value without closing '",
  text: "style='display: inline;",
  expectedAttributes: {
    style: "display: inline;"
  }
}, {
  desc: "Add an attribute wrapped with with double quotes double quote in it",
  text: 'style="display: "inline',
  expectedAttributes: {
    style: "display: ",
    inline: ""
  }
}, {
  desc: "Add an attribute wrapped with single quotes with single quote in it",
  text: "style='display: 'inline",
  expectedAttributes: {
    style: "display: ",
    inline: ""
  }
}, {
  desc: "Add an attribute with no value",
  text: "disabled",
  expectedAttributes: {
    disabled: ""
  }
}, {
  desc: "Add multiple attributes with no value",
  text: "disabled autofocus",
  expectedAttributes: {
    disabled: "",
    autofocus: ""
  }
}, {
  desc: "Add multiple attributes with no value, and some with value",
  text: "disabled name='name' data-test='test' autofocus",
  expectedAttributes: {
    disabled: "",
    autofocus: "",
    name: "name",
    'data-test': "test"
  }
}, {
  desc: "Add attribute with xmlns",
  text: "xmlns:edi='http://ecommerce.example.org/schema'",
  expectedAttributes: {
    'xmlns:edi': "http://ecommerce.example.org/schema"
  }
}];

let test = asyncTest(function*() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);
  yield runAddAttributesTests(TEST_DATA, "div", inspector)
});
