






const TEST_URL = EXAMPLE_URL + "doc_innerHTML.html"

function* getMarkers(front) {
  const { promise, resolve } = Promise.defer();
  const handler = (_, name, markers) => {
    if (name === "markers") {
      resolve(markers);
    }
  };
  front.on("timeline-data", handler);

  yield front.startRecording({ withMarkers: true, withTicks: true });

  const markers = yield promise;
  front.off("timeline-data", handler);
  yield front.stopRecording();

  return markers;
}

function* spawnTest () {
  let { target, front } = yield initBackend(TEST_URL);

  const markers = yield getMarkers(front);
  info("markers = " + JSON.stringify(markers, null, 2));
  ok(markers.some(m => m.name === "Parse HTML" && m.stack != undefined),
     "Should get some Parse HTML markers");

  yield removeTab(target.tab);
  finish();
}
