


function run_test() {
  Components.utils.import("resource:///modules/TelURIParser.jsm")

  
   do_check_eq(TelURIParser.parseURI('tel', 'tel:+1234'), '+1234');

  
  do_check_eq(TelURIParser.parseURI('tel', 'tel:+1234_123'), '+1234');

  
  do_check_eq(TelURIParser.parseURI('tel', 'tel:+-.()1234567890'), '+-.()1234567890');

  
  do_check_eq(TelURIParser.parseURI('tel', 'tel:1234'), '1234');

  
  do_check_eq(TelURIParser.parseURI('tel', 'tel:-.()1234567890ABCDpw'), '-.()1234567890ABCDpw');

  
  do_check_eq(TelURIParser.parseURI('tel', 'tel:-.()1234567890ABCDpw_'), '-.()1234567890ABCDpw');

  
  do_check_eq(TelURIParser.parseURI('tel', 'tel:123;isub=123'), '123');

  
  do_check_eq(TelURIParser.parseURI('tel', 'tel:123;postd=123'),  '123');

  
  do_check_eq(TelURIParser.parseURI('tel', 'tel:123;phone-context=+0321'), '+0321123');

  
  do_check_eq(TelURIParser.parseURI('tel', 'tel:123;isub=123;postd=123;phone-context=+0321'), '+0321123');
}
