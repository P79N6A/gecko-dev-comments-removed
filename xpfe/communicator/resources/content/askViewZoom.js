






































var dialog;
var args;

function onLoad() {
  args = window.arguments[0];
  args.zoomOK = false;

  dialog = {};
  dialog.OKButton = document.documentElement.getButton("accept");

  dialog.input = document.getElementById("zoomValue");
  dialog.input.value = args.value;
  dialog.input.select();
  dialog.input.focus();

  moveToAlertPosition();
  doEnabling();
}

function onAccept() {
  var zoom = parseFloat(dialog.input.value);
  if (!isNaN(zoom) && zoom >= args.zoomMin && zoom <= args.zoomMax) {
    args.value = zoom;
    args.zoomOK = true;
  }
  return args.zoomOK;
}

function doEnabling() {
  var enable = false;
  if (dialog.input.value) {
    var zoom = parseFloat(dialog.input.value);
    if (!isNaN(zoom) && zoom >= args.zoomMin && zoom <= args.zoomMax)
      enable = true;
  }

  dialog.OKButton.disabled = !enable;
}
