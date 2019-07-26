



var gInstanceUID;











function notifyParent(status) {
  if (!gInstanceUID) {
    gInstanceUID = window.location.search.substr(1);
  }

  window.parent.postMessage({
    uid: gInstanceUID,
    status: status
  }, "*");
}


















function onParentMessage(event) {
  var start = document.getElementById("startWrapper");
  var stop = document.getElementById("stopWrapper");
  var msg = JSON.parse(event.data);

  if (msg.task === "onStarted") {
    start.style.display = "none";
    start.querySelector("button").removeAttribute("disabled");
    stop.style.display = "inline";
  } else if (msg.task === "onStopped") {
    stop.style.display = "none";
    stop.querySelector("button").removeAttribute("disabled");
    start.style.display = "inline";
  }
}

window.addEventListener("message", onParentMessage);





function initUI() {
  gLightMode = true;
  gJavaScriptOnly = true;

  var container = document.createElement("div");
  container.id = "ui";

  gMainArea = document.createElement("div");
  gMainArea.id = "mainarea";

  container.appendChild(gMainArea);
  document.body.appendChild(container);

  var startButton = document.createElement("button");
  startButton.innerHTML = "Start";
  startButton.addEventListener("click", function (event) {
    event.target.setAttribute("disabled", true);
    notifyParent("start");
  }, false);

  var stopButton = document.createElement("button");
  stopButton.innerHTML = "Stop";
  stopButton.addEventListener("click", function (event) {
    event.target.setAttribute("disabled", true);
    notifyParent("stop");
  }, false);

  var controlPane = document.createElement("div");
  controlPane.className = "controlPane";
  controlPane.innerHTML =
    "<p id='startWrapper'>Click <span class='btn'></span> to start profiling.</p>" +
    "<p id='stopWrapper'>Click <span class='btn'></span> to stop profiling.</p>";

  controlPane.querySelector("#startWrapper > span.btn").appendChild(startButton);
  controlPane.querySelector("#stopWrapper > span.btn").appendChild(stopButton);

  gMainArea.appendChild(controlPane);
}