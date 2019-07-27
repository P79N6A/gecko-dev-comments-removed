



const TEST_MSG = "ContentSearchTest";
const SERVICE_EVENT_TYPE = "ContentSearchService";
const CLIENT_EVENT_TYPE = "ContentSearchClient";


content.addEventListener(SERVICE_EVENT_TYPE, event => {
  
  
  
  
  
  sendAsyncMessage(TEST_MSG, Components.utils.waiveXrays(event.detail));
});


addMessageListener(TEST_MSG, msg => {
  content.dispatchEvent(
    new content.CustomEvent(CLIENT_EVENT_TYPE, {
      detail: msg.data,
    })
  );
});
