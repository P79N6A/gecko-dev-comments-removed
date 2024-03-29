








function run_test() {
  run_next_test();
}

add_task(function test() {
  let { JITOptimizations } = devtools.require("devtools/performance/jit");

  let rawSites = [];
  rawSites.push(gRawSite2);
  rawSites.push(gRawSite2);
  rawSites.push(gRawSite1);
  rawSites.push(gRawSite1);
  rawSites.push(gRawSite2);
  rawSites.push(gRawSite3);

  let jit = new JITOptimizations(rawSites, gStringTable.stringTable);
  let sites = jit.optimizationSites;

  let [first, second, third] = sites;

  equal(first.id, 0, "site id is array index");
  equal(first.samples, 3, "first OptimizationSiteProfile has correct sample count");
  equal(first.data.line, 34, "includes OptimizationSite as reference under `data`");
  equal(second.id, 1, "site id is array index");
  equal(second.samples, 2, "second OptimizationSiteProfile has correct sample count");
  equal(second.data.line, 12, "includes OptimizationSite as reference under `data`");
  equal(third.id, 2, "site id is array index");
  equal(third.samples, 1, "third OptimizationSiteProfile has correct sample count");
  equal(third.data.line, 78, "includes OptimizationSite as reference under `data`");
});

let gStringTable = new RecordingUtils.UniqueStrings();

function uniqStr(s) {
  return gStringTable.getOrAddStringIndex(s);
}

let gRawSite1 = {
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
      [uniqStr("Inlined"), uniqStr("SomeGetter3")]
    ]
  }
};

let gRawSite2 = {
  line: 34,
  types: [{
    mirType: uniqStr("Int32"),
    site: uniqStr("Receiver")
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

let gRawSite3 = {
  line: 78,
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
      [uniqStr("GenericSuccess"), uniqStr("SomeGetter3")]
    ]
  }
};
