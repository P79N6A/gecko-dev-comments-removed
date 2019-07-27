



"use strict"

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/osfile.jsm")

let XULStore = null;
let browserURI = "chrome://browser/content/browser.xul";
let aboutURI = "about:config";

function run_test() {
  do_get_profile();
  run_next_test();
}

function checkValue(uri, id, attr, reference) {
  let value = XULStore.getValue(uri, id, attr);
  do_check_true(value === reference);
}

function checkValueExists(uri, id, attr, exists) {
  do_check_eq(XULStore.hasValue(uri, id, attr), exists);
}

function getIDs(uri) {
  let it = XULStore.getIDsEnumerator(uri);
  let result = [];

  while (it.hasMore()) {
    let value = it.getNext();
    result.push(value);
  }

  result.sort();
  return result;
}

function getAttributes(uri, id) {
  let it = XULStore.getAttributeEnumerator(uri, id);

  let result = [];

  while (it.hasMore()) {
    let value = it.getNext();
    result.push(value);
  }

  result.sort();
  return result;
}

function checkArrays(a, b) {
  a.sort();
  b.sort();
  do_check_true(a.toString() == b.toString());
}

function checkOldStore() {
  checkArrays(['addon-bar', 'main-window', 'sidebar-title'], getIDs(browserURI));
  checkArrays(['collapsed'], getAttributes(browserURI, 'addon-bar'));
  checkArrays(['height', 'screenX', 'screenY', 'sizemode', 'width'],
              getAttributes(browserURI, 'main-window'));
  checkArrays(['value'], getAttributes(browserURI, 'sidebar-title'));

  checkValue(browserURI, "addon-bar", "collapsed", "true");
  checkValue(browserURI, "main-window", "width", "994");
  checkValue(browserURI, "main-window", "height", "768");
  checkValue(browserURI, "main-window", "screenX", "4");
  checkValue(browserURI, "main-window", "screenY", "22");
  checkValue(browserURI, "main-window", "sizemode", "normal");
  checkValue(browserURI, "sidebar-title", "value", "");

  checkArrays(['lockCol', 'prefCol'], getIDs(aboutURI));
  checkArrays(['ordinal'], getAttributes(aboutURI, 'lockCol'));
  checkArrays(['ordinal', 'sortDirection'], getAttributes(aboutURI, 'prefCol'));

  checkValue(aboutURI, "prefCol", "ordinal", "1");
  checkValue(aboutURI, "prefCol", "sortDirection", "ascending");
  checkValue(aboutURI, "lockCol", "ordinal", "3");
}

add_task(function* testImport(){
  let src = "localstore.rdf";
  let dst = OS.Path.join(OS.Constants.Path.profileDir, src);

  yield OS.File.copy(src, dst);

  
  XULStore = Cc["@mozilla.org/xul/xulstore;1"].getService(Ci.nsIXULStore);
  checkOldStore();
});

add_task(function* testTruncation() {
  let dos = Array(8192).join("~");
  
  Assert.throws(() => XULStore.setValue(browserURI, dos, "foo", "foo"), /NS_ERROR_ILLEGAL_VALUE/);

  
  Assert.throws(() => XULStore.setValue(browserURI, "foo", dos, "foo"), /NS_ERROR_ILLEGAL_VALUE/);

  
  XULStore.setValue(browserURI, "dos", "dos", dos);
  dos =XULStore.getValue(browserURI, "dos", "dos");
  do_check_true(dos.length == 4096)
  XULStore.removeValue(browserURI, "dos", "dos")
});

add_task(function* testGetValue() {
  
  checkValue(browserURI, "side-window", "height", "");

  
  checkValue(browserURI, "main-window", "width", "994");
});

add_task(function* testHasValue() {
  
  checkValueExists(browserURI, "side-window", "height", false);

  
  checkValueExists(browserURI, "main-window", "width", true);
});

add_task(function* testSetValue() {
  
  checkValue(browserURI, "side-bar", "width", "");
  XULStore.setValue(browserURI, "side-bar", "width", "1000");
  checkValue(browserURI, "side-bar", "width", "1000");
  checkArrays(["addon-bar", "main-window", "side-bar", "sidebar-title"], getIDs(browserURI));
  checkArrays(["width"], getAttributes(browserURI, 'side-bar'));

  
  checkValue(browserURI, "side-bar", "width", "1000");
  XULStore.setValue(browserURI, "side-bar", "width", "1024");
  checkValue(browserURI, "side-bar", "width", "1024");
  checkArrays(["addon-bar", "main-window", "side-bar", "sidebar-title"], getIDs(browserURI));
  checkArrays(["width"], getAttributes(browserURI, 'side-bar'));

  
  checkValue(browserURI, "side-bar", "height", "");
  XULStore.setValue(browserURI, "side-bar", "height", "1000");
  checkValue(browserURI, "side-bar", "height", "1000");
  checkArrays(["addon-bar", "main-window", "side-bar", "sidebar-title"], getIDs(browserURI));
  checkArrays(["width", "height"], getAttributes(browserURI, 'side-bar'));
});

add_task(function* testRemoveValue() {
  
  checkValue(browserURI, "side-bar", "width", "1024");
  XULStore.removeValue(browserURI, "side-bar", "width");
  checkValue(browserURI, "side-bar", "width", "");
  checkValueExists(browserURI, "side-bar", "width", false);
  checkArrays(["addon-bar", "main-window", "side-bar", "sidebar-title"], getIDs(browserURI));
  checkArrays(["height"], getAttributes(browserURI, 'side-bar'));

  
  checkValue(browserURI, "side-bar", "height", "1000");
  XULStore.removeValue(browserURI, "side-bar", "height");
  checkValue(browserURI, "side-bar", "height", "");
  checkArrays(["addon-bar", "main-window", "sidebar-title"], getIDs(browserURI));

  
  XULStore.removeValue(browserURI, "main-window", "bar");

  
  XULStore.removeValue(browserURI, "foo", "bar");

  
  let nonDocURI = "chrome://example/content/other.xul";
  XULStore.removeValue(nonDocURI, "foo", "bar");

  
  XULStore.removeValue(browserURI, "addon-bar", "collapsed");
  checkArrays([], getAttributes(browserURI, "addon-bar"));
  XULStore.removeValue(browserURI, "main-window", "width");
  XULStore.removeValue(browserURI, "main-window", "height");
  XULStore.removeValue(browserURI, "main-window", "screenX");
  XULStore.removeValue(browserURI, "main-window", "screenY");
  XULStore.removeValue(browserURI, "main-window", "sizemode");
  checkArrays([], getAttributes(browserURI, "main-window"));
  XULStore.removeValue(browserURI, "sidebar-title", "value");
  checkArrays([], getAttributes(browserURI, "sidebar-title"));
  checkArrays([], getIDs(browserURI));

  
  XULStore.removeValue(aboutURI, "prefCol", "ordinal");
  XULStore.removeValue(aboutURI, "prefCol", "sortDirection");
  checkArrays([], getAttributes(aboutURI, "prefCol"));
  XULStore.removeValue(aboutURI, "lockCol", "ordinal");
  checkArrays([], getAttributes(aboutURI, "lockCol"));
  checkArrays([], getIDs(aboutURI));
});
