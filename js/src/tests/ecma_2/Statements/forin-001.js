
















var SECTION = "forin-001";
var VERSION = "ECMA_2";
var TITLE   = "The for...in  statement";
var BUGNUMBER="330890";
var BUGNUMBER="http://scopus.mcom.com/bugsplat/show_bug.cgi?id=344855";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

ForIn_1( { length:4, company:"netscape", year:2000, 0:"zero" } );
ForIn_2( { length:4, company:"netscape", year:2000, 0:"zero" } );
ForIn_3( { length:4, company:"netscape", year:2000, 0:"zero" } );



ForIn_8({ length:4, company:"netscape", year:2000, 0:"zero" });

test();







function ForIn_1( object ) {
  PropertyArray = new Array();
  ValueArray = new Array();

  for ( PropertyArray[PropertyArray.length] in object ) {
    ValueArray[ValueArray.length] =
      object[PropertyArray[PropertyArray.length-1]];
  }

  for ( var i = 0; i < PropertyArray.length; i++ ) {
    new TestCase(
      SECTION,
      "object[" + PropertyArray[i] +"]",
      object[PropertyArray[i]],
      ValueArray[i]
      );
  }

  new TestCase(
    SECTION,
    "object.length",
    PropertyArray.length,
    object.length );
}





function ForIn_2( object ) {
  PropertyArray = new Array();
  ValueArray = new Array();
  var i = 0;

  for ( PropertyArray[i++] in object ) {
    ValueArray[ValueArray.length] =
      object[PropertyArray[PropertyArray.length-1]];
  }

  for ( i = 0; i < PropertyArray.length; i++ ) {
    new TestCase(
      SECTION,
      "object[" + PropertyArray[i] +"]",
      object[PropertyArray[i]],
      ValueArray[i]
      );
  }

  new TestCase(
    SECTION,
    "object.length",
    PropertyArray.length,
    object.length );
}






function ForIn_3( object ) {
  var checkBreak = "pass";
  var properties = new Array();
  var values = new Array();

  for ( properties[properties.length] in object ) {
    values[values.length] = object[properties[properties.length-1]];
    break;
    checkBreak = "fail";
  }

  new TestCase(
    SECTION,
    "check break out of for...in",
    "pass",
    checkBreak );

  new TestCase(
    SECTION,
    "properties.length",
    1,
    properties.length );

  new TestCase(
    SECTION,
    "object["+properties[0]+"]",
    values[0],
    object[properties[0]] );
}




function ForIn_4( object ) {
  var result1 = 0;
  var result2 = 0;
  var result3 = 0;
  var result4 = 0;
  var i = 0;
  var property = new Array();

butterbean: {
    result1++;

    for ( property[i++] in object ) {
      result2++;
      break;
      result4++;
    }
    result3++;
  }

  new TestCase(
    SECTION,
    "verify labeled statement is only executed once",
    true,
    result1 == 1 );

  new TestCase(
    SECTION,
    "verify statements in for loop are evaluated",
    true,
    result2 == i );

  new TestCase(
    SECTION,
    "verify break out of labeled for...in loop",
    true,
    result4 == 0 );

  new TestCase(
    SECTION,
    "verify break out of labeled block",
    true,
    result3 == 0 );
}




function ForIn_5 (object) {
  var result1 = 0;
  var result2 = 0;
  var result3 = 0;
  var result4 = 0;
  var i = 0;
  var property = new Array();

bigredbird: {
    result1++;
    for ( property[i++] in object ) {
      result2++;
      break bigredbird;
      result4++;
    }
    result3++;
  }

  new TestCase(
    SECTION,
    "verify labeled statement is only executed once",
    true,
    result1 == 1 );

  new TestCase(
    SECTION,
    "verify statements in for loop are evaluated",
    true,
    result2 == i );

  new TestCase(
    SECTION,
    "verify break out of labeled for...in loop",
    true,
    result4 == 0 );

  new TestCase(
    SECTION,
    "verify break out of labeled block",
    true,
    result3 == 0 );
}




function ForIn_7( object ) {
  var result1 = 0;
  var result2 = 0;
  var result3 = 0;
  var result4 = 0;
  var i = 0;
  var property = new Array();

bigredbird:
  for ( property[i++] in object ) {
    result2++;
    continue bigredbird;
    result4++;
  }

  new TestCase(
    SECTION,
    "verify statements in for loop are evaluated",
    true,
    result2 == i );

  new TestCase(
    SECTION,
    "verify break out of labeled for...in loop",
    true,
    result4 == 0 );

  new TestCase(
    SECTION,
    "verify break out of labeled block",
    true,
    result3 == 1 );
}






function ForIn_8( object ) {
  var checkBreak = "pass";
  var properties = new Array();
  var values = new Array();

  for ( properties[properties.length] in object ) {
    values[values.length] = object[properties[properties.length-1]];
    break;
    checkBreak = "fail";
  }

  new TestCase(
    SECTION,
    "check break out of for...in",
    "pass",
    checkBreak );

  new TestCase(
    SECTION,
    "properties.length",
    1,
    properties.length );

  new TestCase(
    SECTION,
    "object["+properties[0]+"]",
    values[0],
    object[properties[0]] );
}

