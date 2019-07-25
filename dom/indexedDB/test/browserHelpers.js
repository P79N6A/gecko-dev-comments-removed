




let testGenerator = testSteps();

let testResult;
let testException;

function testFinishedCallback(result, exception)
{
  throw new Error("Bad testFinishedCallback!");
}

function runTest()
{
  testGenerator.next();
}

function finishTestNow()
{
  if (testGenerator) {
    testGenerator.close();
    delete testGenerator;
  }
}

function finishTest()
{
  setTimeout(finishTestNow, 0);
  setTimeout(testFinishedCallback, 0, testResult, testException);
}

function grabEventAndContinueHandler(event)
{
  testGenerator.send(event);
}

function errorHandler(event)
{
  throw new Error("indexedDB error, code " + event.target.errorCode);
}

function continueToNextStep()
{
  SimpleTest.executeSoon(function() {
    testGenerator.next();
  });
}
