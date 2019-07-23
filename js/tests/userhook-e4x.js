








































var gCheckInterval = 1000;



function userOnStart()
{
  registerDialogCloser();
}

function userOnBeforePage()
{
  dlog('userOnBeforePage');
}

var gReport;

function userOnAfterPage()
{
  dlog('userOnAfterPage');

  var win = gSpider.mDocument.defaultView;
  if (win.wrappedJSObject)
  {
    win = win.wrappedJSObject;
  }
  win.__Report = win.reportHTML;
  win.reportHTML = function () { win.__Report(); gPageCompleted = true; };


  win.reportCallBack = function (testwin)
    {
      if (testwin.wrappedJSObject)
      {
        testwin = testwin.wrappedJSObject;
      }
      var gTestcases = testwin.gTestcases;
      if (typeof gTestcases == 'undefined')
      {
        return;
      }
      for (var i = 0; i < gTestcases.length; i++)
      {
        var testcase = gTestcases[i];
        cdump('test: '    + testcase.path + ' ' +
              'bug: '         + testcase.bugnumber + ' ' +
              'result: ' + (testcase.passed ? 'PASSED':'FAILED') + ' ' +
              'type: browser ' +
              'description: ' + testcase.description + ' ' +
              'expected: '    + testcase.expect + ' ' +
              'actual: '      + testcase.actual + ' ' +
              'reason: '      + testcase.reason);
      }
    };


  win.document.forms.testCases.failures.checked = true;

  win.document.forms.testCases.doctype.value = "standards";

  win.selectAll('e4x');


  win.setTimeout("executeList()", 10000);
}

function userOnStop()
{
  
  closeDialog();
  unregisterDialogCloser();
}


gConsoleListener.onConsoleMessage =
  function userOnConsoleMessage(s)
{
  dump(s);
};
