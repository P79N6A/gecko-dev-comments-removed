



































EXPORTED_SYMBOLS = ["baz", "qux"]

function baz() {
  return "baz";
}

var qux = {}
Components.utils.import("resource://test/recursive_importA.jsm", qux);

