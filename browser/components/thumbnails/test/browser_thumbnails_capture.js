






function runTests() {
  
  yield addTab("data:text/html,<body bgcolor=ff0000></body>");
  checkCurrentThumbnailColor(255, 0, 0, "we have a red thumbnail");

  
  yield navigateTo("data:text/html,<body bgcolor=00ff00></body>");
  checkCurrentThumbnailColor(0, 255, 0, "we have a green thumbnail");

  
  yield navigateTo("data:text/html,<body bgcolor=0000ff></body>");
  checkCurrentThumbnailColor(0, 0, 255, "we have a blue thumbnail");
}









function checkCurrentThumbnailColor(aRed, aGreen, aBlue, aMessage) {
  let tab = gBrowser.selectedTab;
  let cw = tab.linkedBrowser.contentWindow;

  let canvas = PageThumbs.capture(cw);
  let ctx = canvas.getContext("2d");

  checkCanvasColor(ctx, aRed, aGreen, aBlue, aMessage);
}
