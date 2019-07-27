



Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.importGlobalProperties(['Blob']);

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

function do_check_true(cond, text) {
  
  
  
  if (!cond)
    throw "Failed check: " + text;
}

function BlobComponent() {
  this.wrappedJSObject = this;
}
BlobComponent.prototype =
{
  doTest: function() {
    
    let testContent = "<a id=\"a\"><b id=\"b\">hey!<\/b><\/a>";
    
    var f1 = new Blob([testContent], {"type" : "text/xml"});

    
    do_check_true(f1 instanceof Ci.nsIDOMBlob, "Should be a DOM Blob");

    do_check_true(!(f1 instanceof Ci.nsIDOMFile), "Should not be a DOM File");

    do_check_true(f1.type == "text/xml", "Wrong type");

    do_check_true(f1.size == testContent.length, "Wrong content size");

    var f2 = new Blob();
    do_check_true(f2.size == 0, "Wrong size");
    do_check_true(f2.type == "", "Wrong type");

    var threw = false;
    try {
      
      var f2 = new Blob(Date(132131532));
    } catch (e) {
      threw = true;
    }
    do_check_true(threw, "Passing a random object should fail");

    return true;
  },

  
  classDescription: "Blob in components scope code",
  classID: Components.ID("{06215993-a3c2-41e3-bdfd-0a3a2cc0b65c}"),
  contractID: "@mozilla.org/tests/component-blob;1",

  
  flags: 0,

  getInterfaces: function getInterfaces(aCount) {
    var interfaces = [Components.interfaces.nsIClassInfo];
    aCount.value = interfaces.length;
    return interfaces;
  },

  getScriptableHelper: function getScriptableHelper() {
    return null;
  },

  
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces.nsIClassInfo])
};

var gComponentsArray = [BlobComponent];
this.NSGetFactory = XPCOMUtils.generateNSGetFactory(gComponentsArray);
