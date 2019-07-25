

function delayedSnapshot(seekTimeInSeconds) {
  
  window.setTimeout(finish, 10, seekTimeInSeconds);
}

function finish(seekTimeInSeconds) {
  document.documentElement.pauseAnimations();
  if (seekTimeInSeconds)
    document.documentElement.setCurrentTime(seekTimeInSeconds);
  document.documentElement.removeAttribute("class");
}

function click(targetId) {
  var evt = document.createEvent("MouseEvents");
  evt.initMouseEvent("click", true, true, window,
    0, 0, 0, 0, 0, false, false, false, false, 0, null);
  var target = document.getElementById(targetId);
  target.dispatchEvent(evt);
}
