



































EXPORTED_SYMBOLS = ["foo", "bar"]

function foo() {
  return "foo";
}

var bar = {}
Components.utils.import("resource://test/recursive_importB.jsm", bar);
