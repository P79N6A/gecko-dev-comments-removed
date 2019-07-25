




var gRemainingTests = 0;

function waitForWorkerFinish() {
  if (gRemainingTests == 0) {
    SimpleTest.waitForExplicitFinish();
  }
  ++gRemainingTests;
}

function finish() {
  --gRemainingTests;
  if (gRemainingTests == 0) {
    SimpleTest.finish();
  }
}

