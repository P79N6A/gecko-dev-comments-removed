

function setTimeAndSnapshot(timeInSeconds, pauseFlag) {
  var svg = document.documentElement;
  if (pauseFlag) {
    svg.pauseAnimations();
  }
  svg.setCurrentTime(timeInSeconds);
  
  setTimeout('document.documentElement.removeAttribute("class")', 0);
}
