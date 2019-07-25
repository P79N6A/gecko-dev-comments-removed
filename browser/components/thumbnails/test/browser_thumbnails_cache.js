







function runTests() {
  
  yield addTab("data:text/html,<body bgcolor=ff0000></body>");
  let cw = gBrowser.selectedTab.linkedBrowser.contentWindow;

  
  let canvas = PageThumbs.capture(cw);

  
  yield PageThumbs.store("key", canvas, next);

  let {width, height} = canvas;
  let thumb = PageThumbs.getThumbnailURL("key", width, height);

  
  yield addTab("data:text/html,<img src='" + thumb + "'/>" + 
               "<canvas width=" + width + " height=" + height + "/>");

  cw = gBrowser.selectedTab.linkedBrowser.contentWindow;
  let [img, canvas] = cw.document.querySelectorAll("img, canvas");

  
  let ctx = canvas.getContext("2d");
  ctx.drawImage(img, 0, 0, width, height);
  checkCanvasColor(ctx, 255, 0, 0, "we have a red image and canvas");
}
