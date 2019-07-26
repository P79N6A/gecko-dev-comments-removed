




var testGenerator = testSteps();

function testSteps()
{
  
  run_test_in_child("./GlobalObjectsChild.js", function() {
    do_test_finished();
    continueToNextStep();
  });
  yield undefined;

  finishTest();
  yield undefined;
}
