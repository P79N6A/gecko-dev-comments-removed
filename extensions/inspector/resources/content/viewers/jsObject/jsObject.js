














































var viewer;
var bundle;



window.addEventListener("load", JSObjectViewer_initialize, false);

function JSObjectViewer_initialize()
{
  bundle = document.getElementById("inspector-bundle");
  viewer = new JSObjectViewer();
  viewer.initialize(parent.FrameExchange.receiveData(window));
}
