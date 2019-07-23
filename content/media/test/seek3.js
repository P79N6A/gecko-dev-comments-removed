function test_seek3(v, seekTime, is, ok, finish) {


var startPassed = false;
var completed = false;

function startTest() {
  if (completed)
    return false;

  v.currentTime=seekTime;
  return false;
}

function seekStarted() {
  if (completed)
    return false;

  startPassed = true;
  return false;
}

function seekEnded() {
  if (completed)
    return false;

  var t = v.currentTime;
  ok(Math.abs(t - seekTime) <= 0.1, "Video currentTime should be around " + seekTime + ": " + t);
  completed = true;
  finish();
  return false;
}

v.addEventListener("loadedmetadata", startTest, false);
v.addEventListener("seeking", seekStarted, false);
v.addEventListener("seeked", seekEnded, false);

}
