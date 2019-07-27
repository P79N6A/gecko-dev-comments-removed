



"use strict";











addMessageListener("Test:ToggleAnimationPlayer", function(msg) {
  let {selector, animationIndex, pause} = msg.data;
  let node = superQuerySelector(selector);
  if (!node) {
    return;
  }

  let animation = node.getAnimations()[animationIndex];
  if (pause) {
    animation.pause();
  } else {
    animation.play();
  }

  sendAsyncMessage("Test:ToggleAnimationPlayer");
});









addMessageListener("Test:SetAnimationPlayerCurrentTime", function(msg) {
  let {selector, animationIndex, currentTime} = msg.data;
  let node = superQuerySelector(selector);
  if (!node) {
    return;
  }

  let animation = node.getAnimations()[animationIndex];
  animation.currentTime = currentTime;

  sendAsyncMessage("Test:SetAnimationPlayerCurrentTime");
});









addMessageListener("Test:SetAnimationPlayerPlaybackRate", function(msg) {
  let {selector, animationIndex, playbackRate} = msg.data;
  let node = superQuerySelector(selector);
  if (!node) {
    return;
  }

  let player = node.getAnimations()[animationIndex];
  player.playbackRate = playbackRate;

  sendAsyncMessage("Test:SetAnimationPlayerPlaybackRate");
});








addMessageListener("Test:GetAnimationPlayerState", function(msg) {
  let {selector, animationIndex} = msg.data;
  let node = superQuerySelector(selector);
  if (!node) {
    return;
  }

  let animation = node.getAnimations()[animationIndex];
  animation.ready.then(() => {
    sendAsyncMessage("Test:GetAnimationPlayerState", animation.playState);
  });
});











function superQuerySelector(superSelector, root=content.document) {
  let frameIndex = superSelector.indexOf("||");
  if (frameIndex === -1) {
    return root.querySelector(superSelector);
  } else {
    let rootSelector = superSelector.substring(0, frameIndex).trim();
    let childSelector = superSelector.substring(frameIndex+2).trim();
    root = root.querySelector(rootSelector);
    if (!root || !root.contentWindow) {
      return null;
    }

    return superQuerySelector(childSelector, root.contentWindow.document);
  }
}
