


addMessageListener("AboutHome:SearchTriggered", function (msg) {
  sendAsyncMessage("AboutHomeTest:CheckRecordedSearch", msg.data);
});
