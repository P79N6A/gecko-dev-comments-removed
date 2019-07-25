



var BS = '\\';
var DQUOTE = '"'; 
 
var reference = [
                 [ 
                  "Content-Disposition: attachment; foobar=" + DQUOTE + (BS + DQUOTE) + DQUOTE, 
                  DQUOTE],
                 [ 
                  "Content-Disposition: attachment; foobar=" + DQUOTE + 'a' + (BS + DQUOTE) + 'b' + DQUOTE, 
                  'a' + DQUOTE + 'b'],
                 [ 
                  "Content-Disposition: attachment; foobar=" + DQUOTE + (BS + "x") + DQUOTE, 
                  "x"],
                ];

function run_test() {

  var mhp = Components.classes["@mozilla.org/network/mime-hdrparam;1"]
                      .getService(Components.interfaces.nsIMIMEHeaderParam);

  var unused = { value : null };

  for (var i = 0; i < reference.length; ++i) {
    dump("Testing " + reference[i] + "\n");
    do_check_eq(mhp.getParameter(reference[i][0], "foobar", "UTF-8", true, unused),
                reference[i][1]);
  }
}

