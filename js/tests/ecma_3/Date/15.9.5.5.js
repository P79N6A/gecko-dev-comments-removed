





































var gTestfile = '15.9.5.5.js';





















var SECTION = "15.9.5.5";
var VERSION = "ECMA_3"; 
var TITLE   = "Date.prototype.toLocaleString()"; 
  
var status = '';
var actual = ''; 
var expect = '';


startTest();
writeHeaderToLog( SECTION + " "+ TITLE);



status = "typeof (now.toLocaleString())"; 
actual =   typeof (now.toLocaleString());
expect = "string";
addTestCase();

status = "Date.prototype.toLocaleString.length";  
actual =  Date.prototype.toLocaleString.length;
expect =  0;  
addTestCase();


status = "Math.abs(Date.parse(now.toLocaleString()) - now.valueOf()) < 1000";  
actual =   Math.abs(Date.parse(now.toLocaleString()) -  now.valueOf()) < 1000;
expect = true;
addTestCase();




addDateTestCase(0);
addDateTestCase(TZ_ADJUST);  

  

addDateTestCase(TIME_1900);
addDateTestCase(TIME_1900 -TZ_ADJUST);

  

addDateTestCase(TIME_2000);
addDateTestCase(TIME_2000 -TZ_ADJUST);

   

addDateTestCase(UTC_29_FEB_2000);
addDateTestCase(UTC_29_FEB_2000 - 1000);   
addDateTestCase(UTC_29_FEB_2000 - TZ_ADJUST);



addDateTestCase(UTC_1_JAN_2005);
addDateTestCase(UTC_1_JAN_2005 - 1000);
addDateTestCase(UTC_1_JAN_2005-TZ_ADJUST);
  



test();



function addTestCase()
{
  AddTestCase(
    status,
    expect,
    actual);
}


function addDateTestCase(date_given_in_milliseconds)
{
  var givenDate = new Date(date_given_in_milliseconds);

  status = 'Date.parse('   +   givenDate   +   ').toLocaleString())';  
  actual =  Date.parse(givenDate.toLocaleString());
  expect = date_given_in_milliseconds;
  addTestCase();
}

