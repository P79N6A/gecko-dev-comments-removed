function run_test() {
  var Ci = Components.interfaces;
  var Cc = Components.classes;

  
  load("results.js");   

  var ParserUtils =  Cc["@mozilla.org/parserutils;1"].getService(Ci.nsIParserUtils);
  var sanitizeFlags = ParserUtils.SanitizerCidEmbedsOnly|ParserUtils.SanitizerDropForms|ParserUtils.SanitizerDropNonCSSPresentation;
  
  
  


  for (var item in vectors) {
    var evil = vectors[item].data;
    var sanitized = vectors[item].sanitized;
    var out = ParserUtils.sanitize(evil, sanitizeFlags);
    do_check_eq(sanitized, out);
  }
}
