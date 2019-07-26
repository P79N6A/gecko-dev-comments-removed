



var gInstanceUID;
















function notifyParent(status, data={}) {
  if (!gInstanceUID) {
    gInstanceUID = window.location.search.substr(1);
  }

  window.parent.postMessage({
    uid: gInstanceUID,
    status: status,
    data: data
  }, "*");
}


















function onParentMessage(event) {
  var start = document.getElementById("startWrapper");
  var stop = document.getElementById("stopWrapper");
  var profilerMessage = document.getElementById("profilerMessage");
  var msg = JSON.parse(event.data);

  switch (msg.task) {
    case "onStarted":
      if (msg.isCurrent) {
        start.style.display = "none";
        start.querySelector("button").removeAttribute("disabled");
        stop.style.display = "inline";
      } else {
        start.querySelector("button").setAttribute("disabled", true);
        var text = gStrings.getFormatStr("profiler.alreadyRunning", [msg.uid]);
        profilerMessage.textContent = text;
        profilerMessage.style.display = "block";
        notifyParent("disabled");
      }
      break;
    case "onStopped":
      if (msg.isCurrent) {
        stop.style.display = "none";
        stop.querySelector("button").removeAttribute("disabled");
        start.style.display = "inline";
      } else {
        start.querySelector("button").removeAttribute("disabled");
        profilerMessage.textContent = "";
        profilerMessage.style.display = "none";
        notifyParent("enabled");
      }
      break;
    case "receiveProfileData":
      loadProfile(JSON.stringify(msg.rawProfile));
  }
}

window.addEventListener("message", onParentMessage);





function initUI() {
  gLightMode = true;

  gFileList = { profileParsingFinished: function () {} };
  gInfoBar = { display: function () {} };

  var container = document.createElement("div");
  container.id = "ui";

  gMainArea = document.createElement("div");
  gMainArea.id = "mainarea";

  container.appendChild(gMainArea);
  document.body.appendChild(container);

  var startButton = document.createElement("button");
  startButton.innerHTML = gStrings.getStr("profiler.start");
  startButton.addEventListener("click", function (event) {
    event.target.setAttribute("disabled", true);
    notifyParent("start");
  }, false);

  var stopButton = document.createElement("button");
  stopButton.innerHTML = gStrings.getStr("profiler.stop");
  stopButton.addEventListener("click", function (event) {
    event.target.setAttribute("disabled", true);
    notifyParent("stop");
  }, false);

  var controlPane = document.createElement("div");
  var startProfiling = gStrings.getFormatStr("profiler.startProfiling",
    ["<span class='btn'></span>"]);
  var stopProfiling = gStrings.getFormatStr("profiler.stopProfiling",
    ["<span class='btn'></span>"]);

  controlPane.className = "controlPane";
  controlPane.innerHTML =
    "<p id='startWrapper'>" + startProfiling + "</p>" +
    "<p id='stopWrapper'>" + stopProfiling + "</p>" +
    "<p id='profilerMessage'></p>";

  controlPane.querySelector("#startWrapper > span.btn").appendChild(startButton);
  controlPane.querySelector("#stopWrapper > span.btn").appendChild(stopButton);

  gMainArea.appendChild(controlPane);
}






function enterFinishedProfileUI() {
  var cover = document.createElement("div");
  cover.className = "finishedProfilePaneBackgroundCover";

  var pane = document.createElement("table");
  var rowIndex = 0;
  var currRow;

  pane.style.width = "100%";
  pane.style.height = "100%";
  pane.border = "0";
  pane.cellPadding = "0";
  pane.cellSpacing = "0";
  pane.borderCollapse = "collapse";
  pane.className = "finishedProfilePane";

  gBreadcrumbTrail = new BreadcrumbTrail();
  currRow = pane.insertRow(rowIndex++);
  currRow.insertCell(0).appendChild(gBreadcrumbTrail.getContainer());

  gHistogramView = new HistogramView();
  currRow = pane.insertRow(rowIndex++);
  currRow.insertCell(0).appendChild(gHistogramView.getContainer());

  if (gMeta && gMeta.videoCapture) {
    gVideoPane = new VideoPane(gMeta.videoCapture);
    gVideoPane.onTimeChange(videoPaneTimeChange);
    currRow = pane.insertRow(rowIndex++);
    currRow.insertCell(0).appendChild(gVideoPane.getContainer());
  }

  var tree = document.createElement("div");
  tree.className = "treeContainer";
  tree.style.width = "100%";
  tree.style.height = "100%";

  gTreeManager = new ProfileTreeManager();
  gTreeManager.treeView.setColumns([
    { name: "sampleCount", title: gStrings["Running Time"] },
    { name: "selfSampleCount", title: gStrings["Self"] },
    { name: "resource", title: "" }
  ]);

  currRow = pane.insertRow(rowIndex++);
  currRow.style.height = "100%";

  var cell = currRow.insertCell(0);
  cell.appendChild(tree);
  tree.appendChild(gTreeManager.getContainer());

  gPluginView = new PluginView();
  tree.appendChild(gPluginView.getContainer());

  gMainArea.appendChild(cover);
  gMainArea.appendChild(pane);

  var currentBreadcrumb = gSampleFilters;
  gBreadcrumbTrail.add({
    title: gStrings["Complete Profile"],
    enterCallback: function () {
      gSampleFilters = [];
      filtersChanged();
    }
  });

  if (currentBreadcrumb == null || currentBreadcrumb.length == 0) {
    gTreeManager.restoreSerializedSelectionSnapshot(gRestoreSelection);
    viewOptionsChanged();
  }

  for (var i = 0; i < currentBreadcrumb.length; i++) {
    var filter = currentBreadcrumb[i];
    var forceSelection = null;
    if (gRestoreSelection != null && i == currentBreadcrumb.length - 1) {
      forceSelection = gRestoreSelection;
    }
    switch (filter.type) {
      case "FocusedFrameSampleFilter":
        focusOnSymbol(filter.name, filter.symbolName);
        gBreadcrumbTrail.enterLastItem(forceSelection);
      case "FocusedCallstackPrefixSampleFilter":
        focusOnCallstack(filter.focusedCallstack, filter.name, false);
        gBreadcrumbTrail.enterLastItem(forceSelection);
      case "FocusedCallstackPostfixSampleFilter":
        focusOnCallstack(filter.focusedCallstack, filter.name, true);
        gBreadcrumbTrail.enterLastItem(forceSelection);
      case "RangeSampleFilter":
        gHistogramView.selectRange(filter.start, filter.end);
        gBreadcrumbTrail.enterLastItem(forceSelection);
    }
  }

  toggleJavascriptOnly();
}

function enterProgressUI() {
  var pane = document.createElement("div");
  var label = document.createElement("a");
  var bar = document.createElement("progress");
  var string = gStrings.getStr("profiler.loading");

  pane.className = "profileProgressPane";
  pane.appendChild(label);
  pane.appendChild(bar);

  var reporter = new ProgressReporter();
  reporter.addListener(function (rep) {
    var progress = rep.getProgress();

    if (label.textContent !== string) {
      label.textContent = string;
    }

    if (isNaN(progress)) {
      bar.removeAttribute("value");
    } else {
      bar.value = progress;
    }
  });

  gMainArea.appendChild(pane);
  Parser.updateLogSetting();

  return reporter;
}