























































var gTestfile = '15.9.5.4.js';
var SECTION = "15.9.5.4";
var VERSION = "ECMA_3"; 
var TITLE   = "Date.prototype.toTimeString()";
  
var status = '';
var actual = ''; 
var expect = '';
var givenDate;
var year = '';
var regexp = '';
var reducedDateString = '';
var hopeThisIsTimeString = '';
var cnEmptyString = '';
var cnERR ='OOPS! FATAL ERROR: no regexp match in extractTimeString()';

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);



status = "typeof (now.toTimeString())"; 
actual =   typeof (now.toTimeString());
expect = "string";
addTestCase();

status = "Date.prototype.toTimeString.length";  
actual =  Date.prototype.toTimeString.length;
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
  givenDate = new Date(date_given_in_milliseconds);
  
  status = '('  +  givenDate  +  ').toTimeString()';  
  actual = givenDate.toTimeString();
  expect = extractTimeString(givenDate);
  addTestCase();
}













function extractTimeString(date)
{
  regexp = new RegExp(date.toDateString() + '(.*)' + '$');

  try
  {
    hopeThisIsTimeString = date.toString().match(regexp)[1];
  }
  catch(e)
  {
    return cnERR;
  }

  
  return trimL(trimR(hopeThisIsTimeString));
}


function trimL(s)
{
  if (!s) {return cnEmptyString;};
  for (var i = 0; i!=s.length; i++) {if (s[i] != ' ') {break;}}
  return s.substring(i);
}


function trimR(s)
{
  if (!s) {return cnEmptyString;};
  for (var i = (s.length - 1); i!=-1; i--) {if (s[i] != ' ') {break;}}
  return s.substring(0, i+1);
}
