

addMessageListener("foo", function (message) {
  sendAsyncMessage("bar", message);
});

