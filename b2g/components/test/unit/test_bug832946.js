


function run_test() {
  Components.utils.import("resource:///modules/TelURIParser.jsm")

  
  do_check_eq(TelURIParser.parseURI('tel', 'tel:#1234*'), null);
  do_check_eq(TelURIParser.parseURI('tel', 'tel:*1234#'), null);
  do_check_eq(TelURIParser.parseURI('tel', 'tel:*1234*'), null);
  do_check_eq(TelURIParser.parseURI('tel', 'tel:#1234#'), null);
  do_check_eq(TelURIParser.parseURI('tel', 'tel:*#*#7780#*#*'), null);
  do_check_eq(TelURIParser.parseURI('tel', 'tel:*1234AB'), null);

  
  do_check_eq(TelURIParser.parseURI('tel', 'tel:*1234'), '*1234');
  do_check_eq(TelURIParser.parseURI('tel', 'tel:#1234'), '#1234');
}
