




































gTestsuite = 'ecma_2';

var TZ_DIFF = getTimeZoneDiff();







function getTimeZoneDiff()
{
  return -((new Date(2000, 1, 1)).getTimezoneOffset())/60;
}
