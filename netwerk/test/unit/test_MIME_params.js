





const Cr = Components.results;

var BS = '\\';
var DQUOTE = '"'; 














var tests = [
  
  ["attachment;",
   "attachment", Cr.NS_ERROR_INVALID_ARG],

  
  ["attachment; filename=basic",
   "attachment", "basic"],

  
  ["attachment; filename*=UTF-8''extended",
   "attachment", "extended"],

  
  ["attachment; filename=basic; filename*=UTF-8''extended",
   "attachment", "extended"],

  
  ["attachment; filename*=UTF-8''extended; filename=basic",
   "attachment", "extended"],

  
  ["attachment; filename=first; filename=wrong",
   "attachment", "first"],

  
  
  ["filename=old",
   "filename=old", "old"],

  ["attachment; filename*=UTF-8''extended",
   "attachment", "extended"],

  
  ["attachment; filename*0=foo; filename*1=bar",
   "attachment", "foobar",
   ],

  
  ["attachment; filename*0=first; filename*0=wrong; filename=basic",
   "attachment", "first",
   ],

  
  ["attachment; filename*0=first; filename*1=second; filename*0=wrong",
   "attachment", "firstsecond",
   ],

  
  ["attachment; filename=basic; filename*0=foo; filename*1=bar",
   "attachment", "foobar",
   ],

  
  
  ["attachment; filename=basic; filename*0=first; filename*0=wrong; filename*=UTF-8''extended",
   "attachment", "extended"],

  
  
  ["attachment; filename=basic; filename*=UTF-8''extended; filename*0=foo; filename*1=bar",
   "attachment", "extended"],

  
  
  ["attachment; filename*0=foo; filename*2=bar",
   "attachment", "foo",
   ],

  
  ["attachment; filename*0=foo; filename*01=bar",
   "attachment", "foo",
   ],

  
  ["attachment; filename=basic; filename*0*=UTF-8''multi;\r\n"
    + " filename*1=line;\r\n" 
    + " filename*2*=%20extended",
   "attachment", "multiline extended",
   ],

  
  
  ["attachment; filename=basic; filename*0*=UTF-8''multi;\r\n"
    + " filename*1=line;\r\n" 
    + " filename*3*=%20extended",
   "attachment", "multiline",
   ],

  
  
  ["attachment; filename=basic; filename*0*=UTF-8''multi;\r\n"
    + " filename*1=line;\r\n" 
    + " filename*0*=UTF-8''wrong;\r\n"
    + " filename*1=bad;\r\n"
    + " filename*2=evil",
   "attachment", "multiline",
   ],

  
  
  ["attachment; filename=basic; filename*0=UTF-8''multi\r\n;"
    + " filename*=UTF-8''extended;\r\n"
    + " filename*1=line;\r\n" 
    + " filename*2*=%20extended",
   "attachment", "extended"],

  
  ["attachment; filename*0=UTF-8''unescaped;\r\n"
    + " filename*1*=%20so%20includes%20UTF-8''%20in%20value", 
   "attachment", "UTF-8''unescaped so includes UTF-8'' in value",
   ],

  
  ["attachment; filename=basic; filename*0=UTF-8''unescaped;\r\n"
    + " filename*1*=%20so%20includes%20UTF-8''%20in%20value", 
   "attachment", "UTF-8''unescaped so includes UTF-8'' in value",
   ],

  
  
  ["attachment; filename=basic; filename*1=multi;\r\n"
    + " filename*2=line;\r\n" 
    + " filename*3*=%20extended",
   "attachment", "basic"],

  
  ["attachment; filename=basic; filename*0*=UTF-8''0;\r\n"
    + " filename*1=1; filename*2=2;filename*3=3;filename*4=4;filename*5=5;\r\n" 
    + " filename*6=6; filename*7=7;filename*8=8;filename*9=9;filename*10=a;\r\n"
    + " filename*11=b; filename*12=c;filename*13=d;filename*14=e;filename*15=f\r\n",
   "attachment", "0123456789abcdef",
   ],

  
  ["attachment; filename=basic; filename*0*=UTF-8''0;\r\n"
    + " filename*1=1; filename*2=2;filename*3=3;filename*4=4;filename*5=5;\r\n" 
    + " filename*6=6; filename*7=7;filename*8=8;filename*9=9;filename*10=a;\r\n"
    + " filename*11=b; filename*12=c;filename*14=e\r\n",
   "attachment", "0123456789abc",
   ],

  
  
  ["attachment; filename*1=multi;\r\n"
    + " filename*2=line;\r\n" 
    + " filename*3*=%20extended",
   "attachment", Cr.NS_ERROR_INVALID_ARG],
   
  

  
  ["attachment; filename=foo.html", 
   "attachment", "foo.html",
   "attachment", "foo.html"],

  
  ["; filename=foo.html", 
   Cr.NS_ERROR_FIRST_HEADER_FIELD_COMPONENT_EMPTY, "foo.html",
   Cr.NS_ERROR_FIRST_HEADER_FIELD_COMPONENT_EMPTY, "foo.html"],
  
  
  ["filename=foo.html", 
   "filename=foo.html", "foo.html",
   "filename=foo.html", "foo.html"],
   
  

  
  ["attachment; filename=basic; filename*0*=UTF-8''0;\r\n"
    + " filename*1=1; filename*2=2;filename*3=3;filename*4=4;filename*5=5;\r\n" 
    + " filename*6=6; filename*7=7;filename*8=8;filename*9=9;filename*10=a;\r\n"
    + " filename*11=b; filename*12=c;filename*13=d;filename*15=f;filename*14=e;\r\n",
   "attachment", "0123456789abcdef",
   ],

  
  ["attachment; filename=basic; filename*0*=UTF-8''0;\r\n"
    + " filename*1a=1\r\n",
   "attachment", "0",
   ],

  
  ["attachment; filename=basic; filename*0*=UTF-8''0;\r\n"
    + " filename*0=bad; filename*1=1;\r\n",
   "attachment", "0",
   ],

  
  ["attachment; filename=basic; filename*0*=UTF-8''0;\r\n"
    + " filename*11111111111111111111111111111111111111111111111111111111111=1",
   "attachment", "0",
   ],

  
  ["attachment; filename=basic; filename*0*=UTF-8''0;\r\n"
    + " filename*-1=1",
   "attachment", "0",
   ],

  
  ["attachment; filename=basic; filename*0=\"0\";\r\n"
    + " filename*1=1;\r\n"
    + " filename*2*=%32",
   "attachment", "012",
   ],

  
  ["attachment; filename=basic; filename**=UTF-8''0\r\n",
   "attachment", "basic",
   "attachment", "basic"],


  
  

  ["attachment;filename=IT839\x04\xB5(m8)2.pdf;",
   "attachment", "IT839\u0004\u00b5(m8)2.pdf"],

  
   
  
  ["attachment; filename=" + DQUOTE + (BS + DQUOTE) + DQUOTE, 
   "attachment", DQUOTE],
  
  
  ["attachment; filename=" + DQUOTE + 'a' + (BS + DQUOTE) + 'b' + DQUOTE, 
   "attachment", "a" + DQUOTE + "b"],
  
  
  ["attachment; filename=" + DQUOTE + (BS + "x") + DQUOTE, 
   "attachment", "x"],
   
  
  ["attachment; filename=" + DQUOTE + DQUOTE, 
   "attachment", ""],
  
  
  ["attachment; filename=", 
   "attachment", ""],    

  
  ["attachment; filename==?ISO-8859-1?Q?foo-=E4.html?=",
   "attachment", "foo-\u00e4.html",
   "attachment", "=?ISO-8859-1?Q?foo-=E4.html?="],

  ["attachment; filename=\"=?ISO-8859-1?Q?foo-=E4.html?=\"",
   "attachment", "foo-\u00e4.html",
   "attachment", "=?ISO-8859-1?Q?foo-=E4.html?="],

  
  ["attachment; filename=\"=?ISO-8859-1?Q?foo-=E4.html?=\"; filename*=UTF-8''5987",
   "attachment", "5987"],

  
  
  
  
  
  
  ["attachment; filename*=utf-8''%41", 
   "attachment", "A"],

  
  ["attachment; filename*=" + DQUOTE + "utf-8''%41" + DQUOTE, 
   "attachment", "A"],
  
  

  
  
  
  
  ["attachment; filename*=UTF-8''foo-%41.html", 
   "attachment", "foo-A.html"],

  
  ["attachment; filename *=UTF-8''foo-%41.html", 
   "attachment", Cr.NS_ERROR_INVALID_ARG],
   
  
  ["attachment; filename X", 
   "attachment", Cr.NS_ERROR_INVALID_ARG],

  
  ["attachment; filename = foo-A.html", 
   "attachment", "foo-A.html"],   

  
  

  
  ["attachment; filename*=''foo", 
   "attachment", "foo"],      
  
  

  
  ["attachment; filename*=a''foo", 
   "attachment", "foo"],      

  
  

  
  ["attachment; filename*=UTF-8'foo-%41.html", 
   "attachment", "foo-A.html"],
  
  

  
  ["attachment; filename*=foo-%41.html", 
   "attachment","foo-A.html"],
  
  

  
  ["attachment; filename*=UTF-8'foo-%41.html; filename=bar.html", 
   "attachment", "foo-A.html"],
  
  

  
  

  
  ["attachment; filename*=ISO-8859-1''%c3%a4", 
   "attachment", "\u00c3\u00a4"],

  
  
  ["attachment; filename*=ISO-8859-1''%e2%82%ac", 
   "attachment", "\u00e2\u201a\u00ac"],

  
  ["attachment; filename*=UTF-8''A%e4B", 
   "attachment", Cr.NS_ERROR_INVALID_ARG],

  
  ["attachment; filename*=UTF-8''A%e4B; filename=fallback", 
   "attachment", "fallback"],

  
  ["attachment; filename*0*=UTF-8''A%e4B; filename=fallback", 
   "attachment", "fallback"],

  
  ["attachment; filename*0*=ISO-8859-15''euro-sign%3d%a4; filename*=ISO-8859-1''currency-sign%3d%a4", 
   "attachment", "currency-sign=\u00a4"],

  
  ["attachment; filename*=ISO-8859-1''currency-sign%3d%a4; filename*0*=ISO-8859-15''euro-sign%3d%a4", 
   "attachment", "currency-sign=\u00a4"],

  
  

  ["attachment; filename*=\"a%20b\"", 
   "attachment", "a b"],

  

  ["attachment; filename=\"", 
   "attachment", ""], 

  
  
  ["attachment; filename=foo; trouble", 
   "attachment", "foo"], 

  
  ["attachment; filename=foo; trouble ", 
   "attachment", "foo"], 

  ["attachment", 
   "attachment", Cr.NS_ERROR_INVALID_ARG], 

  

  ['attachment; filename=basic; filename*0="foo"; filename*1="\\b\\a\\r.html"', 
   "attachment", "foobar.html",
   ],

  
  ['attachment; filename=basic; filename*0="foo"; filename*1="\\b\\a\\', 
   "attachment", "fooba\\",
   ],

  
  

  ["attachment; extension=bla filename=foo", 
   "attachment", Cr.NS_ERROR_INVALID_ARG], 

  ["attachment; filename=foo extension=bla", 
   "attachment", "foo"], 

  ["attachment filename=foo", 
   "attachment", Cr.NS_ERROR_INVALID_ARG], 

  

  ["attachment; filename*=UTF-8''f%oo; filename=bar", 
   "attachment", "bar"], 

  ["attachment; filename*=UTF-8''foo%; filename=bar", 
   "attachment", "bar"], 

  
  ['attachment; filename="\\b\\a\\', 
   "attachment", "ba\\"], 
];

