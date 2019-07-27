




const { 'classes': Cc, 'interfaces': Ci, 'utils': Cu } = Components;

let testGenerator = testSteps();

if (!window.runTest) {
  window.runTest = function()
  {
    SimpleTest.waitForExplicitFinish();

    testGenerator.next();
  }
}

function finishTest()
{
  SimpleTest.executeSoon(function() {
    testGenerator.close();
    SimpleTest.finish();
  });
}

function grabEventAndContinueHandler(event)
{
  testGenerator.send(event);
}

function continueToNextStep()
{
  SimpleTest.executeSoon(function() {
    testGenerator.next();
  });
}

function errorHandler(event)
{
  throw new Error("indexedDB error, code " + event.target.error.name);
}
