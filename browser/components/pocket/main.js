











































var pktUI = (function() {

	
	var inited = false;
	var currentPanelDidShow, currentPanelDidHide;
	var _isHidden = false;
	
	var _notificationTimeout;
	
	var prefBranch = Services.prefs.getBranch("browser.pocket.settings.");
	
	


	function onLoad() {
		
		if (inited)
			return;
		
		
		
		
		
		if (!prefBranch.prefHasUserValue('installed')) {
		
			
			if (Social.getManifestByOrigin("https://getpocket.com")) {
				Social.uninstallProvider("https://getpocket.com", function(){  });
			}
			
			
			prefBranch.setBoolPref('hasLegacyExtension', hasLegacyExtension());
		
			var id = "pocket-menu-button";
	        var toolbar = document.getElementById("nav-bar");
			
	        var before = null;
			
			if (toolbar.currentSet.match("bookmarks-menu-button")) {
	            var elem = document.getElementById("bookmarks-menu-button");
	            if (elem)
					before = elem.nextElementSibling;
			}
			
			
        	toolbar.insertItem(id, before);
	
	        toolbar.setAttribute("currentset", toolbar.currentSet);
	        document.persist(toolbar.id, "currentset");
	        
	        prefBranch.setBoolPref('installed', true);
		}
		
		
		document.getElementById('contentAreaContextMenu').addEventListener("popupshowing", contextOnPopupShowing, false);
		
		
		hideIntegrationIfNeeded();
		
		inited = true;
	}
	
	


	function onUnload() {
	
	}
	
	


	function hideIntegrationIfNeeded() {
		
		var hideIntegration = false;
		
		
		if (prefBranch.getBoolPref('hasLegacyExtension')) {
			if (hasLegacyExtension()) {
				hideIntegration = true; 
			}
			else {
				
				prefBranch.setBoolPref('hasLegacyExtension', false);
			}
		}
		
		
		
		
		
		
		
		if (hideIntegration) {
			
			var elements = ['pocket-menu-button', 'BMB_openPocketWebapp'];
			for(var i=0; i<elements.length; i++) {
				document.getElementById(elements[i]).setAttribute('hidden', true);
			}
			
			_isHidden = true;
		}
		else
			_isHidden = false
	}
	
	
	
    
    


    function pocketButtonOnCommand(event) {
        tryToSaveCurrentPage();
    }
    
    function pocketPanelDidShow(event) {
    	if (currentPanelDidShow) {
    		currentPanelDidShow(event);
        }
    	
    }
    
    function pocketPanelDidHide(event) {
    	if (currentPanelDidHide) {
    		currentPanelDidHide(event);
        }
        
        
        getPanelFrame().setAttribute('src', 'about:blank');
    }


    


     function pocketBookmarkBarOpenPocketCommand(event) {
        openTabWithUrl('https://getpocket.com/a/', true);
     }

    



	
	function contextOnPopupShowing() {

		var saveLinkId = "PKT_context_saveLink";
		var savePageId = "PKT_context_savePage";

		if (isHidden()) {
			gContextMenu.showItem(saveLinkId, false);
			gContextMenu.showItem(savePageId, false);
	    } else if ( (gContextMenu.onSaveableLink || ( gContextMenu.inDirList && gContextMenu.onLink )) ) {
			gContextMenu.showItem(saveLinkId, true);
			gContextMenu.showItem(savePageId, false);
	    } else if (gContextMenu.isTextSelected) {
			gContextMenu.showItem(saveLinkId, false);
			gContextMenu.showItem(savePageId, false);
	    } else if (!gContextMenu.onTextInput) {
			gContextMenu.showItem(saveLinkId, false);
			gContextMenu.showItem(savePageId, true);
	    } else {
			gContextMenu.showItem(saveLinkId, false);
			gContextMenu.showItem(savePageId, false);
	    }
	}

    function pocketContextSaveLinkOnCommand(event) {
    	
        var linkNode = gContextMenu.target || document.popupNode;

        
        if (linkNode.nodeType == Node.TEXT_NODE) {
            linkNode = linkNode.parentNode;
        }

        
        if (linkNode.nodeType != Node.ELEMENT_NODE) {
            return;
        }

        
        
        var currentElement = linkNode;
        while (currentElement !== null) {
            if (currentElement.nodeType == Node.ELEMENT_NODE &&
                currentElement.nodeName.toLowerCase() == 'a')
            {
                
                linkNode = currentElement;
                break;
            }
            currentElement = currentElement.parentNode;
        }

        var link = linkNode.href;
        tryToSaveUrl(link);

        event.stopPropagation();
    }

    function pocketContextSavePageOnCommand(event) {
        tryToSaveCurrentPage();
    }


    

    


	function tryToSaveCurrentPage() {
		tryToSaveUrl(getCurrentUrl(), getCurrentTitle());
	}
     
    function tryToSaveUrl(url, title) {

    	
    	if (pktApi.isUserLoggedIn()) {
    		saveAndShowConfirmation(url, title);
            return;
    	}

    	
    	showSignUp();
    }


    

    


    function showSignUp() {
    	showPanel("chrome://browser/content/pocket/panels/signup.html", {
    		onShow: function() {
                
                resizePanel({
                        width: 300,
                        height: 550
                });
            },
			onHide: panelDidHide,
    	});
    }

    


    function saveAndShowConfirmation(url, title) {

        
        
        if (typeof url === 'undefined') { return; }
        if (!url.startsWith("http") && !url.startsWith('https')) { return; };

    	showPanel("chrome://browser/content/pocket/panels/saved.html?premiumStatus=" + (pktApi.isPremiumUser() ? '1' : '0'), {
    		onShow: function() {
                
                resizePanel({
                        width: 350,
                        height: 266
                });

                var options = {
                    success: function(data, response) {

                        var item = data.item;
                        var successResponse = {
                            status: "success",
                            item: item
                        };
                        sendMessage('saveLink', successResponse);
                    },
                    error: function(error, response) {
                        sendErrorMessage('saveLink', error);
                    }
                }

                
                if (typeof title !== "undefined") {
                    options.title = title;
                }

                
				pktApi.addLink(url, options);
			},
			onHide: panelDidHide,
    	});
    }

    


    function showPanel(url, options) {

        
        
        
    	var iframe = getPanelFrame();

		
		registerEventMessages();

    	
    	iframe.setAttribute('src', url);

    	
    	
    	
    	

    	
    	
    	currentPanelDidShow = options.onShow;
    	currentPanelDidHide = options.onHide;
    }

    







    function resizePanel(options) {
        var iframe = getPanelFrame();
        iframe.width = options.width;
        iframe.height = options.height;
        return;

    	
    	getPanel().sizeTo(options.width, options.height);
    	setTimeout(function(){
    		
            var height = document.getElementById('pocket-panel-container').clientHeight + 'px';
	    	getPanelFrame().style.height = height;
	    },1);
    }

    


    function panelDidHide() {
    	console.log("Panel did hide");
    }

    
    

    


    function addMessageListener(messageId, callback) {

		document.addEventListener('PKT_'+messageId, function(e) { 
            
			if (e.target.tagName !== 'PKTMESSAGEFROMPANELELEMENT') {
				return;
            }

            
			callback(JSON.parse(e.target.getAttribute("payload"))[0]);

			
			e.target.parentNode.removeChild(e.target);

		}, false, true);

    }

    


    function removeMessageListener(messageId, callback) {
    	document.removeMessageListener('PKT_'+messageId, callback);
    }

    


    function sendMessage(messageId, payload) {

    	var doc = getPanelFrame().contentWindow.document;

		var AnswerEvt = doc.createElement("PKTMessage");
	    AnswerEvt.setAttribute("payload", JSON.stringify([payload]));

	    doc.documentElement.appendChild(AnswerEvt);

	    var event = doc.createEvent("HTMLEvents");
	    event.initEvent('PKT_'+messageId, true, false);
	    AnswerEvt.dispatchEvent(event);
    }

    


    function sendErrorMessage(messageId, error) {
		var errorResponse = {status: "error", error: error.message};
		sendMessage(messageId, errorResponse);
	}

    


    function registerEventMessages() {

    	
    	var iframe = getPanelFrame();

    	
    	if (iframe.getAttribute('did_init') == 1) {
    		return;
        }

    	iframe.setAttribute('did_init', 1);

		
		
		
		
		addMessageListener("show", function(payload) {
			
			sendMessage('show');
		});

        
        addMessageListener("openTabWithUrl", function(payload) {
            var activate = true;
            if (typeof payload.activate !== "undefined") {
                activate = payload.activate;
            }
            openTabWithUrl(payload.url, activate);
            sendMessage("openTabWithUrlResponse", url);
        });

		
		addMessageListener("close", function(payload) {
			getPanel().hidePopup();
		});

		
		addMessageListener("getCurrentURL", function(payload) {
			sendMessage('getCurrentURLResponse', getCurrentUrl());
		});

		
		addMessageListener("listenerReady", function(payload) {
			console.log('got a listener init');
		});

		addMessageListener("resizePanel", function(payload) {
			resizePanel(payload);
		});

		
		addMessageListener("getTags", function(payload) {
			pktApi.getTags(function(tags, usedTags) {
				sendMessage('getTagsResponse', {tags, usedTags});
			});
		});

		
		addMessageListener("getSuggestedTags", function(payload) {
			var responseMessageId = "getSuggestedTagsResponse";

			pktApi.getSuggestedTagsForURL(payload.url, {
				success: function(data, response) {
					var suggestedTags = data.suggested_tags;
					var successResponse = {
						status: "success",
						value: {
							"suggestedTags" : suggestedTags
						}
					}
					sendMessage(responseMessageId, successResponse);
				},
				error: function(error, response) {
					sendErrorMessage(responseMessageId, error);
				}
			})
		});

		
		addMessageListener("addTags", function(payload) {
			var responseMessageId = "addTagsResponse";

			pktApi.addTagsToURL(payload.url, payload.tags, {
				success: function(data, response) {
				  var successResponse = {status: "success"};
				  sendMessage(responseMessageId, successResponse);
				},
				error: function(error, response) {
				  sendErrorMessage(responseMessageId, error);
				}
			});
		});

		
		addMessageListener("deleteItem", function(payload) {
			var responseMessageId = "deleteItemResponse";

			pktApi.deleteItem(payload.itemId, {
				success: function(data, response) {
				  var successResponse = {status: "success"};
				  sendMessage(responseMessageId, successResponse);
				},
				error: function(error, response) {
					sendErrorMessage(responseMessageId, error);
				}
			})
		});
	}
	
	
	
	



	function openTabWithUrl(url, activate) {
        var tab = gBrowser.addTab(url);
        if (activate) {
            gBrowser.selectedTab = tab;
        }
	}
    
    
    
    
    function getCurrentUrl() {
    	return getBrowser().currentURI.spec;
    }
    
    function getCurrentTitle() {
        return getBrowser().contentTitle;
    }
    
    function getPanel() {
        var frame = getPanelFrame();
        var panel = frame;
        while (panel && panel.localName != "panel") {
            panel = panel.parentNode;
        }
    	return panel;
    }
    
    function getPanelFrame() {
    	return document.getElementById('pocket-panel-iframe');
    }
    
    function hasLegacyExtension() {
    	return !!document.getElementById('RIL_urlbar_add');
    }
    
    function isHidden() {
    	return _isHidden;
    }
    
    function isUserLoggedIntoFxA() {
    	
    	var user = fxAccounts.getSignedInUser();
    	if (user && user.email)
    		return true;
    	
    	return false;
    }
    
    


    
    function showPocketAnimation() {
    	
    	
    	
    	
    	
    	
    
    	function getCenteringTransformForRects(rectToPosition, referenceRect) {
	      let topDiff = referenceRect.top - rectToPosition.top;
	      let leftDiff = referenceRect.left - rectToPosition.left;
	      let heightDiff = referenceRect.height - rectToPosition.height;
	      let widthDiff = referenceRect.width - rectToPosition.width;
	      return [(leftDiff + .5 * widthDiff) + "px", (topDiff + .5 * heightDiff) + "px"];
	    }
	
	    if (_notificationTimeout) {
	      clearTimeout(this._notificationTimeout);
	   	}
	
		var button = document.getElementById('pocket-menu-button');
		var bookmarksButton = document.getElementById('bookmarks-menu-button');
		var notifier = document.getElementById("pocketed-notification-anchor");
		var dropmarkerNotifier = document.getElementById("bookmarked-notification-dropmarker-anchor");
	
		
    	
    	
    	
    	if (bookmarksButton.nextElementSibling != button)
    		return;
	
	    if (notifier.style.transform == '') {
	      
	      let dropmarker = document.getAnonymousElementByAttribute(bookmarksButton, "anonid", "dropmarker");
	      let dropmarkerIcon = document.getAnonymousElementByAttribute(dropmarker, "class", "dropmarker-icon");
	      let dropmarkerStyle = getComputedStyle(dropmarkerIcon);
	      
	      
	      let isRTL = getComputedStyle(button).direction == "rtl"; 
	      let buttonRect = button.getBoundingClientRect();
	      let notifierRect = notifier.getBoundingClientRect();
	      let dropmarkerRect = dropmarkerIcon.getBoundingClientRect();
	      let dropmarkerNotifierRect = dropmarkerNotifier.getBoundingClientRect();
	
	      
	      let [translateX, translateY] = getCenteringTransformForRects(notifierRect, buttonRect);
	      let starIconTransform = "translate(" +  (translateX) + ", " + translateY + ")";
	      if (isRTL) {
	        starIconTransform += " scaleX(-1)";
	      }
	
	      
	      [translateX, translateY] = getCenteringTransformForRects(dropmarkerNotifierRect, dropmarkerRect);
	      let dropmarkerTransform = "translate(" + translateX + ", " + translateY + ")";
	
	      
	      notifier.style.transform = starIconTransform;
	      dropmarkerNotifier.style.transform = dropmarkerTransform;
	
	      let dropmarkerAnimationNode = dropmarkerNotifier.firstChild;
	      dropmarkerAnimationNode.style.MozImageRegion = dropmarkerStyle.MozImageRegion;
	      dropmarkerAnimationNode.style.listStyleImage = dropmarkerStyle.listStyleImage;
	    }
	
	    let isInOverflowPanel = button.getAttribute("overflowedItem") == "true";
	    if (!isInOverflowPanel) {
	      notifier.setAttribute("notification", "finish");
	      button.setAttribute("notification", "finish");
	      dropmarkerNotifier.setAttribute("notification", "finish");
	    }
	
	    _notificationTimeout = setTimeout( () => {
	      notifier.removeAttribute("notification");
	      dropmarkerNotifier.removeAttribute("notification");
	      button.removeAttribute("notification");
	
	      dropmarkerNotifier.style.transform = '';
	      notifier.style.transform = '';
	    }, 1000);
    }
    
    
	


    return {
    	onLoad: onLoad,
    	onUnload: onUnload,

    	pocketButtonOnCommand: pocketButtonOnCommand,
    	pocketPanelDidShow: pocketPanelDidShow,
    	pocketPanelDidHide: pocketPanelDidHide,

        pocketContextSaveLinkOnCommand,
        pocketContextSavePageOnCommand,

        pocketBookmarkBarOpenPocketCommand,

    	tryToSaveUrl: tryToSaveUrl,
    	
		isHidden
    };
}());
