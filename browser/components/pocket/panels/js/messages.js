


var Messaging = (function() {

	function addMessageListener(messageId, callback) {

		document.addEventListener('PKT_' + messageId, function(e) {

			callback(JSON.parse(e.target.getAttribute("payload"))[0]);

			
			

		},false);

	}

	function removeMessageListener(messageId, callback) {

		document.removeEventListener('PKT_' + messageId, callback);

	}

	function sendMessage(messageId, payload, callback) {

		
		if (callback) {
			 
	        var messageResponseId = messageId + "Response";
	        var responseListener = function(responsePayload) {
	            callback(responsePayload);
	            removeMessageListener(messageResponseId, responseListener);
	        }

	        addMessageListener(messageResponseId, responseListener);
	    }

	    
		var element = document.createElement("PKTMessageFromPanelElement");
		element.setAttribute("payload", JSON.stringify([payload]));
		document.documentElement.appendChild(element);

		var evt = document.createEvent("Events");
		evt.initEvent('PKT_'+messageId, true, false);
		element.dispatchEvent(evt);
	}


    


    return {
        addMessageListener : addMessageListener,
        removeMessageListener : removeMessageListener,
        sendMessage: sendMessage
    };
}());