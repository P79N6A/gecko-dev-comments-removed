




































gTestfile = 'regress-7703.js';








var SECTION = "js1_2";       
var VERSION = "JS1_2"; 
var TITLE   = "Regression test for bugzilla # 7703";       
var BUGNUMBER = "http://bugzilla.mozilla.org/show_bug.cgi?id=7703";     

startTest();               



















types = [];
function inspect(object) {
  for (prop in object) {
    var x = object[prop];
    types[types.length] = (typeof x);
  }
}

var o = {a: 1, b: 2};
inspect(o);

AddTestCase( "inspect(o),length",   2,       types.length );
AddTestCase( "inspect(o)[0]",      "number", types[0] );
AddTestCase( "inspect(o)[1]",      "number", types[1] );

types_2 = [];

function inspect_again(object) {
  for (prop in object) {
    types_2[types_2.length] = (typeof object[prop]);
  }
}

inspect_again(o);
AddTestCase( "inspect_again(o),length",   2,       types.length );
AddTestCase( "inspect_again(o)[0]",      "number", types[0] );
AddTestCase( "inspect_again(o)[1]",      "number", types[1] );


test();       

