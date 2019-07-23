
















































pref("keyword.URL", "http://www.google.com/search?ie=UTF-8&oe=utf-8&q=");
pref("keyword.enabled", false);
pref("general.useragent.locale", "chrome://navigator/locale/navigator.properties");

pref("general.config.obscure_value", 13); 

pref("general.warnOnAboutConfig", true);


pref("browser.bookmarks.max_backups",       5);

pref("browser.cache.disk.enable",           true);
pref("browser.cache.disk.capacity",         51200);
pref("browser.cache.memory.enable",         true);


pref("browser.cache.disk_cache_ssl",        false);

pref("browser.cache.check_doc_frequency",   3);

pref("browser.cache.offline.enable",           true);

pref("browser.cache.offline.capacity",         10240);



pref("browser.sessionhistory.max_total_viewers", -1);

pref("browser.display.use_document_fonts",  1);  
pref("browser.display.use_document_colors", true);
pref("browser.display.use_system_colors",   false);
pref("browser.display.foreground_color",    "#000000");
pref("browser.display.background_color",    "#FFFFFF");
pref("browser.display.force_inline_alttext", false); 



pref("browser.display.normal_lineheight_calc_control", 2);
pref("browser.display.show_image_placeholders", true); 

pref("browser.display.auto_quality_min_font_size", 60);
pref("browser.anchor_color",                "#0000EE");
pref("browser.active_color",                "#EE0000");
pref("browser.visited_color",               "#551A8B");
pref("browser.underline_anchors",           true);
pref("browser.blink_allowed",               true);
pref("browser.enable_automatic_image_resizing", false);


pref("browser.send_pings", false);
pref("browser.send_pings.max_per_link", 1);           
pref("browser.send_pings.require_same_host", false);  

pref("browser.display.use_focus_colors",    false);
pref("browser.display.focus_background_color", "#117722");
pref("browser.display.focus_text_color",     "#ffffff");
pref("browser.display.focus_ring_width",     1);
pref("browser.display.focus_ring_on_anything", false);

pref("browser.helperApps.alwaysAsk.force",  false);
pref("browser.helperApps.neverAsk.saveToDisk", "");
pref("browser.helperApps.neverAsk.openFile", "");


pref("browser.chrome.toolbar_tips",         true);

pref("browser.chrome.toolbar_style",        2);


pref("browser.chrome.image_icons.max_size", 1024);

pref("browser.triple_click_selects_paragraph", true);

pref("gfx.color_management.enabled", false);
pref("gfx.color_management.display_profile", "");

pref("accessibility.browsewithcaret", false);
pref("accessibility.warn_on_browsewithcaret", true);

#ifndef XP_MACOSX





pref("accessibility.tabfocus", 7);
pref("accessibility.tabfocus_applies_to_xul", false);



pref("ui.scrollToClick", 0);

#else

pref("accessibility.tabfocus_applies_to_xul", true);
#endif

pref("accessibility.usetexttospeech", "");
pref("accessibility.usebrailledisplay", "");
pref("accessibility.accesskeycausesactivation", true);


pref("accessibility.typeaheadfind", true);
pref("accessibility.typeaheadfind.autostart", true);




pref("accessibility.typeaheadfind.casesensitive", 0);
pref("accessibility.typeaheadfind.linksonly", true);
pref("accessibility.typeaheadfind.startlinksonly", false);
pref("accessibility.typeaheadfind.timeout", 4000);
pref("accessibility.typeaheadfind.enabletimeout", true);
pref("accessibility.typeaheadfind.soundURL", "beep");
pref("accessibility.typeaheadfind.enablesound", true);
pref("accessibility.typeaheadfind.prefillwithselection", true);

pref("browser.history_expire_days", 9);


pref("browser.frames.enabled", true);


pref("browser.forms.submit.backwards_compatible", true);



pref("browser.tabs.autoHide", true);
pref("browser.tabs.forceHide", false);
pref("browser.tabs.warnOnClose", true);
pref("browser.tabs.warnOnCloseOther", true);
pref("browser.tabs.warnOnOpen", true);
pref("browser.tabs.maxOpenBeforeWarn", 15);

pref("browser.tabs.loadGroup", 1);

pref("toolkit.scrollbox.scrollIncrement", 20);
pref("toolkit.scrollbox.clickToScroll.scrollDelay", 150);






pref("browser.tabs.loadOnNewTab", 0);
pref("browser.windows.loadOnNewWindow", 1);


pref("view_source.syntax_highlight", true);
pref("view_source.wrap_long_lines", false);


pref("nglayout.events.dispatchLeftClickOnly", true);


pref("nglayout.enable_drag_images", true);


pref("nglayout.debug.enable_xbl_forms", false);




pref("slider.snapMultiplier", 0);


pref("application.use_ns_plugin_finder", false);


pref("browser.fixup.alternate.enabled", true);
pref("browser.fixup.alternate.prefix", "www.");
pref("browser.fixup.alternate.suffix", ".com");
pref("browser.fixup.hide_user_pass", true);










pref("print.print_headerleft", "&T");
pref("print.print_headercenter", "");
pref("print.print_headerright", "&U");
pref("print.print_footerleft", "&PT");
pref("print.print_footercenter", "");
pref("print.print_footerright", "&D");
pref("print.show_print_progress", true);





pref("print.use_global_printsettings", true);


pref("print.use_native_print_dialog", false);


pref("print.save_print_settings", true);

pref("print.whileInPrintPreview", true);


pref("print.always_cache_old_pres", false);



pref("print.print_edge_top", 0); 
pref("print.print_edge_left", 0); 
pref("print.print_edge_right", 0); 
pref("print.print_edge_bottom", 0); 




pref("extensions.spellcheck.inline.max-misspellings", 500);




pref("editor.use_custom_colors", false);
pref("editor.htmlWrapColumn", 72);
pref("editor.singleLine.pasteNewlines",     1);
pref("editor.quotesPreformatted",            false);
pref("editor.use_css",                       true);
pref("editor.css.default_length_unit",       "px");
pref("editor.resizing.preserve_ratio",       true);
pref("editor.positioning.offset",            0);






pref("capability.policy.default_policynames", "mailnews");

pref("capability.policy.default.DOMException.code", "allAccess");
pref("capability.policy.default.DOMException.message", "allAccess");
pref("capability.policy.default.DOMException.name", "allAccess");
pref("capability.policy.default.DOMException.result", "allAccess");
pref("capability.policy.default.DOMException.toString.get", "allAccess");

pref("capability.policy.default.History.back.get", "allAccess");
pref("capability.policy.default.History.current", "UniversalBrowserRead");
pref("capability.policy.default.History.forward.get", "allAccess");
pref("capability.policy.default.History.go.get", "allAccess");
pref("capability.policy.default.History.item", "UniversalBrowserRead");
pref("capability.policy.default.History.next", "UniversalBrowserRead");
pref("capability.policy.default.History.previous", "UniversalBrowserRead");
pref("capability.policy.default.History.toString", "UniversalBrowserRead");

pref("capability.policy.default.HTMLDocument.open.get", "allAccess");

pref("capability.policy.default.Location.hash.set", "allAccess");
pref("capability.policy.default.Location.href.set", "allAccess");
pref("capability.policy.default.Location.replace.get", "allAccess");

pref("capability.policy.default.Navigator.preference", "allAccess");
pref("capability.policy.default.Navigator.preferenceinternal.get", "UniversalPreferencesRead");
pref("capability.policy.default.Navigator.preferenceinternal.set", "UniversalPreferencesWrite");

pref("capability.policy.default.Window.blur.get", "allAccess");
pref("capability.policy.default.Window.close.get", "allAccess");
pref("capability.policy.default.Window.closed.get", "allAccess");
pref("capability.policy.default.Window.document.get", "allAccess");
pref("capability.policy.default.Window.focus.get", "allAccess");
pref("capability.policy.default.Window.frames.get", "allAccess");
pref("capability.policy.default.Window.history.get", "allAccess");
pref("capability.policy.default.Window.length.get", "allAccess");
pref("capability.policy.default.Window.location", "allAccess");
pref("capability.policy.default.Window.opener.get", "allAccess");
pref("capability.policy.default.Window.parent.get", "allAccess");
pref("capability.policy.default.Window.self.get", "allAccess");
pref("capability.policy.default.Window.top.get", "allAccess");
pref("capability.policy.default.Window.window.get", "allAccess");

pref("capability.policy.default.Selection.addSelectionListener", "UniversalXPConnect");
pref("capability.policy.default.Selection.removeSelectionListener", "UniversalXPConnect");


pref("capability.policy.mailnews.sites", "mailbox: imap: news:");

