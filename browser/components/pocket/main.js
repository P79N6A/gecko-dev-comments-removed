










































Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
  "resource://gre/modules/PrivateBrowsingUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ReaderMode",
  "resource://gre/modules/ReaderMode.jsm");

var pktUI = (function() {

	
	var inited = false;
	var _currentPanelDidShow;
    var _currentPanelDidHide;
	var _isHidden = false;
	var _notificationTimeout;

    
    
    
    var _panelId = 0;

	var prefBranch = Services.prefs.getBranch("browser.pocket.settings.");

	var overflowMenuWidth = 230;
	var overflowMenuHeight = 475;
	var savePanelWidth = 350;
	var savePanelHeights = {collapsed: 153, expanded: 272};

	


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
    	if (_currentPanelDidShow) {
    		_currentPanelDidShow(event);
        }
    	
    }
    
    function pocketPanelDidHide(event) {
    	if (_currentPanelDidHide) {
    		_currentPanelDidHide(event);
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
        getFirefoxAccountSignedInUser(function(userdata)
        {
            var fxasignedin = (typeof userdata == 'object' && userdata !== null) ? '1' : '0';
            var startheight = 490;
            var inOverflowMenu = isInOverflowMenu();
            
            if (inOverflowMenu) 
            {
            	startheight = overflowMenuHeight;
            }
            else if (pktApi.getSignupAB().indexOf('storyboard') > -1)
            {
                startheight = 460;
                if (fxasignedin == '1')
                {
                    startheight = 406;
                }
            }
            else
            {
                if (fxasignedin == '1')
                {
                    startheight = 436;
                }
            }
            var variant;
            if (inOverflowMenu)
            {
                variant = 'overflow';
            }
            else
            {
                variant = pktApi.getSignupAB();
            }
            var panelId = showPanel("about:pocket-signup?pockethost=" + Services.prefs.getCharPref("browser.pocket.site") + "&fxasignedin=" + fxasignedin + "&variant=" + variant + '&inoverflowmenu=' + inOverflowMenu + "&locale=" + getUILocale(), {
            		onShow: function() {
                    },
        			onHide: panelDidHide,
            		width: inOverflowMenu ? overflowMenuWidth : 300,
            		height: startheight
            	});
            });
    }

    


    function saveAndShowConfirmation(url, title) {

        
        if (typeof url !== 'undefined' && url.startsWith("about:reader?url=")) {
            url = ReaderMode.getOriginalUrl(url);
        }

        var isValidURL = (typeof url !== 'undefined' && (url.startsWith("http") || url.startsWith('https')));

        var inOverflowMenu = isInOverflowMenu();
        var startheight = pktApi.isPremiumUser() && isValidURL ? savePanelHeights.expanded : savePanelHeights.collapsed;
        if (inOverflowMenu) {
        	startheight = overflowMenuHeight;
        }

    	var panelId = showPanel("about:pocket-saved?pockethost=" + Services.prefs.getCharPref("browser.pocket.site") + "&premiumStatus=" + (pktApi.isPremiumUser() ? '1' : '0') + '&inoverflowmenu='+inOverflowMenu + "&locale=" + getUILocale(), {
    		onShow: function() {
                var saveLinkMessageId = 'saveLink';

                
                if (!isValidURL) {
                    
                    var error = {
                        message: 'Only links can be saved',
                        localizedKey: "onlylinkssaved"
                    };
                    pktUIMessaging.sendErrorMessageToPanel(panelId, saveLinkMessageId, error);
                    return;
                }

                
                if (!navigator.onLine) {
                    
                    var error = {
                        message: 'You must be connected to the Internet in order to save to Pocket. Please connect to the Internet and try again.'
                    };
                    pktUIMessaging.sendErrorMessageToPanel(panelId, saveLinkMessageId, error);
                    return;
                }

                
                var options = {
                    success: function(data, request) {
                        var item = data.item;
                        var successResponse = {
                            status: "success",
                            item: item
                        };
                        pktUIMessaging.sendMessageToPanel(panelId, saveLinkMessageId, successResponse);
                    },
                    error: function(error, request) {
                        
                        if (request.status === 401) {
                            showSignUp();
                            return;
                        }

                        
                        
                        var errorMessage = error.message || "There was an error when trying to save to Pocket.";
                        var panelError = { message: errorMessage}

                        
                        pktUIMessaging.sendErrorMessageToPanel(panelId, saveLinkMessageId, panelError);
                    }
                }

                
                if (typeof title !== "undefined") {
                    options.title = title;
                }

                
				pktApi.addLink(url, options);
			},
			onHide: panelDidHide,
    		width: inOverflowMenu ? overflowMenuWidth : savePanelWidth,
    		height: startheight
    	});
    }

    


    function showPanel(url, options) {

        
        _panelId += 1;
        url += ("&panelId=" + _panelId);

        
        
        
    	var iframe = getPanelFrame();

		
		registerEventMessages();

    	
    	iframe.setAttribute('src', url);

    	
    	
    	
    	

    	
    	
    	_currentPanelDidShow = options.onShow;
    	_currentPanelDidHide = options.onHide;

    	resizePanel({
    		width: options.width,
    		height: options.height
    	});
        return _panelId;
    }

    







    function resizePanel(options) {
        var iframe = getPanelFrame();
        var subview = getSubview();

        if (subview) {
          
          iframe.style.width = "100%";
          iframe.style.height = subview.clientHeight + "px";
        } else {
          
          iframe.style.width  = options.width  + "px";
          iframe.style.height = options.height + "px";
        }
    }

    


    function panelDidHide() {
        
    }

    


    function registerEventMessages() {
    	var iframe = getPanelFrame();

    	
        var didInitAttributeKey = 'did_init';
        var didInitMessageListener = iframe.getAttribute(didInitAttributeKey);
    	if (typeof didInitMessageListener !== "undefined" && didInitMessageListener == 1) {
            return;
        }
    	iframe.setAttribute(didInitAttributeKey, 1);

		
		
		
		
        var _showMessageId = "show";
		pktUIMessaging.addMessageListener(_showMessageId, function(panelId, data) {
			
			pktUIMessaging.sendMessageToPanel(panelId, _showMessageId);
		});

        
        var _openTabWithUrlMessageId = "openTabWithUrl";
        pktUIMessaging.addMessageListener(_openTabWithUrlMessageId, function(panelId, data) {

            
            var activate = true;
            if (typeof data.activate !== "undefined") {
                activate = data.activate;
            }

            var url = data.url;
            openTabWithUrl(url, activate);
            pktUIMessaging.sendResponseMessageToPanel(panelId, _openTabWithUrlMessageId, url);
        });

		
        var _closeMessageId = "close";
		pktUIMessaging.addMessageListener(_closeMessageId, function(panelId, data) {
			getPanel().hidePopup();
		});

		
        var _getCurrentURLMessageId = "getCurrentURL";
		pktUIMessaging.addMessageListener(_getCurrentURLMessageId, function(panelId, data) {
            pktUIMessaging.sendResponseMessageToPanel(panelId, _getCurrentURLMessageId, getCurrentUrl());
		});

        var _resizePanelMessageId = "resizePanel";
		pktUIMessaging.addMessageListener(_resizePanelMessageId, function(panelId, data) {
			resizePanel(data);
        });

		
		pktUIMessaging.addMessageListener("listenerReady", function(panelId, data) {

		});

		pktUIMessaging.addMessageListener("collapseSavePanel", function(panelId, data) {
			if (!pktApi.isPremiumUser() && !isInOverflowMenu())
				resizePanel({width:savePanelWidth, height:savePanelHeights.collapsed});
		});

		pktUIMessaging.addMessageListener("expandSavePanel", function(panelId, data) {
			if (!isInOverflowMenu())
				resizePanel({width:savePanelWidth, height:savePanelHeights.expanded});
		});

		
		var _getTagsMessageId = "getTags";
        pktUIMessaging.addMessageListener(_getTagsMessageId, function(panelId, data) {
			pktApi.getTags(function(tags, usedTags) {
                pktUIMessaging.sendResponseMessageToPanel(panelId, _getTagsMessageId, {
                    tags: tags,
                    usedTags: usedTags
                });
			});
		});

		
        var _getSuggestedTagsMessageId = "getSuggestedTags";
		pktUIMessaging.addMessageListener(_getSuggestedTagsMessageId, function(panelId, data) {
			pktApi.getSuggestedTagsForURL(data.url, {
				success: function(data, response) {
					var suggestedTags = data.suggested_tags;
					var successResponse = {
						status: "success",
						value: {
							suggestedTags: suggestedTags
						}
					}
                    pktUIMessaging.sendResponseMessageToPanel(panelId, _getSuggestedTagsMessageId, successResponse);
				},
				error: function(error, response) {
                    pktUIMessaging.sendErrorResponseMessageToPanel(panelId, _getSuggestedTagsMessageId, error);
				}
			})
		});

		
        var _addTagsMessageId = "addTags";
		pktUIMessaging.addMessageListener(_addTagsMessageId, function(panelId, data) {
			pktApi.addTagsToURL(data.url, data.tags, {
				success: function(data, response) {
				    var successResponse = {status: "success"};
                    pktUIMessaging.sendResponseMessageToPanel(panelId, _addTagsMessageId, successResponse);
				},
				error: function(error, response) {
                    pktUIMessaging.sendErrorResponseMessageToPanel(panelId, _addTagsMessageId, error);
				}
			});
		});

		
        var _deleteItemMessageId = "deleteItem";
		pktUIMessaging.addMessageListener(_deleteItemMessageId, function(panelId, data) {
			pktApi.deleteItem(data.itemId, {
				success: function(data, response) {
				    var successResponse = {status: "success"};
                    pktUIMessaging.sendResponseMessageToPanel(panelId, _deleteItemMessageId, successResponse);
				},
				error: function(error, response) {
				    pktUIMessaging.sendErrorResponseMessageToPanel(panelId, _deleteItemMessageId, error);
				}
			})
		});

        var _initL10NMessageId = "initL10N";
        pktUIMessaging.addMessageListener(_initL10NMessageId, function(panelId, data) {
            var strings = {};
            var bundle = Services.strings.createBundle("chrome://browser/locale/browser-pocket.properties");
            var e = bundle.getSimpleEnumeration();
            while(e.hasMoreElements()) {
                var str = e.getNext().QueryInterface(Components.interfaces.nsIPropertyElement);
                strings[str.key] = str.value;
            }
            pktUIMessaging.sendResponseMessageToPanel(panelId, _initL10NMessageId, { strings: strings });
        });

	}

	

	



	function openTabWithUrl(url) {
        let recentWindow = Services.wm.getMostRecentWindow("navigator:browser");
        if (!recentWindow) {
          Cu.reportError("Pocket: No open browser windows to openTabWithUrl");
          return;
        }

        
        
        
        if (!PrivateBrowsingUtils.isWindowPrivate(recentWindow) ||
            PrivateBrowsingUtils.permanentPrivateBrowsing) {
          recentWindow.openUILinkIn(url, "tab");
          return;
        }

        let windows = Services.wm.getEnumerator("navigator:browser");
        while (windows.hasMoreElements()) {
          let win = windows.getNext();
          if (!PrivateBrowsingUtils.isWindowPrivate(win)) {
            win.openUILinkIn(url, "tab");
            return;
          }
        }

        
        recentWindow.openUILinkIn(url, "window");
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
        var frame = document.getElementById('pocket-panel-iframe');
        if (!frame) {
            var frameParent = document.getElementById("PanelUI-pocketView").firstChild;
            frame = document.createElement("iframe");
            frame.id = 'pocket-panel-iframe';
            frame.setAttribute("type", "content");
            frameParent.appendChild(frame);
        }
        return frame;
    }

    function getSubview() {
        var frame = getPanelFrame();
        var view = frame;
        while (view && view.localName != "panelview") {
            view = view.parentNode;
        }

        if (view && view.getAttribute("current") == "true")
            return view;
        return null;
    }

    function isInOverflowMenu() {
        var subview = getSubview();
        return !!subview;
    }

    function hasLegacyExtension() {
    	return !!document.getElementById('RIL_urlbar_add');
    }

    function isHidden() {
    	return _isHidden;
    }

    function getFirefoxAccountSignedInUser(callback) {
	    fxAccounts.getSignedInUser().then(userData => {
    		callback(userData);
    	}).then(null, error => {
      		callback();
	    });
    }
    
    function getUILocale() {
    	var locale = Cc["@mozilla.org/chrome/chrome-registry;1"].
             getService(Ci.nsIXULChromeRegistry).
             getSelectedLocale("browser");
        return locale;
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
	
	    let isInOverflowMenu = button.getAttribute("overflowedItem") == "true";
	    if (!isInOverflowMenu) {
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
    	getPanelFrame: getPanelFrame,

        openTabWithUrl: openTabWithUrl,

    	pocketButtonOnCommand: pocketButtonOnCommand,
    	pocketPanelDidShow: pocketPanelDidShow,
    	pocketPanelDidHide: pocketPanelDidHide,

        pocketContextSaveLinkOnCommand,
        pocketContextSavePageOnCommand,

        pocketBookmarkBarOpenPocketCommand,

    	tryToSaveUrl: tryToSaveUrl
    };
}());



var pktUIMessaging = (function() {

    


    function prefixedMessageId(messageId) {
        return 'PKT_' + messageId;
    }

    


    function addMessageListener(messageId, callback) {
        document.addEventListener(prefixedMessageId(messageId), function(e) { 
            
            if (e.target.tagName !== 'PKTMESSAGEFROMPANELELEMENT') {
                return;
            }

            
            var payload = JSON.parse(e.target.getAttribute("payload"))[0];
            var panelId = payload.panelId;
            var data = payload.data;
            callback(panelId, data);

            
            e.target.parentNode.removeChild(e.target);

        }, false, true);
    }

    


    function removeMessageListener(messageId, callback) {
        document.removeEventListener(prefixedMessageId(messageId), callback);
    }


    


    function sendMessageToPanel(panelId, messageId, payload) {

        if (!isPanelIdValid(panelId)) { return; };

        var panelFrame = pktUI.getPanelFrame();
        if (!isPocketPanelFrameValid(panelFrame)) { return; }

        var doc = panelFrame.contentWindow.document;
        var documentElement = doc.documentElement;

        
        var panelMessageId = prefixedMessageId(panelId + '_' + messageId);

        var AnswerEvt = doc.createElement("PKTMessage");
        AnswerEvt.setAttribute("payload", JSON.stringify([payload]));
        documentElement.appendChild(AnswerEvt);

        var event = doc.createEvent("HTMLEvents");
        event.initEvent(panelMessageId, true, false);
        AnswerEvt.dispatchEvent(event);
    }

    function sendResponseMessageToPanel(panelId, messageId, payload) {
        var responseMessageId = messageId + "Response";
        sendMessageToPanel(panelId, responseMessageId, payload);
    }

    



    function sendErrorMessageToPanel(panelId, messageId, error) {
        var errorResponse = {status: "error", error: error};
        sendMessageToPanel(panelId, messageId, errorResponse);
    }

    function sendErrorResponseMessageToPanel(panelId, messageId, error) {
        var errorResponse = {status: "error", error: error};
        sendResponseMessageToPanel(panelId, messageId, errorResponse);
    }

    



    function isPanelIdValid(panelId) {
        
        
        
        
        
        
        if (panelId === 0) {
            console.warn("Tried to send message to panel with id 0.")
            return false;
        }

        return true
    }

    function isPocketPanelFrameValid(panelFrame) {
        
        
        if (typeof panelFrame === "undefined") {
            console.warn("Pocket panel frame is undefined");
            return false;
        }

        var contentWindow = panelFrame.contentWindow;
        if (typeof contentWindow == "undefined") {
            console.warn("Pocket panel frame content window is undefined");
            return false;
        }

        var doc = contentWindow.document;
        if (typeof doc === "undefined") {
            console.warn("Pocket panel frame content window document is undefined");
            return false;
        }

        var documentElement = doc.documentElement;
        if (typeof documentElement === "undefined") {
            console.warn("Pocket panel frame content window document document element is undefined");
            return false;
        }

        return true;
    }

    


    return {
        addMessageListener: addMessageListener,
        removeMessageListener: removeMessageListener,
        sendMessageToPanel: sendMessageToPanel,
        sendResponseMessageToPanel: sendResponseMessageToPanel,
        sendErrorMessageToPanel: sendErrorMessageToPanel,
        sendErrorResponseMessageToPanel: sendErrorResponseMessageToPanel
    }
}());
