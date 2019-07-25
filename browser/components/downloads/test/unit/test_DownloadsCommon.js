








function testFormatTimeLeft(aSeconds, aExpectedValue, aExpectedUnitString)
{
  let expected = "";
  if (aExpectedValue) {
    
    expected = DownloadsCommon.strings[aExpectedUnitString](aExpectedValue);
  }
  do_check_eq(DownloadsCommon.formatTimeLeft(aSeconds), expected);
}

function run_test()
{
  testFormatTimeLeft(      0,   "", "");
  testFormatTimeLeft(      1,  "1", "shortTimeLeftSeconds");
  testFormatTimeLeft(     29, "29", "shortTimeLeftSeconds");
  testFormatTimeLeft(     30, "30", "shortTimeLeftSeconds");
  testFormatTimeLeft(     31,  "1", "shortTimeLeftMinutes");
  testFormatTimeLeft(     60,  "1", "shortTimeLeftMinutes");
  testFormatTimeLeft(     89,  "1", "shortTimeLeftMinutes");
  testFormatTimeLeft(     90,  "2", "shortTimeLeftMinutes");
  testFormatTimeLeft(     91,  "2", "shortTimeLeftMinutes");
  testFormatTimeLeft(   3600,  "1", "shortTimeLeftHours");
  testFormatTimeLeft(  86400, "24", "shortTimeLeftHours");
  testFormatTimeLeft( 169200, "47", "shortTimeLeftHours");
  testFormatTimeLeft( 172800,  "2", "shortTimeLeftDays");
  testFormatTimeLeft(8553600, "99", "shortTimeLeftDays");
  testFormatTimeLeft(8640000, "99", "shortTimeLeftDays");
}
