



























































var gTestfile = '15.9.5.7.js';
var SECTION = "15.9.5.7";
var VERSION = "ECMA_3"; 
var TITLE   = "Date.prototype.toLocaleTimeString()";
  
var status = '';
var actual = ''; 
var expect = '';
var givenDate;
var year = '';
var regexp = '';
var TimeString = '';
var reducedDateString = '';
var hopeThisIsLocaleTimeString = '';
var cnERR ='OOPS! FATAL ERROR: no regexp match in extractLocaleTimeString()';

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);



status = "typeof (now.toLocaleTimeString())"; 
actual =   typeof (now.toLocaleTimeString());
expect = "string";
addTestCase();

status = "Date.prototype.toLocaleTimeString.length";  
actual =  Date.prototype.toLocaleTimeString.length;
expect =  0;  
addTestCase();


addDateTestCase(0);
addDateTestCase(TZ_ADJUST);
  

addDateTestCase(TIME_1900);
addDateTestCase(TIME_1900 - TZ_ADJUST);


addDateTestCase(TIME_2000);
addDateTestCase(TIME_2000 - TZ_ADJUST);
   

addDateTestCase(UTC_29_FEB_2000);
addDateTestCase(UTC_29_FEB_2000 - 1000);
addDateTestCase(UTC_29_FEB_2000 - TZ_ADJUST);


addDateTestCase( TIME_NOW);
addDateTestCase( TIME_NOW - TZ_ADJUST);


addDateTestCase(UTC_1_JAN_2005);
addDateTestCase(UTC_1_JAN_2005 - 1000);
addDateTestCase(UTC_1_JAN_2005 - TZ_ADJUST);

test();

function addTestCase()
{
  new TestCase(
    SECTION,
    status,
    expect,
    actual);
}


function addDateTestCase(date_given_in_milliseconds)
{
  var s = 'new Date(' +  date_given_in_milliseconds + ')';
  givenDate = new Date(date_given_in_milliseconds);
  
  status = 'd = ' + s +
    '; d == new Date(d.toDateString() + " " + d.toLocaleTimeString())';  
  expect = givenDate.toString();
  actual = new Date(givenDate.toDateString() +
                    ' ' + givenDate.toLocaleTimeString()).toString();
  addTestCase();
}

