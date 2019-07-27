







function* spawnTest() {
  let { target, panel } = yield initPerformance(MARKERS_URL);
  let { $, $$, EVENTS, PerformanceController, OverviewView, WaterfallView } = panel.panelWin;

  
  
  
  
  WaterfallView._prepareWaterfallTree = markers => {
    return { submarkers: markers };
  };

  const MARKER_TYPES = [
    "Styles", "Reflow", "Paint", "ConsoleTime", "TimeStamp"
  ];

  yield startRecording(panel);
  ok(true, "Recording has started.");

  yield waitUntil(() => {
    
    let markers = PerformanceController.getCurrentRecording().getMarkers();
    return MARKER_TYPES.every(type => markers.some(m => m.name === type));
  });

  yield stopRecording(panel);
  ok(true, "Recording has ended.");

  info("No need to select everything in the timeline.");
  info("All the markers should be displayed by default.");

  let bars = $$(".waterfall-marker-bar");
  let markers = PerformanceController.getCurrentRecording().getMarkers();

  ok(bars.length >= MARKER_TYPES.length, `Got at least ${MARKER_TYPES.length} markers (1)`);
  ok(markers.length >= MARKER_TYPES.length, `Got at least ${MARKER_TYPES.length} markers (2)`);

  const tests = {
    ConsoleTime: function (marker) {
      info("Got `ConsoleTime` marker with data: " + JSON.stringify(marker));
      shouldHaveStack($, "startStack", marker);
      shouldHaveStack($, "endStack", marker);
      shouldHaveLabel($, "Timer Name:", "!!!", marker);
      return true;
    },
    TimeStamp: function (marker) {
      info("Got `TimeStamp` marker with data: " + JSON.stringify(marker));
      shouldHaveLabel($, "Label:", "go", marker);
      shouldHaveStack($, "stack", marker);
      return true;
    },
    Styles: function (marker) {
      info("Got `Styles` marker with data: " + JSON.stringify(marker));
      if (marker.restyleHint) {
        shouldHaveLabel($, "Restyle Hint:", marker.restyleHint.replace(/eRestyle_/g, ""), marker);
      }
      if (marker.stack) {
        shouldHaveStack($, "stack", marker);
        return true;
      }
    },
    Reflow: function (marker) {
      info("Got `Reflow` marker with data: " + JSON.stringify(marker));
      if (marker.stack) {
        shouldHaveStack($, "stack", marker);
        return true;
      }
    }
  };

  
  
  
  let testsDone = [];

  for (let i = 0; i < bars.length; i++) {
    let bar = bars[i];
    let m = markers[i];
    EventUtils.sendMouseEvent({ type: "mousedown" }, bar);

    if (tests[m.name]) {
      if (testsDone.indexOf(m.name) === -1) {
        let fullTestComplete = tests[m.name](m);
        if (fullTestComplete) {
          testsDone.push(m.name);
        }
      }
    } else {
      info(`TODO: Need to add marker details tests for ${m.name}`);
    }

    if (testsDone.length === Object.keys(tests).length) {
      break;
    }
  }

  yield teardown(panel);
  finish();
}

function shouldHaveStack ($, type, marker) {
  ok($(`#waterfall-details .marker-details-stack[type=${type}]`), `${marker.name} has a stack: ${type}`);
}

function shouldHaveLabel ($, name, value, marker) {
  info(name);
  let $name = $(`#waterfall-details .marker-details-labelcontainer .marker-details-labelname[value="${name}"]`);
  let $value = $name.parentNode.querySelector(".marker-details-labelvalue");
  is($value.getAttribute("value"), value, `${marker.name} has correct label for ${name}:${value}`);
}
