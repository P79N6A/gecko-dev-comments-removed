



"use strict";










addMessageListener("Test:ToggleAnimationPlayer", function(msg) {
  let {animationIndex, pause} = msg.data;
  let {node} = msg.objects;

  let player = node.getAnimationPlayers()[animationIndex];
  if (pause) {
    player.pause();
  } else {
    player.play();
  }

  sendAsyncMessage("Test:ToggleAnimationPlayer");
});
