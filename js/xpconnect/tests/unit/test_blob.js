


 
function run_test() {   
  do_load_manifest("component-blob.manifest");
  const contractID = "@mozilla.org/tests/component-blob;1";
  do_check_true(contractID in Components.classes);
  var foo = Components.classes[contractID]
                      .createInstance(Components.interfaces.nsIClassInfo);
  do_check_true(Boolean(foo));
  do_check_true(foo.contractID == contractID);
  do_check_true(!!foo.wrappedJSObject);
  do_check_true(foo.wrappedJSObject.doTest());

}
