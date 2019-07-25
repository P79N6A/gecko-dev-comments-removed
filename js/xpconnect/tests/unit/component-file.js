


































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Ci = Components.interfaces;

function do_check_true(cond, text) {
  
  
  
  if (!cond)
    throw "Failed check: " + text;
}

function FileComponent() {
  this.wrappedJSObject = this;
}
FileComponent.prototype =
{
  doTest: function() {
    

    
    var file = Components.classes["@mozilla.org/file/directory_service;1"]
               .getService(Ci.nsIProperties)
               .get("CurWorkD", Ci.nsIFile);
    file.append("xpcshell.ini");

    
    var f1 = File(file.path);
    
    var f2 = new File(file.path);
    
    var f3 = File(file);
    var f4 = new File(file);
    
    var f5 = File(file.path, "foopy");
    var f6 = new File(file, Date(123123154));

    
    do_check_true(f1 instanceof Ci.nsIDOMFile, "Should be a DOM File");
    do_check_true(f2 instanceof Ci.nsIDOMFile, "Should be a DOM File");
    do_check_true(f3 instanceof Ci.nsIDOMFile, "Should be a DOM File");
    do_check_true(f4 instanceof Ci.nsIDOMFile, "Should be a DOM File");
    do_check_true(f5 instanceof Ci.nsIDOMFile, "Should be a DOM File");
    do_check_true(f6 instanceof Ci.nsIDOMFile, "Should be a DOM File");

    do_check_true(f1.name == "xpcshell.ini", "Should be the right file");
    do_check_true(f2.name == "xpcshell.ini", "Should be the right file");
    do_check_true(f3.name == "xpcshell.ini", "Should be the right file");
    do_check_true(f4.name == "xpcshell.ini", "Should be the right file");
    do_check_true(f5.name == "xpcshell.ini", "Should be the right file");
    do_check_true(f6.name == "xpcshell.ini", "Should be the right file");

    do_check_true(f1.type = "text/plain", "Should be the right type");
    do_check_true(f2.type = "text/plain", "Should be the right type");
    do_check_true(f3.type = "text/plain", "Should be the right type");
    do_check_true(f4.type = "text/plain", "Should be the right type");
    do_check_true(f5.type = "text/plain", "Should be the right type");
    do_check_true(f6.type = "text/plain", "Should be the right type");

    var threw = false;
    try {
      
      var f7 = File();
    } catch (e) {
      threw = true;
    }
    do_check_true(threw, "No ctor arguments should throw");

    var threw = false;
    try {
      
      var f7 = File(Date(132131532));
    } catch (e) {
      threw = true;
    }
    do_check_true(threw, "Passing a random object should fail");

    var threw = false
    try {
      
      var dir = Components.classes["@mozilla.org/file/directory_service;1"]
                          .getService(Ci.nsIProperties)
                          .get("CurWorkD", Ci.nsIFile);
      var f7 = File(dir)
    } catch (e) {
      threw = true;
    }
    do_check_true(threw, "Can't create a File object for a directory");

    return true;
  },

  
  classDescription: "File in components scope code",
  classID: Components.ID("{da332370-91d4-464f-a730-018e14769cab}"),
  contractID: "@mozilla.org/tests/component-file;1",

  
  implementationLanguage: Components.interfaces.nsIProgrammingLanguage.JAVASCRIPT,
  flags: 0,

  getInterfaces: function getInterfaces(aCount) {
    var interfaces = [Components.interfaces.nsIClassInfo];
    aCount.value = interfaces.length;
    return interfaces;
  },

  getHelperForLanguage: function getHelperForLanguage(aLanguage) {
    return null;
  },

  
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces.nsIClassInfo])
};

var gComponentsArray = [FileComponent];
var NSGetFactory = XPCOMUtils.generateNSGetFactory(gComponentsArray);
