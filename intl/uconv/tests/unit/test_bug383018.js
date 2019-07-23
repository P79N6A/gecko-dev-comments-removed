


function run_test() {
  var ccm = Components.classes["@mozilla.org/charset-converter-manager;1"];
  var ccms = ccm.getService(Components.interfaces.nsICharsetConverterManager);

  var alias = ccms.getCharsetAlias("ascii");
  
  do_check_eq(alias, "us-ascii");
}
