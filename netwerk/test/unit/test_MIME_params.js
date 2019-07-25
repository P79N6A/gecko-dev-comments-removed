







const Cr = Components.results;






var tests = [
  
  ["attachment;", 
    Cr.NS_ERROR_INVALID_ARG, Cr.NS_ERROR_INVALID_ARG],

  
  ["attachment; filename=basic",
   "basic", "basic"],

  
  ["attachment; filename*=UTF-8''extended",
   "extended", "extended"],

  
  ["attachment; filename=basic; filename*=UTF-8''extended",
   "extended", "extended"],

  
  ["attachment; filename*=UTF-8''extended; filename=basic",
   "extended", "extended"],

  
  ["attachment; filename=first; filename=wrong",
   "first", "first"],

  
  ["filename=old",
   "old", "old"],

  ["attachment; filename*=UTF-8''extended",
   "extended", "extended"],

  ["attachment; filename*0=foo; filename*1=bar",
   "foobar", Cr.NS_ERROR_INVALID_ARG],

  
  ["attachment; filename*0=first; filename*0=wrong; filename=basic",
   "first", "basic"],

  
  ["attachment; filename*0=first; filename*1=second; filename*0=wrong",
   "firstsecond", Cr.NS_ERROR_INVALID_ARG],

  
  ["attachment; filename=basic; filename*0=foo; filename*1=bar",
   "foobar", "basic"],

  
  ["attachment; filename=basic; filename*0=first; filename*0=wrong; filename*=UTF-8''extended",
   "extended", "extended"],

  
  ["attachment; filename=basic; filename*=UTF-8''extended; filename*0=foo; filename*1=bar",
   "extended", "extended"],

  
  ["attachment; filename*0=foo; filename*2=bar",
   "foo", Cr.NS_ERROR_INVALID_ARG],

  
  ["attachment; filename*0=foo; filename*01=bar",
   "foo", Cr.NS_ERROR_INVALID_ARG],

  
  ["attachment; filename=basic; filename*0*=UTF-8''multi\r\n"
    + " filename*1=line\r\n" 
    + " filename*2*=%20extended",
   "multiline extended", "basic"],

  
  ["attachment; filename=basic; filename*0*=UTF-8''multi\r\n"
    + " filename*1=line\r\n" 
    + " filename*3*=%20extended",
   "multiline", "basic"],

  
  
  ["attachment; filename=basic; filename*0*=UTF-8''multi\r\n"
    + " filename*1=line\r\n" 
    + " filename*0*=UTF-8''wrong\r\n"
    + " filename*1=bad\r\n"
    + " filename*2=evil",
   "multiline", "basic"],

  
  ["attachment; filename=basic; filename*0=UTF-8''multi\r\n"
    + " filename*=UTF-8''extended\r\n"
    + " filename*1=line\r\n" 
    + " filename*2*=%20extended",
   "extended", "extended"],

  
  ["attachment; filename*0=UTF-8''unescaped\r\n"
    + " filename*1*=%20so%20includes%20UTF-8''%20in%20value", 
   "UTF-8''unescaped so includes UTF-8'' in value", Cr.NS_ERROR_INVALID_ARG],

  
  ["attachment; filename=basic; filename*0=UTF-8''unescaped\r\n"
    + " filename*1*=%20so%20includes%20UTF-8''%20in%20value", 
   "UTF-8''unescaped so includes UTF-8'' in value", "basic"],

  
  ["attachment; filename=basic; filename*1=multi\r\n"
    + " filename*2=line\r\n" 
    + " filename*3*=%20extended",
   "basic", "basic"],

  
  ["attachment; filename=basic; filename*0*=UTF-8''0\r\n"
    + " filename*1=1; filename*2=2;filename*3=3;filename*4=4;filename*5=5\r\n" 
    + " filename*6=6; filename*7=7;filename*8=8;filename*9=9;filename*10=a\r\n"
    + " filename*11=b; filename*12=c;filename*13=d;filename*14=e;filename*15=f\r\n",
   "0123456789abcdef", "basic"],

  
  ["attachment; filename=basic; filename*0*=UTF-8''0\r\n"
    + " filename*1=1; filename*2=2;filename*3=3;filename*4=4;filename*5=5\r\n" 
    + " filename*6=6; filename*7=7;filename*8=8;filename*9=9;filename*10=a\r\n"
    + " filename*11=b; filename*12=c;filename*13=d;filename*15=f;filename*14=e\r\n",
   "0123456789abcd" , "basic"],

  
  ["attachment; filename=basic; filename*0*=UTF-8''0\r\n"
    + " filename*1=1; filename*2=2;filename*3=3;filename*4=4;filename*5=5\r\n" 
    + " filename*6=6; filename*7=7;filename*8=8;filename*9=9;filename*10=a\r\n"
    + " filename*11=b; filename*12=c;filename*14=e\r\n",
   "0123456789abc", "basic"],

  
  ["attachment; filename*1=multi\r\n"
    + " filename*2=line\r\n" 
    + " filename*3*=%20extended",
   Cr.NS_ERROR_INVALID_ARG, Cr.NS_ERROR_INVALID_ARG],

];

function do_tests(whichRFC)
{
  var mhp = Components.classes["@mozilla.org/network/mime-hdrparam;1"]
                      .getService(Components.interfaces.nsIMIMEHeaderParam);

  var unused = { value : null };

  for (var i = 0; i < tests.length; ++i) {
    dump("Testing " + tests[i] + "\n");
    try {
      var result;
      if (whichRFC == 1)
        result = mhp.getParameter(tests[i][0], "filename", "UTF-8", true, unused);
      else 
        result = mhp.getParameter5987(tests[i][0], "filename", "UTF-8", true, unused);
      do_check_eq(result, tests[i][whichRFC]);
    } 
    catch (e) {
      
      if (e.result) {
        
        try { 
          do_check_eq(e.result, tests[i][whichRFC]); 
        } catch(e) {}  
      }
      continue;
    }
  }
}

function run_test() {

  
  do_tests(1);

  
  do_tests(2);
}