pref("capability.policy.mailnews.*.attributes.get", "noAccess");
pref("capability.policy.mailnews.*.baseURI.get", "noAccess");
pref("capability.policy.mailnews.*.data.get", "noAccess");
pref("capability.policy.mailnews.*.getAttribute", "noAccess");
pref("capability.policy.mailnews.HTMLDivElement.getAttribute", "sameOrigin");
pref("capability.policy.mailnews.*.getAttributeNS", "noAccess");
pref("capability.policy.mailnews.*.getNamedItem", "noAccess");
pref("capability.policy.mailnews.*.getNamedItemNS", "noAccess");
pref("capability.policy.mailnews.*.host.get", "noAccess");
pref("capability.policy.mailnews.*.hostname.get", "noAccess");
pref("capability.policy.mailnews.*.href.get", "noAccess");
pref("capability.policy.mailnews.*.innerHTML.get", "noAccess");
pref("capability.policy.mailnews.*.lowSrc.get", "noAccess");
pref("capability.policy.mailnews.*.nodeValue.get", "noAccess");
pref("capability.policy.mailnews.*.pathname.get", "noAccess");
pref("capability.policy.mailnews.*.protocol.get", "noAccess");
pref("capability.policy.mailnews.*.src.get", "noAccess");
pref("capability.policy.mailnews.*.substringData.get", "noAccess");
pref("capability.policy.mailnews.*.text.get", "noAccess");
pref("capability.policy.mailnews.*.title.get", "noAccess");
pref("capability.policy.mailnews.DOMException.toString", "noAccess");
pref("capability.policy.mailnews.HTMLAnchorElement.toString", "noAccess");
pref("capability.policy.mailnews.HTMLDocument.domain", "noAccess");
pref("capability.policy.mailnews.HTMLDocument.URL", "noAccess");
pref("capability.policy.mailnews.Location.toString", "noAccess");
pref("capability.policy.mailnews.Range.toString", "noAccess");
pref("capability.policy.mailnews.Window.blur", "noAccess");
pref("capability.policy.mailnews.Window.focus", "noAccess");
pref("capability.policy.mailnews.Window.innerWidth.set", "noAccess");
pref("capability.policy.mailnews.Window.innerHeight.set", "noAccess");
pref("capability.policy.mailnews.Window.moveBy", "noAccess");
pref("capability.policy.mailnews.Window.moveTo", "noAccess");
pref("capability.policy.mailnews.Window.name.set", "noAccess");
pref("capability.policy.mailnews.Window.outerHeight.set", "noAccess");
pref("capability.policy.mailnews.Window.outerWidth.set", "noAccess");
pref("capability.policy.mailnews.Window.resizeBy", "noAccess");
pref("capability.policy.mailnews.Window.resizeTo", "noAccess");
pref("capability.policy.mailnews.Window.screenX.set", "noAccess");
pref("capability.policy.mailnews.Window.screenY.set", "noAccess");
pref("capability.policy.mailnews.Window.sizeToContent", "noAccess");
pref("capability.policy.mailnews.document.load", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.channel", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.getInterface", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.responseXML", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.responseText", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.status", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.statusText", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.abort", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.getAllResponseHeaders", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.getResponseHeader", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.open", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.send", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.setRequestHeader", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.readyState", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.overrideMimeType", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.onload", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.onerror", "noAccess");
pref("capability.policy.mailnews.XMLHttpRequest.onreadystatechange", "noAccess");
pref("capability.policy.mailnews.XMLSerializer.serializeToString", "noAccess");
pref("capability.policy.mailnews.XMLSerializer.serializeToStream", "noAccess");
pref("capability.policy.mailnews.DOMParser.parseFromString", "noAccess");
pref("capability.policy.mailnews.DOMParser.parseFromStream", "noAccess");
pref("capability.policy.mailnews.SOAPCall.transportURI", "noAccess");
pref("capability.policy.mailnews.SOAPCall.verifySourceHeader", "noAccess");
pref("capability.policy.mailnews.SOAPCall.invoke", "noAccess");
pref("capability.policy.mailnews.SOAPCall.asyncInvoke", "noAccess");
pref("capability.policy.mailnews.SOAPResponse.fault", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.styleURI", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.getAssociatedEncoding", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.setEncoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.getEncoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.setDecoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.setDecoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.getDecoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.defaultEncoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.defaultDecoder", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.schemaCollection", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.encode", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.decode", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.mapSchemaURI", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.unmapSchemaURI", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.getInternalSchemaURI", "noAccess");
pref("capability.policy.mailnews.SOAPEncoding.getExternalSchemaURI", "noAccess");
pref("capability.policy.mailnews.SOAPFault.element", "noAccess");
pref("capability.policy.mailnews.SOAPFault.faultNamespaceURI", "noAccess");
pref("capability.policy.mailnews.SOAPFault.faultCode", "noAccess");
pref("capability.policy.mailnews.SOAPFault.faultString", "noAccess");
pref("capability.policy.mailnews.SOAPFault.faultActor", "noAccess");
pref("capability.policy.mailnews.SOAPFault.detail", "noAccess");
pref("capability.policy.mailnews.SOAPHeaderBlock.actorURI", "noAccess");
pref("capability.policy.mailnews.SOAPHeaderBlock.mustUnderstand", "noAccess");
pref("capability.policy.mailnews.SOAPParameter", "noAccess");
pref("capability.policy.mailnews.SOAPPropertyBagMutator.propertyBag", "noAccess");
pref("capability.policy.mailnews.SOAPPropertyBagMutator.addProperty", "noAccess");
pref("capability.policy.mailnews.SchemaLoader.load", "noAccess");
pref("capability.policy.mailnews.SchemaLoader.loadAsync", "noAccess");
pref("capability.policy.mailnews.SchemaLoader.processSchemaElement", "noAccess");
pref("capability.policy.mailnews.SchemaLoader.onLoad", "noAccess");
pref("capability.policy.mailnews.SchemaLoader.onError", "noAccess");
pref("capability.policy.mailnews.WSDLLoader.load", "noAccess");
pref("capability.policy.mailnews.WSDLLoader.loadAsync", "noAccess");
pref("capability.policy.mailnews.WSDLLoader.onLoad", "noAccess");
pref("capability.policy.mailnews.WSDLLoader.onError", "noAccess");
pref("capability.policy.mailnews.WebServiceProxyFactory.createProxy", "noAccess");
pref("capability.policy.mailnews.WebServiceProxyFactory.createProxyAsync", "noAccess");
pref("capability.policy.mailnews.WebServiceProxyFactory.onLoad", "noAccess");
pref("capability.policy.mailnews.WebServiceProxyFactory.onError", "noAccess");


pref("capability.policy.default.XMLHttpRequest.channel", "noAccess");
pref("capability.policy.default.XMLHttpRequest.getInterface", "noAccess");
pref("capability.policy.default.XMLHttpRequest.open-uri", "allAccess");
pref("capability.policy.default.DOMParser.parseFromStream", "noAccess");


pref("capability.policy.default.Clipboard.cutcopy", "noAccess");
pref("capability.policy.default.Clipboard.paste", "noAccess");


pref("dom.disable_image_src_set",           false);
pref("dom.disable_window_flip",             false);
pref("dom.disable_window_move_resize",      false);
pref("dom.disable_window_status_change",    false);

pref("dom.disable_window_open_feature.titlebar",    false);
pref("dom.disable_window_open_feature.close",       false);
pref("dom.disable_window_open_feature.toolbar",     false);
pref("dom.disable_window_open_feature.location",    false);
pref("dom.disable_window_open_feature.directories", false);
pref("dom.disable_window_open_feature.personalbar", false);
pref("dom.disable_window_open_feature.menubar",     false);
pref("dom.disable_window_open_feature.scrollbars",  false);
pref("dom.disable_window_open_feature.resizable",   false);
pref("dom.disable_window_open_feature.minimizable", false);
pref("dom.disable_window_open_feature.status",      true);

pref("dom.allow_scripts_to_close_windows",          false);

pref("dom.disable_open_during_load",                false);
pref("dom.popup_maximum",                           20);
pref("dom.popup_allowed_events", "change click dblclick mouseup reset submit");
pref("dom.disable_open_click_delay", 1000);

pref("dom.storage.enabled", true);





pref("privacy.popups.disable_from_plugins", 2);

pref("dom.event.contextmenu.enabled",       true);

pref("javascript.enabled",                  true);
pref("javascript.allow.mailnews",           false);
pref("javascript.options.strict",           false);
pref("javascript.options.relimit",          false);


pref("security.enable_java",                true);
pref("advanced.mailftp",                    false);
pref("image.animation_mode",                "normal");










pref("network.protocol-handler.external-default", true);      
pref("network.protocol-handler.warn-external-default", true); 


pref("network.protocol-handler.external.hcp", false);
pref("network.protocol-handler.external.vbscript", false);
pref("network.protocol-handler.external.javascript", false);
pref("network.protocol-handler.external.data", false);
pref("network.protocol-handler.external.ms-help", false);
pref("network.protocol-handler.external.shell", false);
pref("network.protocol-handler.external.vnd.ms.radio", false);
#ifdef XP_MACOSX
pref("network.protocol-handler.external.help", false);
#endif
pref("network.protocol-handler.external.disk", false);
pref("network.protocol-handler.external.disks", false);
pref("network.protocol-handler.external.afp", false);
pref("network.protocol-handler.external.moz-icon", false);










pref("network.protocol-handler.expose-all", true);




pref("network.hosts.smtp_server",           "mail");
pref("network.hosts.pop_server",            "mail");


pref("network.http.version", "1.1");	  





pref("network.http.proxy.version", "1.1");    

                                              


pref("network.http.use-cache", true);



pref("network.http.default-socket-type", "");

pref("network.http.keep-alive", true); 
pref("network.http.proxy.keep-alive", true);
pref("network.http.keep-alive.timeout", 300);


pref("network.http.max-connections", 24);




pref("network.http.max-connections-per-server", 8);




pref("network.http.max-persistent-connections-per-server", 2);




pref("network.http.max-persistent-connections-per-proxy", 4);





pref("network.http.request.max-start-delay", 10);


pref("network.http.accept.default", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
pref("network.http.sendRefererHeader",      2); 



pref("network.http.sendSecureXSiteReferrer", true);


pref("network.http.redirection-limit", 20);



pref("network.http.accept-encoding" ,"gzip,deflate");

pref("network.http.pipelining"      , false);
pref("network.http.proxy.pipelining", false);


pref("network.http.pipelining.maxrequests" , 4);





pref("network.enableIDN", true);




pref("network.IDN_show_punycode", false);









pref("network.IDN.whitelist.ac", true);
pref("network.IDN.whitelist.at", true);
pref("network.IDN.whitelist.br", true);
pref("network.IDN.whitelist.ch", true);
pref("network.IDN.whitelist.cl", true);
pref("network.IDN.whitelist.cn", true);
pref("network.IDN.whitelist.de", true);
pref("network.IDN.whitelist.dk", true);
pref("network.IDN.whitelist.fi", true);
pref("network.IDN.whitelist.gr", true);
pref("network.IDN.whitelist.hu", true);
pref("network.IDN.whitelist.io", true);
pref("network.IDN.whitelist.is", true);
pref("network.IDN.whitelist.jp", true);
pref("network.IDN.whitelist.kr", true);
pref("network.IDN.whitelist.li", true);
pref("network.IDN.whitelist.lt", true);
pref("network.IDN.whitelist.no", true);
pref("network.IDN.whitelist.se", true);
pref("network.IDN.whitelist.sh", true);
pref("network.IDN.whitelist.th", true);
pref("network.IDN.whitelist.tm", true);
pref("network.IDN.whitelist.tw", true);
pref("network.IDN.whitelist.vn", true);


pref("network.IDN.whitelist.biz", true);
pref("network.IDN.whitelist.cat", true);
pref("network.IDN.whitelist.info", true);
pref("network.IDN.whitelist.museum", true);
pref("network.IDN.whitelist.org", true);





pref("network.IDN.blacklist_chars", "\u0020\u00A0\u00BC\u00BD\u01C3\u0337\u0338\u05C3\u05F4\u06D4\u0702\u115F\u1160\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200A\u200B\u2024\u2027\u2028\u2029\u202F\u2039\u203A\u2044\u205F\u2154\u2155\u2156\u2159\u215A\u215B\u215F\u2215\u23AE\u29F6\u29F8\u2AFB\u2AFD\u2FF0\u2FF1\u2FF2\u2FF3\u2FF4\u2FF5\u2FF6\u2FF7\u2FF8\u2FF9\u2FFA\u2FFB\u3000\u3002\u3014\u3015\u3033\u3164\u321D\u321E\u33AE\u33AF\u33C6\u33DF\uFE14\uFE15\uFE3F\uFE5D\uFE5E\uFEFF\uFF0E\uFF0F\uFF61\uFFA0\uFFF9\uFFFA\uFFFB\uFFFC\uFFFD");




pref("network.dns.ipv4OnlyDomains", ".doubleclick.net");


pref("network.dns.disableIPv6", false);



pref("network.standard-url.escape-utf8", true);



pref("network.standard-url.encode-utf8", true);



pref("network.standard-url.encode-query-utf8", false);


pref("network.ftp.idleConnectionTimeout", 300);





pref("network.dir.format", 2);


pref("network.prefetch-next", true);











pref("network.negotiate-auth.trusted-uris", "");

pref("network.negotiate-auth.delegation-uris", "");


pref("network.negotiate-auth.allow-proxies", true);


pref("network.negotiate-auth.gsslib", "");


pref("network.negotiate-auth.using-native-gsslib", true);

#ifdef XP_WIN


pref("network.auth.use-sspi", true);

#endif





pref("network.automatic-ntlm-auth.allow-proxies", true);
pref("network.automatic-ntlm-auth.trusted-uris", "");







pref("network.ntlm.send-lm-response", false);




pref("network.hosts.nntp_server",           "news.mozilla.org");

pref("permissions.default.image",           1); 
pref("network.proxy.type",                  0);
pref("network.proxy.ftp",                   "");
pref("network.proxy.ftp_port",              0);
pref("network.proxy.gopher",                "");
pref("network.proxy.gopher_port",           0);
pref("network.proxy.http",                  "");
pref("network.proxy.http_port",             0);
pref("network.proxy.ssl",                   "");
pref("network.proxy.ssl_port",              0);
pref("network.proxy.socks",                 "");
pref("network.proxy.socks_port",            0);
pref("network.proxy.socks_version",         5);
pref("network.proxy.socks_remote_dns",      false);
pref("network.proxy.no_proxies_on",         "localhost, 127.0.0.1");
pref("network.proxy.failover_timeout",      1800); 
pref("network.online",                      true); 
pref("network.cookie.cookieBehavior",       0); 
pref("network.cookie.disableCookieForMailNews", true); 
pref("network.cookie.lifetimePolicy",       0); 
pref("network.cookie.alwaysAcceptSessionCookies", false);
pref("network.cookie.prefsMigrated",        false);
pref("network.cookie.lifetime.days",        90);


pref("network.proxy.autoconfig_url", "");



pref("network.proxy.autoconfig_retry_interval_min", 5);    
pref("network.proxy.autoconfig_retry_interval_max", 300);  

pref("converter.html2txt.structs",          true); 
pref("converter.html2txt.header_strategy",  1); 

pref("intl.accept_languages",               "chrome://navigator/locale/navigator.properties");
pref("intl.accept_charsets",                "iso-8859-1,*,utf-8");
pref("intl.collationOption",                "chrome://navigator-platform/locale/navigator.properties");
pref("intl.menuitems.alwaysappendaccesskeys","chrome://navigator/locale/navigator.properties");
pref("intl.menuitems.insertseparatorbeforeaccesskeys","chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.static",     "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.more1",      "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.more2",      "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.more3",      "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.more4",      "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.more5",      "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.unicode",    "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.mailedit",           "chrome://navigator/locale/navigator.properties");
pref("intl.charsetmenu.browser.cache",      "");
pref("intl.charsetmenu.mailview.cache",     "");
pref("intl.charsetmenu.composer.cache",     "");
pref("intl.charsetmenu.browser.cache.size", 5);
pref("intl.charset.detector",               "chrome://navigator/locale/navigator.properties");
pref("intl.charset.default",                "chrome://navigator-platform/locale/navigator.properties");
pref("intl.content.langcode",               "chrome://communicator-region/locale/region.properties");
pref("intl.locale.matchOS",                 false);



pref("intl.fallbackCharsetList.ISO-8859-1", "windows-1252");
pref("font.language.group",                 "chrome://navigator/locale/navigator.properties");

pref("images.dither", "auto");
pref("security.directory",              "");

pref("signed.applets.codebase_principal_support", false);
pref("security.checkloaduri", true);
pref("security.xpconnect.plugin.unrestricted", true);

pref("security.dialog_enable_delay", 2000);




pref("ui.key.accelKey", 17);
pref("ui.key.menuAccessKey", 18);
pref("ui.key.generalAccessKey", -1);




pref("ui.key.chromeAccess", 4);
pref("ui.key.contentAccess", 5);

pref("ui.key.menuAccessKeyFocuses", false); 
pref("ui.key.saveLink.shift", true); 


pref("middlemouse.paste", false);
pref("middlemouse.openNewWindow", true);
pref("middlemouse.contentLoadURL", false);
pref("middlemouse.scrollbarPosition", false);


pref("clipboard.autocopy", false);


pref("mousewheel.transaction.timeout", 1500);

pref("mousewheel.transaction.ignoremovedelay", 100);


pref("mousewheel.withnokey.action",0);
pref("mousewheel.withnokey.numlines",1);	
pref("mousewheel.withnokey.sysnumlines",true);
pref("mousewheel.withcontrolkey.action",0);
pref("mousewheel.withcontrolkey.numlines",1);
pref("mousewheel.withcontrolkey.sysnumlines",true);

pref("mousewheel.withshiftkey.action",0);
pref("mousewheel.withshiftkey.numlines",1);
pref("mousewheel.withshiftkey.sysnumlines",true);
pref("mousewheel.withaltkey.action",2);
pref("mousewheel.withaltkey.numlines",1);
pref("mousewheel.withaltkey.sysnumlines",false);
pref("mousewheel.withmetakey.action",0);
pref("mousewheel.withmetakey.numlines",1);
pref("mousewheel.withmetakey.sysnumlines",true);



#ifdef XP_WIN
#define HORIZSCROLL_AVAILABLE
#endif
#ifdef XP_MACOSX
#define HORIZSCROLL_AVAILABLE







#endif
#ifdef XP_OS2
#define HORIZSCROLL_AVAILABLE
#endif
#ifdef HORIZSCROLL_AVAILABLE

pref("mousewheel.horizscroll.withnokey.action",0);
pref("mousewheel.horizscroll.withnokey.numlines",1);
pref("mousewheel.horizscroll.withnokey.sysnumlines",true);
pref("mousewheel.horizscroll.withcontrolkey.action",0);
pref("mousewheel.horizscroll.withcontrolkey.numlines",1);
pref("mousewheel.horizscroll.withcontrolkey.sysnumlines",true);
pref("mousewheel.horizscroll.withshiftkey.action",0);
pref("mousewheel.horizscroll.withshiftkey.numlines",1);
pref("mousewheel.horizscroll.withshiftkey.sysnumlines",true);
pref("mousewheel.horizscroll.withaltkey.action",2);
pref("mousewheel.horizscroll.withaltkey.numlines",-1);
pref("mousewheel.horizscroll.withaltkey.sysnumlines",false);
pref("mousewheel.horizscroll.withmetakey.action",0);
pref("mousewheel.horizscroll.withmetakey.numlines",1);
pref("mousewheel.horizscroll.withmetakey.sysnumlines",true);
#endif
#ifndef HORIZSCROLL_AVAILABLE

pref("mousewheel.horizscroll.withnokey.action",2);
pref("mousewheel.horizscroll.withnokey.numlines",-1);
pref("mousewheel.horizscroll.withnokey.sysnumlines",false);
pref("mousewheel.horizscroll.withcontrolkey.action",2);
pref("mousewheel.horizscroll.withcontrolkey.numlines",-1);
pref("mousewheel.horizscroll.withcontrolkey.sysnumlines",false);
pref("mousewheel.horizscroll.withshiftkey.action",2);
pref("mousewheel.horizscroll.withshiftkey.numlines",-1);
pref("mousewheel.horizscroll.withshiftkey.sysnumlines",false);
pref("mousewheel.horizscroll.withaltkey.action",2);
pref("mousewheel.horizscroll.withaltkey.numlines",-1);
pref("mousewheel.horizscroll.withaltkey.sysnumlines",false);
pref("mousewheel.horizscroll.withmetakey.action",2);
pref("mousewheel.horizscroll.withmetakey.numlines",-1);
pref("mousewheel.horizscroll.withmetakey.sysnumlines",false);
#endif

pref("profile.confirm_automigration",true);




pref("profile.migration_behavior",0);
pref("profile.migration_directory", "");








pref("profile.seconds_until_defunct", -1);

pref("profile.manage_only_at_launch", false);

pref("prefs.converted-to-utf8",false);










pref("bidi.direction", 1);






pref("bidi.texttype", 1);






pref("bidi.controlstextmode", 1);








pref("bidi.numeral", 0);






pref("bidi.support", 1);





pref("bidi.characterset", 1);



pref("bidi.edit.delete_immediately", false);





pref("bidi.edit.caret_movement_style", 2);


pref("layout.word_select.eat_space_to_next_word", false);
pref("layout.word_select.stop_at_punctuation", true);









pref("layout.selection.caret_style", 0);




pref("layout.enable_japanese_specific_transform", false);


pref("layout.frames.force_resizability", false);


pref("layout.css.report_errors", true);






pref("layout.scrollbar.side", 0);


pref("capability.policy.default.SOAPCall.invokeVerifySourceHeader", "allAccess");


pref("plugin.override_internal_types", false);
pref("plugin.expose_full_path", false); 



pref("browser.popups.showPopupBlocker", true);



pref("viewmanager.do_doublebuffering", true);



pref("roaming.default.files", "bookmarks.html,abook.mab,cookies.txt");

pref("roaming.showInitialWarning", true);


pref("config.use_system_prefs", false);


pref("config.use_system_prefs.accessibility", false);

















pref("editor.resizing.preserve_ratio",       true);
pref("editor.positioning.offset",            0);

pref("dom.max_chrome_script_run_time", 20);
pref("dom.max_script_run_time", 10);

pref("svg.enabled", true);

#ifdef XP_WIN
pref("font.name.serif.ar", "Times New Roman");
pref("font.name.sans-serif.ar", "Arial");
pref("font.name.monospace.ar", "Courier New");
pref("font.name.cursive.ar", "Comic Sans MS");

pref("font.name.serif.el", "Times New Roman");
pref("font.name.sans-serif.el", "Arial");
pref("font.name.monospace.el", "Courier New");
pref("font.name.cursive.el", "Comic Sans MS");

pref("font.name.serif.he", "Narkisim");
pref("font.name.sans-serif.he", "Arial");
pref("font.name.monospace.he", "Fixed Miriam Transparent");
pref("font.name.cursive.he", "Guttman Yad");
pref("font.name-list.serif.he", "Narkisim, David");
pref("font.name-list.monospace.he", "Fixed Miriam Transparent, Miriam Fixed, Rod, Courier New");
pref("font.name-list.cursive.he", "Guttman Yad, Ktav, Arial");






pref("font.name.serif.ja", "ＭＳ Ｐ明朝"); 
pref("font.name.sans-serif.ja", "ＭＳ Ｐゴシック"); 
pref("font.name.monospace.ja", "ＭＳ ゴシック"); 
pref("font.name-list.serif.ja", "MS PMincho, ＭＳ Ｐ明朝, MS Mincho, MS PGothic, MS Gothic");
pref("font.name-list.sans-serif.ja", "MS PGothic, ＭＳ Ｐゴシック, MS Gothic, MS PMincho, MS Mincho");
pref("font.name-list.monospace.ja", "MS Gothic, ＭＳ ゴシック, MS Mincho, ＭＳ 明朝, MS PGothic, MS PMincho");

pref("font.name.serif.ko", "바탕"); 
pref("font.name.sans-serif.ko", "굴림"); 
pref("font.name.monospace.ko", "굴림체"); 
pref("font.name.cursive.ko", "궁서"); 

pref("font.name-list.serif.ko", "Batang, 바탕, Gulim, 굴림"); 
pref("font.name-list.sans-serif.ko", "Gulim, 굴림"); 
pref("font.name-list.monospace.ko", "GulimChe, 굴림체"); 
pref("font.name-list.cursive.ko", "Gungseo, 궁서"); 

pref("font.name.serif.th", "Times New Roman");
pref("font.name.sans-serif.th", "Arial");
pref("font.name.monospace.th", "Courier New");
pref("font.name.cursive.th", "Comic Sans MS");

pref("font.name.serif.tr", "Times New Roman");
pref("font.name.sans-serif.tr", "Arial");
pref("font.name.monospace.tr", "Courier New");
pref("font.name.cursive.tr", "Comic Sans MS");

pref("font.name.serif.x-baltic", "Times New Roman");
pref("font.name.sans-serif.x-baltic", "Arial");
pref("font.name.monospace.x-baltic", "Courier New");
pref("font.name.cursive.x-baltic", "Comic Sans MS");

pref("font.name.serif.x-central-euro", "Times New Roman");
pref("font.name.sans-serif.x-central-euro", "Arial");
pref("font.name.monospace.x-central-euro", "Courier New");
pref("font.name.cursive.x-central-euro", "Comic Sans MS");

pref("font.name.serif.x-cyrillic", "Times New Roman");
pref("font.name.sans-serif.x-cyrillic", "Arial");
pref("font.name.monospace.x-cyrillic", "Courier New");
pref("font.name.cursive.x-cyrillic", "Comic Sans MS");

pref("font.name.serif.x-unicode", "Times New Roman");
pref("font.name.sans-serif.x-unicode", "Arial");
pref("font.name.monospace.x-unicode", "Courier New");
pref("font.name.cursive.x-unicode", "Comic Sans MS");

pref("font.name.serif.x-western", "Times New Roman");
pref("font.name.sans-serif.x-western", "Arial");
pref("font.name.monospace.x-western", "Courier New");
pref("font.name.cursive.x-western", "Comic Sans MS");

pref("font.name.serif.zh-CN", "宋体"); 
pref("font.name.sans-serif.zh-CN", "宋体"); 
pref("font.name.monospace.zh-CN", "宋体"); 
pref("font.name-list.serif.zh-CN", "MS Song, 宋体, SimSun");
pref("font.name-list.sans-serif.zh-CN", "MS Song, 宋体, SimSun");
pref("font.name-list.monospace.zh-CN", "MS Song, 宋体, SimSun");



pref("font.name.serif.zh-TW", "Times New Roman"); 
pref("font.name.sans-serif.zh-TW", "Arial"); 
pref("font.name.monospace.zh-TW", "細明體");  
pref("font.name-list.serif.zh-TW", "新細明體,PMingLiu,細明體,MingLiU"); 
pref("font.name-list.sans-serif.zh-TW", "新細明體,PMingLiU,細明體,MingLiU");
pref("font.name-list.monospace.zh-TW", "MingLiU,細明體");



pref("font.name.serif.zh-HK", "Times New Roman"); 
pref("font.name.sans-serif.zh-HK", "Arial"); 
pref("font.name.monospace.zh-HK", "細明體_HKSCS"); 
pref("font.name-list.serif.zh-HK", "細明體_HKSCS, MingLiu_HKSCS, Ming(for ISO10646), MingLiU, 細明體"); 
pref("font.name-list.sans-serif.zh-HK", "細明體_HKSCS, MingLiU_HKSCS, Ming(for ISO10646), MingLiU, 細明體");  
pref("font.name-list.monospace.zh-HK", "MingLiU_HKSCS,  細明體_HKSCS, Ming(for ISO10646), MingLiU, 細明體");

pref("font.name.serif.x-devanagari", "Mangal");
pref("font.name.sans-serif.x-devanagari", "Raghindi");
pref("font.name.monospace.x-devanagari", "Mangal");
pref("font.name-list.serif.x-devanagari", "Mangal, Raghindi");
pref("font.name-list.monospace.x-devanagari", "Mangal, Raghindi");

pref("font.name.serif.x-tamil", "Latha");
pref("font.name.sans-serif.x-tamil", "Code2000");
pref("font.name.monospace.x-tamil", "Latha");
pref("font.name-list.serif.x-tamil", "Latha, Code2000");
pref("font.name-list.monospace.x-tamil", "Latha, Code2000");

# http:

pref("font.name.serif.x-armn", "Sylfaen");
pref("font.name.sans-serif.x-armn", "Arial AMU");
pref("font.name.monospace.x-armn", "Arial AMU");
pref("font.name-list.serif.x-armn", "Sylfaen,Arial Unicode MS, Code2000");
pref("font.name-list.monospace.x-armn", "Arial AMU, Arial Unicode MS, Code2000");

pref("font.name.serif.x-beng", "Akaash");
pref("font.name.sans-serif.x-beng", "Likhan");
pref("font.name.monospace.x-beng", "Mitra Mono");
pref("font.name-list.serif.x-beng", "Akaash, Ekushey Punarbhaba, Code2000, Arial Unicode MS"); 
pref("font.name-list.monospace.x-beng", "Likhan, Mukti Narrow, Code 2000, Arial Unicode MS");

pref("font.name.serif.x-cans", "Aboriginal Serif");
pref("font.name.sans-serif.x-cans", "Aboriginal Sans");
pref("font.name.monospace.x-cans", "Aboriginal Sans");
pref("font.name-list.serif.x-cans", "Aboriginal Serif, BJCree Uni");
pref("font.name-list.monospace.x-cans", "Aboriginal Sans, OskiDakelh, Pigiarniq, Uqammaq");

pref("font.name.serif.x-ethi", "Visual Geez Unicode");
pref("font.name.sans-serif.x-ethi", "GF Zemen Unicode");
pref("font.name.cursive.x-ethi", "Visual Geez Unicode Title");
pref("font.name.monospace.x-ethi", "Ethiopia Jiret");
pref("font.name-list.serif.x-ethi", "Visual Geez Unicode, Visual Geez Unicode Agazian, Code2000");
pref("font.name-list.monospace.x-ethi", "Ethiopia Jiret, Code2000");

pref("font.name.serif.x-geor", "Sylfaen");
pref("font.name.sans-serif.x-geor", "BPG Classic 99U");
pref("font.name.monospace.x-geor", "Code2000");
pref("font.name-list.serif.x-geor", "Sylfaen, BPG Paata Khutsuri U, TITUS Cyberbit Basic"); 
pref("font.name-list.monospace.x-geor", "BPG Classic 99U, Code2000, Arial Unicode MS");

pref("font.name.serif.x-gujr", "Shruti");
pref("font.name.sans-serif.x-gujr", "Shruti");
pref("font.name.monospace.x-gujr", "Code2000");
pref("font.name-list.serif.x-gujr", "Shruti, Code2000, Arial Unicode MS"); 
pref("font.name-list.monospace.x-gujr", "Code2000, Shruti, Arial Unicode MS");

pref("font.name.serif.x-guru", "Raavi");
pref("font.name.sans-serif.x-guru", "Code2000");
pref("font.name.monospace.x-guru", "Code2000");
pref("font.name-list.serif.x-guru", "Raavi, Saab, Code2000, Arial Unicode MS"); 
pref("font.name-list.monospace.x-guru", "Code2000, Raavi, Saab, Arial Unicode MS");

pref("font.name.serif.x-khmr", "PhnomPenh OT");
pref("font.name.sans-serif.x-khmr", "Khmer OS");
pref("font.name.monospace.x-khmr", "Code2000");
pref("font.name-list.serif.x-khmr", "PhnomPenh OT,.Mondulkiri U GR 1.5, Khmer OS");
pref("font.name-list.monospace.x-khmr", "Code2000, Khmer OS, Khmer OS System");

pref("font.name.serif.x-mlym", "Kartika");
pref("font.name.sans-serif.x-mlym", "Anjali-Beta");
pref("font.name.monospace.x-mlym", "Code2000");
pref("font.name-list.serif.x-mlym", "Kartika, ThoolikaUnicode, Code2000, Arial Unicode MS");
pref("font.name-list.monospace.x-mlym", "Code2000, Anjali-Beta");

pref("font.default.ar", "sans-serif");
pref("font.size.variable.ar", 16);
pref("font.size.fixed.ar", 13);

pref("font.default.el", "serif");
pref("font.size.variable.el", 16);
pref("font.size.fixed.el", 13);

pref("font.default.he", "sans-serif");
pref("font.size.variable.he", 16);
pref("font.size.fixed.he", 13);

pref("font.default.ja", "sans-serif");
pref("font.size.variable.ja", 16);
pref("font.size.fixed.ja", 16);

pref("font.default.ko", "sans-serif");
pref("font.size.variable.ko", 16);
pref("font.size.fixed.ko", 16);

pref("font.default.th", "serif");
pref("font.size.variable.th", 16);
pref("font.size.fixed.th", 13);

pref("font.default.tr", "serif");
pref("font.size.variable.tr", 16);
pref("font.size.fixed.tr", 13);

pref("font.default.x-baltic", "serif");
pref("font.size.variable.x-baltic", 16);
pref("font.size.fixed.x-baltic", 13);

pref("font.default.x-central-euro", "serif");
pref("font.size.variable.x-central-euro", 16);
pref("font.size.fixed.x-central-euro", 13);

pref("font.default.x-cyrillic", "serif");
pref("font.size.variable.x-cyrillic", 16);
pref("font.size.fixed.x-cyrillic", 13);

pref("font.default.x-devanagari", "serif");
pref("font.size.variable.x-devanagari", 16);
pref("font.size.fixed.x-devanagari", 13);

pref("font.default.x-tamil", "serif");
pref("font.size.variable.x-tamil", 16);
pref("font.size.fixed.x-tamil", 13);

pref("font.default.x-armn", "serif");
pref("font.size.variable.x-armn", 16);
pref("font.size.fixed.x-armn", 13);

pref("font.default.x-beng", "serif");
pref("font.size.variable.x-beng", 16);
pref("font.size.fixed.x-beng", 13);

pref("font.default.x-cans", "serif");
pref("font.size.variable.x-cans", 16);
pref("font.size.fixed.x-cans", 13);

pref("font.default.x-ethi", "serif");
pref("font.size.variable.x-ethi", 16);
pref("font.size.fixed.x-ethi", 13);

pref("font.default.x-geor", "serif");
pref("font.size.variable.x-geor", 16);
pref("font.size.fixed.x-geor", 13);

pref("font.default.x-gujr", "serif");
pref("font.size.variable.x-gujr", 16);
pref("font.size.fixed.x-gujr", 13);

pref("font.default.x-guru", "serif");
pref("font.size.variable.x-guru", 16);
pref("font.size.fixed.x-guru", 13);

pref("font.default.x-khmr", "serif");
pref("font.size.variable.x-khmr", 16);
pref("font.size.fixed.x-khmr", 13);

pref("font.default.x-mlym", "serif");
pref("font.size.variable.x-mlym", 16);
pref("font.size.fixed.x-mlym", 13);

pref("font.default.x-unicode", "serif");
pref("font.size.variable.x-unicode", 16);
pref("font.size.fixed.x-unicode", 13);

pref("font.default.x-western", "serif");
pref("font.size.variable.x-western", 16);
pref("font.size.fixed.x-western", 13);

pref("font.default.zh-CN", "sans-serif");
pref("font.size.variable.zh-CN", 16);
pref("font.size.fixed.zh-CN", 16);

pref("font.default.zh-TW", "sans-serif");
pref("font.size.variable.zh-TW", 16);
pref("font.size.fixed.zh-TW", 16);

pref("font.default.zh-HK", "sans-serif");
pref("font.size.variable.zh-HK", 16);
pref("font.size.fixed.zh-HK", 16);

pref("ui.key.menuAccessKeyFocuses", true);


pref("layout.word_select.eat_space_to_next_word", true);


pref("slider.snapMultiplier", 6);



pref("print.print_extra_margin", 90); 


pref("print.use_native_print_dialog", true);


pref("print.extend_native_print_dialog", true);



pref("plugin.scan.SunJRE", "1.3");


pref("plugin.scan.Acrobat", "5.0");


pref("plugin.scan.Quicktime", "5.0");


pref("plugin.scan.WindowsMediaPlayer", "7.0");



pref("plugin.scan.plid.all", true);









pref("network.autodial-helper.enabled", true);






pref("advanced.system.supportDDEExec", true);


pref("intl.jis0208.map", "CP932");


pref("intl.keyboard.per_window_layout", false);

# WINNT
#endif

#ifdef XP_MACOSX

pref("browser.drag_out_of_frame_style", 1);
pref("ui.key.saveLink.shift", false); 
pref("ui.click_hold_context_menus", false);



pref("font.name.serif.ar", "Lucida Grande");
pref("font.name.sans-serif.ar", "Lucida Grande");
pref("font.name.monospace.ar", "Monaco");
pref("font.name.cursive.ar", "XXX.cursive");
pref("font.name.fantasy.ar", "XXX.fantasy");
pref("font.name-list.serif.ar", "Lucida Grande");
pref("font.name-list.sans-serif.ar", "Lucida Grande");
pref("font.name-list.monospace.ar", "Monaco");
pref("font.name-list.cursive.ar", "XXX.cursive");
pref("font.name-list.fantasy.ar", "XXX.fantasy");

pref("font.name.serif.el", "Lucida Grande");
pref("font.name.sans-serif.el", "Lucida Grande");
pref("font.name.monospace.el", "Monaco");
pref("font.name.cursive.el", "XXX.cursive");
pref("font.name.fantasy.el", "XXX.fantasy");
pref("font.name-list.serif.el", "Lucida Grande");
pref("font.name-list.sans-serif.el", "Lucida Grande");
pref("font.name-list.monospace.el", "Monaco");
pref("font.name-list.cursive.el", "XXX.cursive");
pref("font.name-list.fantasy.el", "XXX.fantasy");

pref("font.name.serif.he", "Lucida Grande");
pref("font.name.sans-serif.he", "Lucida Grande");
pref("font.name.monospace.he", "Monaco");
pref("font.name.cursive.he", "XXX.cursive");
pref("font.name.fantasy.he", "XXX.fantasy");
pref("font.name-list.serif.he", "Lucida Grande");
pref("font.name-list.sans-serif.he", "Lucida Grande");
pref("font.name-list.monospace.he", "Monaco");
pref("font.name-list.cursive.he", "XXX.cursive");
pref("font.name-list.fantasy.he", "XXX.fantasy");

pref("font.name.serif.ja", "ヒラギノ明朝 Pro"); 
pref("font.name.sans-serif.ja", "ヒラギノ角ゴ Pro"); 
pref("font.name.monospace.ja", "Osaka−等幅"); 
pref("font.name.cursive.ja", "XXX.cursive");
pref("font.name.fantasy.ja", "XXX.fantasy");
pref("font.name-list.serif.ja", "ヒラギノ明朝 Pro"); 
pref("font.name-list.sans-serif.ja", "ヒラギノ角ゴ Pro"); 
pref("font.name-list.monospace.ja", "Osaka−等幅"); 
pref("font.name-list.cursive.ja", "XXX.cursive");
pref("font.name-list.fantasy.ja", "XXX.fantasy");

pref("font.name.serif.ko", "AppleMyungjo"); 
pref("font.name.sans-serif.ko", "AppleGothic"); 
pref("font.name.monospace.ko", "AppleGothic"); 
pref("font.name.cursive.ko", "XXX.cursive");
pref("font.name.fantasy.ko", "XXX.fantasy");
pref("font.name-list.serif.ko", "AppleMyungjo"); 
pref("font.name-list.sans-serif.ko", "AppleGothic"); 
pref("font.name-list.monospace.ko", "AppleGothic"); 
pref("font.name-list.cursive.ko", "XXX.cursive");
pref("font.name-list.fantasy.ko", "XXX.fantasy");

pref("font.name.serif.th", "Lucida Grande");
pref("font.name.sans-serif.th", "Lucida Grande");
pref("font.name.monospace.th", "Monaco");
pref("font.name.cursive.th", "XXX.cursive");
pref("font.name.fantasy.th", "XXX.fantasy");
pref("font.name-list.serif.th", "Lucida Grande");
pref("font.name-list.sans-serif.th", "Lucida Grande");
pref("font.name-list.monospace.th", "Monaco");
pref("font.name-list.cursive.th", "XXX.cursive");
pref("font.name-list.fantasy.th", "XXX.fantasy");

pref("font.name.serif.tr", "Times");
pref("font.name.sans-serif.tr", "Helvetica");
pref("font.name.monospace.tr", "Courier");
pref("font.name.cursive.tr", "Apple Chancery");
pref("font.name.fantasy.tr", "Papyrus");
pref("font.name-list.serif.tr", "Times");
pref("font.name-list.sans-serif.tr", "Helvetica");
pref("font.name-list.monospace.tr", "Courier");
pref("font.name-list.cursive.tr", "Apple Chancery");
pref("font.name-list.fantasy.tr", "Papyrus");

pref("font.name.serif.x-baltic", "Times");
pref("font.name.sans-serif.x-baltic", "Helvetica");
pref("font.name.monospace.x-baltic", "Courier");
pref("font.name.cursive.x-baltic", "Apple Chancery");
pref("font.name.fantasy.x-baltic", "Papyrus");
pref("font.name-list.serif.x-baltic", "Times");
pref("font.name-list.sans-serif.x-baltic", "Helvetica");
pref("font.name-list.monospace.x-baltic", "Courier");
pref("font.name-list.cursive.x-baltic", "Apple Chancery");
pref("font.name-list.fantasy.x-baltic", "Papyrus");

pref("font.name.serif.x-central-euro", "Times");
pref("font.name.sans-serif.x-central-euro", "Helvetica");
pref("font.name.monospace.x-central-euro", "Courier");
pref("font.name.cursive.x-central-euro", "Apple Chancery");
pref("font.name.fantasy.x-central-euro", "Papyrus");
pref("font.name-list.serif.x-central-euro", "Times");
pref("font.name-list.sans-serif.x-central-euro", "Helvetica");
pref("font.name-list.monospace.x-central-euro", "Courier");
pref("font.name-list.cursive.x-central-euro", "Apple Chancery");
pref("font.name-list.fantasy.x-central-euro", "Papyrus");

pref("font.name.serif.x-cyrillic", "Times CY");
pref("font.name.sans-serif.x-cyrillic", "Helvetica CY");
pref("font.name.monospace.x-cyrillic", "Monaco CY");
pref("font.name.cursive.x-cyrillic", "Geneva CY");
pref("font.name.fantasy.x-cyrillic", "Charcoal CY");
pref("font.name-list.serif.x-cyrillic", "Times CY");
pref("font.name-list.sans-serif.x-cyrillic", "Helvetica CY");
pref("font.name-list.monospace.x-cyrillic", "Monaco CY");
pref("font.name-list.cursive.x-cyrillic", "Geneva CY");
pref("font.name-list.fantasy.x-cyrillic", "Charcoal CY");

pref("font.name.serif.x-unicode", "Times");
pref("font.name.sans-serif.x-unicode", "Helvetica");
pref("font.name.monospace.x-unicode", "Courier");
pref("font.name.cursive.x-unicode", "Apple Chancery");
pref("font.name.fantasy.x-unicode", "Papyrus");
pref("font.name-list.serif.x-unicode", "Times");
pref("font.name-list.sans-serif.x-unicode", "Helvetica");
pref("font.name-list.monospace.x-unicode", "Courier");
pref("font.name-list.cursive.x-unicode", "Apple Chancery");
pref("font.name-list.fantasy.x-unicode", "Papyrus");

pref("font.name.serif.x-western", "Times");
pref("font.name.sans-serif.x-western", "Helvetica");
pref("font.name.monospace.x-western", "Courier");
pref("font.name.cursive.x-western", "Apple Chancery");
pref("font.name.fantasy.x-western", "Papyrus");
pref("font.name-list.serif.x-western", "Times");
pref("font.name-list.sans-serif.x-western", "Helvetica");
pref("font.name-list.monospace.x-western", "Courier");
pref("font.name-list.cursive.x-western", "Apple Chancery");
pref("font.name-list.fantasy.x-western", "Papyrus");

pref("font.name.serif.zh-CN", "Song");
pref("font.name.sans-serif.zh-CN", "Hei");
pref("font.name.monospace.zh-CN", "Hei");
pref("font.name.cursive.zh-CN", "XXX.cursive");
pref("font.name.fantasy.zh-CN", "XXX.fantasy");
pref("font.name-list.serif.zh-CN", "Song");
pref("font.name-list.sans-serif.zh-CN", "Hei");
pref("font.name-list.monospace.zh-CN", "Hei");
pref("font.name-list.cursive.zh-CN", "XXX.cursive");
pref("font.name-list.fantasy.zh-CN", "XXX.fantasy");

pref("font.name.serif.zh-TW", "Apple LiSung Light"); 
pref("font.name.sans-serif.zh-TW", "Apple LiGothic Medium");  
pref("font.name.monospace.zh-TW", "Apple LiGothic Medium");  
pref("font.name.cursive.zh-TW", "XXX.cursive");
pref("font.name.fantasy.zh-TW", "XXX.fantasy");
pref("font.name-list.serif.zh-TW", "Apple LiSung Light"); 
pref("font.name-list.sans-serif.zh-TW", "Apple LiGothic Medium");  
pref("font.name-list.monospace.zh-TW", "Apple LiGothic Medium");  
pref("font.name-list.cursive.zh-TW", "XXX.cursive");
pref("font.name-list.fantasy.zh-TW", "XXX.fantasy");

pref("font.name.serif.zh-HK", "儷宋 Pro");
pref("font.name.sans-serif.zh-HK", "儷黑 Pro");
pref("font.name.monospace.zh-HK", "儷黑 Pro");
pref("font.name.cursive.zh-HK", "XXX.cursive");
pref("font.name.fantasy.zh-HK", "XXX.fantasy");
pref("font.name-list.serif.zh-HK", "儷宋 Pro");
pref("font.name-list.sans-serif.zh-HK", "儷黑 Pro");
pref("font.name-list.monospace.zh-HK", "儷黑 Pro");
pref("font.name-list.cursive.zh-HK", "XXX.cursive");
pref("font.name-list.fantasy.zh-HK", "XXX.fantasy");

pref("font.default.ar", "sans-serif");
pref("font.size.variable.ar", 15);
pref("font.size.fixed.ar", 13);

pref("font.default.el", "serif");
pref("font.size.variable.el", 16);
pref("font.size.fixed.el", 13);

pref("font.default.he", "sans-serif");
pref("font.size.variable.he", 15);
pref("font.size.fixed.he", 13);

pref("font.default.ja", "sans-serif");
pref("font.size.variable.ja", 14);
pref("font.size.fixed.ja", 14);

pref("font.default.ko", "sans-serif");
pref("font.size.variable.ko", 16);
pref("font.size.fixed.ko", 16);

pref("font.default.th", "serif");
pref("font.size.variable.th", 16);
pref("font.size.fixed.th", 13);

pref("font.default.tr", "serif");
pref("font.size.variable.tr", 16);
pref("font.size.fixed.tr", 13);

pref("font.default.x-baltic", "serif");
pref("font.size.variable.x-baltic", 16);
pref("font.size.fixed.x-baltic", 13);

pref("font.default.x-central-euro", "serif");
pref("font.size.variable.x-central-euro", 16);
pref("font.size.fixed.x-central-euro", 13);

pref("font.default.x-cyrillic", "serif");
pref("font.size.variable.x-cyrillic", 16);
pref("font.size.fixed.x-cyrillic", 13);

pref("font.default.x-unicode", "serif");
pref("font.size.variable.x-unicode", 16);
pref("font.size.fixed.x-unicode", 13);

pref("font.default.x-western", "serif");
pref("font.size.variable.x-western", 16);
pref("font.size.fixed.x-western", 13);

pref("font.default.zh-CN", "sans-serif");
pref("font.size.variable.zh-CN", 15);
pref("font.size.fixed.zh-CN", 16);

pref("font.default.zh-TW", "sans-serif");
pref("font.size.variable.zh-TW", 15);
pref("font.size.fixed.zh-TW", 16);

pref("font.default.zh-HK", "sans-serif");
pref("font.size.variable.zh-HK", 15);
pref("font.size.fixed.zh-HK", 16);

pref("browser.urlbar.clickAtEndSelects", false);



pref("ui.key.menuAccessKey", 0);
pref("ui.key.accelKey", 224);




pref("ui.key.generalAccessKey", -1);




pref("ui.key.chromeAccess", 2);
pref("ui.key.contentAccess", 2);



pref("print.print_extra_margin", 90); 


pref("print.use_native_print_dialog", true);





pref("network.dns.disableIPv6", true);

# XP_MACOSX
#endif

#ifdef XP_OS2

pref("ui.key.menuAccessKeyFocuses", true);
pref("layout.css.dpi", -1); 

pref("font.alias-list", "sans,sans-serif,serif,monospace");




pref("font.name.serif.ar", "Tms Rmn");
pref("font.name.sans-serif.ar", "Helv");
pref("font.name.monospace.ar", "Courier");

pref("font.name.serif.el", "Tms Rmn");
pref("font.name.sans-serif.el", "Helv");
pref("font.name.monospace.el", "Courier");

pref("font.name.serif.he", "Tms Rmn");
pref("font.name.sans-serif.he", "Helv");
pref("font.name.monospace.he", "Courier");

pref("font.name.serif.ja", "Times New Roman WT J");
pref("font.name-list.serif.ja", "Times New Roman WT J, Times New Roman MT 30, Tms Rmn");
pref("font.name.sans-serif.ja", "Helv");
pref("font.name.monospace.ja", "Courier");

pref("font.name.serif.ko", "Times New Roman WT K");
pref("font.name-list.serif.ko", "Times New Roman WT K, Times New Roman MT 30, Tms Rmn");
pref("font.name.sans-serif.ko", "Helv");
pref("font.name.monospace.ko", "Courier");

pref("font.name.serif.th", "Tms Rmn");
pref("font.name.sans-serif.th", "Helv");
pref("font.name.monospace.th", "Courier");

pref("font.name.serif.tr", "Tms Rmn");
pref("font.name.sans-serif.tr", "Helv");
pref("font.name.monospace.tr", "Courier");

pref("font.name.serif.x-baltic", "Tms Rmn");
pref("font.name.sans-serif.x-baltic", "Helv");
pref("font.name.monospace.x-baltic", "Courier");

pref("font.name.serif.x-central-euro", "Tms Rmn");
pref("font.name.sans-serif.x-central-euro", "Tms Rmn");
pref("font.name.monospace.x-central-euro", "Courier");

pref("font.name.serif.x-cyrillic", "Tms Rmn");
pref("font.name.sans-serif.x-cyrillic", "Tms Rmn");
pref("font.name.monospace.x-cyrillic", "Courier");


pref("font.name.serif.x-unicode", "Times New Roman MT 30");
pref("font.name-list.serif.x-unicode", "Times New Roman MT 30, Times New Roman WT J, Times New Roman WT SC, Times New Roman WT TC, Times New Roman WT K, Tms Rmn");
pref("font.name.sans-serif.x-unicode", "Times New Roman MT 30");
pref("font.name-list.sans-serif.x-unicode", "Times New Roman MT 30, Times New Roman WT J, Times New Roman WT SC, Times New Roman WT TC, Times New Roman WT K, Helv");
pref("font.name.monospace.x-unicode", "Times New Roman MT 30");
pref("font.name-list.monospace.x-unicode", "Times New Roman MT 30, Times New Roman WT J, Times New Roman WT SC, Times New Roman WT TC, Times New Roman WT K, Courier");
pref("font.name.fantasy.x-unicode", "Times New Roman MT 30");
pref("font.name-list.fantasy.x-unicode", "Times New Roman MT 30, Times New Roman WT J, Times New Roman WT SC, Times New Roman WT TC, Times New Roman WT K, Helv");
pref("font.name.cursive.x-unicode", "Times New Roman MT 30");
pref("font.name-list.cursive.x-unicode", "Times New Roman MT 30, Times New Roman WT J, Times New Roman WT SC, Times New Roman WT TC, Times New Roman WT K, Helv");

pref("font.name.serif.x-western", "Tms Rmn");
pref("font.name.sans-serif.x-western", "Helv");
pref("font.name.monospace.x-western", "Courier");

pref("font.name.serif.zh-CN", "Times New Roman WT SC");
pref("font.name-list.serif.zh_CN", "Times New Roman WT SC, Times New Roman MT 30, Tms Rmn");
pref("font.name.sans-serif.zh-CN", "Helv");
pref("font.name.monospace.zh-CN", "Courier");

pref("font.name.serif.zh-TW", "Times New Roman WT TC");
pref("font.name-list.serif.zh-TW", "Times New Roman WT TC, Times New Roman MT 30, Tms Rmn");
pref("font.name.sans-serif.zh-TW", "Helv");
pref("font.name.monospace.zh-TW", "Courier");


pref("font.name.serif.zh-HK", "Times New Roman WT TC");
pref("font.name-list.serif.zh-HK", "Times New Roman WT TC, Times New Roman MT 30, Tms Rmn");
pref("font.name.sans-serif.zh-HK", "Helv");
pref("font.name.monospace.zh-HK", "Courier");

pref("font.default", "serif");

pref("font.default.ar", "sans-serif");
pref("font.size.variable.ar", 16);
pref("font.size.fixed.ar", 13);

pref("font.default.el", "serif");
pref("font.size.variable.el", 16);
pref("font.size.fixed.el", 13);

pref("font.default.he", "sans-serif");
pref("font.size.variable.he", 16);
pref("font.size.fixed.he", 13);

pref("font.default.ja", "sans-serif");
pref("font.size.variable.ja", 16);
pref("font.size.fixed.ja", 16);

pref("font.default.ko", "sans-serif");
pref("font.size.variable.ko", 16);
pref("font.size.fixed.ko", 16);

pref("font.default.th", "serif");
pref("font.size.variable.th", 16);
pref("font.size.fixed.th", 13);

pref("font.default.tr", "serif");
pref("font.size.variable.tr", 16);
pref("font.size.fixed.tr", 13);

pref("font.default.x-baltic", "serif");
pref("font.size.variable.x-baltic", 16);
pref("font.size.fixed.x-baltic", 13);

pref("font.default.x-central-euro", "serif");
pref("font.size.variable.x-central-euro", 16);
pref("font.size.fixed.x-central-euro", 13);

pref("font.default.x-cyrillic", "serif");
pref("font.size.variable.x-cyrillic", 16);
pref("font.size.fixed.x-cyrillic", 13);

pref("font.default.x-devanagari", "serif");
pref("font.size.variable.x-devanagari", 16);
pref("font.size.fixed.x-devanagari", 13);

pref("font.default.x-tamil", "serif");
pref("font.size.variable.x-tamil", 16);
pref("font.size.fixed.x-tamil", 13);

pref("font.default.x-unicode", "serif");
pref("font.size.variable.x-unicode", 16);
pref("font.size.fixed.x-unicode", 13);

pref("font.default.x-western", "serif");
pref("font.size.variable.x-western", 16);
pref("font.size.fixed.x-western", 13);

pref("font.default.zh-CN", "sans-serif");
pref("font.size.variable.zh-CN", 16);
pref("font.size.fixed.zh-CN", 16);

pref("font.default.zh-TW", "sans-serif");
pref("font.size.variable.zh-TW", 16);
pref("font.size.fixed.zh-TW", 16);

pref("font.default.zh-HK", "sans-serif");
pref("font.size.variable.zh-HK", 16);
pref("font.size.fixed.zh-HK", 16);

pref("netinst.profile.show_profile_wizard", true); 

pref("middlemouse.paste", true);


pref("layout.word_select.eat_space_to_next_word", true);
pref("layout.word_select.stop_at_punctuation", false);





pref("browser.display.substitute_vector_fonts", true);



pref("print.print_extra_margin", 90); 

pref("applications.telnet", "telnetpm.exe");
pref("applications.telnet.host", "%host%");
pref("applications.telnet.port", "-p %port%");

pref("mail.compose.max_recycled_windows", 0);


pref("intl.jis0208.map", "IBM943");



pref("network.dns.disableIPv6", true);

# OS2
#endif

#ifdef XP_BEOS

pref("layout.css.dpi", -1); 

pref("intl.font_charset", "");
pref("intl.font_spec_list", "");
pref("mail.signature_date", 0);

pref("font.alias-list", "sans,sans-serif,serif,monospace");

pref("font.default.ar", "sans-serif");
pref("font.size.variable.ar", 16);
pref("font.size.fixed.ar", 13);

pref("font.default.el", "serif");
pref("font.size.variable.el", 16);
pref("font.size.fixed.el", 13);

pref("font.default.he", "sans-serif");
pref("font.size.variable.he", 16);
pref("font.size.fixed.he", 13);

pref("font.default.ja", "sans-serif");
pref("font.size.variable.ja", 16);
pref("font.size.fixed.ja", 16);

pref("font.default.ko", "sans-serif");
pref("font.size.variable.ko", 16);
pref("font.size.fixed.ko", 16);

pref("font.default.th", "serif");
pref("font.size.variable.th", 16);
pref("font.size.fixed.th", 13);

pref("font.default.tr", "serif");
pref("font.size.variable.tr", 16);
pref("font.size.fixed.tr", 13);

pref("font.default.x-baltic", "serif");
pref("font.size.variable.x-baltic", 16);
pref("font.size.fixed.x-baltic", 13);

pref("font.default.x-central-euro", "serif");
pref("font.size.variable.x-central-euro", 16);
pref("font.size.fixed.x-central-euro", 13);

pref("font.default.x-cyrillic", "serif");
pref("font.size.variable.x-cyrillic", 16);
pref("font.size.fixed.x-cyrillic", 13);

pref("font.default.x-unicode", "serif");
pref("font.size.variable.x-unicode", 16);
pref("font.size.fixed.x-unicode", 13);

pref("font.default.x-western", "serif");
pref("font.size.variable.x-western", 16);
pref("font.size.fixed.x-western", 13);

pref("font.default.zh-CN", "sans-serif");
pref("font.size.variable.zh-CN", 16);
pref("font.size.fixed.zh-CN", 16);

pref("font.default.zh-TW", "sans-serif");
pref("font.size.variable.zh-TW", 16);
pref("font.size.fixed.zh-TW", 16);

pref("font.default.zh-HK", "sans-serif");
pref("font.size.variable.zh-HK", 16);
pref("font.size.fixed.zh-HK", 16);






pref("ui.key.accelKey", 18);
pref("ui.key.menuAccessKey", 17);
pref("ui.key.generalAccessKey", -1);




pref("ui.key.chromeAccess", 2);
pref("ui.key.contentAccess", 3);


pref("browser.download.dir", "/boot/home/Downloads");

# BeOS
#endif

#ifndef XP_MACOSX
#ifdef XP_UNIX

pref("network.hosts.smtp_server", "localhost");
pref("network.hosts.pop_server", "pop");
pref("network.protocol-handler.warn-external.file", false);
pref("layout.css.dpi", -1); 
pref("browser.drag_out_of_frame_style", 1);
pref("editor.singleLine.pasteNewlines", 0);


pref("middlemouse.paste", true);
pref("middlemouse.contentLoadURL", true);
pref("middlemouse.openNewWindow", true);
pref("middlemouse.scrollbarPosition", true);


pref("clipboard.autocopy", true);

pref("browser.urlbar.clickSelectsAll", false);







pref("autocomplete.grab_during_popup", true);
pref("autocomplete.ungrab_during_mode_switch", true);



pref("ui.allow_platform_file_picker", true);

pref("helpers.global_mime_types_file", "/etc/mime.types");
pref("helpers.global_mailcap_file", "/etc/mailcap");
pref("helpers.private_mime_types_file", "~/.mime.types");
pref("helpers.private_mailcap_file", "~/.mailcap");
pref("java.global_java_version_file", "/etc/.java/versions");
pref("java.private_java_version_file", "~/.java/versions");
pref("java.default_java_location_solaris", "/usr/j2se");
pref("java.default_java_location_others", "/usr/java");
pref("java.java_plugin_library_name", "javaplugin_oji");
pref("applications.telnet", "xterm -e telnet %h %p");
pref("applications.tn3270", "xterm -e tn3270 %h");
pref("applications.rlogin", "xterm -e rlogin %h");
pref("applications.rlogin_with_user", "xterm -e rlogin %h -l %u");
pref("print.print_command", "lpr ${MOZ_PRINTER_NAME:+'-P'}${MOZ_PRINTER_NAME}");
pref("print.printer_list", ""); 
pref("print.print_reversed", false);
pref("print.print_color", true);
pref("print.print_landscape", false);
pref("print.print_paper_size", 0);



pref("print.print_edge_top", 4); 
pref("print.print_edge_left", 4); 
pref("print.print_edge_right", 4); 
pref("print.print_edge_bottom", 4); 



pref("print.print_extra_margin", 0); 

pref("print.whileInPrintPreview", false);

pref("font.allow_double_byte_special_chars", true);


pref("font.alias-list", "sans,sans-serif,serif,monospace");



#ifdef MOZ_ENABLE_XFT

pref("font.name.serif.el", "serif");
pref("font.name.sans-serif.el", "sans-serif");
pref("font.name.monospace.el", "monospace");

pref("font.name.serif.he", "serif");
pref("font.name.sans-serif.he", "sans-serif");
pref("font.name.monospace.he", "monospace");

pref("font.name.serif.ja", "serif");
pref("font.name.sans-serif.ja", "sans-serif");
pref("font.name.monospace.ja", "monospace");

pref("font.name.serif.ko", "serif");
pref("font.name.sans-serif.ko", "sans-serif");
pref("font.name.monospace.ko", "monospace");



pref("font.name.serif.tr", "Times");
pref("font.name.sans-serif.tr", "Helvetica");
pref("font.name.monospace.tr", "Courier");

pref("font.name.serif.x-baltic", "serif");
pref("font.name.sans-serif.x-baltic", "sans-serif");
pref("font.name.monospace.x-baltic", "monospace");

pref("font.name.serif.x-central-euro", "Times");
pref("font.name.sans-serif.x-central-euro", "Helvetica");
pref("font.name.monospace.x-central-euro", "Courier");

pref("font.name.serif.x-cyrillic", "serif");
pref("font.name.sans-serif.x-cyrillic", "sans-serif");
pref("font.name.monospace.x-cyrillic", "monospace");

pref("font.name.serif.x-unicode", "Times");
pref("font.name.sans-serif.x-unicode", "Helvetica");
pref("font.name.monospace.x-unicode", "Courier");

pref("font.name.serif.x-user-def", "Times");
pref("font.name.sans-serif.x-user-def", "Helvetica");
pref("font.name.monospace.x-user-def", "Courier");

pref("font.name.serif.x-western", "Times");
pref("font.name.sans-serif.x-western", "Helvetica");
pref("font.name.monospace.x-western", "Courier");

pref("font.name.serif.zh-CN", "serif");
pref("font.name.sans-serif.zh-CN", "sans-serif");
pref("font.name.monospace.zh-CN", "monospace");



pref("font.name.serif.zh-HK", "serif");
pref("font.name.sans-serif.zh-HK", "sans-serif");
pref("font.name.monospace.zh-HK", "monospace");


# MOZ_ENABLE_XFT
#else
pref("font.name.serif.el", "misc-fixed-iso8859-7");
pref("font.name.sans-serif.el", "misc-fixed-iso8859-7");
pref("font.name.monospace.el", "misc-fixed-iso8859-7");

pref("font.name.serif.he", "misc-fixed-iso8859-8");
pref("font.name.sans-serif.he", "misc-fixed-iso8859-8");
pref("font.name.monospace.he", "misc-fixed-iso8859-8");

pref("font.name.serif.ja", "jis-fixed-jisx0208.1983-0");
pref("font.name.sans-serif.ja", "jis-fixed-jisx0208.1983-0");
pref("font.name.monospace.ja", "jis-fixed-jisx0208.1983-0");

pref("font.name.serif.ko", "daewoo-mincho-ksc5601.1987-0");
pref("font.name.sans-serif.ko", "daewoo-mincho-ksc5601.1987-0");
pref("font.name.monospace.ko", "daewoo-mincho-ksc5601.1987-0");



pref("font.name.serif.tr", "adobe-times-iso8859-9");
pref("font.name.sans-serif.tr", "adobe-helvetica-iso8859-9");
pref("font.name.monospace.tr", "adobe-courier-iso8859-9");

pref("font.name.serif.x-baltic", "b&h-lucidux serif-iso8859-4");
pref("font.name.sans-serif.x-baltic", "b&h-lucidux sans-iso8859-4");
pref("font.name.monospace.x-baltic", "b&h-lucidux mono-iso8859-4");

pref("font.name.serif.x-central-euro", "adobe-times-iso8859-2");
pref("font.name.sans-serif.x-central-euro", "adobe-helvetica-iso8859-2");
pref("font.name.monospace.x-central-euro", "adobe-courier-iso8859-2");

pref("font.name.serif.x-cyrillic", "cronyx-times-koi8-r");
pref("font.name.sans-serif.x-cyrillic", "cronyx-helvetica-koi8-r");
pref("font.name.monospace.x-cyrillic", "cronyx-courier-koi8-r");

pref("font.name.serif.x-unicode", "adobe-times-iso8859-1");
pref("font.name.sans-serif.x-unicode", "adobe-helvetica-iso8859-1");
pref("font.name.monospace.x-unicode", "adobe-courier-iso8859-1");

pref("font.name.serif.x-user-def", "adobe-times-iso8859-1");
pref("font.name.sans-serif.x-user-def", "adobe-helvetica-iso8859-1");
pref("font.name.monospace.x-user-def", "adobe-courier-iso8859-1");

pref("font.name.serif.x-western", "adobe-times-iso8859-1");
pref("font.name.sans-serif.x-western", "adobe-helvetica-iso8859-1");
pref("font.name.monospace.x-western", "adobe-courier-iso8859-1");

pref("font.name.serif.zh-CN", "isas-song ti-gb2312.1980-0");
pref("font.name.sans-serif.zh-CN", "isas-song ti-gb2312.1980-0");
pref("font.name.monospace.zh-CN", "isas-song ti-gb2312.1980-0");



pref("font.name.serif.zh-HK", "-arphic-Ming for ISO10646-big5hkscs-0");
pref("font.name.sans-serif.zh-HK", "-arphic-Ming for ISO10646-big5hkscs-0");
pref("font.name.monospace.zh-HK", "-arphic-Ming for ISO10646-big5hkscs-0");


# MOZ_ENABLE_XFT
#endif

pref("font.default.ar", "sans-serif");
pref("font.size.variable.ar", 16);
pref("font.size.fixed.ar", 12);

pref("font.default.el", "serif");
pref("font.size.variable.el", 16);
pref("font.size.fixed.el", 12);

pref("font.default.he", "sans-serif");
pref("font.size.variable.he", 16);
pref("font.size.fixed.he", 12);

pref("font.default.ja", "sans-serif");
pref("font.size.variable.ja", 16);
pref("font.size.fixed.ja", 16);

pref("font.default.ko", "sans-serif");
pref("font.size.variable.ko", 16);
pref("font.size.fixed.ko", 16);

pref("font.default.th", "serif");
pref("font.size.variable.th", 16);
pref("font.size.fixed.th", 12);

pref("font.default.tr", "serif");
pref("font.size.variable.tr", 16);
pref("font.size.fixed.tr", 12);

pref("font.default.x-baltic", "serif");
pref("font.size.variable.x-baltic", 16);
pref("font.size.fixed.x-baltic", 12);

pref("font.default.x-central-euro", "serif");
pref("font.size.variable.x-central-euro", 16);
pref("font.size.fixed.x-central-euro", 12);

pref("font.default.x-cyrillic", "serif");
pref("font.size.variable.x-cyrillic", 16);
pref("font.size.fixed.x-cyrillic", 12);

pref("font.default.x-unicode", "serif");
pref("font.size.variable.x-unicode", 16);
pref("font.size.fixed.x-unicode", 12);

pref("font.default.x-western", "serif");
pref("font.size.variable.x-western", 16);
pref("font.size.fixed.x-western", 12);

pref("font.default.zh-CN", "sans-serif");
pref("font.size.variable.zh-CN", 16);
pref("font.size.fixed.zh-CN", 16);

pref("font.default.zh-TW", "sans-serif");
pref("font.size.variable.zh-TW", 16);
pref("font.size.fixed.zh-TW", 16);

pref("font.default.zh-HK", "sans-serif");
pref("font.size.variable.zh-HK", 16);
pref("font.size.fixed.zh-HK", 16);


pref("font.scale.outline.min",      6);


pref("font.FreeType2.enable", false);
pref("font.freetype2.shared-library", "libfreetype.so.6");


pref("font.FreeType2.autohinted", false);
pref("font.FreeType2.unhinted", true);

pref("font.antialias.min",        10);
pref("font.embedded_bitmaps.max", 1000000);
pref("font.scale.tt_bitmap.dark_text.min", 64);
pref("font.scale.tt_bitmap.dark_text.gain", "0.8");




pref("font.FreeType2.printing", true);



pref("font.scale.aa_bitmap.enable", true);
pref("font.scale.aa_bitmap.always", false);
pref("font.scale.aa_bitmap.min", 6);
pref("font.scale.aa_bitmap.undersize", 80);
pref("font.scale.aa_bitmap.oversize", 120);
pref("font.scale.aa_bitmap.dark_text.min", 64);
pref("font.scale.aa_bitmap.dark_text.gain", "0.5");
pref("font.scale.aa_bitmap.light_text.min", 64);
pref("font.scale.aa_bitmap.light_text.gain", "1.3");

pref("font.scale.bitmap.min",       12);
pref("font.scale.bitmap.undersize", 80);
pref("font.scale.bitmap.oversize",  120);

pref("font.scale.outline.min.ja",      10);
pref("font.scale.aa_bitmap.min.ja",    12);
pref("font.scale.aa_bitmap.always.ja", false);
pref("font.scale.bitmap.min.ja",       16);
pref("font.scale.bitmap.undersize.ja", 80);
pref("font.scale.bitmap.oversize.ja",  120);

pref("font.scale.outline.min.ko",      10);
pref("font.scale.aa_bitmap.min.ko",    12);
pref("font.scale.aa_bitmap.always.ko", false);
pref("font.scale.bitmap.min.ko",       16);
pref("font.scale.bitmap.undersize.ko", 80);
pref("font.scale.bitmap.oversize.ko",  120);

pref("font.scale.outline.min.zh-CN",      10);
pref("font.scale.aa_bitmap.min.zh-CN",    12);
pref("font.scale.aa_bitmap.always.zh-CN", false);
pref("font.scale.bitmap.min.zh-CN",       16);
pref("font.scale.bitmap.undersize.zh-CN", 80);
pref("font.scale.bitmap.oversize.zh-CN",  120);

pref("font.scale.outline.min.zh-TW",      10);
pref("font.scale.aa_bitmap.min.zh-TW",    12);
pref("font.scale.aa_bitmap.always.zh-TW", false);
pref("font.scale.bitmap.min.zh-TW",       16);
pref("font.scale.bitmap.undersize.zh-TW", 80);
pref("font.scale.bitmap.oversize.zh-TW",  120);

pref("font.scale.outline.min.zh-HK",      10);
pref("font.scale.aa_bitmap.min.zh-HK",    12);
pref("font.scale.aa_bitmap.always.zh-HK", false);
pref("font.scale.bitmap.min.zh-HK",       16);
pref("font.scale.bitmap.undersize.zh-HK", 80);
pref("font.scale.bitmap.oversize.zh-HK",  120);



pref("font.min-size.variable.ja", 10);
pref("font.min-size.fixed.ja", 10);

pref("font.min-size.variable.ko", 10);
pref("font.min-size.fixed.ko", 10);

pref("font.min-size.variable.zh-CN", 10);
pref("font.min-size.fixed.zh-CN", 10);

pref("font.min-size.variable.zh-TW", 10);
pref("font.min-size.fixed.zh-TW", 10);

pref("font.min-size.variable.zh-HK", 10);
pref("font.min-size.fixed.zh-HK", 10);




























pref("print.postscript.paper_size",    "letter");
pref("print.postscript.orientation",   "portrait");
pref("print.postscript.print_command", "lpr ${MOZ_PRINTER_NAME:+'-P'}${MOZ_PRINTER_NAME}");

# XP_UNIX
#endif
#endif

#if MOZ_WIDGET_TOOLKIT==photon


pref("font.name.serif.x-western", "serif");
pref("font.name.sans-serif.x-western", "sans-serif");
pref("font.name.monospace.x-western", "monospace");
pref("font.name.cursive.x-western", "cursive");
pref("font.name.fantasy.x-western", "fantasy");

pref("font.name.serif.el", "serif");
pref("font.name.sans-serif.el", "sans-serif");
pref("font.name.monospace.el", "monospace");

pref("font.name.serif.he", "serif");
pref("font.name.sans-serif.he", "sans-serif");
pref("font.name.monospace.he", "monospace");

pref("font.name.serif.ja", "serif");
pref("font.name.sans-serif.ja", "sans-serif");
pref("font.name.monospace.ja", "monospace");

pref("font.name.serif.ko", "serif");
pref("font.name.sans-serif.ko", "sans-serif");
pref("font.name.monospace.ko", "monospace");

pref("font.name.serif.tr", "serif");
pref("font.name.sans-serif.tr", "sans-serif");
pref("font.name.monospace.tr", "monospace");

pref("font.name.serif.x-baltic", "serif");
pref("font.name.sans-serif.x-baltic", "sans-serif");
pref("font.name.monospace.x-baltic", "monospace");

pref("font.name.serif.x-central-euro", "serif");
pref("font.name.sans-serif.x-central-euro", "sans-serif");
pref("font.name.monospace.x-central-euro", "monospace");

pref("font.name.serif.x-cyrillic", "serif");
pref("font.name.sans-serif.x-cyrillic", "sans-serif");
pref("font.name.monospace.x-cyrillic", "monospace");

pref("font.name.serif.x-unicode", "serif");
pref("font.name.sans-serif.x-unicode", "sans-serif");
pref("font.name.monospace.x-unicode", "monospace");

pref("font.name.serif.x-user-def", "serif");
pref("font.name.sans-serif.x-user-def", "sans-serif");
pref("font.name.monospace.x-user-def", "monospace");

pref("font.name.serif.zh-CN", "serif");
pref("font.name.sans-serif.zh-CN", "sans-serif");
pref("font.name.monospace.zh-CN", "monospace");

pref("font.size.variable.x-western", 14);
pref("font.size.fixed.x-western", 12);

pref("applications.telnet", "pterm telnet %h %p");
pref("applications.tn3270", "pterm tn3270 %h");
pref("applications.rlogin", "pterm rlogin %h");
pref("applications.rlogin_with_user", "pterm rlogin %h -l %u");



pref("print.print_extra_margin", 90); 

# photon
#endif

#if OS_ARCH==OpenVMS

pref("mail.use_builtin_movemail", false);

pref("helpers.global_mime_types_file", "/sys$manager/netscape/mime.types");
pref("helpers.global_mailcap_file", "/sys$manager/netscape/mailcap");
pref("helpers.private_mime_types_file", "/sys$login/.mime.types");
pref("helpers.private_mailcaptypes_file", "/sys$login/.mailcap");

pref("applications.telnet", "create /term /detach \"telnet %h %p\"");
pref("applications.tn3270", "create /term /detach \"telnet /term=IBM-3278-5 %h %p\"");
pref("applications.rlogin", "create /term /detach \"rlogin %h\"");
pref("applications.rlogin_with_user", "create /term /detach \"rlogin %h -l %u\"");


pref("print.postscript.print_command", "print /delete");

pref("print.print_command", "print /delete");
pref("print.print_color", false);

pref("browser.cache.disk.capacity", 4096);
pref("plugin.soname.list", "");

# OpenVMS
#endif

#if OS_ARCH==AIX


pref("font.name.serif.ja", "dt-interface system-jisx0208.1983-0");
pref("font.name.sans-serif.ja", "dt-interface system-jisx0208.1983-0");
pref("font.name.monospace.ja", "dt-interface user-jisx0208.1983-0");


pref("font.name.serif.x-cyrillic", "dt-interface system-iso8859-5");
pref("font.name.sans-serif.x-cyrillic", "dt-interface system-iso8859-5");
pref("font.name.monospace.x-cyrillic", "dt-interface user-iso8859-5");


pref("font.name.serif.x-unicode", "dt-interface system-ucs2.cjk_japan-0");
pref("font.name.sans-serif.x-unicode", "dt-interface system-ucs2.cjk_japan-0");
pref("font.name.monospace.x-unicode", "dt-interface user-ucs2.cjk_japan-0");

# AIX
#endif

#ifdef SOLARIS

pref("print.postscript.print_command", "lp -c -s ${MOZ_PRINTER_NAME:+'-d'}${MOZ_PRINTER_NAME}");
pref("print.print_command", "lp -c -s ${MOZ_PRINTER_NAME:+'-d'}${MOZ_PRINTER_NAME}");

# Solaris
#endif


pref("signon.rememberSignons",              true);
pref("signon.expireMasterPassword",         false);
pref("signon.SignonFileName",               "signons.txt"); 
pref("signon.SignonFileName2",              "signons2.txt");
pref("signon.autofillForms",                true); 
pref("signon.debug",                        false); 
