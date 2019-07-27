















Components.utils.import('resource://gre/modules/Services.jsm');
Components.utils.import('chrome://shumway/content/ShumwayCom.jsm');

var externalInterfaceWrapper = {
  callback: function (call) {
    if (!shumwayComAdapter.onExternalCallback) {
      return undefined;
    }
    return shumwayComAdapter.onExternalCallback(
      Components.utils.cloneInto(JSON.parse(call), content));
  }
};



var shumwayComAdapter;

function sendMessage(action, data, sync) {
  var detail = {action: action, data: data, sync: sync};
  if (!sync) {
    sendAsyncMessage('Shumway:message', detail);
    return;
  }
  var result = String(sendSyncMessage('Shumway:message', detail));
  result = result == 'undefined' ? undefined : JSON.parse(result);
  return Components.utils.cloneInto(result, content);
}

function enableDebug() {
  sendAsyncMessage('Shumway:enableDebug', null);
}

addMessageListener('Shumway:init', function (message) {
  var environment = message.data;

  sendAsyncMessage('Shumway:running', {}, {
    externalInterface: externalInterfaceWrapper
  });

  shumwayComAdapter = ShumwayCom.createAdapter(content, {
    sendMessage: sendMessage,
    enableDebug: enableDebug,
    getEnvironment: function () { return environment; }
  });

  content.wrappedJSObject.runViewer();
});

addMessageListener('Shumway:loadFile', function (message) {
  if (!shumwayComAdapter.onLoadFileCallback) {
    return;
  }
  shumwayComAdapter.onLoadFileCallback(Components.utils.cloneInto(message.data, content));
});
