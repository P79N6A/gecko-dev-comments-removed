







function test() {
  let { FrameNode } = devtools.require("devtools/profiler/tree-model");

  ok(FrameNode.isContent({ location: "http://foo" }),
    "Verifying content/chrome frames is working properly.");
  ok(FrameNode.isContent({ location: "https://foo" }),
    "Verifying content/chrome frames is working properly.");
  ok(FrameNode.isContent({ location: "file://foo" }),
    "Verifying content/chrome frames is working properly.");

  ok(!FrameNode.isContent({ location: "chrome://foo" }),
    "Verifying content/chrome frames is working properly.");
  ok(!FrameNode.isContent({ location: "resource://foo" }),
    "Verifying content/chrome frames is working properly.");

  ok(!FrameNode.isContent({ location: "chrome://foo -> http://bar" }),
    "Verifying content/chrome frames is working properly.");
  ok(!FrameNode.isContent({ location: "chrome://foo -> https://bar" }),
    "Verifying content/chrome frames is working properly.");
  ok(!FrameNode.isContent({ location: "chrome://foo -> file://bar" }),
    "Verifying content/chrome frames is working properly.");

  ok(!FrameNode.isContent({ location: "resource://foo -> http://bar" }),
    "Verifying content/chrome frames is working properly.");
  ok(!FrameNode.isContent({ location: "resource://foo -> https://bar" }),
    "Verifying content/chrome frames is working properly.");
  ok(!FrameNode.isContent({ location: "resource://foo -> file://bar" }),
    "Verifying content/chrome frames is working properly.");

  ok(!FrameNode.isContent({ category: 1, location: "chrome://foo" }),
    "Verifying content/chrome frames is working properly.");
  ok(!FrameNode.isContent({ category: 1, location: "resource://foo" }),
    "Verifying content/chrome frames is working properly.");

  ok(!FrameNode.isContent({ category: 1, location: "file://foo -> http://bar" }),
    "Verifying content/chrome frames is working properly.");
  ok(!FrameNode.isContent({ category: 1, location: "file://foo -> https://bar" }),
    "Verifying content/chrome frames is working properly.");
  ok(!FrameNode.isContent({ category: 1, location: "file://foo -> file://bar" }),
    "Verifying content/chrome frames is working properly.");

  finish();
}
