








var succeed = [
  ["Content-Disposition: attachment; filename=basic; filename*=UTF-8''extended",
  "extended"],
  ["Content-Disposition: attachment; filename*=UTF-8''extended; filename=basic",
  "extended"],
  ["Content-Disposition: attachment; filename=basic",
  "basic"],
  ["Content-Disposition: attachment; filename*=UTF-8''extended",
  "extended"],
  ["Content-Disposition: attachment; filename*0=foo; filename*1=bar",
  "foobar"],




  
  ["Content-Disposition: attachment; filename=basic; filename*=UTF-8''extended; filename*0=foo; filename*1=bar",
  "extended"],










  ["Content-Disposition: attachment; filename=basic; filename*0*=UTF-8''multi\r\n"
    + " filename*1=line\r\n" 
    + " filename*2*=%20extended",
  "multiline extended"],







  
  ["Content-Disposition: attachment; filename=basic; filename*0=UTF-8''multi\r\n"
    + " filename*=UTF-8''extended\r\n"
    + " filename*1=line\r\n" 
    + " filename*2*=%20extended",
  "extended"],
  
  ["Content-Disposition: attachment; filename*0=UTF-8''unescaped\r\n"
    + " filename*1*=%20so%20includes%20UTF-8''%20in%20value", 
  "UTF-8''unescaped so includes UTF-8'' in value"],













];

var broken = [
  ["Content-Disposition: attachment; filename*1=multi\r\n"
    + " filename*2=line\r\n" 
    + " filename*3*=%20extended",
  "param continuation must start from 0: should fail"],
];


function run_test() {

  var mhp = Components.classes["@mozilla.org/network/mime-hdrparam;1"]
                      .getService(Components.interfaces.nsIMIMEHeaderParam);

  var unused = { value : null };

  for (var i = 0; i < succeed.length; ++i) {
    dump("Testing " + succeed[i] + "\n");
    try {
      do_check_eq(mhp.getParameter(succeed[i][0], "filename", "UTF-8", true, unused),
                  succeed[i][1]);
    } catch (e) {}
  }

  
  for (var i = 0; i < broken.length; ++i) {
    dump("Testing " + broken[i] + "\n");
    try {
      var result = mhp.getParameter(broken[i][0], "filename", "UTF-8", true, unused);
      
      do_check_eq(broken[i][1], "instead got: " + result);
    } catch (e) {
      
      if (e.result)
        do_check_eq(e.result, Components.results.NS_ERROR_OUT_OF_MEMORY);
    }
  }
}

