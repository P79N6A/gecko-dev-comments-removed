





































var gTestfile = '15.9.5.6.js';





















var SECTION = "15.9.5.6";
var VERSION = "ECMA_3"; 
var TITLE   = "Date.prototype.toLocaleDateString()"; 
 
var status = '';
var actual = ''; 
var expect = '';


startTest();
writeHeaderToLog( SECTION + " "+ TITLE);



status = "typeof (now.toLocaleDateString())"; 
actual =   typeof (now.toLocaleDateString());
expect = "string";
addTestCase();

status = "Date.prototype.toLocaleDateString.length";  
actual =  Date.prototype.toLocaleDateString.length;
expect =  0;  
addTestCase();



status = "(Date.parse(now.toLocaleDateString()) - (midnight(now)).valueOf()) == 0";  
actual =   (Date.parse(now.toLocaleDateString()) - (midnight(now)).valueOf()) == 0;
expect = true;
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



addDateTestCase(UTC_1_JAN_2005);
addDateTestCase(UTC_1_JAN_2005 - 1000);
addDateTestCase(UTC_1_JAN_2005 - TZ_ADJUST);
  



test();



function addTestCase()
{
  new TestCase(
    gTestfile,
    status,
    expect,
    actual);
}


function addDateTestCase(date_given_in_milliseconds)
{
  var givenDate = new Date(date_given_in_milliseconds);

  status = 'Date.parse('   +   givenDate   +   ').toLocaleDateString())';  
  actual =  Date.parse(givenDate.toLocaleDateString());
  expect = Date.parse(midnight(givenDate));
  addTestCase();
}


function midnight(givenDate)
{
  
  return new Date(givenDate.getFullYear(), givenDate.getMonth(), givenDate.getDate());
}