var rfc5987paramtests = [
  [ 
    "UTF-8'language'value", "value", "language", Cr.NS_OK ],
  [ 
    "UTF-8''1%202", "1 2", "", Cr.NS_OK ],
  [ 
    "UTF-8''%c2%a3%20and%20%e2%82%ac%20rates", "\u00a3 and \u20ac rates", "", Cr.NS_OK ],
  [ 
    "''abc", "", "", Cr.NS_ERROR_INVALID_ARG ],
  [ 
    "ISO-8859-1''%A3%20rates", "", "", Cr.NS_ERROR_INVALID_ARG ],
  [ 
    "foo''abc", "", "", Cr.NS_ERROR_INVALID_ARG ],
  [ 
    "abc", "", "", Cr.NS_ERROR_INVALID_ARG ],
  [ 
    "'abc", "", "", Cr.NS_ERROR_INVALID_ARG ],
  [ 
    "UTF-8''a b", "", "", Cr.NS_ERROR_INVALID_ARG ],
  [ 
    "UTF-8''a%zz", "", "", Cr.NS_ERROR_INVALID_ARG ],
  [ 
    "UTF-8''a%b", "", "", Cr.NS_ERROR_INVALID_ARG ],
  [ 
    "UTF-8''a%", "", "", Cr.NS_ERROR_INVALID_ARG ],
  [ 
    "UTF-8''%A3%20rates", "", "", 0x8050000E   ],
];

