


"use strict";

const { Ci } = require("chrome");
const { InputPort } = require("./system");
const { getFrameElement, getOuterId,
        getOwnerBrowserWindow } = require("../window/utils");
const { isnt } = require("../lang/functional");
const { foldp, lift, merges, keepIf } = require("../event/utils");
const { object } = require("../util/sequence");
const { compose } = require("../lang/functional");
const { LastClosed } = require("./browser");
const { patch } = require("diffpatcher/index");

const Document = Ci.nsIDOMDocument;

const isntNull = isnt(null);

const frameID = frame => frame.id;
const browserID = compose(getOuterId, getOwnerBrowserWindow);

const isInnerFrame = frame =>
  frame && frame.hasAttribute("data-is-sdk-inner-frame");





const getFrame = document =>
  document && document.defaultView && getFrameElement(document.defaultView);

const FrameInput = function(options) {
  const input = keepIf(isInnerFrame, null,
                       lift(getFrame, new InputPort(options)));
  return lift(frame => {
    if (!frame) return frame;
    const [id, owner] = [frameID(frame), browserID(frame)];
    return object([id, {owners: object([owner, options.update])}]);
  }, input);
};

const LastLoading = new FrameInput({topic: "document-element-inserted",
                                    update: {readyState: "loading"}});
exports.LastLoading = LastLoading;

const LastInteractive = new FrameInput({topic: "content-document-interactive",
                                        update: {readyState: "interactive"}});
exports.LastInteractive = LastInteractive;

const LastLoaded = new FrameInput({topic: "content-document-loaded",
                                   update: {readyState: "complete"}});
exports.LastLoaded = LastLoaded;

const LastUnloaded = new FrameInput({topic: "content-page-hidden",
                                    update: null});
exports.LastUnloaded = LastUnloaded;

















const Frames = foldp(patch, {}, merges([
  LastLoading,
  LastInteractive,
  LastLoaded,
  LastUnloaded,
  new InputPort({ id: "frame-mailbox" }),
  new InputPort({ id: "frame-change" }),
  new InputPort({ id: "frame-changed" })
]));
exports.Frames = Frames;
