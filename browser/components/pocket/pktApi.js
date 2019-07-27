











































var pktApi = (function() {

    



    
    
    var pocketAPIhost = Services.prefs.getCharPref("browser.pocket.hostname");

    
    var baseAPIUrl = "https://" + pocketAPIhost + "/v3";


    


    var oAuthConsumerKey = Services.prefs.getCharPref("browser.pocket.oAuthConsumerKey");

	


	var prefBranch = Services.prefs.getBranch("browser.pocket.settings.");

    



    var extend = function(out) {
        out = out || {};

        for (var i = 1; i < arguments.length; i++) {
            if (!arguments[i])
                continue;

            for (var key in arguments[i]) {
                if (arguments[i].hasOwnProperty(key))
                    out[key] = arguments[i][key];
                }
            }
        return out;
    }

    



    






     function getSetting(key) {
     	
     	
     	
		if (!prefBranch.prefHasUserValue(key))
			return;
		
		return prefBranch.getComplexValue(key, Components.interfaces.nsISupportsString).data;
     }

     






    function setSetting(key, value) {
     	
     	
     	
     	if (!value)
     		prefBranch.clearUserPref(key);
     	else
     	{
     		
     		var str = Components.classes["@mozilla.org/supports-string;1"].createInstance(Components.interfaces.nsISupportsString);
			str.data = value;
     		prefBranch.setComplexValue(key, Components.interfaces.nsISupportsString, str);
     	}
    }

    



    



    function getCookiesFromPocket() {
    
        var cookieManager = Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager2);
        var pocketCookies = cookieManager.getCookiesFromHost(pocketAPIhost);
        var cookies = {};
        while (pocketCookies.hasMoreElements()) {
            var cookie = pocketCookies.getNext().QueryInterface(Ci.nsICookie2);
            cookies[cookie.name] = cookie.value;
        }
        return cookies;
    }

    



    function getAccessToken() {
        var pocketCookies = getCookiesFromPocket();

        
        if (typeof pocketCookies['ftv1'] === "undefined") {
            return undefined;
        }

        
        var sessionId = pocketCookies['fsv1'];
        var lastSessionId = getSetting('fsv1');
        if (sessionId !== lastSessionId) {
            clearUserData();
            setSetting("fsv1", sessionId);
        }

        
        return pocketCookies['ftv1'];
    }

    



    function isUserLoggedIn() {
        return (typeof getAccessToken() !== "undefined");
    }

    



    

















    function apiRequest(options) {
        if ((typeof options === "undefined") || (typeof options.path === "undefined")) {
            return false;
        }

        var url = baseAPIUrl + options.path;
        var data = options.data || {};
        data.locale_lang = window.navigator.language;
        data.consumer_key = oAuthConsumerKey;

        var request = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Components.interfaces.nsIXMLHttpRequest);
        request.open("POST", url, true);
		request.onreadystatechange = function(e){
			if (request.readyState == 4) {
				if (request.status === 200) {
                    if (options.success) {
                        options.success(JSON.parse(request.response), request);
                    }
                    return;
                }

                
                if (options.error) {
                    
                    var errorMessage = request.getResponseHeader("X-Error");
                    if (typeof errorMessage !== "undefined") {
                        options.error(new Error(errorMessage), request);
                        return;
                    }

                    
                    options.error(new Error(request.statusText), request);
                }
			}
		};

		
		

		
		request.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded; charset=UTF-8');
		request.setRequestHeader('X-Accept',' application/json');

        
		var str = [];
		for(var p in data) {
			if (data.hasOwnProperty(p)) {
				str.push(encodeURIComponent(p) + "=" + encodeURIComponent(data[p]));
			}
		}

		request.send(str.join("&"));

        return true;
    }

    


    function clearUserData() {
        
        setSetting("premium_status", undefined);
        setSetting("latestSince", undefined);
        setSetting("tags", undefined);
        setSetting("usedTags", undefined);

    }

    






    function addLink(url, options) {

        var since = getSetting('latestSince');
        var accessToken = getAccessToken();

        var sendData = {
            access_token: accessToken,
            url: url,
            since: since ? since : 0
        };

        var title = options.title;
        if (title !== "undefined") {
            sendData.title = title;
        };

        return apiRequest({
            path: "/firefox/save",
            data: sendData,
            success: function(data) {

                
                var tags = data.tags;
                if ((typeof tags !== "undefined") && Array.isArray(tags)) {
                    
                    setSetting('tags', JSON.stringify(data.tags));
                }

                
                var premiumStatus = data.premium_status;
                if (typeof premiumStatus !== "undefined") {
                    
                    setSetting("premium_status", premiumStatus);
                }

                
                setSetting('latestSince', data.since);

                if (options.success) {
                    options.success.apply(options, Array.apply(null, arguments));
                }
            },
            error: options.error
        });

        return sendAction(action, options);
    }

    







    function deleteItem(itemId, options) {
        var action = {
            action: "delete",
            item_id: itemId
        };
        return sendAction(action, options);
    }

    









    function sendAction(action, options) {
        
        
        if (typeof options.actionInfo !== 'undefined') {
            action = extend(action, options.actionInfo);
        }
        return sendActions([action], options);
    }

    






    function sendActions(actions, options) {
        return apiRequest({
            path: "/send",
            data: {
                access_token: getAccessToken(),
                actions: JSON.stringify(actions)
            },
            success: options.success,
            error: options.error
        });
    }

    



    









    function addTagsToItem(itemId, tags, options) {
        return addTags({item_id: itemId}, tags, options);
    }

    









    function addTagsToURL(url, tags, options) {
        return addTags({url: url}, tags, options);
    }

    









    function addTags(actionPart, tags, options) {
        
        var action = {
            action: "tags_add",
            tags: tags
        };
        action = extend(action, actionPart);

        
        var finalSuccessCallback = options.success;

        
        options.success = function(data) {

            
            var usedTagsJSON = getSetting("usedTags");
            var usedTags = usedTagsJSON ? JSON.parse(usedTagsJSON) : {};

            
            for (var i = 0; i < tags.length; i++) {
                var tagToSave = tags[i].trim();
                var newUsedTagObject = {
                    "tag": tagToSave,
                    "timestamp": new Date()
                };
                usedTags[tagToSave] = newUsedTagObject;
            }
            setSetting("usedTags", JSON.stringify(usedTags));

            
            if (finalSuccessCallback) {
                finalSuccessCallback(data);
            }
        };

        
        return sendAction(action, options);
    }

    




    function getTags(callback) {

        var tagsFromSettings = function() {
            var tagsJSON = getSetting("tags");
            if (typeof tagsJSON !== "undefined") {
                return JSON.parse(tagsJSON)
            }
            return [];
        }

        var sortedUsedTagsFromSettings = function() {
            
            var usedTags = [];

            var usedTagsJSON = getSetting("usedTags");
            if (typeof usedTagsJSON !== "undefined") {
                var usedTagsObject = JSON.parse(usedTagsJSON);
                var usedTagsObjectArray = [];
                for (var tagKey in usedTagsObject) {
                    usedTagsObjectArray.push(usedTagsObject[tagKey]);
                }

                
                usedTagsObjectArray.sort(function(a, b) {
                    a = new Date(a.timestamp);
                    b = new Date(b.timestamp);
                    return a < b ? -1 : a > b ? 1 : 0;
                });

                
                for (var j = 0; j < usedTagsObjectArray.length; j++) {
                    usedTags.push(usedTagsObjectArray[j].tag);
                }

                
                usedTags.reverse();
            }

            return usedTags;
        }

        if (callback) {
            var tags = tagsFromSettings();
            var usedTags = sortedUsedTagsFromSettings();
            callback(tags, usedTags);
        }
    }

    


    function isPremiumUser() {
        return getSetting('premium_status') == 1;
    }

    







    function getSuggestedTagsForItem(itemId, options) {
        return getSuggestedTags({item_id: itemId}, options);
    }

    







    function getSuggestedTagsForURL(url, options) {
        return getSuggestedTags({url: url}, options);
    }

    



    function getSuggestedTags(data, options) {

        data = data || {};
        options = options || {};

        data.access_token = getAccessToken();

        return apiRequest({
            path: "/getSuggestedTags",
            data: data,
            success: options.success,
            error: options.error
        });
    }

    


    return {
        isUserLoggedIn : isUserLoggedIn,
        clearUserData: clearUserData,
        addLink: addLink,
        deleteItem: deleteItem,
        addTagsToItem: addTagsToItem,
        addTagsToURL: addTagsToURL,
        getTags: getTags,
        isPremiumUser: isPremiumUser,
        getSuggestedTagsForItem: getSuggestedTagsForItem,
        getSuggestedTagsForURL: getSuggestedTagsForURL
    };
}());