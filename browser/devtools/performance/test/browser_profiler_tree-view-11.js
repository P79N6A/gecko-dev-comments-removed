







const RecordingUtils = devtools.require("devtools/performance/recording-utils");
const { CATEGORY_MASK } = devtools.require("devtools/performance/global");

function* spawnTest() {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, $, $$, window, PerformanceController } = panel.panelWin;
  let { OverviewView, DetailsView, JITOptimizationsView, JsCallTreeView, RecordingsView } = panel.panelWin;

  let profilerData = { threads: [gThread] };

  Services.prefs.setBoolPref(JIT_PREF, true);
  Services.prefs.setBoolPref(PLATFORM_DATA_PREF, false);
  Services.prefs.setBoolPref(INVERT_PREF, false);

  
  
  yield startRecording(panel);
  yield stopRecording(panel);

  yield DetailsView.selectView("js-calltree");

  yield injectAndRenderProfilerData();

  let rows = $$("#js-calltree-view .call-tree-item");
  is(rows.length, 4, "4 call tree rows exist");
  for (let row of rows) {
    let name = $(".call-tree-name", row).value;
    switch (name) {
      case "A":
        ok($(".opt-icon", row), "found an opt icon on a leaf node with opt data");
        break;
      case "C":
        ok(!$(".opt-icon", row), "frames without opt data do not have an icon");
        break;
      case "Gecko":
        ok(!$(".opt-icon", row), "meta category frames with opt data do not have an icon");
        break;
      case "(root)":
        ok(!$(".opt-icon", row), "root frame certainly does not have opt data");
        break;
      default:
        ok(false, `Unidentified frame: ${name}`);
        break;
    }
  }

  yield teardown(panel);
  finish();

  function *injectAndRenderProfilerData() {
    
    info("Injecting mock profile data");
    let recording = PerformanceController.getCurrentRecording();
    recording._profile = profilerData;

    
    let rendered = once(JsCallTreeView, EVENTS.JS_CALL_TREE_RENDERED);
    JsCallTreeView.render(OverviewView.getTimeInterval());
    yield rendered;
  }
}

let gUniqueStacks = new RecordingUtils.UniqueStacks();

function uniqStr(s) {
  return gUniqueStacks.getOrAddStringIndex(s);
}




let gThread = RecordingUtils.deflateThread({
  samples: [{
    time: 0,
    frames: [
      { location: "(root)" }
    ]
  }, {
    time: 5,
    frames: [
      { location: "(root)" },
      { location: "A (http://foo:1)" },
    ]
  }, {
    time: 5 + 1,
    frames: [
      { location: "(root)" },
      { location: "C (http://foo/bar/baz:56)" }
    ]
  }, {
    time: 5 + 1 + 2,
    frames: [
      { location: "(root)" },
      { category: CATEGORY_MASK("other"),  location: "PlatformCode" }
    ]
  }],
  markers: []
}, gUniqueStacks);


let gRawSite1 = {
  _testFrameInfo: { name: "A", line: "12", file: "@baz" },
  line: 12,
  column: 2,
  types: [{
    mirType: uniqStr("Object"),
    site: uniqStr("A (http://foo/bar/bar:12)"),
    typeset: [{
        keyedBy: uniqStr("constructor"),
        name: uniqStr("Foo"),
        location: uniqStr("A (http://foo/bar/baz:12)")
    }, {
        keyedBy: uniqStr("primitive"),
        location: uniqStr("self-hosted")
    }]
  }],
  attempts: {
    schema: {
      outcome: 0,
      strategy: 1
    },
    data: [
      [uniqStr("Failure1"), uniqStr("SomeGetter1")],
      [uniqStr("Failure2"), uniqStr("SomeGetter2")],
      [uniqStr("Failure3"), uniqStr("SomeGetter3")]
    ]
  }
};

gThread.frameTable.data.forEach((frame) => {
  const LOCATION_SLOT = gThread.frameTable.schema.location;
  const OPTIMIZATIONS_SLOT = gThread.frameTable.schema.optimizations;

  let l = gThread.stringTable[frame[LOCATION_SLOT]];
  switch (l) {
  case "A (http://foo:1)":
    frame[LOCATION_SLOT] = uniqStr("A (http://foo:1)");
    frame[OPTIMIZATIONS_SLOT] = gRawSite1;
    break;
  case "PlatformCode":
    frame[LOCATION_SLOT] = uniqStr("PlatformCode");
    frame[OPTIMIZATIONS_SLOT] = gRawSite1;
    break;
  }
});
