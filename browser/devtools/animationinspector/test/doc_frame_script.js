



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










addMessageListener("Test:SetNodeStyle", function(msg) {
  let {propertyName, propertyValue} = msg.data;
  let {node} = msg.objects;

  node.style[propertyName] = propertyValue;

  sendAsyncMessage("Test:SetNodeStyle");
});








addMessageListener("Test:GetAnimationPlayerState", function(msg) {
  let {animationIndex} = msg.data;
  let {node} = msg.objects;

  let player = node.getAnimationPlayers()[animationIndex];
  player.ready.then(() => {
    sendAsyncMessage("Test:GetAnimationPlayerState", player.playState);
  });
});
