
















































pref("keyword.URL", "http://www.google.com/search?ie=UTF-8&oe=utf-8&q=");
pref("keyword.enabled", false);
pref("general.useragent.locale", "chrome://global/locale/intl.properties");
pref("general.useragent.compatMode.firefox", false);

pref("general.config.obscure_value", 13); 

pref("general.warnOnAboutConfig", true);


pref("browser.bookmarks.max_backups",       5);

pref("browser.cache.disk.enable",           true);

pref("browser.cache.disk.smart_size.first_run", true);

pref("browser.cache.disk.smart_size.enabled", true);

#ifndef WINCE
pref("browser.cache.disk.capacity",         256000);
#else
pref("browser.cache.disk.capacity",         20000);
#endif
pref("browser.cache.memory.enable",         true);


pref("browser.cache.disk_cache_ssl",        true);

pref("browser.cache.check_doc_frequency",   3);

pref("browser.cache.offline.enable",           true);
#ifndef WINCE

pref("browser.cache.offline.capacity",         512000);



pref("offline-apps.quota.max",        204800);



pref("offline-apps.quota.warn",        51200);
#else

pref("browser.cache.offline.capacity", 15000);
pref("offline-apps.quota.max",          7000);
pref("offline-apps.quota.warn",         4000);
#endif


pref("dom.indexedDB.enabled", true);

pref("dom.indexedDB.warningQuota", 50);



pref("browser.sessionhistory.max_total_viewers", -1);

pref("browser.sessionhistory.optimize_eviction", true);

pref("ui.use_native_colors", true);
pref("ui.use_native_popup_windows", false);
pref("ui.click_hold_context_menus", false);
pref("browser.display.use_document_fonts",  1);  
pref("browser.display.use_document_colors", true);
pref("browser.display.use_system_colors",   false);
pref("browser.display.foreground_color",    "#000000");
pref("browser.display.background_color",    "#FFFFFF");
pref("browser.display.force_inline_alttext", false); 



pref("browser.display.normal_lineheight_calc_control", 2);
pref("browser.display.show_image_placeholders", true); 

pref("browser.display.auto_quality_min_font_size", 20);
pref("browser.anchor_color",                "#0000EE");
pref("browser.active_color",                "#EE0000");
pref("browser.visited_color",               "#551A8B");
pref("browser.underline_anchors",           true);
pref("browser.blink_allowed",               true);
pref("browser.enable_automatic_image_resizing", false);
pref("browser.enable_click_image_resizing", true);


pref("browser.autofocus", true);


pref("browser.send_pings", false);
pref("browser.send_pings.max_per_link", 1);           
pref("browser.send_pings.require_same_host", false);  

pref("browser.display.use_focus_colors",    false);
pref("browser.display.focus_background_color", "#117722");
pref("browser.display.focus_text_color",     "#ffffff");
pref("browser.display.focus_ring_width",     1);
pref("browser.display.focus_ring_on_anything", false);


pref("browser.display.focus_ring_style", 1);

pref("browser.helperApps.alwaysAsk.force",  false);
pref("browser.helperApps.neverAsk.saveToDisk", "");
pref("browser.helperApps.neverAsk.openFile", "");


pref("browser.chrome.toolbar_tips",         true);

pref("browser.chrome.toolbar_style",        2);


pref("browser.chrome.image_icons.max_size", 1024);

pref("browser.triple_click_selects_paragraph", true);



pref("media.enforce_same_site_origin", false);


pref("media.cache_size", 512000);

#ifdef MOZ_RAW
pref("media.raw.enabled", true);
#endif
#ifdef MOZ_OGG
pref("media.ogg.enabled", true);
#endif
#ifdef MOZ_WAVE
pref("media.wave.enabled", true);
#endif
#ifdef MOZ_WEBM
pref("media.webm.enabled", true);
#endif


pref("media.autoplay.enabled", true);



pref("gfx.color_management.mode", 2);
pref("gfx.color_management.display_profile", "");
pref("gfx.color_management.rendering_intent", 0);

pref("gfx.downloadable_fonts.enabled", true);
pref("gfx.downloadable_fonts.fallback_delay", 3000);
pref("gfx.downloadable_fonts.sanitize", true);


#ifdef XP_MACOSX

pref("gfx.font_rendering.harfbuzz.scripts", 7);
#else

pref("gfx.font_rendering.harfbuzz.scripts", 3);
#endif

#ifdef XP_WIN
#ifndef WINCE
pref("gfx.font_rendering.directwrite.enabled", false);
pref("gfx.font_rendering.directwrite.use_gdi_table_loading", true);
#endif
#endif

pref("accessibility.browsewithcaret", false);
pref("accessibility.warn_on_browsewithcaret", true);

pref("accessibility.browsewithcaret_shortcut.enabled", true);

#ifndef XP_MACOSX





pref("accessibility.tabfocus", 7);
pref("accessibility.tabfocus_applies_to_xul", false);



pref("accessibility.win32.force_disabled", false);



pref("ui.scrollToClick", 0);

#else

pref("accessibility.tabfocus_applies_to_xul", true);
#endif

pref("accessibility.usetexttospeech", "");
pref("accessibility.usebrailledisplay", "");
pref("accessibility.accesskeycausesactivation", true);
pref("accessibility.mouse_focuses_formcontrol", false);


pref("accessibility.typeaheadfind", true);
pref("accessibility.typeaheadfind.autostart", true);




pref("accessibility.typeaheadfind.casesensitive", 0);
pref("accessibility.typeaheadfind.linksonly", true);
pref("accessibility.typeaheadfind.startlinksonly", false);
pref("accessibility.typeaheadfind.timeout", 4000);
pref("accessibility.typeaheadfind.enabletimeout", true);
pref("accessibility.typeaheadfind.soundURL", "beep");
pref("accessibility.typeaheadfind.enablesound", false);
pref("accessibility.typeaheadfind.prefillwithselection", true);


pref("gfx.use_text_smoothing_setting", false);


pref("browser.frames.enabled", true);


pref("toolkit.autocomplete.richBoundaryCutoff", 200);

pref("toolkit.scrollbox.smoothScroll", true);
pref("toolkit.scrollbox.scrollIncrement", 20);
pref("toolkit.scrollbox.clickToScroll.scrollDelay", 150);


pref("view_source.syntax_highlight", true);
pref("view_source.wrap_long_lines", false);
pref("view_source.editor.external", false);
pref("view_source.editor.path", "");


pref("view_source.editor.args", "");


pref("nglayout.events.dispatchLeftClickOnly", true);


pref("nglayout.enable_drag_images", true);




pref("slider.snapMultiplier", 0);


pref("application.use_ns_plugin_finder", false);


pref("browser.fixup.alternate.enabled", true);
pref("browser.fixup.alternate.prefix", "www.");
pref("browser.fixup.alternate.suffix", ".com");
pref("browser.fixup.hide_user_pass", true);


pref("browser.urlbar.autocomplete.enabled", true);










pref("print.print_headerleft", "&T");
pref("print.print_headercenter", "");
pref("print.print_headerright", "&U");
pref("print.print_footerleft", "&PT");
pref("print.print_footercenter", "");
pref("print.print_footerright", "&D");
pref("print.show_print_progress", true);





pref("print.use_global_printsettings", true);


pref("print.save_print_settings", true);

pref("print.whileInPrintPreview", true);


pref("print.always_cache_old_pres", false);








pref("print.print_unwriteable_margin_top",    -1);
pref("print.print_unwriteable_margin_left",   -1);
pref("print.print_unwriteable_margin_right",  -1);
pref("print.print_unwriteable_margin_bottom", -1);





pref("print.print_edge_top", 0);
pref("print.print_edge_left", 0);
pref("print.print_edge_right", 0);
pref("print.print_edge_bottom", 0);




pref("extensions.spellcheck.inline.max-misspellings", 500);




pref("editor.use_custom_colors", false);
pref("editor.singleLine.pasteNewlines",      2);
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

pref("capability.policy.default.Location.hash.set", "allAccess");
pref("capability.policy.default.Location.href.set", "allAccess");
pref("capability.policy.default.Location.replace.get", "allAccess");

pref("capability.policy.default.Navigator.preference", "allAccess");
pref("capability.policy.default.Navigator.preferenceinternal.get", "UniversalPreferencesRead");
pref("capability.policy.default.Navigator.preferenceinternal.set", "UniversalPreferencesWrite");

