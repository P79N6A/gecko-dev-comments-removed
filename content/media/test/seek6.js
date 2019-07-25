function test_seek6(v, seekTime, is, ok, finish) {



var seekCount = 0;
var completed = false;
var interval;
var sum = 0;

function poll() {
  sum += v.currentTime;
}

function startTest() {
  if (completed)
    return false;
  interval = setInterval(poll, 10);
  v.currentTime = Math.random() * v.duration;
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
  } else {
    v.currentTime = Math.random() * v.duration;
  }
  return false;
}

v.addEventListener("loadedmetadata", startTest, false);
v.addEventListener("seeked", seekEnded, false);

}
