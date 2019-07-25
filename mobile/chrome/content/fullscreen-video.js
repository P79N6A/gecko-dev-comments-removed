




































function closeFullScreen() {
  sendAsyncMessage("Browser:FullScreenVideo:Close");
}

addEventListener("click", function(aEvent) {
  if (aEvent.target.id == "close")
    closeFullScreen();
}, false);

addEventListener("CloseVideo", closeFullScreen, false);
