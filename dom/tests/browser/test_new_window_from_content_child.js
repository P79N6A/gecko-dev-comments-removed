




function handleClickItem(aMessage) {
  let itemId = aMessage.data.details;
  content.console.log("Getting item with ID: " + itemId);
  let item = content.document.getElementById(itemId);
  item.click();
}

function handleAllowUnload(aMessage) {
  content.onbeforeunload = null;
  sendSyncMessage("TEST:allow-unload:done");
}

addMessageListener("TEST:click-item", handleClickItem);
addMessageListener("TEST:allow-unload", handleAllowUnload);