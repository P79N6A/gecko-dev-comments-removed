




var archiveReaderEnabled = false;

var testGenerator = testSteps();

function runTest()
{
  enableArchiveReader();

  SimpleTest.waitForExplicitFinish();
  testGenerator.next();
}

function finishTest()
{
  resetArchiveReader();

  SimpleTest.executeSoon(function() {
    testGenerator.close();
    SimpleTest.finish();
  });
}

function enableArchiveReader()
{
  archiveReaderEnabled = SpecialPowers.getBoolPref("dom.archivereader.enabled");
  SpecialPowers.setBoolPref("dom.archivereader.enabled", true);
}

function resetArchiveReader()
{
  SpecialPowers.setBoolPref("dom.archivereader.enabled", archiveReaderEnabled);
}