pref("capability.policy.default.Window.blur.get", "allAccess");
pref("capability.policy.default.Window.close.get", "allAccess");
pref("capability.policy.default.Window.closed.get", "allAccess");
pref("capability.policy.default.Window.focus.get", "allAccess");
pref("capability.policy.default.Window.frames.get", "allAccess");
pref("capability.policy.default.Window.history.get", "allAccess");
pref("capability.policy.default.Window.length.get", "allAccess");
pref("capability.policy.default.Window.location", "allAccess");
pref("capability.policy.default.Window.opener.get", "allAccess");
pref("capability.policy.default.Window.parent.get", "allAccess");
pref("capability.policy.default.Window.postMessage.get", "allAccess");
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
pref("capability.policy.mailnews.*.getAttributeNode", "noAccess");
pref("capability.policy.mailnews.*.getAttributeNodeNS", "noAccess");
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
pref("capability.policy.mailnews.*.textContent", "noAccess");
pref("capability.policy.mailnews.*.title.get", "noAccess");
pref("capability.policy.mailnews.*.wholeText", "noAccess");
pref("capability.policy.mailnews.DOMException.toString", "noAccess");
pref("capability.policy.mailnews.HTMLAnchorElement.toString", "noAccess");
pref("capability.policy.mailnews.HTMLDocument.domain", "noAccess");
pref("capability.policy.mailnews.HTMLDocument.URL", "noAccess");
pref("capability.policy.mailnews.*.documentURI", "noAccess");
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
pref("dom.disable_window_open_feature.personalbar", false);
pref("dom.disable_window_open_feature.menubar",     false);
pref("dom.disable_window_open_feature.scrollbars",  false);
pref("dom.disable_window_open_feature.resizable",   true);
pref("dom.disable_window_open_feature.minimizable", false);
pref("dom.disable_window_open_feature.status",      true);

pref("dom.allow_scripts_to_close_windows",          false);

pref("dom.disable_open_during_load",                false);
pref("dom.popup_maximum",                           20);
pref("dom.popup_allowed_events", "change click dblclick mouseup reset submit");
pref("dom.disable_open_click_delay", 1000);

pref("dom.storage.enabled", true);
pref("dom.storage.default_quota",      5120);

pref("dom.send_after_paint_to_content", false);


pref("dom.min_timeout_value", 4);

pref("dom.min_background_timeout_value", 1000);


#ifndef XP_WIN
pref("content.sink.pending_event_mode", 0);
#endif





pref("privacy.popups.disable_from_plugins", 2);


pref("privacy.donottrackheader.enabled",    false);

pref("dom.event.contextmenu.enabled",       true);

pref("javascript.enabled",                  true);
pref("javascript.options.strict",           false);
#ifdef DEBUG
pref("javascript.options.strict.debug",     true);
#endif
pref("javascript.options.relimit",          true);
pref("javascript.options.tracejit.content",  true);
pref("javascript.options.tracejit.chrome",   true);
pref("javascript.options.methodjit.content", true);
pref("javascript.options.methodjit.chrome",  true);
pref("javascript.options.jitprofiling.content", true);
pref("javascript.options.jitprofiling.chrome",  true);
pref("javascript.options.methodjit_always", false);




pref("javascript.options.mem.high_water_mark", 128);
pref("javascript.options.mem.max", -1);
pref("javascript.options.mem.gc_frequency",   300);
pref("javascript.options.mem.gc_per_compartment", true);
pref("javascript.options.mem.log", false);


pref("advanced.mailftp",                    false);
pref("image.animation_mode",                "normal");


pref("security.fileuri.strict_origin_policy", true);










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





pref("network.http.version", "1.1");	  





pref("network.http.proxy.version", "1.1");    

                                              


pref("network.http.use-cache", true);



pref("network.http.default-socket-type", "");

pref("network.http.keep-alive", true); 
pref("network.http.proxy.keep-alive", true);





pref("network.http.keep-alive.timeout", 115);





pref("network.http.max-connections", 256);




pref("network.http.max-connections-per-server", 15);




pref("network.http.max-persistent-connections-per-server", 6);




pref("network.http.max-persistent-connections-per-proxy", 8);





pref("network.http.request.max-start-delay", 10);


