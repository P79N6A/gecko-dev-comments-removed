




































 
function run_test() {
  var file = do_get_file("syntax_error.jsm");
  var ios = Components.classes["@mozilla.org/network/io-service;1"]
                      .getService(Components.interfaces.nsIIOService);
  var uri = ios.newFileURI(file);

  try {
    Components.utils.import(uri.spec);
    do_throw("Failed to report any error at all");
  } catch (e) {
    do_check_neq(/^SyntaxError:/(e + ''), null);
  }
}
