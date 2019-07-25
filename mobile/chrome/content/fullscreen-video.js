




































function closeFullScreen() {
  sendAsyncMessage("Browser:FullScreenVideo:Close");
}

addEventListener("click", function(aEvent) {
  if (aEvent.target.id == "close")
    closeFullScreen();
}, false);

addEventListener("CloseVideo", closeFullScreen, false);

addEventListener("PlayVideo", function() {
  sendAsyncMessage("Browser:FullScreenVideo:Play");
}, false);

addEventListener("PauseVideo", function() {
  sendAsyncMessage("Browser:FullScreenVideo:Pause");
}, false);

