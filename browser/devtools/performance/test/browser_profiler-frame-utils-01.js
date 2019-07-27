







const CONTENT_LOCATIONS = [
  "hello/<.world (https://foo/bar.js:123:987)",
  "hello/<.world (http://foo/bar.js:123:987)",
  "hello/<.world (http://foo/bar.js:123)",
  "hello/<.world (http://foo/bar.js#baz:123:987)",
  "hello/<.world (http://foo/bar.js?myquery=params&search=1:123:987)",
  "hello/<.world (http://foo/#bar:123:987)",
  "hello/<.world (http://foo/:123:987)",
  "hello/<.world (app://myfxosapp/file.js:100:1)",

  
  "hello/<.world (http://localhost:8888/file.js:100:1)",
  "hello/<.world (http://localhost:8888/file.js:100)",

  
  
  "hello/<.world (http://localhost:8888/:1",
  "hello/<.world (http://localhost:8888/:100:50",
].map(argify);

const CHROME_LOCATIONS = [
  { location: "Startup::XRE_InitChildProcess", line: 456, column: 123 },
  { location: "chrome://browser/content/content.js", line: 456, column: 123 },
  "setTimeout_timer (resource://gre/foo.js:123:434)",
  "hello/<.world (jar:file://Users/mcurie/Dev/jetpacks.js)",
  "hello/<.world (resource://foo.js -> http://bar/baz.js:123:987)",
  "EnterJIT",
].map(argify);

function test() {
  const { isContent, parseLocation } = devtools.require("devtools/performance/frame-utils");

  for (let frame of CONTENT_LOCATIONS) {
    ok(isContent.apply(null, frameify(frame)), `${frame[0]} should be considered a content frame.`);
  }

  for (let frame of CHROME_LOCATIONS) {
    ok(!isContent.apply(null, frameify(frame)), `${frame[0]} should not be considered a content frame.`);
  }

  
  const FIELDS = ["functionName", "fileName", "hostName", "url", "line", "column", "host", "port"];
  const PARSED_CONTENT = [
    ["hello/<.world", "bar.js", "foo", "https://foo/bar.js", 123, 987, "foo", null],
    ["hello/<.world", "bar.js", "foo", "http://foo/bar.js", 123, 987, "foo", null],
    ["hello/<.world", "bar.js", "foo", "http://foo/bar.js", 123, null, "foo", null],
    ["hello/<.world", "bar.js", "foo", "http://foo/bar.js#baz", 123, 987, "foo", null],
    ["hello/<.world", "bar.js", "foo", "http://foo/bar.js?myquery=params&search=1", 123, 987, "foo", null],
    ["hello/<.world", "/", "foo", "http://foo/#bar", 123, 987, "foo", null],
    ["hello/<.world", "/", "foo", "http://foo/", 123, 987, "foo", null],
    ["hello/<.world", "file.js", "myfxosapp", "app://myfxosapp/file.js", 100, 1, "myfxosapp", null],
    ["hello/<.world", "file.js", "localhost", "http://localhost:8888/file.js", 100, 1, "localhost:8888", 8888],
    ["hello/<.world", "file.js", "localhost", "http://localhost:8888/file.js", 100, null, "localhost:8888", 8888],
    ["hello/<.world", "/", "localhost", "http://localhost:8888/", 1, null, "localhost:8888", 8888],
    ["hello/<.world", "/", "localhost", "http://localhost:8888/", 100, 50, "localhost:8888", 8888],
  ];

  for (let i = 0; i < PARSED_CONTENT.length; i++) {
    let parsed = parseLocation.apply(null, CONTENT_LOCATIONS[i]);
    for (let j = 0; j < FIELDS.length; j++) {
      is(parsed[FIELDS[j]], PARSED_CONTENT[i][j], `${CONTENT_LOCATIONS[i]} was parsed to correct ${FIELDS[j]}`);
    }
  }

  const PARSED_CHROME = [
    ["Startup::XRE_InitChildProcess", null, null, null, 456, 123, null, null],
    ["chrome://browser/content/content.js", null, null, null, 456, 123, null, null],
    ["setTimeout_timer", "foo.js", null, "resource://gre/foo.js", 123, 434, null, null],
    ["hello/<.world (jar:file://Users/mcurie/Dev/jetpacks.js)", null, null, null, null, null, null, null],
    ["hello/<.world", "baz.js", "bar", "http://bar/baz.js", 123, 987, "bar", null],
    ["EnterJIT", null, null, null, null, null, null, null],
  ];

  for (let i = 0; i < PARSED_CHROME.length; i++) {
    let parsed = parseLocation.apply(null, CHROME_LOCATIONS[i]);
    for (let j = 0; j < FIELDS.length; j++) {
      is(parsed[FIELDS[j]], PARSED_CHROME[i][j], `${CHROME_LOCATIONS[i]} was parsed to correct ${FIELDS[j]}`);
    }
  }

  finish();
}





function argify (val) {
  if (typeof val === "string") {
    return [val];
  } else {
    return [val.location, val.line, val.column];
  }
}





function frameify(val) {
  return [{ location: val[0] }];
}
