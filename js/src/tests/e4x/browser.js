




































var SUMMARY = '';
var DESCRIPTION = '';
var EXPECTED = '';
var ACTUAL = '';
var MSG = '';
var SECTION = '';

window.onerror = function (msg, page, line)
{
  optionsPush();

  if (typeof SUMMARY == 'undefined')
  {
    SUMMARY = 'Unknown';
  }
  if (typeof SECTION == 'undefined')
  {
    SECTION = 'Unknown';
  }
  if (typeof DESCRIPTION == 'undefined')
  {
    DESCRIPTION = 'Unknown';
  }
  if (typeof EXPECTED == 'undefined')
  {
    EXPECTED = 'Unknown';
  }

  var testcase = new TestCase(gTestfile, SUMMARY + DESCRIPTION +
                              ' Section ' + SECTION, EXPECTED, "error");

  testcase.passed = false;

  testcase.reason = page + ':' + line + ': ' + msg;

  reportFailure(SECTION, msg);

  gDelayTestDriverEnd = false;
  jsTestDriverEnd();

  optionsReset();
};

options('xml');