function do_tests(whichRFC)
{
  var mhp = Components.classes["@mozilla.org/network/mime-hdrparam;1"]
                      .getService(Components.interfaces.nsIMIMEHeaderParam);

  var unused = { value : null };

  for (var i = 0; i < tests.length; ++i) {
    dump("Testing #" + i + ": " + tests[i] + "\n");

    
    var expectedDt = tests[i].length == 3 || whichRFC == 0 ? tests[i][1] : tests[i][3];

    try {
      var result;
      
      if (whichRFC == 0)
        result = mhp.getParameter(tests[i][0], "", "UTF-8", true, unused);
      else 
        result = mhp.getParameterHTTP(tests[i][0], "", "UTF-8", true, unused);

      do_check_eq(result, expectedDt);
    } 
    catch (e) {
      
      if (e.result) {
        
        try { 
          do_check_eq(e.result, expectedDt); 
        } catch(e) {}  
      }
      continue;
    }

    
    var expectedFn = tests[i].length == 3 || whichRFC == 0 ? tests[i][2] : tests[i][4];

    try {
      var result;
      
      if (whichRFC == 0)
        result = mhp.getParameter(tests[i][0], "filename", "UTF-8", true, unused);
      else 
        result = mhp.getParameterHTTP(tests[i][0], "filename", "UTF-8", true, unused);

      do_check_eq(result, expectedFn);
    } 
    catch (e) {
      
      if (e.result) {
        
        try { 
          do_check_eq(e.result, expectedFn); 
        } catch(e) {}  
      }
      continue;
    }
  }
}

function test_decode5987Param() {
  var mhp = Components.classes["@mozilla.org/network/mime-hdrparam;1"]
                      .getService(Components.interfaces.nsIMIMEHeaderParam);

  for (var i = 0; i < rfc5987paramtests.length; ++i) {
    dump("Testing #" + i + ": " + rfc5987paramtests[i] + "\n");

    var lang = {};
    try {
      var decoded = mhp.decodeRFC5987Param(rfc5987paramtests[i][0], lang);
      if (rfc5987paramtests[i][3] == Cr.NS_OK) {
        do_check_eq(rfc5987paramtests[i][1], decoded);
        do_check_eq(rfc5987paramtests[i][2], lang.value);
      }
      else {
        do_check_eq(rfc5987paramtests[i][3], "instead got: " + decoded);
      }
    }
    catch (e) {
      do_check_eq(rfc5987paramtests[i][3], e.result);
    }
  }
}

function run_test() {

  
  do_tests(0);

  
  do_tests(1);

  
  test_decode5987Param();
}
