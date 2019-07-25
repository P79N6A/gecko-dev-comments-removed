




































function closeFullScreen() {
  sendAsyncMessage("Browser:FullScreenVideo:Close");
}

function startPlayback(aEvent) {
  let target = aEvent.originalTarget;
  contentObject._sendMouseEvent("mousedown", target, 0, 0);
  contentObject._sendMouseEvent("mousemove", target, 0, 0);
  contentObject._sendMouseEvent("mouseup", target, 0, 0);
}

addEventListener("click", function(aEvent) {
  if (aEvent.target.id == "close")
    closeFullScreen();
}, false);

addEventListener("CloseVideo", closeFullScreen, false);
addEventListener("StartVideo", startPlayback, false);

addEventListener("PlayVideo", function() {
  sendAsyncMessage("Browser:FullScreenVideo:Play");
}, false);

addEventListener("PauseVideo", function() {
  sendAsyncMessage("Browser:FullScreenVideo:Pause");
}, false);

