



function run_test() {
  let svc =  Components.classes["@mozilla.org/charset-converter-manager;1"]
                       .getService(Components.interfaces.nsICharsetConverterManager);

  
  do_check_eq(svc.getCharsetAlias("Windows-1255"), "windows-1255");

  try {
    svc.getCharsetAlias("no such thing");
    do_throw("Calling getCharsetAlias with invalid value should throw.");
  }
  catch (ex) {
    
  }

  do_check_eq(svc.getCharsetTitle("windows-1255"), "Hebrew (Windows-1255)");
  try {
    svc.getCharsetTitle("no such thing");
    do_throw("Calling getCharsetTitle with invalid value should throw.");
  }
  catch (ex) {
    
  }
}
