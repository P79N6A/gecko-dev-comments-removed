



addMessageListener("Eyedropper:RequestContentScreenshot", sendContentScreenshot);

function sendContentScreenshot() {
  let canvas = content.document.createElementNS("http://www.w3.org/1999/xhtml", "canvas");
  let width = content.innerWidth;
  let height = content.innerHeight;
  canvas.width = width;
  canvas.height = height;
  canvas.mozOpaque = true;

  let ctx = canvas.getContext("2d");

  ctx.drawWindow(content, content.scrollX, content.scrollY, width, height, "#fff");

  sendAsyncMessage("Eyedropper:Screenshot", canvas.toDataURL());
}
