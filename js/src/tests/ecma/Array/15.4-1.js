





































gTestfile = '15.4-1.js';













var SECTION = "15.4-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Array Objects";

writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase(SECTION,
             "var myarr = new Array(); myarr[Math.pow(2,32)-2]='hi'; myarr[Math.pow(2,32)-2]",
             "hi",
             eval("var myarr = new Array(); myarr[Math.pow(2,32)-2]='hi'; myarr[Math.pow(2,32)-2]")
  );

new TestCase(SECTION,
             "var myarr = new Array(); myarr[Math.pow(2,32)-2]='hi'; myarr.length",
             (Math.pow(2,32)-1),
             eval("var myarr = new Array(); myarr[Math.pow(2,32)-2]='hi'; myarr.length")
  );

new TestCase(SECTION,
             "var myarr = new Array(); myarr[Math.pow(2,32)-3]='hi'; myarr[Math.pow(2,32)-3]",
             "hi",
             eval("var myarr = new Array(); myarr[Math.pow(2,32)-3]='hi'; myarr[Math.pow(2,32)-3]")
  );

new TestCase(SECTION,
             "var myarr = new Array(); myarr[Math.pow(2,32)-3]='hi'; myarr.length",
             (Math.pow(2,32)-2),
             eval("var myarr = new Array(); myarr[Math.pow(2,32)-3]='hi'; myarr.length")
  );

new TestCase(SECTION,
             "var myarr = new Array(); myarr[Math.pow(2,31)-2]='hi'; myarr[Math.pow(2,31)-2]",
             "hi",
             eval("var myarr = new Array(); myarr[Math.pow(2,31)-2]='hi'; myarr[Math.pow(2,31)-2]")
  );

new TestCase(SECTION,
             "var myarr = new Array(); myarr[Math.pow(2,31)-2]='hi'; myarr.length",
             (Math.pow(2,31)-1),
             eval("var myarr = new Array(); myarr[Math.pow(2,31)-2]='hi'; myarr.length")
  );

new TestCase(SECTION,
             "var myarr = new Array(); myarr[Math.pow(2,31)-1]='hi'; myarr[Math.pow(2,31)-1]",
             "hi",
             eval("var myarr = new Array(); myarr[Math.pow(2,31)-1]='hi'; myarr[Math.pow(2,31)-1]")
  );

new TestCase(SECTION,
             "var myarr = new Array(); myarr[Math.pow(2,31)-1]='hi'; myarr.length",
             (Math.pow(2,31)),
             eval("var myarr = new Array(); myarr[Math.pow(2,31)-1]='hi'; myarr.length")
  );

new TestCase(SECTION,
             "var myarr = new Array(); myarr[Math.pow(2,31)]='hi'; myarr[Math.pow(2,31)]",
             "hi",
             eval("var myarr = new Array(); myarr[Math.pow(2,31)]='hi'; myarr[Math.pow(2,31)]")
  );

new TestCase(SECTION,
             "var myarr = new Array(); myarr[Math.pow(2,31)]='hi'; myarr.length",
             (Math.pow(2,31)+1),
             eval("var myarr = new Array(); myarr[Math.pow(2,31)]='hi'; myarr.length")
  );

new TestCase(SECTION,
             "var myarr = new Array(); myarr[Math.pow(2,30)-2]='hi'; myarr[Math.pow(2,30)-2]",
             "hi",
             eval("var myarr = new Array(); myarr[Math.pow(2,30)-2]='hi'; myarr[Math.pow(2,30)-2]")
  );

new TestCase(SECTION,
             "var myarr = new Array(); myarr[Math.pow(2,30)-2]='hi'; myarr.length",
             (Math.pow(2,30)-1),
             eval("var myarr = new Array(); myarr[Math.pow(2,30)-2]='hi'; myarr.length")
  );

test();

