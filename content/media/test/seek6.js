function test_seek6(v, seekTime, is, ok, finish) {



var seekCount = 0;
var completed = false;
var interval;

function startTest() {
  if (completed)
    return false;
  interval = setInterval(function() { v.currentTime=Math.random()*v.duration; }, 10);
  return false;
}

function seekEnded() {
  if (completed)
    return false;

  seekCount++;
  ok(true, "Seek " + seekCount);
  if (seekCount == 3) {
    clearInterval(interval);
    completed = true;
    finish();
  }
  return false;
}

v.addEventListener("loadedmetadata", startTest, false);
v.addEventListener("seeked", seekEnded, false);

}
