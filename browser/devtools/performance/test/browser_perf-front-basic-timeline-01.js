






function spawnTest () {
  let { target, front } = yield initBackend(SIMPLE_URL);

  let lastMemoryDelta = 0;
  let lastTickDelta = 0;

  let counters = {
    markers: [],
    memory: [],
    ticks: []
  };

  let deferreds = {
    markers: Promise.defer(),
    memory: Promise.defer(),
    ticks: Promise.defer()
  };

  front.on("markers", handler);
  front.on("memory", handler);
  front.on("ticks", handler);

  yield front.startRecording({ withMemory: true, withTicks: true });

  yield Promise.all(Object.keys(deferreds).map(type => deferreds[type].promise));

  yield front.stopRecording();

  is(counters.markers.length, 1, "one marker event fired.");
  is(counters.memory.length, 3, "three memory events fired.");
  is(counters.ticks.length, 3, "three ticks events fired.");

  yield removeTab(target.tab);
  finish();

  function handler (name, ...args) {
    if (name === "markers") {
      let [markers] = args;
      ok(markers[0].start, "received atleast one marker with `start`");
      ok(markers[0].end, "received atleast one marker with `end`");
      ok(markers[0].name, "received atleast one marker with `name`");

      counters.markers.push(markers);
      front.off(name, handler);
      deferreds[name].resolve();
    }
    else if (name === "memory") {
      let [delta, measurement] = args;
      is(typeof delta, "number", "received `delta` in memory event");
      ok(delta > lastMemoryDelta, "received `delta` in memory event");
      ok(measurement.total, "received `total` in memory event");

      counters.memory.push({ delta, measurement });
      lastMemoryDelta = delta;
    }
    else if (name === "ticks") {
      let [delta, timestamps] = args;
      ok(delta > lastTickDelta, "received `delta` in ticks event");

      
      

      counters.ticks.push({ delta, timestamps });
      lastTickDelta = delta;
    }
    else {
      throw new Error("unknown event " + name);
    }

    if (name === "markers" && counters[name].length === 1 ||
        name === "memory" && counters[name].length === 3 ||
        name === "ticks" && counters[name].length === 3) {
      front.off(name, handler);
      deferreds[name].resolve();
    }
  };
}