pref("network.http.accept.default", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
pref("network.http.sendRefererHeader",      2); 



pref("network.http.sendSecureXSiteReferrer", true);


pref("network.http.redirection-limit", 20);




pref("network.http.accept-encoding", "gzip, deflate");

pref("network.http.pipelining"      , false);
pref("network.http.pipelining.ssl"  , false); 
pref("network.http.proxy.pipelining", false);


pref("network.http.pipelining.maxrequests" , 4);


pref("network.http.prompt-temp-redirect", true);









pref("network.http.qos", 0);




pref("network.http.connection-retry-timeout", 250);





pref("network.ftp.data.qos", 0);
pref("network.ftp.control.qos", 0);








pref("network.websocket.override-security-block", false);
pref("network.websocket.enabled", true);





pref("network.jar.open-unsafe-types", false);



pref("network.enableIDN", true);




pref("network.IDN_show_punycode", false);









pref("network.IDN.whitelist.ac", true);
pref("network.IDN.whitelist.ar", true);
pref("network.IDN.whitelist.at", true);
pref("network.IDN.whitelist.br", true);
pref("network.IDN.whitelist.ch", true);
pref("network.IDN.whitelist.cl", true);
pref("network.IDN.whitelist.cn", true);
pref("network.IDN.whitelist.de", true);
pref("network.IDN.whitelist.dk", true);
pref("network.IDN.whitelist.es", true);
pref("network.IDN.whitelist.fi", true);
pref("network.IDN.whitelist.gr", true);
pref("network.IDN.whitelist.hu", true);
pref("network.IDN.whitelist.il", true);
pref("network.IDN.whitelist.io", true);
pref("network.IDN.whitelist.ir", true);
pref("network.IDN.whitelist.is", true);
pref("network.IDN.whitelist.jp", true);
pref("network.IDN.whitelist.kr", true);
pref("network.IDN.whitelist.li", true);
pref("network.IDN.whitelist.lt", true);
pref("network.IDN.whitelist.lu", true);
pref("network.IDN.whitelist.no", true);
pref("network.IDN.whitelist.nu", true);
pref("network.IDN.whitelist.nz", true);
pref("network.IDN.whitelist.pl", true);
pref("network.IDN.whitelist.pr", true);
pref("network.IDN.whitelist.se", true);
pref("network.IDN.whitelist.sh", true);
pref("network.IDN.whitelist.th", true);
pref("network.IDN.whitelist.tm", true);
pref("network.IDN.whitelist.tw", true);
pref("network.IDN.whitelist.ua", true);
pref("network.IDN.whitelist.vn", true);



pref("network.IDN.whitelist.xn--mgbaam7a8h", true); 

pref("network.IDN.whitelist.xn--fiqz9s", true); 
pref("network.IDN.whitelist.xn--fiqs8s", true); 

pref("network.IDN.whitelist.xn--wgbh1c", true);

pref("network.IDN.whitelist.xn--j6w193g", true);

pref("network.IDN.whitelist.xn--mgba3a4f16a", true);
pref("network.IDN.whitelist.xn--mgba3a4fra", true);

pref("network.IDN.whitelist.xn--mgbayh7gpa", true);

pref("network.IDN.whitelist.xn--wgbl6a", true);

pref("network.IDN.whitelist.xn--p1ai", true);

pref("network.IDN.whitelist.xn--mgberp4a5d4ar", true); 
pref("network.IDN.whitelist.xn--mgberp4a5d4a87g", true);
pref("network.IDN.whitelist.xn--mgbqly7c0a67fbc", true);
pref("network.IDN.whitelist.xn--mgbqly7cvafr", true);

pref("network.IDN.whitelist.xn--kpry57d", true);  
pref("network.IDN.whitelist.xn--kprw13d", true);  


pref("network.IDN.whitelist.biz", true);
pref("network.IDN.whitelist.cat", true);
pref("network.IDN.whitelist.info", true);
pref("network.IDN.whitelist.museum", true);
pref("network.IDN.whitelist.org", true);
pref("network.IDN.whitelist.tel", true);






pref("network.IDN.whitelist.xn--0zwm56d", true);
pref("network.IDN.whitelist.xn--11b5bs3a9aj6g", true);
pref("network.IDN.whitelist.xn--80akhbyknj4f", true);
pref("network.IDN.whitelist.xn--9t4b11yi5a", true);
pref("network.IDN.whitelist.xn--deba0ad", true);
pref("network.IDN.whitelist.xn--g6w251d", true);
pref("network.IDN.whitelist.xn--hgbk6aj7f53bba", true);
pref("network.IDN.whitelist.xn--hlcj6aya9esc7a", true);
pref("network.IDN.whitelist.xn--jxalpdlp", true);
pref("network.IDN.whitelist.xn--kgbechtv", true);
pref("network.IDN.whitelist.xn--zckzah", true);





pref("network.IDN.blacklist_chars", "\u0020\u00A0\u00BC\u00BD\u00BE\u01C3\u02D0\u0337\u0338\u0589\u05C3\u05F4\u0609\u060A\u066A\u06D4\u0701\u0702\u0703\u0704\u115F\u1160\u1735\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200A\u200B\u2024\u2027\u2028\u2029\u202F\u2039\u203A\u2041\u2044\u2052\u205F\u2153\u2154\u2155\u2156\u2157\u2158\u2159\u215A\u215B\u215C\u215D\u215E\u215F\u2215\u2236\u23AE\u2571\u29F6\u29F8\u2AFB\u2AFD\u2FF0\u2FF1\u2FF2\u2FF3\u2FF4\u2FF5\u2FF6\u2FF7\u2FF8\u2FF9\u2FFA\u2FFB\u3000\u3002\u3014\u3015\u3033\u3164\u321D\u321E\u33AE\u33AF\u33C6\u33DF\uA789\uFE14\uFE15\uFE3F\uFE5D\uFE5E\uFEFF\uFF0E\uFF0F\uFF61\uFFA0\uFFF9\uFFFA\uFFFB\uFFFC\uFFFD");




pref("network.dns.ipv4OnlyDomains", "");


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








pref("network.auth.force-generic-ntlm", false);





pref("network.automatic-ntlm-auth.allow-proxies", true);
pref("network.automatic-ntlm-auth.trusted-uris", "");







pref("network.ntlm.send-lm-response", false);

pref("permissions.default.image",           1); 

pref("network.proxy.type",                  5);
pref("network.proxy.ftp",                   "");
pref("network.proxy.ftp_port",              0);
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
pref("network.cookie.thirdparty.sessionOnly", false);
pref("network.cookie.lifetimePolicy",       0); 
pref("network.cookie.alwaysAcceptSessionCookies", false);
pref("network.cookie.prefsMigrated",        false);
pref("network.cookie.lifetime.days",        90);


pref("network.proxy.autoconfig_url", "");



pref("network.proxy.autoconfig_retry_interval_min", 5);    
pref("network.proxy.autoconfig_retry_interval_max", 300);  

pref("converter.html2txt.structs",          true); 
pref("converter.html2txt.header_strategy",  1); 

pref("intl.accept_languages",               "chrome://global/locale/intl.properties");
pref("intl.menuitems.alwaysappendaccesskeys","chrome://global/locale/intl.properties");
pref("intl.menuitems.insertseparatorbeforeaccesskeys","chrome://global/locale/intl.properties");
pref("intl.charsetmenu.browser.static",     "chrome://global/locale/intl.properties");
pref("intl.charsetmenu.browser.more1",      "ISO-8859-1, ISO-8859-15, IBM850, x-mac-roman, windows-1252, ISO-8859-14, ISO-8859-7, x-mac-greek, windows-1253, x-mac-icelandic, ISO-8859-10, ISO-8859-3");
pref("intl.charsetmenu.browser.more2",      "ISO-8859-4, ISO-8859-13, windows-1257, IBM852, ISO-8859-2, x-mac-ce, windows-1250, x-mac-croatian, IBM855, ISO-8859-5, ISO-IR-111, KOI8-R, x-mac-cyrillic, windows-1251, IBM866, KOI8-U, x-mac-ukrainian, ISO-8859-16, x-mac-romanian");
pref("intl.charsetmenu.browser.more3",      "GB2312, gbk, gb18030, HZ-GB-2312, ISO-2022-CN, Big5, Big5-HKSCS, x-euc-tw, EUC-JP, ISO-2022-JP, Shift_JIS, EUC-KR, x-windows-949, x-johab, ISO-2022-KR");
pref("intl.charsetmenu.browser.more4",      "armscii-8, GEOSTD8, TIS-620, ISO-8859-11, windows-874, IBM857, ISO-8859-9, x-mac-turkish, windows-1254, x-viet-tcvn5712, VISCII, x-viet-vps, windows-1258, x-mac-devanagari, x-mac-gujarati, x-mac-gurmukhi");
pref("intl.charsetmenu.browser.more5",      "ISO-8859-6, windows-1256, IBM864, ISO-8859-8-I, windows-1255, ISO-8859-8, IBM862");
pref("intl.charsetmenu.browser.unicode",    "UTF-8, UTF-16LE, UTF-16BE");
pref("intl.charsetmenu.mailedit",           "chrome://global/locale/intl.properties");
pref("intl.charsetmenu.browser.cache",      "");
pref("intl.charsetmenu.mailview.cache",     "");
pref("intl.charsetmenu.composer.cache",     "");
pref("intl.charsetmenu.browser.cache.size", 5);
pref("intl.charset.detector",               "chrome://global/locale/intl.properties");
pref("intl.charset.default",                "chrome://global-platform/locale/intl.properties");
pref("intl.ellipsis",                       "chrome://global-platform/locale/intl.properties");
pref("intl.locale.matchOS",                 false);



pref("intl.fallbackCharsetList.ISO-8859-1", "windows-1252");
pref("font.language.group",                 "chrome://global/locale/intl.properties");


pref("intl.uidirection.ar", "rtl");
pref("intl.uidirection.he", "rtl");
pref("intl.uidirection.fa", "rtl");


pref("intl.hyphenation-alias.en", "en-us");

pref("intl.hyphenation-alias.en-*", "en-us");

pref("font.mathfont-family", "STIXNonUnicode, STIXSizeOneSym, STIXSize1, STIXGeneral, Standard Symbols L, DejaVu Sans, Cambria Math");



pref("font.blacklist.underline_offset", "FangSong,Gulim,GulimChe,MingLiU,MingLiU-ExtB,MingLiU_HKSCS,MingLiU-HKSCS-ExtB,MS Gothic,MS Mincho,MS PGothic,MS PMincho,MS UI Gothic,PMingLiU,PMingLiU-ExtB,SimHei,SimSun,SimSun-ExtB,Hei,Kai,Apple LiGothic,Apple LiSung,Osaka");

pref("images.dither", "auto");
pref("security.directory",              "");

pref("signed.applets.codebase_principal_support", false);
pref("security.checkloaduri", true);
pref("security.xpconnect.plugin.unrestricted", true);

pref("security.dialog_enable_delay", 2000);

pref("security.csp.enable", true);
pref("security.csp.debug", false);




pref("ui.key.accelKey", 17);
pref("ui.key.menuAccessKey", 18);
pref("ui.key.generalAccessKey", -1);




pref("ui.key.chromeAccess", 4);
pref("ui.key.contentAccess", 5);

pref("ui.key.menuAccessKeyFocuses", false); 
pref("ui.key.saveLink.shift", true); 


pref("ui.use_activity_cursor", false);


pref("middlemouse.paste", false);
pref("middlemouse.openNewWindow", true);
pref("middlemouse.contentLoadURL", false);
pref("middlemouse.scrollbarPosition", false);


pref("clipboard.autocopy", false);


pref("mousewheel.transaction.timeout", 1500);

pref("mousewheel.transaction.ignoremovedelay", 100);


pref("mousewheel.enable_pixel_scrolling", true);




pref("mousewheel.acceleration.start", -1);

pref("mousewheel.acceleration.factor", 10);










pref("mousewheel.system_scroll_override_on_root_content.vertical.factor", 200);
pref("mousewheel.system_scroll_override_on_root_content.horizontal.factor", 200);


pref("mousewheel.withnokey.action",0);
pref("mousewheel.withnokey.numlines",6);
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

pref("profile.confirm_automigration",true);




pref("profile.migration_behavior",0);
pref("profile.migration_directory", "");








pref("profile.seconds_until_defunct", -1);

pref("profile.manage_only_at_launch", false);

pref("prefs.converted-to-utf8",false);










pref("bidi.direction", 1);






pref("bidi.texttype", 1);










pref("bidi.numeral", 0);






pref("bidi.support", 1);



pref("bidi.edit.delete_immediately", false);





pref("bidi.edit.caret_movement_style", 2);




pref("bidi.browser.ui", false);


pref("layout.word_select.eat_space_to_next_word", false);
pref("layout.word_select.stop_at_punctuation", true);









pref("layout.selection.caret_style", 0);




pref("layout.enable_japanese_specific_transform", false);


pref("layout.frames.force_resizability", false);


pref("layout.css.report_errors", true);


pref("layout.css.visited_links_enabled", true);





pref("layout.css.dpi", -1);







pref("layout.css.devPixelsPerPx", "1.0");






pref("layout.scrollbar.side", 0);




pref("layout.frame_rate", -1);











pref("layout.frame_rate.precise", false);


pref("capability.policy.default.SOAPCall.invokeVerifySourceHeader", "allAccess");


pref("plugin.override_internal_types", false);
pref("plugin.expose_full_path", false); 



pref("browser.popups.showPopupBlocker", true);



pref("viewmanager.do_doublebuffering", true);


pref("config.use_system_prefs", false);


pref("config.use_system_prefs.accessibility", false);


pref("gestures.enable_single_finger_input", true);

















pref("editor.resizing.preserve_ratio",       true);
pref("editor.positioning.offset",            0);

pref("dom.max_chrome_script_run_time", 20);
pref("dom.max_script_run_time", 10);

#ifndef DEBUG


pref("dom.ipc.plugins.timeoutSecs", 45);


pref("dom.ipc.plugins.processLaunchTimeoutSecs", 45);
#else

pref("dom.ipc.plugins.timeoutSecs", 0);
pref("dom.ipc.plugins.processLaunchTimeoutSecs", 0);
#endif



pref("dom.ipc.plugins.java.enabled", false);

#ifndef ANDROID
#ifndef XP_MACOSX
#ifdef XP_UNIX

pref("dom.ipc.plugins.enabled.libvlcplugin.so", false);
pref("dom.ipc.plugins.enabled.nppdf.so", false);
pref("dom.ipc.plugins.enabled.602plugin.so", false);
#endif
#endif
#endif

pref("svg.smil.enabled", true);

pref("font.minimum-size.ar", 0);
pref("font.minimum-size.x-armn", 0);
pref("font.minimum-size.x-beng", 0);
pref("font.minimum-size.x-baltic", 0);
pref("font.minimum-size.x-central-euro", 0);
pref("font.minimum-size.zh-CN", 0);
pref("font.minimum-size.zh-HK", 0);
pref("font.minimum-size.zh-TW", 0);
pref("font.minimum-size.x-cyrillic", 0);
pref("font.minimum-size.x-devanagari", 0);
pref("font.minimum-size.x-ethi", 0);
pref("font.minimum-size.x-geor", 0);
pref("font.minimum-size.el", 0);
pref("font.minimum-size.x-gujr", 0);
pref("font.minimum-size.x-guru", 0);
pref("font.minimum-size.he", 0);
pref("font.minimum-size.ja", 0);
pref("font.minimum-size.x-knda", 0);
pref("font.minimum-size.x-khmr", 0);
pref("font.minimum-size.ko", 0);
pref("font.minimum-size.x-mlym", 0);
pref("font.minimum-size.x-orya", 0);
pref("font.minimum-size.x-sinh", 0);
pref("font.minimum-size.x-tamil", 0);
pref("font.minimum-size.x-telu", 0);
pref("font.minimum-size.x-tibt", 0);
pref("font.minimum-size.th", 0);
pref("font.minimum-size.tr", 0);
pref("font.minimum-size.x-cans", 0);
pref("font.minimum-size.x-western", 0);
pref("font.minimum-size.x-unicode", 0);
pref("font.minimum-size.x-user-def", 0);

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
pref("font.name-list.serif.ja", "MS PMincho, MS Mincho, MS PGothic, MS Gothic");
pref("font.name-list.sans-serif.ja", "MS PGothic, MS Gothic, MS PMincho, MS Mincho");
pref("font.name-list.monospace.ja", "MS Gothic, MS Mincho, MS PGothic, MS PMincho");

pref("font.name.serif.ko", "바탕"); 
pref("font.name.sans-serif.ko", "굴림"); 
pref("font.name.monospace.ko", "굴림체"); 
pref("font.name.cursive.ko", "궁서"); 

pref("font.name-list.serif.ko", "Batang, Gulim"); 
pref("font.name-list.sans-serif.ko", "Gulim"); 
pref("font.name-list.monospace.ko", "GulimChe"); 
pref("font.name-list.cursive.ko", "Gungseo"); 

pref("font.name.serif.th", "Tahoma");
pref("font.name.sans-serif.th", "Tahoma");
pref("font.name.monospace.th", "Tahoma");
pref("font.name.cursive.th", "Tahoma");

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
pref("font.name-list.serif.zh-CN", "MS Song, SimSun");
pref("font.name-list.sans-serif.zh-CN", "MS Song, SimSun");
pref("font.name-list.monospace.zh-CN", "MS Song, SimSun");



pref("font.name.serif.zh-TW", "Times New Roman"); 
pref("font.name.sans-serif.zh-TW", "Arial");
pref("font.name.monospace.zh-TW", "細明體");  
pref("font.name-list.serif.zh-TW", "PMingLiu, MingLiU"); 
pref("font.name-list.sans-serif.zh-TW", "PMingLiU, MingLiU");
pref("font.name-list.monospace.zh-TW", "MingLiU");



pref("font.name.serif.zh-HK", "Times New Roman"); 
pref("font.name.sans-serif.zh-HK", "Arial");
pref("font.name.monospace.zh-HK", "細明體_HKSCS"); 
pref("font.name-list.serif.zh-HK", "MingLiu_HKSCS, Ming(for ISO10646), MingLiU"); 
pref("font.name-list.sans-serif.zh-HK", "MingLiU_HKSCS, Ming(for ISO10646), MingLiU");  
pref("font.name-list.monospace.zh-HK", "MingLiU_HKSCS, Ming(for ISO10646), MingLiU");

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

pref("font.name.serif.x-beng", "Vrinda");
pref("font.name.sans-serif.x-beng", "Vrinda");
pref("font.name.monospace.x-beng", "Mitra Mono");
pref("font.name-list.serif.x-beng", "Vrinda, Akaash, Likhan, Ekushey Punarbhaba, Code2000, Arial Unicode MS"); 
pref("font.name-list.sans-serif.x-beng", "Vrinda, Akaash, Likhan, Ekushey Punarbhaba, Code2000, Arial Unicode MS"); 
pref("font.name-list.monospace.x-beng", "Likhan, Mukti Narrow, Code2000, Arial Unicode MS");

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

pref("font.name.serif.x-mlym", "Rachana_w01");
pref("font.name.sans-serif.x-mlym", "Rachana_w01");
pref("font.name.monospace.x-mlym", "Rachana_w01");
pref("font.name-list.serif.x-mlym", "AnjaliOldLipi, Kartika, ThoolikaUnicode, Code2000, Arial Unicode MS");
pref("font.name-list.sans-serif.x-mlym", "AnjaliOldLipi, Kartika, ThoolikaUnicode, Code2000, Arial Unicode MS");
pref("font.name-list.monospace.x-mlym", "AnjaliOldLipi, Kartika, ThoolikaUnicode, Code2000, Arial Unicode MS");

pref("font.name.serif.x-orya", "ori1Uni");
pref("font.name.sans-serif.x-orya", "ori1Uni");
pref("font.name.monospace.x-orya", "ori1Uni");
pref("font.name-list.serif.x-orya", "Kalinga, ori1Uni, Code2000, Arial Unicode MS");
pref("font.name-list.sans-serif.x-orya", "Kalinga, ori1Uni, Code2000, Arial Unicode MS");
pref("font.name-list.monospace.x-orya", "Kalinga, ori1Uni, Code2000, Arial Unicode MS");

pref("font.name.serif.x-telu", "Gautami");
pref("font.name.sans-serif.x-telu", "Gautami");
pref("font.name.monospace.x-telu", "Gautami");
pref("font.name-list.serif.x-telu", "Gautami, Akshar Unicode, Code2000, Arial Unicode MS");
pref("font.name-list.sans-serif.x-telu", "Gautami, Akshar Unicode, Code2000, Arial Unicode MS");
pref("font.name-list.monospace.x-telu", "Gautami, Akshar Unicode, Code2000, Arial Unicode MS");

pref("font.name.serif.x-knda", "Tunga");
pref("font.name.sans-serif.x-knda", "Tunga");
pref("font.name.monospace.x-knda", "Tunga");
pref("font.name-list.serif.x-knda", "Tunga, AksharUnicode, Code2000, Arial Unicode MS");
pref("font.name-list.sans-serif.x-knda", "Tunga, AksharUnicode, Code2000, Arial Unicode MS");
pref("font.name-list.monospace.x-knda", "Tunga, AksharUnicode, Code2000, Arial Unicode MS");

pref("font.name.serif.x-sinh", "Iskoola Pota");
pref("font.name.sans-serif.x-sinh", "Iskoola Pota");
pref("font.name.monospace.x-sinh", "Iskoola Pota");
pref("font.name-list.serif.x-sinh", "Iskoola Pota, AksharUnicode");
pref("font.name-list.sans-serif.x-sinh", "Iskoola Pota, AksharUnicode");
pref("font.name-list.monospace.x-sinh", "Iskoola Pota, AksharUnicode");

pref("font.name.serif.x-tibt", "Tibetan Machine Uni");
pref("font.name.sans-serif.x-tibt", "Tibetan Machine Uni");
pref("font.name.monospace.x-tibt", "Tibetan Machine Uni");
pref("font.name-list.serif.x-tibt", "Tibetan Machine Uni, Jomolhari, Microsoft Himalaya");
pref("font.name-list.sans-serif.x-tibt", "Tibetan Machine Uni, Jomolhari, Microsoft Himalaya");
pref("font.name-list.monospace.x-tibt", "Tibetan Machine Uni, Jomolhari, Microsoft Himalaya");

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
pref("font.minimum-size.th", 10);

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

pref("font.default.x-orya", "serif");
pref("font.size.variable.x-orya", 16);
pref("font.size.fixed.x-orya", 13);

pref("font.default.x-telu", "serif");
pref("font.size.variable.x-telu", 16);
pref("font.size.fixed.x-telu", 13);

pref("font.default.x-knda", "serif");
pref("font.size.variable.x-knda", 16);
pref("font.size.fixed.x-knda", 13);

pref("font.default.x-sinh", "serif");
pref("font.size.variable.x-sinh", 16);
pref("font.size.fixed.x-sinh", 13);

pref("font.default.x-tibt", "serif");
pref("font.size.variable.x-tibt", 16);
pref("font.size.fixed.x-tibt", 13);

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


pref("font.mathfont-family", "STIXNonUnicode, STIXSizeOneSym, STIXSize1, STIXGeneral, Symbol, DejaVu Sans, Cambria Math");




pref("gfx.font_rendering.cleartype.use_for_downloadable_fonts", true);


pref("gfx.font_rendering.cleartype.always_use_for_content", false);




























pref("gfx.font_rendering.cleartype_params.gamma", -1);
pref("gfx.font_rendering.cleartype_params.enhanced_contrast", -1);
pref("gfx.font_rendering.cleartype_params.cleartype_level", -1);
pref("gfx.font_rendering.cleartype_params.pixel_structure", -1);
pref("gfx.font_rendering.cleartype_params.rendering_mode", -1);

pref("ui.key.menuAccessKeyFocuses", true);


pref("layout.word_select.eat_space_to_next_word", true);


pref("slider.snapMultiplier", 6);



pref("print.print_extra_margin", 90); 


pref("print.extend_native_print_dialog", true);



pref("plugin.scan.SunJRE", "1.3");


pref("plugin.scan.Acrobat", "5.0");


pref("plugin.scan.Quicktime", "5.0");


pref("plugin.scan.WindowsMediaPlayer", "7.0");



pref("plugin.scan.plid.all", true);



pref("network.autodial-helper.enabled", true);






pref("advanced.system.supportDDEExec", true);


pref("intl.keyboard.per_window_layout", false);

#ifdef NS_ENABLE_TSF

pref("intl.enable_tsf_support", false);



pref("intl.tsf.on_layout_change_interval", 100);
#endif

#ifdef WINCE

pref("ui.panel.default_level_parent", true);
#else

pref("ui.panel.default_level_parent", false);
#endif

pref("mousewheel.system_scroll_override_on_root_content.enabled", true);



pref("mousewheel.emulate_at_wm_scroll", false);



pref("ui.trackpoint_hack.enabled", -1);




pref("ui.window_class_override", "");




pref("ui.elantech_gesture_hacks.enabled", -1);

# WINNT
#endif

#ifdef XP_MACOSX

pref("browser.drag_out_of_frame_style", 1);
pref("ui.key.saveLink.shift", false); 






pref("font.name.serif.ar", "Al Bayan");
pref("font.name.sans-serif.ar", "Geeza Pro");
pref("font.name.monospace.ar", "Geeza Pro");
pref("font.name.cursive.ar", "DecoType Naskh");
pref("font.name.fantasy.ar", "KufiStandardGK");
pref("font.name-list.serif.ar", "Al Bayan");
pref("font.name-list.sans-serif.ar", "Geeza Pro");
pref("font.name-list.monospace.ar", "Geeza Pro");
pref("font.name-list.cursive.ar", "DecoType Naskh");
pref("font.name-list.fantasy.ar", "KufiStandardGK");

pref("font.name.serif.el", "Lucida Grande");
pref("font.name.sans-serif.el", "Lucida Grande");
pref("font.name.monospace.el", "Lucida Grande");
pref("font.name.cursive.el", "Lucida Grande");
pref("font.name.fantasy.el", "Lucida Grande");
pref("font.name-list.serif.el", "Lucida Grande");
pref("font.name-list.sans-serif.el", "Lucida Grande");
pref("font.name-list.monospace.el", "Lucida Grande");
pref("font.name-list.cursive.el", "Lucida Grande");
pref("font.name-list.fantasy.el", "Lucida Grande");

pref("font.name.serif.he", "Times New Roman");
pref("font.name.sans-serif.he", "Arial");
pref("font.name.monospace.he", "Courier New");
pref("font.name.cursive.he", "Times New Roman");
pref("font.name.fantasy.he", "Times New Roman");
pref("font.name-list.serif.he", "Times New Roman");
pref("font.name-list.sans-serif.he", "Arial");
pref("font.name-list.monospace.he", "Courier New");
pref("font.name-list.cursive.he", "Times New Roman");
pref("font.name-list.fantasy.he", "Times New Roman");

pref("font.name.serif.ja", "Hiragino Mincho Pro"); 
pref("font.name.sans-serif.ja", "Hiragino Kaku Gothic Pro"); 
pref("font.name.monospace.ja", "Osaka-Mono"); 
pref("font.name-list.serif.ja", "Hiragino Mincho Pro"); 
pref("font.name-list.sans-serif.ja", "Hiragino Kaku Gothic Pro"); 
pref("font.name-list.monospace.ja", "Osaka-Mono"); 

pref("font.name.serif.ko", "AppleMyungjo"); 
pref("font.name.sans-serif.ko", "AppleGothic"); 
pref("font.name.monospace.ko", "AppleGothic"); 
pref("font.name-list.serif.ko", "AppleMyungjo"); 
pref("font.name-list.sans-serif.ko", "AppleGothic"); 
pref("font.name-list.monospace.ko", "AppleGothic"); 

pref("font.name.serif.th", "Thonburi");
pref("font.name.sans-serif.th", "Thonburi");
pref("font.name.monospace.th", "Ayuthaya");
pref("font.name-list.serif.th", "Thonburi");
pref("font.name-list.sans-serif.th", "Thonburi");
pref("font.name-list.monospace.th", "Ayuthaya");

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

pref("font.name.serif.x-armn", "Mshtakan");
pref("font.name.sans-serif.x-armn", "Mshtakan");
pref("font.name.monospace.x-armn", "Mshtakan");
pref("font.name-list.serif.x-armn", "Mshtakan");
pref("font.name-list.sans-serif.x-armn", "Mshtakan");
pref("font.name-list.monospace.x-armn", "Mshtakan");
 
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




pref("font.name.serif.x-beng", "সোলাইমান লিপি");
pref("font.name.sans-serif.x-beng", "রূপালী");
pref("font.name.monospace.x-beng", "রূপালী");
pref("font.name-list.serif.x-beng", "সোলাইমান লিপি");
pref("font.name-list.sans-serif.x-beng", "রূপালী");
pref("font.name-list.monospace.x-beng", "রূপালী");

pref("font.name.serif.x-cans", "Euphemia UCAS");
pref("font.name.sans-serif.x-cans", "Euphemia UCAS");
pref("font.name.monospace.x-cans", "Euphemia UCAS");
pref("font.name-list.serif.x-cans", "Euphemia UCAS");
pref("font.name-list.sans-serif.x-cans", "Euphemia UCAS");
pref("font.name-list.monospace.x-cans", "Euphemia UCAS");

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

pref("font.name.serif.x-devanagari", "Devanagari MT");
pref("font.name.sans-serif.x-devanagari", "Devanagari MT");
pref("font.name.monospace.x-devanagari", "Devanagari MT");
pref("font.name-list.serif.x-devanagari", "Devanagari MT");
pref("font.name-list.sans-serif.x-devanagari", "Devanagari MT");
pref("font.name-list.monospace.x-devanagari", "Devanagari MT");




pref("font.name.serif.x-ethi", "Abyssinica SIL");
pref("font.name.sans-serif.x-ethi", "Abyssinica SIL");
pref("font.name.monospace.x-ethi", "Abyssinica SIL");
pref("font.name-list.serif.x-ethi", "Abyssinica SIL");
pref("font.name-list.sans-serif.x-ethi", "Abyssinica SIL");
pref("font.name-list.monospace.x-ethi", "Abyssinica SIL");





pref("font.name.serif.x-geor", "TITUS Cyberbit Basic");
pref("font.name.sans-serif.x-geor", "Zuzumbo");
pref("font.name.monospace.x-geor", "Zuzumbo");
pref("font.name-list.serif.x-geor", "TITUS Cyberbit Basic"); 
pref("font.name-list.sans-serif.x-geor", "Zuzumbo");
pref("font.name-list.monospace.x-geor", "Zuzumbo");

pref("font.name.serif.x-gujr", "Gujarati MT");
pref("font.name.sans-serif.x-gujr", "Gujarati MT");
pref("font.name.monospace.x-gujr", "Gujarati MT");
pref("font.name-list.serif.x-gujr", "Gujarati MT"); 
pref("font.name-list.sans-serif.x-gujr", "Gujarati MT");
pref("font.name-list.monospace.x-gujr", "Gujarati MT");

pref("font.name.serif.x-guru", "Gurmukhi MT");
pref("font.name.sans-serif.x-guru", "Gurmukhi MT");
pref("font.name.monospace.x-guru", "Gurmukhi MT");
pref("font.name-list.serif.x-guru", "Gurmukhi MT"); 
pref("font.name-list.sans-serif.x-guru", "Gurmukhi MT");
pref("font.name-list.monospace.x-guru", "Gurmukhi MT");













pref("font.name.serif.x-telu", "Pothana");
pref("font.name.sans-serif.x-telu", "Pothana");
pref("font.name.monospace.x-telu", "Pothana");
pref("font.name-list.serif.x-telu", "Pothana");
pref("font.name-list.sans-serif.x-telu", "Pothana");
pref("font.name-list.monospace.x-telu", "Pothana");




pref("font.name.serif.x-knda", "Kedage");
pref("font.name.sans-serif.x-knda", "Kedage");
pref("font.name.monospace.x-knda", "Kedage");
pref("font.name-list.serif.x-knda", "Kedage");
pref("font.name-list.sans-serif.x-knda", "Kedage");
pref("font.name-list.monospace.x-knda", "Kedage");




pref("font.name.serif.x-tamil", "InaiMathi");
pref("font.name.sans-serif.x-tamil", "InaiMathi");
pref("font.name.monospace.x-tamil", "InaiMathi");
pref("font.name-list.serif.x-tamil", "InaiMathi");
pref("font.name-list.sans-serif.x-tamil", "InaiMathi");
pref("font.name-list.monospace.x-tamil", "InaiMathi");


pref("font.name.serif.x-tibt", "Kailasa");
pref("font.name.sans-serif.x-tibt", "Kailasa");
pref("font.name.monospace.x-tibt", "Kailasa");
pref("font.name-list.serif.x-tibt", "Kailasa");
pref("font.name-list.sans-serif.x-tibt", "Kailasa");
pref("font.name-list.monospace.x-tibt", "Kailasa");

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

pref("font.name.serif.zh-CN", "STSong");
pref("font.name.sans-serif.zh-CN", "STHeiti");
pref("font.name.monospace.zh-CN", "STHeiti");
pref("font.name-list.serif.zh-CN", "STSong");
pref("font.name-list.sans-serif.zh-CN", "STHeiti");
pref("font.name-list.monospace.zh-CN", "STHeiti");

pref("font.name.serif.zh-TW", "Apple LiSung"); 
pref("font.name.sans-serif.zh-TW", "Apple LiGothic");  
pref("font.name.monospace.zh-TW", "Apple LiGothic");  
pref("font.name-list.serif.zh-TW", "Apple LiSung"); 
pref("font.name-list.sans-serif.zh-TW", "Apple LiGothic");  
pref("font.name-list.monospace.zh-TW", "Apple LiGothic");  

pref("font.name.serif.zh-HK", "LiSong Pro");
pref("font.name.sans-serif.zh-HK", "LiHei Pro");
pref("font.name.monospace.zh-HK", "LiHei Pro");
pref("font.name-list.serif.zh-HK", "LiSong Pro");
pref("font.name-list.sans-serif.zh-HK", "LiHei Pro");
pref("font.name-list.monospace.zh-HK", "LiHei Pro");

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
pref("font.minimum-size.th", 10);

pref("font.default.tr", "serif");
pref("font.size.variable.tr", 16);
pref("font.size.fixed.tr", 13);

pref("font.default.x-armn", "serif");
pref("font.size.variable.x-armn", 16);
pref("font.size.fixed.x-armn", 13);

pref("font.default.x-baltic", "serif");
pref("font.size.variable.x-baltic", 16);
pref("font.size.fixed.x-baltic", 13);

pref("font.default.x-beng", "serif");
pref("font.size.variable.x-beng", 16);
pref("font.size.fixed.x-beng", 13);

pref("font.default.x-cans", "serif");
pref("font.size.variable.x-cans", 16);
pref("font.size.fixed.x-cans", 13);

pref("font.default.x-central-euro", "serif");
pref("font.size.variable.x-central-euro", 16);
pref("font.size.fixed.x-central-euro", 13);

pref("font.default.x-cyrillic", "serif");
pref("font.size.variable.x-cyrillic", 16);
pref("font.size.fixed.x-cyrillic", 13);

pref("font.default.x-devanagari", "serif");
pref("font.size.variable.x-devanagari", 16);
pref("font.size.fixed.x-devanagari", 13);

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

pref("font.default.x-tamil", "serif");
pref("font.size.variable.x-tamil", 16);
pref("font.size.fixed.x-tamil", 13);

pref("font.default.x-orya", "serif");
pref("font.size.variable.x-orya", 16);
pref("font.size.fixed.x-orya", 13);

pref("font.default.x-telu", "serif");
pref("font.size.variable.x-telu", 16);
pref("font.size.fixed.x-telu", 13);

pref("font.default.x-knda", "serif");
pref("font.size.variable.x-knda", 16);
pref("font.size.fixed.x-knda", 13);

pref("font.default.x-sinh", "serif");
pref("font.size.variable.x-sinh", 16);
pref("font.size.fixed.x-sinh", 13);

pref("font.default.x-tibt", "serif");
pref("font.size.variable.x-tibt", 16);
pref("font.size.fixed.x-tibt", 13);

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


pref("font.mathfont-family", "STIXNonUnicode, STIXSizeOneSym, STIXSize1, STIXGeneral, Symbol, DejaVu Sans, Cambria Math");



pref("font.single-face-list", "Osaka-Mono");



pref("font.preload-names-list", "Hiragino Kaku Gothic Pro,Hiragino Mincho Pro,STSong");



pref("ui.key.menuAccessKey", 0);
pref("ui.key.accelKey", 224);




pref("ui.key.generalAccessKey", -1);




pref("ui.key.chromeAccess", 2);
pref("ui.key.contentAccess", 2);



pref("print.print_extra_margin", 90); 


pref("ui.panel.default_level_parent", false);

pref("ui.plugin.cancel_composition_at_input_source_changed", false);

pref("mousewheel.system_scroll_override_on_root_content.enabled", false);

# XP_MACOSX
#endif

#ifdef XP_OS2

pref("ui.key.menuAccessKeyFocuses", true);

pref("font.alias-list", "sans,sans-serif,serif,monospace,Tms Rmn,Helv,Courier,Times New Roman");

pref("font.mathfont-family", "STIXNonUnicode, STIXSizeOneSym, STIXSize1, STIXGeneral, DejaVu Sans");






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
pref("font.name-list.serif.ja", "Times New Roman WT J, Times New Roman WT, Times New Roman MT 30, Tms Rmn");
pref("font.name.sans-serif.ja", "Helv");
pref("font.name.monospace.ja", "Kochi Gothic");
pref("font.name-list.monospace.ja", "Kochi Gothic, Kochi Mincho, Courier New, Courier");

pref("font.name.serif.ko", "Times New Roman WT K");
pref("font.name-list.serif.ko", "Times New Roman WT K, Times New Roman WT, Times New Roman MT 30, Tms Rmn");
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
pref("font.name.sans-serif.x-central-euro", "Helv");
pref("font.name.monospace.x-central-euro", "Courier");

pref("font.name.serif.x-cyrillic", "Tms Rmn");
pref("font.name.sans-serif.x-cyrillic", "Helv");
pref("font.name.monospace.x-cyrillic", "Courier");






pref("font.name.serif.x-unicode", "Times New Roman MT 30");
pref("font.name-list.serif.x-unicode", "DejaVu Serif, FreeSerif, Times New Roman WT, Times New Roman MT 30, Tms Rmn");
pref("font.name.sans-serif.x-unicode", "Lucida Sans Unicode");
pref("font.name-list.sans-serif.x-unicode", "DejaVu Sans, FreeSans, Arial Unicode, Lucida Sans Unicode, Helv");
pref("font.name.monospace.x-unicode", "DejaVu Sans Mono");
pref("font.name-list.monospace.x-unicode", "DejaVu Sans Mono, FreeMono, Andale Mono, Courier New, Courier");
pref("font.name.fantasy.x-unicode", "Times New Roman MT 30");
pref("font.name-list.fantasy.x-unicode", "DejaVu Serif, FreeSerif, Times New Roman WT, Times New Roman MT 30");
pref("font.name.cursive.x-unicode", "Times New Roman MT 30");
pref("font.name-list.cursive.x-unicode", "DejaVu Serif, FreeSerif, Times New Roman WT, Times New Roman MT 30");

pref("font.name.serif.x-western", "Tms Rmn");
pref("font.name.sans-serif.x-western", "Helv");
pref("font.name.monospace.x-western", "Courier");

pref("font.name.serif.zh-CN", "Times New Roman WT SC");
pref("font.name-list.serif.zh_CN", "Times New Roman WT SC, Times New Roman MT 30, Times New Roman WT, Tms Rmn");
pref("font.name.sans-serif.zh-CN", "Helv");
pref("font.name.monospace.zh-CN", "Courier");

pref("font.name.serif.zh-TW", "Times New Roman WT TC");
pref("font.name-list.serif.zh-TW", "Times New Roman WT TC, Times New Roman MT 30, Times New Roman WT, Tms Rmn");
pref("font.name.sans-serif.zh-TW", "Helv");
pref("font.name.monospace.zh-TW", "Courier");


pref("font.name.serif.zh-HK", "Times New Roman WT TC");
pref("font.name-list.serif.zh-HK", "Times New Roman WT TC, Times New Roman MT 30, Times New Roman WT, Tms Rmn");
pref("font.name.sans-serif.zh-HK", "Helv");
pref("font.name.monospace.zh-HK", "Courier");

pref("font.default", "serif");

pref("font.default.ar", "serif");
pref("font.size.variable.ar", 16);
pref("font.size.fixed.ar", 13);

pref("font.default.el", "serif");
pref("font.size.variable.el", 16);
pref("font.size.fixed.el", 13);

pref("font.default.he", "serif");
pref("font.size.variable.he", 16);
pref("font.size.fixed.he", 13);

pref("font.default.ja", "serif");
pref("font.size.variable.ja", 16);
pref("font.size.fixed.ja", 16);

pref("font.default.ko", "serif");
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

pref("font.default.zh-CN", "serif");
pref("font.size.variable.zh-CN", 16);
pref("font.size.fixed.zh-CN", 16);

pref("font.default.zh-TW", "serif");
pref("font.size.variable.zh-TW", 16);
pref("font.size.fixed.zh-TW", 16);

pref("font.default.zh-HK", "serif");
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



pref("network.dns.disableIPv6", true);




pref("ui.panel.default_level_parent", false);

pref("mousewheel.system_scroll_override_on_root_content.enabled", false);

# OS2
#endif

#ifdef ANDROID

pref("network.protocol-handler.warn-external.file", false);
pref("browser.drag_out_of_frame_style", 1);


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
pref("print.print_command", "lpr ${MOZ_PRINTER_NAME:+-P\"$MOZ_PRINTER_NAME\"}");
pref("print.printer_list", ""); 
pref("print.print_reversed", false);
pref("print.print_color", true);
pref("print.print_landscape", false);
pref("print.print_paper_size", 0);



pref("print.print_extra_margin", 0); 

pref("font.allow_double_byte_special_chars", true);


pref("font.alias-list", "sans,sans-serif,serif,monospace");



pref("font.name.serif.el", "Droid Serif");
pref("font.name.sans-serif.el", "Droid Sans");
pref("font.name.monospace.el", "Droid Sans Mono");

pref("font.name.serif.he", "Droid Serif");
pref("font.name.sans-serif.he", "Droid Sans");
pref("font.name.monospace.he", "Droid Sans Mono");

pref("font.name.serif.ja", "Droid Serif");
pref("font.name.sans-serif.ja", "Droid Sans Japanese");
pref("font.name.monospace.ja", "Droid Sans Mono");

pref("font.name.serif.ko", "Droid Serif");
pref("font.name.sans-serif.ko", "Droid Sans");
pref("font.name.monospace.ko", "Droid Sans Mono");

pref("font.name.serif.th", "Droid Serif");
pref("font.name.sans-serif.th", "Droid Sans");
pref("font.name.monospace.th", "Droid Sans Mono");

pref("font.name.serif.tr", "Droid Serif");
pref("font.name.sans-serif.tr", "Droid Sans");
pref("font.name.monospace.tr", "Droid Sans Mono");

pref("font.name.serif.x-baltic", "Droid Serif");
pref("font.name.sans-serif.x-baltic", "Droid Sans");
pref("font.name.monospace.x-baltic", "Droid Sans Mono");

pref("font.name.serif.x-central-euro", "Droid Serif");
pref("font.name.sans-serif.x-central-euro", "Droid Sans");
pref("font.name.monospace.x-central-euro", "Droid Sans Mono");

pref("font.name.serif.x-cyrillic", "Droid Serif");
pref("font.name.sans-serif.x-cyrillic", "Droid Sans");
pref("font.name.monospace.x-cyrillic", "Droid Sans Mono");

pref("font.name.serif.x-unicode", "Droid Serif");
pref("font.name.sans-serif.x-unicode", "Droid Sans");
pref("font.name.monospace.x-unicode", "Droid Sans Mono");

pref("font.name.serif.x-user-def", "Droid Serif");
pref("font.name.sans-serif.x-user-def", "Droid Sans");
pref("font.name.monospace.x-user-def", "Droid Sans Mono");

pref("font.name.serif.x-western", "Droid Serif");
pref("font.name.sans-serif.x-western", "Droid Sans");
pref("font.name.monospace.x-western", "Droid Sans Mono");

pref("font.name.serif.zh-CN", "Droid Serif");
pref("font.name.sans-serif.zh-CN", "Droid Sans");
pref("font.name.monospace.zh-CN", "Droid Sans Mono");



pref("font.name.serif.zh-HK", "Droid Serif");
pref("font.name.sans-serif.zh-HK", "Droid Sans");
pref("font.name.monospace.zh-HK", "Droid Sans Mono");



pref("font.default.ar", "sans-serif");
pref("font.size.variable.ar", 16);
pref("font.size.fixed.ar", 12);

pref("font.default.el", "sans-serif");
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
pref("font.size.fixed.th", 13);
pref("font.minimum-size.th", 13);

pref("font.default.tr", "sans-serif");
pref("font.size.variable.tr", 16);
pref("font.size.fixed.tr", 12);

pref("font.default.x-baltic", "sans-serif");
pref("font.size.variable.x-baltic", 16);
pref("font.size.fixed.x-baltic", 12);

pref("font.default.x-central-euro", "sans-serif");
pref("font.size.variable.x-central-euro", 16);
pref("font.size.fixed.x-central-euro", 12);

pref("font.default.x-cyrillic", "sans-serif");
pref("font.size.variable.x-cyrillic", 16);
pref("font.size.fixed.x-cyrillic", 12);

pref("font.default.x-unicode", "sans-serif");
pref("font.size.variable.x-unicode", 16);
pref("font.size.fixed.x-unicode", 12);

pref("font.default.x-user-def", "sans-serif");
pref("font.size.variable.x-user-def", 16);
pref("font.size.fixed.x-user-def", 12);

pref("font.default.x-western", "sans-serif");
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

pref("font.default.x-orya", "serif");
pref("font.size.variable.x-orya", 16);
pref("font.size.fixed.x-orya", 13);

pref("font.default.x-telu", "serif");
pref("font.size.variable.x-telu", 16);
pref("font.size.fixed.x-telu", 13);

pref("font.default.x-knda", "serif");
pref("font.size.variable.x-knda", 16);
pref("font.size.fixed.x-knda", 13);

pref("font.default.x-sinh", "serif");
pref("font.size.variable.x-sinh", 16);
pref("font.size.fixed.x-sinh", 13);

pref("font.default.x-tibt", "serif");
pref("font.size.variable.x-tibt", 16);
pref("font.size.fixed.x-tibt", 13);



pref("print.postscript.paper_size",    "letter");
pref("print.postscript.orientation",   "portrait");
pref("print.postscript.print_command", "lpr ${MOZ_PRINTER_NAME:+-P\"$MOZ_PRINTER_NAME\"}");








pref("ui.panel.default_level_parent", true);

pref("mousewheel.system_scroll_override_on_root_content.enabled", false);

# ANDROID
#endif

#ifndef ANDROID
#ifndef XP_MACOSX
#ifdef XP_UNIX

pref("network.protocol-handler.warn-external.file", false);
pref("browser.drag_out_of_frame_style", 1);


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
pref("print.print_command", "lpr ${MOZ_PRINTER_NAME:+-P\"$MOZ_PRINTER_NAME\"}");
pref("print.printer_list", ""); 
pref("print.print_reversed", false);
pref("print.print_color", true);
pref("print.print_landscape", false);
pref("print.print_paper_size", 0);



pref("print.print_extra_margin", 0); 

pref("font.allow_double_byte_special_chars", true);


pref("font.alias-list", "sans,sans-serif,serif,monospace");



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

pref("font.name.serif.th", "serif");
pref("font.name.sans-serif.th", "sans-serif");
pref("font.name.monospace.th", "monospace");

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

pref("font.name.serif.x-western", "serif");
pref("font.name.sans-serif.x-western", "sans-serif");
pref("font.name.monospace.x-western", "monospace");

pref("font.name.serif.zh-CN", "serif");
pref("font.name.sans-serif.zh-CN", "sans-serif");
pref("font.name.monospace.zh-CN", "monospace");



pref("font.name.serif.zh-HK", "serif");
pref("font.name.sans-serif.zh-HK", "sans-serif");
pref("font.name.monospace.zh-HK", "monospace");



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
pref("font.size.fixed.th", 13);
pref("font.minimum-size.th", 13);

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

pref("font.default.x-user-def", "serif");
pref("font.size.variable.x-user-def", 16);
pref("font.size.fixed.x-user-def", 12);

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

pref("font.default.x-orya", "serif");
pref("font.size.variable.x-orya", 16);
pref("font.size.fixed.x-orya", 13);

pref("font.default.x-telu", "serif");
pref("font.size.variable.x-telu", 16);
pref("font.size.fixed.x-telu", 13);

pref("font.default.x-knda", "serif");
pref("font.size.variable.x-knda", 16);
pref("font.size.fixed.x-knda", 13);

pref("font.default.x-sinh", "serif");
pref("font.size.variable.x-sinh", 16);
pref("font.size.fixed.x-sinh", 13);

pref("font.default.x-tibt", "serif");
pref("font.size.variable.x-tibt", 16);
pref("font.size.fixed.x-tibt", 13);



pref("print.postscript.paper_size",    "letter");
pref("print.postscript.orientation",   "portrait");
pref("print.postscript.print_command", "lpr ${MOZ_PRINTER_NAME:+-P\"$MOZ_PRINTER_NAME\"}");










pref("ui.panel.default_level_parent", true);

pref("mousewheel.system_scroll_override_on_root_content.enabled", false);

# XP_UNIX
#endif
#endif
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

pref("print.postscript.print_command", "lp -c -s ${MOZ_PRINTER_NAME:+-d\"$MOZ_PRINTER_NAME\"}");
pref("print.print_command", "lp -c -s ${MOZ_PRINTER_NAME:+-d\"$MOZ_PRINTER_NAME\"}");

# Solaris
#endif


pref("signon.rememberSignons",              true);
pref("signon.SignonFileName",               "signons.txt"); 
pref("signon.SignonFileName2",              "signons2.txt"); 
pref("signon.SignonFileName3",              "signons3.txt"); 
pref("signon.autofillForms",                true);
pref("signon.autologin.proxy",              false);
pref("signon.debug",                        false);


pref("browser.formfill.debug",            false);
pref("browser.formfill.enable",           true);
pref("browser.formfill.expire_days",      180);
pref("browser.formfill.saveHttpsForms",   true);
pref("browser.formfill.agedWeight",       2);
pref("browser.formfill.bucketSize",       1);
pref("browser.formfill.maxTimeGroupings", 25);
pref("browser.formfill.timeGroupingSize", 604800);
pref("browser.formfill.boundaryWeight",   25);
pref("browser.formfill.prefixWeight",     5);


pref("browser.zoom.full", false);
pref("zoom.minPercent", 30);
pref("zoom.maxPercent", 300);
pref("toolkit.zoomManager.zoomValues", ".3,.5,.67,.8,.9,1,1.1,1.2,1.33,1.5,1.7,2,2.4,3");



pref("image.cache.size", 5242880);


pref("image.cache.timeweight", 500);


pref("image.http.accept", "image/png,image/*;q=0.8,*/*;q=0.5");







pref("image.mem.discardable", true);



pref("image.mem.decodeondraw", false);




pref("image.mem.min_discard_timeout_ms", 120000);


pref("image.mem.decode_bytes_at_a_time", 200000);


pref("image.mem.max_ms_before_yield", 400);


pref("image.mem.max_bytes_for_sync_decode", 150000);


pref("webgl.force-enabled", false);
pref("webgl.disabled", false);
pref("webgl.shader_validator", true);
pref("webgl.force_osmesa", false);
pref("webgl.osmesalib", "");
pref("webgl.verbose", false);
pref("webgl.prefer-native-gl", false);

#ifdef XP_WIN
#ifndef WINCE

pref("network.tcp.sendbuffer", 131072);
#endif
#endif

#ifdef WINCE
pref("mozilla.widget.disable-native-theme", true);
pref("gfx.color_management.mode", 0);
#endif


pref("layers.acceleration.disabled", false);


pref("layers.acceleration.force-enabled", false);

#ifdef XP_WIN
#ifndef WINCE

pref("gfx.direct2d.disabled", false);


pref("gfx.direct2d.force-enabled", false);

pref("layers.prefer-opengl", false);
pref("layers.prefer-d3d9", false);
#endif
#endif


pref("geo.enabled", true);


pref("accelerometer.enabled", true);


pref("html5.parser.enable", true);

pref("html5.offmainthread", true);



pref("html5.flushtimer.initialdelay", 120);


pref("html5.flushtimer.subsequentdelay", 120);


pref("browser.history.allowPushState", true);
pref("browser.history.allowReplaceState", true);
pref("browser.history.allowPopState", true);
pref("browser.history.maxStateObjectSize", 655360);


pref("xpinstall.whitelist.required", true);
pref("extensions.alwaysUnpack", false);

pref("network.buffer.cache.count", 24);
pref("network.buffer.cache.size",  32768);


pref("notification.feature.enabled", false);
