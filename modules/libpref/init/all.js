




















pref("keyword.enabled", false);
pref("general.useragent.locale", "chrome://global/locale/intl.properties");
pref("general.useragent.compatMode.firefox", false);



pref("general.useragent.site_specific_overrides", true);

pref("general.config.obscure_value", 13); 

pref("general.warnOnAboutConfig", true);


pref("browser.bookmarks.max_backups",       5);


pref("browser.cache.auto_delete_cache_version", 0);



pref("browser.cache.use_new_backend",       0);
pref("browser.cache.use_new_backend_temp",  true);

pref("browser.cache.disk.enable",           true);

pref("browser.cache.disk.smart_size.first_run", true);

pref("browser.cache.disk.smart_size.enabled", true);

pref("browser.cache.disk.smart_size.use_old_max", true);

pref("browser.cache.disk.capacity",         256000);






pref("browser.cache.disk.free_space_soft_limit", 5120); 
pref("browser.cache.disk.free_space_hard_limit", 1024); 


pref("browser.cache.disk.max_entry_size",    51200);  
pref("browser.cache.memory.enable",         true);




pref("browser.cache.memory.max_entry_size",  5120);








pref("browser.cache.disk.max_chunks_memory_usage", 10240);
pref("browser.cache.disk.max_priority_chunks_memory_usage", 10240);

pref("browser.cache.disk_cache_ssl",        true);

pref("browser.cache.check_doc_frequency",   3);

pref("browser.cache.disk.metadata_memory_limit", 250); 

pref("browser.cache.disk.preload_chunk_count", 4); 

pref("browser.cache.frecency_half_life_hours", 6);

pref("browser.cache.offline.enable",           true);

pref("offline-apps.allow_by_default",          true);


pref("browser.cache.offline.capacity",         512000);



pref("offline-apps.quota.warn",        51200);






pref("browser.cache.compression_level", 0);


pref("dom.abortablepromise.enabled", false);


pref("dom.quotaManager.testing", false);


pref("dom.indexedDB.enabled", true);

pref("dom.indexedDB.experimental", false);

pref("dom.indexedDB.logging.enabled", true);

pref("dom.indexedDB.logging.details", true);

pref("dom.indexedDB.logging.profiler-marks", false);


pref("dom.workers.enabled", true);

pref("dom.workers.maxPerDomain", 20);


pref("dom.workers.sharedWorkers.enabled", true);


pref("dom.workers.websocket.enabled", true);


pref("dom.serviceWorkers.enabled", false);


pref("dom.enable_performance", true);


pref("dom.enable_resource_timing", true);


pref("dom.enable_user_timing", true);


pref("dom.performance.enable_user_timing_logging", false);


pref("dom.gamepad.enabled", true);
#ifdef RELEASE_BUILD
pref("dom.gamepad.non_standard_events.enabled", false);
#else
pref("dom.gamepad.non_standard_events.enabled", true);
#endif


pref("dom.keyboardevent.code.enabled", true);




pref("dom.keyboardevent.dispatch_during_composition", false);


pref("dom.undo_manager.enabled", false);



pref("dom.url.encode_decode_hash", true);




#ifdef NIGHTLY_BUILD
pref("dom.compartment_per_addon", true);
#else
pref("dom.compartment_per_addon", false);
#endif



pref("browser.sessionhistory.max_total_viewers", -1);

pref("ui.use_native_colors", true);
pref("ui.click_hold_context_menus", false);

pref("ui.menu.incremental_search.timeout", 1000);
pref("browser.display.use_document_fonts",  1);  



pref("browser.display.document_color_use", 0);
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
pref("browser.helperApps.deleteTempFileOnExit", false);


pref("browser.chrome.toolbar_tips",         true);

pref("browser.chrome.toolbar_style",        2);


pref("browser.chrome.image_icons.max_size", 1024);

pref("browser.triple_click_selects_paragraph", true);


pref("print.shrink-to-fit.scale-limit-percent", 20);


pref("media.cache_size", 512000);


pref("media.cache_resume_threshold", 999999);



pref("media.cache_readahead_limit", 999999);


pref("media.volume_scale", "1.0");


pref("media.wakelock_timeout", 2000);



pref("media.play-stand-alone", true);

pref("media.hardware-video-decoding.enabled", true);

pref("media.decoder.heuristic.dormant.enabled", true);
pref("media.decoder.heuristic.dormant.timeout", 60000);

#ifdef MOZ_WMF
pref("media.windows-media-foundation.enabled", true);
#endif
#ifdef MOZ_DIRECTSHOW
pref("media.directshow.enabled", true);
#endif
#ifdef MOZ_FMP4
pref("media.fragmented-mp4.enabled", true);
pref("media.fragmented-mp4.ffmpeg.enabled", false);
pref("media.fragmented-mp4.gmp.enabled", false);
#if defined(XP_WIN) && defined(MOZ_WMF) || defined(XP_MACOSX) || defined(MOZ_WIDGET_GONK)

pref("media.fragmented-mp4.exposed", true);
#else
pref("media.fragmented-mp4.exposed", false);
#endif



pref("media.fragmented-mp4.use-blank-decoder", false);
#endif
#ifdef MOZ_RAW
pref("media.raw.enabled", true);
#endif
pref("media.ogg.enabled", true);
pref("media.opus.enabled", true);
#ifdef MOZ_WAVE
pref("media.wave.enabled", true);
#endif
#ifdef MOZ_WEBM
pref("media.webm.enabled", true);
#if defined(MOZ_FMP4) && defined(MOZ_WMF)
pref("media.webm.intel_decoder.enabled", false);
#endif
#endif
#ifdef MOZ_GSTREAMER
pref("media.gstreamer.enabled", true);
pref("media.gstreamer.enable-blacklist", true);
#endif
#ifdef MOZ_APPLEMEDIA
pref("media.apple.mp3.enabled", true);
pref("media.apple.mp4.enabled", true);
#endif
#ifdef MOZ_WEBRTC
pref("media.navigator.enabled", true);
pref("media.navigator.video.enabled", true);
pref("media.navigator.load_adapt", true);
pref("media.navigator.load_adapt.measure_interval",1000);
pref("media.navigator.load_adapt.avg_seconds",3);
pref("media.navigator.load_adapt.high_load","0.90");
pref("media.navigator.load_adapt.low_load","0.40");
pref("media.navigator.video.default_fps",30);
pref("media.navigator.video.default_minfps",10);

pref("media.webrtc.debug.trace_mask", 0);
pref("media.webrtc.debug.multi_log", false);
pref("media.webrtc.debug.aec_log_dir", "");
pref("media.webrtc.debug.log_file", "");
pref("media.webrtc.debug.aec_dump_max_size", 4194304); 

#ifdef MOZ_WIDGET_GONK
pref("media.navigator.video.default_width",320);
pref("media.navigator.video.default_height",240);
pref("media.peerconnection.enabled", true);
pref("media.peerconnection.video.enabled", true);
pref("media.navigator.video.max_fs", 1200); 
pref("media.navigator.video.max_fr", 30);
pref("media.navigator.video.h264.level", 12); 
pref("media.navigator.video.h264.max_br", 700); 
pref("media.navigator.video.h264.max_mbps", 11880); 
pref("media.peerconnection.video.h264_enabled", false);
pref("media.getusermedia.aec", 4);


pref("media.peerconnection.video.min_bitrate", 100);
pref("media.peerconnection.video.start_bitrate", 220);
pref("media.peerconnection.video.max_bitrate", 1000);
#else
pref("media.navigator.video.default_width",0);  
pref("media.navigator.video.default_height",0); 
pref("media.peerconnection.enabled", true);
pref("media.peerconnection.video.enabled", true);
pref("media.navigator.video.max_fs", 12288); 
pref("media.navigator.video.max_fr", 60);
pref("media.navigator.video.h264.level", 31); 
pref("media.navigator.video.h264.max_br", 0);
pref("media.navigator.video.h264.max_mbps", 0);
pref("media.peerconnection.video.h264_enabled", false);
pref("media.getusermedia.aec", 1);
pref("media.getusermedia.browser.enabled", true);


pref("media.peerconnection.video.min_bitrate", 200);
pref("media.peerconnection.video.start_bitrate", 300);
pref("media.peerconnection.video.max_bitrate", 2000);
#endif
pref("media.navigator.permission.disabled", false);
pref("media.peerconnection.default_iceservers", "[{\"urls\": [\"stun:stun.services.mozilla.com\"]}]");
pref("media.peerconnection.ice.loopback", false); 
pref("media.peerconnection.use_document_iceservers", true);




pref("media.peerconnection.identity.enabled", false);
pref("media.peerconnection.identity.timeout", 10000);
pref("media.peerconnection.ice.loopback", false); 



pref("media.peerconnection.turn.disable", false);
#if defined(MOZ_WEBRTC_HARDWARE_AEC_NS)
pref("media.getusermedia.aec_enabled", false);
pref("media.getusermedia.noise_enabled", false);
#else
pref("media.getusermedia.aec_enabled", true);
pref("media.getusermedia.noise_enabled", true);
#endif
pref("media.getusermedia.noise", 1);
pref("media.getusermedia.agc_enabled", false);
pref("media.getusermedia.agc", 1);


#if defined(XP_MACOSX)
pref("media.peerconnection.capture_delay", 50);
pref("media.getusermedia.playout_delay", 10);
#elif defined(XP_WIN)
pref("media.peerconnection.capture_delay", 50);
pref("media.getusermedia.playout_delay", 40);
#elif defined(ANDROID)
pref("media.peerconnection.capture_delay", 100);
pref("media.getusermedia.playout_delay", 100);

pref("media.navigator.hardware.vp8_encode.acceleration_enabled", false);
pref("media.navigator.hardware.vp8_decode.acceleration_enabled", false);
#elif defined(XP_LINUX)
pref("media.peerconnection.capture_delay", 70);
pref("media.getusermedia.playout_delay", 50);
#else

pref("media.peerconnection.capture_delay", 50);
pref("media.getusermedia.playout_delay", 50);
#endif
#endif

#if !defined(ANDROID)
pref("media.getusermedia.screensharing.enabled", true);
#endif

#ifdef RELEASE_BUILD
pref("media.getusermedia.screensharing.allowed_domains", "webex.com,*.webex.com,ciscospark.com,*.ciscospark.com,projectsquared.com,*.projectsquared.com,*.room.co,room.co,beta.talky.io,talky.io,*.clearslide.com,appear.in,*.appear.in,tokbox.com,*.tokbox.com,*.sso.francetelecom.fr,*.si.francetelecom.fr,*.sso.infra.ftgroup,*.multimedia-conference.orange-business.com,*.espacecollaboration.orange-business.com,free.gotomeeting.com,g2m.me,*.g2m.me,example.com");
#else
 
pref("media.getusermedia.screensharing.allowed_domains", "mozilla.github.io,webex.com,*.webex.com,ciscospark.com,*.ciscospark.com,projectsquared.com,*.projectsquared.com,*.room.co,room.co,beta.talky.io,talky.io,*.clearslide.com,appear.in,*.appear.in,tokbox.com,*.tokbox.com,*.sso.francetelecom.fr,*.si.francetelecom.fr,*.sso.infra.ftgroup,*.multimedia-conference.orange-business.com,*.espacecollaboration.orange-business.com,free.gotomeeting.com,g2m.me,*.g2m.me,example.com");
#endif

pref("media.getusermedia.screensharing.allow_on_old_platforms", false);


pref("media.webvtt.enabled", true);
pref("media.webvtt.regions.enabled", false);


pref("media.track.enabled", false);





#if defined(XP_WIN) || defined(XP_MACOSX)
pref("media.mediasource.enabled", true);
#else
pref("media.mediasource.enabled", false);
#endif

#ifdef RELEASE_BUILD
pref("media.mediasource.whitelist", true);
#else
pref("media.mediasource.whitelist", false);
#endif 

pref("media.mediasource.mp4.enabled", true);
pref("media.mediasource.webm.enabled", false);

#ifdef MOZ_WEBSPEECH
pref("media.webspeech.recognition.enable", false);
pref("media.webspeech.synth.enabled", false);
#endif
#ifdef MOZ_WEBM_ENCODER
pref("media.encoder.webm.enabled", true);
#endif
#ifdef MOZ_OMX_ENCODER
pref("media.encoder.omx.enabled", true);
#endif


pref("media.autoplay.enabled", true);



pref("media.video-queue.default-size", 10);


pref("media.video_stats.enabled", true);


pref("media.audio_data.enabled", false);


pref("layers.async-pan-zoom.enabled", false);


pref("layout.event-regions.enabled", false);



pref("apz.allow_checkerboarding", true);
pref("apz.asyncscroll.throttle", 100);
pref("apz.asyncscroll.timeout", 300);





pref("apz.axis_lock.mode", 0);
pref("apz.axis_lock.lock_angle", "0.5235987");        
pref("apz.axis_lock.breakout_threshold", "0.03125");  
pref("apz.axis_lock.breakout_angle", "0.3926991");    
pref("apz.axis_lock.direct_pan_angle", "1.047197");   
pref("apz.content_response_timeout", 300);
pref("apz.cross_slide.enabled", false);
pref("apz.danger_zone_x", 50);
pref("apz.danger_zone_y", 100);
pref("apz.enlarge_displayport_when_clipped", false);
pref("apz.fling_accel_base_mult", "1.0");
pref("apz.fling_accel_interval_ms", 500);
pref("apz.fling_accel_supplemental_mult", "1.0");
pref("apz.fling_curve_function_x1", "0.0");
pref("apz.fling_curve_function_y1", "0.0");
pref("apz.fling_curve_function_x2", "1.0");
pref("apz.fling_curve_function_y2", "1.0");
pref("apz.fling_curve_threshold_inches_per_ms", "-1.0");
pref("apz.fling_friction", "0.002");
pref("apz.fling_stop_on_tap_threshold", "0.05");
pref("apz.fling_stopped_threshold", "0.01");
pref("apz.max_velocity_inches_per_ms", "-1.0");
pref("apz.max_velocity_queue_size", 5);
pref("apz.min_skate_speed", "1.0");
pref("apz.num_paint_duration_samples", 3);
pref("apz.overscroll.enabled", false);
pref("apz.overscroll.min_pan_distance_ratio", "1.0");
pref("apz.overscroll.stretch_factor", "0.5");
pref("apz.overscroll.spring_stiffness", "0.001");
pref("apz.overscroll.spring_friction", "0.015");
pref("apz.overscroll.stop_distance_threshold", "5.0");
pref("apz.overscroll.stop_velocity_threshold", "0.01");


pref("apz.printtree", false);

pref("apz.test.logging_enabled", false);
pref("apz.touch_start_tolerance", "0.2222222");  
pref("apz.use_paint_duration", true);
pref("apz.velocity_bias", "1.0");
pref("apz.velocity_relevance_time_ms", 150);
pref("apz.x_stationary_size_multiplier", "3.0");
pref("apz.y_stationary_size_multiplier", "3.5");
pref("apz.zoom_animation_duration_ms", 250);

#if !defined(MOZ_WIDGET_GONK) && !defined(MOZ_WIDGET_ANDROID)

pref("apz.fling_repaint_interval", 16);
pref("apz.smooth_scroll_repaint_interval", 16);
pref("apz.pan_repaint_interval", 16);
pref("apz.x_skate_size_multiplier", "2.5");
pref("apz.y_skate_size_multiplier", "3.5");
#else

pref("apz.fling_repaint_interval", 75);
pref("apz.smooth_scroll_repaint_interval", 75);
pref("apz.pan_repaint_interval", 250);
pref("apz.x_skate_size_multiplier", "1.5");
pref("apz.y_skate_size_multiplier", "2.5");
#endif


pref("apz.test.logging_enabled", false);

#ifdef XP_MACOSX




pref("gfx.hidpi.enabled", 2);
#endif

#if !defined(MOZ_WIDGET_GONK) && !defined(MOZ_WIDGET_ANDROID)


pref("layout.scroll.root-frame-containers", false);
#endif


pref("gfx.layerscope.enabled", false);
pref("gfx.layerscope.port", 23456);



pref("gfx.perf-warnings.enabled", false);



pref("gfx.color_management.mode", 2);
pref("gfx.color_management.display_profile", "");
pref("gfx.color_management.rendering_intent", 0);
pref("gfx.color_management.enablev4", false);

pref("gfx.downloadable_fonts.enabled", true);
pref("gfx.downloadable_fonts.fallback_delay", 3000);



pref("gfx.downloadable_fonts.disable_cache", false);

pref("gfx.downloadable_fonts.woff2.enabled", true);

#ifdef ANDROID
pref("gfx.bundled_fonts.enabled", true);
pref("gfx.bundled_fonts.force-enabled", false);
#endif



pref("gfx.missing_fonts.notify", false);

pref("gfx.filter.nearest.force-enabled", false);


pref("gfx.font_loader.families_per_slice", 3); 
#ifdef XP_WIN
pref("gfx.font_loader.delay", 120000);         
pref("gfx.font_loader.interval", 1000);        
#else
pref("gfx.font_loader.delay", 8000);           
pref("gfx.font_loader.interval", 50);          
#endif


pref("gfx.font_rendering.fallback.always_use_cmaps", false);


pref("gfx.font_rendering.wordcache.charlimit", 32);


pref("gfx.font_rendering.wordcache.maxentries", 10000);

pref("gfx.font_rendering.graphite.enabled", true);

#ifdef XP_WIN
pref("gfx.font_rendering.directwrite.enabled", false);
pref("gfx.font_rendering.directwrite.use_gdi_table_loading", true);
#endif

pref("gfx.font_rendering.opentype_svg.enabled", true);

#ifdef XP_WIN


pref("gfx.canvas.azure.backends", "direct2d1.1,direct2d,skia,cairo");
pref("gfx.content.azure.backends", "direct2d1.1,direct2d,cairo");
#else
#ifdef XP_MACOSX
pref("gfx.content.azure.backends", "cg");
pref("gfx.canvas.azure.backends", "skia");

pref("gfx.canvas.azure.accelerated", false);
#else
pref("gfx.canvas.azure.backends", "cairo");
pref("gfx.content.azure.backends", "cairo");
#endif
#endif

#ifdef MOZ_WIDGET_GTK2
pref("gfx.content.azure.backends", "cairo");
#endif
#ifdef ANDROID
pref("gfx.content.azure.backends", "cairo");
#endif

pref("gfx.work-around-driver-bugs", true);
pref("gfx.prefer-mesa-llvmpipe", false);

pref("gfx.draw-color-bars", false);

pref("accessibility.browsewithcaret", false);
pref("accessibility.warn_on_browsewithcaret", true);

pref("accessibility.browsewithcaret_shortcut.enabled", true);

#ifndef XP_MACOSX





pref("accessibility.tabfocus", 7);
pref("accessibility.tabfocus_applies_to_xul", false);
#else

pref("accessibility.tabfocus_applies_to_xul", true);
#endif




#if !defined(XP_MACOSX) && !defined(MOZ_WIDGET_GTK)
pref("ui.scrollToClick", 0);
#endif


pref("canvas.focusring.enabled", true);
pref("canvas.customfocusring.enabled", false);
pref("canvas.hitregions.enabled", false);
pref("canvas.filters.enabled", false);

pref("canvas.path.enabled", true);












pref("accessibility.force_disabled", 0);

pref("accessibility.ipc_architecture.enabled", true);

#ifdef XP_WIN





pref("accessibility.delay_plugins", false);
pref("accessibility.delay_plugin_time", 10000);
#endif

pref("focusmanager.testmode", false);

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
pref("accessibility.typeaheadfind.enablesound", true);
#ifdef XP_MACOSX
pref("accessibility.typeaheadfind.prefillwithselection", false);
#else
pref("accessibility.typeaheadfind.prefillwithselection", true);
#endif
pref("accessibility.typeaheadfind.matchesCountTimeout", 250);
pref("accessibility.typeaheadfind.matchesCountLimit", 100);


pref("gfx.use_text_smoothing_setting", false);


pref("toolkit.autocomplete.richBoundaryCutoff", 200);


pref("toolkit.osfile.log", false);

pref("toolkit.scrollbox.smoothScroll", true);
pref("toolkit.scrollbox.scrollIncrement", 20);
pref("toolkit.scrollbox.verticalScrollDistance", 3);
pref("toolkit.scrollbox.horizontalScrollDistance", 5);
pref("toolkit.scrollbox.clickToScroll.scrollDelay", 150);

pref("toolkit.telemetry.server", "https://incoming.telemetry.mozilla.org");

pref("toolkit.telemetry.server_owner", "Mozilla");

pref("toolkit.telemetry.infoURL", "https://www.mozilla.org/legal/privacy/firefox.html#telemetry");


pref("toolkit.telemetry.debugSlowSql", false);


pref("toolkit.identity.enabled", false);
pref("toolkit.identity.debug", false);


pref("toolkit.asyncshutdown.timeout.crash", 60000);


pref("devtools.errorconsole.deprecation_warnings", true);


#ifdef MOZ_DEV_EDITION
pref("devtools.chrome.enabled", true);
#else
pref("devtools.chrome.enabled", false);
#endif


pref("devtools.debugger.log", false);
pref("devtools.debugger.log.verbose", false);

#ifdef MOZ_DEV_EDITION
pref("devtools.debugger.remote-enabled", true);
#else
pref("devtools.debugger.remote-enabled", false);
#endif
pref("devtools.debugger.remote-port", 6000);

pref("devtools.debugger.force-local", true);

pref("devtools.debugger.prompt-connection", true);

pref("devtools.debugger.forbid-certified-apps", true);

pref("devtools.apps.forbidden-permissions", "embed-apps,engineering-mode,embed-widgets");


pref("devtools.defaultColorUnit", "hex");


pref("devtools.dump.emit", false);


pref("devtools.discovery.log", false);

pref("devtools.remote.wifi.scan", true);



pref("devtools.remote.wifi.visible", true);

pref("devtools.remote.tls-handshake-timeout", 10000);


pref("devtools.devices.url", "https://code.cdn.mozilla.net/devices/devices.json");


pref("view_source.syntax_highlight", true);
pref("view_source.wrap_long_lines", false);
pref("view_source.editor.external", false);
pref("view_source.editor.path", "");


pref("view_source.editor.args", "");


pref("plain_text.wrap_long_lines", false);


pref("nglayout.enable_drag_images", true);



pref("nglayout.debug.paint_flashing", false);
pref("nglayout.debug.paint_flashing_chrome", false);



pref("nglayout.debug.widget_update_flashing", false);



pref("layout.imagevisibility.enabled", true);



pref("layout.imagevisibility.enabled_for_browser_elements_only", false);
pref("layout.imagevisibility.numscrollportwidths", 0);
pref("layout.imagevisibility.numscrollportheights", 1);




pref("slider.snapMultiplier", 0);


pref("application.use_ns_plugin_finder", false);


pref("browser.fixup.alternate.enabled", true);
pref("browser.fixup.alternate.prefix", "www.");
pref("browser.fixup.alternate.suffix", ".com");
pref("browser.fixup.dns_first_for_single_words", false);
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
pref("editor.use_css",                       false);
pref("editor.css.default_length_unit",       "px");
pref("editor.resizing.preserve_ratio",       true);
pref("editor.positioning.offset",            0);


pref("dom.disable_beforeunload",            false);
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
pref("dom.popup_allowed_events", "change click dblclick mouseup reset submit touchend");
pref("dom.disable_open_click_delay", 1000);

pref("dom.storage.enabled", true);
pref("dom.storage.default_quota",      5120);

pref("dom.send_after_paint_to_content", false);


pref("dom.min_timeout_value", 4);

pref("dom.min_background_timeout_value", 1000);


pref("dom.experimental_forms", false);


pref("dom.forms.number", true);



pref("dom.forms.color", true);


pref("dom.forms.autocomplete.experimental", false);


pref("dom.forms.requestAutocomplete", false);


pref("dom.sysmsg.enabled", false);


pref("dom.webapps.useCurrentProfile", false);

pref("dom.cycle_collector.incremental", true);


#ifndef XP_WIN
pref("content.sink.pending_event_mode", 0);
#endif





pref("privacy.popups.disable_from_plugins", 2);


pref("privacy.donottrackheader.enabled",    false);

pref("privacy.trackingprotection.enabled",  false);

pref("privacy.trackingprotection.pbmode.enabled",  false);

pref("dom.event.contextmenu.enabled",       true);
pref("dom.event.clipboardevents.enabled",   true);
#if defined(XP_WIN) && !defined(RELEASE_BUILD)
pref("dom.event.highrestimestamp.enabled",  true);
#else
pref("dom.event.highrestimestamp.enabled",  false);
#endif

pref("dom.webcomponents.enabled",           false);

pref("javascript.enabled",                  true);
pref("javascript.options.strict",           false);
#ifdef DEBUG
pref("javascript.options.strict.debug",     false);
#endif
pref("javascript.options.baselinejit",      true);
pref("javascript.options.ion",              true);
pref("javascript.options.asmjs",            true);
pref("javascript.options.native_regexp",    true);
pref("javascript.options.parallel_parsing", true);
pref("javascript.options.ion.offthread_compilation", true);




pref("javascript.options.discardSystemSource", false);




pref("javascript.options.mem.high_water_mark", 128);
pref("javascript.options.mem.max", -1);
pref("javascript.options.mem.gc_per_compartment", true);
pref("javascript.options.mem.gc_incremental", true);
pref("javascript.options.mem.gc_incremental_slice_ms", 10);
pref("javascript.options.mem.gc_compacting", true);
pref("javascript.options.mem.log", false);
pref("javascript.options.mem.notify", false);
pref("javascript.options.gc_on_memory_pressure", true);
pref("javascript.options.compact_on_user_inactive", true);

pref("javascript.options.mem.gc_high_frequency_time_limit_ms", 1000);
pref("javascript.options.mem.gc_high_frequency_low_limit_mb", 100);
pref("javascript.options.mem.gc_high_frequency_high_limit_mb", 500);
pref("javascript.options.mem.gc_high_frequency_heap_growth_max", 300);
pref("javascript.options.mem.gc_high_frequency_heap_growth_min", 150);
pref("javascript.options.mem.gc_low_frequency_heap_growth", 150);
pref("javascript.options.mem.gc_dynamic_heap_growth", true);
pref("javascript.options.mem.gc_dynamic_mark_slice", true);
pref("javascript.options.mem.gc_allocation_threshold_mb", 30);
pref("javascript.options.mem.gc_decommit_threshold_mb", 32);
pref("javascript.options.mem.gc_min_empty_chunk_count", 1);
pref("javascript.options.mem.gc_max_empty_chunk_count", 30);

pref("javascript.options.showInConsole", false);


pref("advanced.mailftp",                    false);
pref("image.animation_mode",                "normal");


pref("security.fileuri.strict_origin_policy", true);












pref("network.allow-experiments", true);



pref("network.notify.changed", true);



pref("network.tickle-wifi.enabled", false);
pref("network.tickle-wifi.duration", 400);
pref("network.tickle-wifi.delay", 16);


pref("network.disable.ipc.security", false);


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


pref("network.protocol-handler.external.ttp", false);  
pref("network.protocol-handler.external.ttps", false); 
pref("network.protocol-handler.external.tps", false);  
pref("network.protocol-handler.external.ps", false);   
pref("network.protocol-handler.external.ile", false);  
pref("network.protocol-handler.external.le", false);   










pref("network.protocol-handler.expose-all", true);


pref("network.warnOnAboutNetworking", true);





pref("network.http.version", "1.1");      





pref("network.http.proxy.version", "1.1");    

                                              


pref("network.http.use-cache", true);



pref("network.http.default-socket-type", "");






pref("network.http.keep-alive.timeout", 115);


pref("network.http.response.timeout", 300);





pref("network.http.max-connections", 256);




pref("network.http.max-persistent-connections-per-server", 6);




pref("network.http.max-persistent-connections-per-proxy", 32);





pref("network.http.request.max-start-delay", 10);


pref("network.http.accept.default", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");



pref("network.http.sendRefererHeader",      2);

pref("network.http.referer.spoofSource", false);

pref("network.http.referer.trimmingPolicy", 0);

pref("network.http.referer.XOriginPolicy", 0);



pref("network.http.sendSecureXSiteReferrer", true);


pref("network.http.redirection-limit", 20);




pref("network.http.accept-encoding", "gzip, deflate");

pref("network.http.pipelining"      , false);
pref("network.http.pipelining.ssl"  , false); 
pref("network.http.pipelining.abtest", false);
pref("network.http.proxy.pipelining", false);


pref("network.http.pipelining.maxrequests" , 32);



pref("network.http.pipelining.max-optimistic-requests" , 4);

pref("network.http.pipelining.aggressive", false);
pref("network.http.pipelining.maxsize" , 300000);
pref("network.http.pipelining.reschedule-on-timeout", true);
pref("network.http.pipelining.reschedule-timeout", 1500);



pref("network.http.pipelining.read-timeout", 30000);


pref("network.http.prompt-temp-redirect", false);



pref("network.http.assoc-req.enforce", false);









pref("network.http.qos", 0);




pref("network.http.connection-retry-timeout", 250);



pref("network.http.connection-timeout", 90);




pref("network.http.network-changed.timeout", 5);



pref("network.http.speculative-parallel-limit", 6);



pref("network.http.rendering-critical-requests-prioritization", true);



pref("network.http.fast-fallback-to-IPv4", true);



#ifdef RELEASE_BUILD
pref("network.http.bypass-cachelock-threshold", 200000);
#else
pref("network.http.bypass-cachelock-threshold", 250);
#endif


pref("network.http.spdy.enabled", true);
pref("network.http.spdy.enabled.v3-1", true);
pref("network.http.spdy.enabled.http2draft", true);
pref("network.http.spdy.enabled.http2", true);
pref("network.http.spdy.enabled.deps", true);
pref("network.http.spdy.enforce-tls-profile", true);
pref("network.http.spdy.chunk-size", 16000);
pref("network.http.spdy.timeout", 180);
pref("network.http.spdy.coalesce-hostnames", true);
pref("network.http.spdy.persistent-settings", false);
pref("network.http.spdy.ping-threshold", 58);
pref("network.http.spdy.ping-timeout", 8);
pref("network.http.spdy.send-buffer-size", 131072);
pref("network.http.spdy.allow-push", true);
pref("network.http.spdy.push-allowance", 131072);
pref("network.http.spdy.default-concurrent", 100);



pref("network.http.altsvc.enabled", false);
pref("network.http.altsvc.oe", false);

pref("network.http.diagnostics", false);

pref("network.http.pacing.requests.enabled", true);
pref("network.http.pacing.requests.min-parallelism", 6);
pref("network.http.pacing.requests.hz", 100);
pref("network.http.pacing.requests.burst", 32);


pref("network.http.tcp_keepalive.short_lived_connections", true);

pref("network.http.tcp_keepalive.short_lived_time", 60);

pref("network.http.tcp_keepalive.short_lived_idle_time", 10);

pref("network.http.tcp_keepalive.long_lived_connections", true);
pref("network.http.tcp_keepalive.long_lived_idle_time", 600);

pref("network.http.enforce-framing.http1", false); 
pref("network.http.enforce-framing.soft", true);





pref("network.ftp.data.qos", 0);
pref("network.ftp.control.qos", 0);




pref("network.sts.serve_multiple_events_per_poll_iteration", true);

pref("network.sts.max_time_for_events_between_two_polls", 100);



pref("network.websocket.max-message-size", 2147483647);


pref("network.websocket.auto-follow-http-redirects", false);


pref("network.websocket.timeout.open", 20);



pref("network.websocket.timeout.close", 20);



pref("network.websocket.timeout.ping.request", 0);




pref("network.websocket.timeout.ping.response", 10);



pref("network.websocket.extensions.permessage-deflate", true);




pref("network.websocket.max-connections", 200);



pref("network.websocket.allowInsecureFromHTTPS", false);



pref("network.websocket.delay-failed-reconnects", true);





pref("dom.server-events.enabled", true);

pref("dom.server-events.default-reconnection-time", 5000); 




pref("network.jar.open-unsafe-types", false);




pref("network.IDN_show_punycode", false);















pref("network.IDN.restriction_profile", "moderate");
pref("network.IDN.use_whitelist", true);


pref("network.IDN.whitelist.ac", true);
pref("network.IDN.whitelist.ar", true);
pref("network.IDN.whitelist.at", true);
pref("network.IDN.whitelist.br", true);
pref("network.IDN.whitelist.ca", true);
pref("network.IDN.whitelist.ch", true);
pref("network.IDN.whitelist.cl", true);
pref("network.IDN.whitelist.cn", true);
pref("network.IDN.whitelist.de", true);
pref("network.IDN.whitelist.dk", true);
pref("network.IDN.whitelist.ee", true);
pref("network.IDN.whitelist.es", true);
pref("network.IDN.whitelist.fi", true);
pref("network.IDN.whitelist.fr", true);
pref("network.IDN.whitelist.gr", true);
pref("network.IDN.whitelist.gt", true);
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
pref("network.IDN.whitelist.lv", true);
pref("network.IDN.whitelist.no", true);
pref("network.IDN.whitelist.nu", true);
pref("network.IDN.whitelist.nz", true);
pref("network.IDN.whitelist.pl", true);
pref("network.IDN.whitelist.pm", true);
pref("network.IDN.whitelist.pr", true);
pref("network.IDN.whitelist.re", true);
pref("network.IDN.whitelist.se", true);
pref("network.IDN.whitelist.sh", true);
pref("network.IDN.whitelist.si", true);
pref("network.IDN.whitelist.tf", true);
pref("network.IDN.whitelist.th", true);
pref("network.IDN.whitelist.tm", true);
pref("network.IDN.whitelist.tw", true);
pref("network.IDN.whitelist.ua", true);
pref("network.IDN.whitelist.vn", true);
pref("network.IDN.whitelist.wf", true);
pref("network.IDN.whitelist.yt", true);



pref("network.IDN.whitelist.xn--mgbaam7a8h", true);

pref("network.IDN.whitelist.xn--fiqz9s", true); 
pref("network.IDN.whitelist.xn--fiqs8s", true); 

pref("network.IDN.whitelist.xn--wgbh1c", true);

pref("network.IDN.whitelist.xn--j6w193g", true);

pref("network.IDN.whitelist.xn--mgba3a4f16a", true);
pref("network.IDN.whitelist.xn--mgba3a4fra", true);

pref("network.IDN.whitelist.xn--mgbayh7gpa", true);

pref("network.IDN.whitelist.xn--fzc2c9e2c", true);
pref("network.IDN.whitelist.xn--xkc2al3hye2a", true);

pref("network.IDN.whitelist.xn--wgbl6a", true);

pref("network.IDN.whitelist.xn--90a3ac", true);

pref("network.IDN.whitelist.xn--p1ai", true);

pref("network.IDN.whitelist.xn--mgberp4a5d4ar", true);
pref("network.IDN.whitelist.xn--mgberp4a5d4a87g", true);
pref("network.IDN.whitelist.xn--mgbqly7c0a67fbc", true);
pref("network.IDN.whitelist.xn--mgbqly7cvafr", true);

pref("network.IDN.whitelist.xn--ogbpf8fl", true);

pref("network.IDN.whitelist.xn--o3cw4h", true);

pref("network.IDN.whitelist.xn--kpry57d", true);  
pref("network.IDN.whitelist.xn--kprw13d", true);  


pref("network.IDN.whitelist.asia", true);
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






pref("network.IDN.blacklist_chars", "\u0020\u00A0\u00BC\u00BD\u00BE\u01C3\u02D0\u0337\u0338\u0589\u05C3\u05F4\u0609\u060A\u066A\u06D4\u0701\u0702\u0703\u0704\u115F\u1160\u1735\u2000\u2001\u2002\u2003\u2004\u2005\u2006\u2007\u2008\u2009\u200A\u200B\u200E\u200F\u2024\u2027\u2028\u2029\u202A\u202B\u202C\u202D\u202E\u202F\u2039\u203A\u2041\u2044\u2052\u205F\u2153\u2154\u2155\u2156\u2157\u2158\u2159\u215A\u215B\u215C\u215D\u215E\u215F\u2215\u2236\u23AE\u2571\u29F6\u29F8\u2AFB\u2AFD\u2FF0\u2FF1\u2FF2\u2FF3\u2FF4\u2FF5\u2FF6\u2FF7\u2FF8\u2FF9\u2FFA\u2FFB\u3000\u3002\u3014\u3015\u3033\u3164\u321D\u321E\u33AE\u33AF\u33C6\u33DF\uA789\uFE14\uFE15\uFE3F\uFE5D\uFE5E\uFEFF\uFF0E\uFF0F\uFF61\uFFA0\uFFF9\uFFFA\uFFFB\uFFFC\uFFFD");




pref("network.dns.ipv4OnlyDomains", "");


pref("network.dns.disableIPv6", false);


pref("network.dnsCacheEntries", 400);


pref("network.dnsCacheExpiration", 60);


pref("network.dns.get-ttl", false);



pref("network.dnsCacheExpirationGracePeriod", 60);


pref("network.dns.disablePrefetch", false);


pref("network.dns.offline-localhost", true);



pref("network.standard-url.escape-utf8", true);



pref("network.standard-url.encode-utf8", true);


pref("network.standard-url.max-length", 1048576);


pref("network.ftp.idleConnectionTimeout", 300);





pref("network.dir.format", 2);


pref("network.prefetch-next", true);


pref("network.predictor.enabled", true);
pref("network.predictor.enable-hover-on-ssl", false);
pref("network.predictor.page-degradation.day", 0);
pref("network.predictor.page-degradation.week", 5);
pref("network.predictor.page-degradation.month", 10);
pref("network.predictor.page-degradation.year", 25);
pref("network.predictor.page-degradation.max", 50);
pref("network.predictor.subresource-degradation.day", 1);
pref("network.predictor.subresource-degradation.week", 10);
pref("network.predictor.subresource-degradation.month", 25);
pref("network.predictor.subresource-degradation.year", 50);
pref("network.predictor.subresource-degradation.max", 100);
pref("network.predictor.preconnect-min-confidence", 90);
pref("network.predictor.preresolve-min-confidence", 60);
pref("network.predictor.redirect-likely-confidence", 75);
pref("network.predictor.max-resources-per-entry", 100);
pref("network.predictor.cleaned-up", false);









pref("network.auth.force-generic-ntlm-v1", false);



pref("network.negotiate-auth.trusted-uris", "");

pref("network.negotiate-auth.delegation-uris", "");


pref("network.negotiate-auth.allow-non-fqdn", false);


pref("network.negotiate-auth.allow-proxies", true);


pref("network.negotiate-auth.gsslib", "");


pref("network.negotiate-auth.using-native-gsslib", true);

#ifdef XP_WIN


pref("network.auth.use-sspi", true);

#endif








pref("network.auth.force-generic-ntlm", false);





pref("network.automatic-ntlm-auth.allow-proxies", true);
pref("network.automatic-ntlm-auth.allow-non-fqdn", false);
pref("network.automatic-ntlm-auth.trusted-uris", "");







pref("network.auth.allow-subresource-auth", 1);

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
pref("network.proxy.proxy_over_tls",        true);
pref("network.proxy.no_proxies_on",         "localhost, 127.0.0.1");
pref("network.proxy.failover_timeout",      1800); 
pref("network.online",                      true); 
pref("network.cookie.cookieBehavior",       0); 
#ifdef ANDROID
pref("network.cookie.cookieBehavior",       0); 
#endif
pref("network.cookie.thirdparty.sessionOnly", false);
pref("network.cookie.lifetimePolicy",       0); 
pref("network.cookie.alwaysAcceptSessionCookies", false);
pref("network.cookie.prefsMigrated",        false);
pref("network.cookie.lifetime.days",        90);


pref("network.proxy.autoconfig_url", "");



pref("network.proxy.autoconfig_retry_interval_min", 5);    
pref("network.proxy.autoconfig_retry_interval_max", 300);  


pref("network.stricttransportsecurity.preloadlist", true);

pref("converter.html2txt.structs",          true); 
pref("converter.html2txt.header_strategy",  1); 




pref("converter.html2txt.always_include_ruby", false);

pref("intl.accept_languages",               "chrome://global/locale/intl.properties");
pref("intl.menuitems.alwaysappendaccesskeys","chrome://global/locale/intl.properties");
pref("intl.menuitems.insertseparatorbeforeaccesskeys","chrome://global/locale/intl.properties");
pref("intl.charset.detector",               "chrome://global/locale/intl.properties");
pref("intl.charset.fallback.override",      "");
pref("intl.charset.fallback.tld",           true);
pref("intl.ellipsis",                       "chrome://global-platform/locale/intl.properties");
pref("intl.locale.matchOS",                 false);



pref("intl.fallbackCharsetList.ISO-8859-1", "windows-1252");
pref("font.language.group",                 "chrome://global/locale/intl.properties");


pref("intl.uidirection.ar", "rtl");
pref("intl.uidirection.he", "rtl");
pref("intl.uidirection.fa", "rtl");
pref("intl.uidirection.ug", "rtl");
pref("intl.uidirection.ur", "rtl");


pref("intl.hyphenation-alias.en", "en-us");

pref("intl.hyphenation-alias.en-*", "en-us");

pref("intl.hyphenation-alias.af-*", "af");
pref("intl.hyphenation-alias.bg-*", "bg");
pref("intl.hyphenation-alias.ca-*", "ca");
pref("intl.hyphenation-alias.cy-*", "cy");
pref("intl.hyphenation-alias.da-*", "da");
pref("intl.hyphenation-alias.eo-*", "eo");
pref("intl.hyphenation-alias.es-*", "es");
pref("intl.hyphenation-alias.et-*", "et");
pref("intl.hyphenation-alias.fi-*", "fi");
pref("intl.hyphenation-alias.fr-*", "fr");
pref("intl.hyphenation-alias.gl-*", "gl");
pref("intl.hyphenation-alias.hr-*", "hr");
pref("intl.hyphenation-alias.hsb-*", "hsb");
pref("intl.hyphenation-alias.hu-*", "hu");
pref("intl.hyphenation-alias.ia-*", "ia");
pref("intl.hyphenation-alias.is-*", "is");
pref("intl.hyphenation-alias.it-*", "it");
pref("intl.hyphenation-alias.kmr-*", "kmr");
pref("intl.hyphenation-alias.la-*", "la");
pref("intl.hyphenation-alias.lt-*", "lt");
pref("intl.hyphenation-alias.mn-*", "mn");
pref("intl.hyphenation-alias.nl-*", "nl");
pref("intl.hyphenation-alias.pl-*", "pl");
pref("intl.hyphenation-alias.pt-*", "pt");
pref("intl.hyphenation-alias.ru-*", "ru");
pref("intl.hyphenation-alias.sl-*", "sl");
pref("intl.hyphenation-alias.sv-*", "sv");
pref("intl.hyphenation-alias.tr-*", "tr");
pref("intl.hyphenation-alias.uk-*", "uk");



pref("intl.hyphenation-alias.de", "de-1996");
pref("intl.hyphenation-alias.de-*", "de-1996");
pref("intl.hyphenation-alias.de-AT-1901", "de-1901");
pref("intl.hyphenation-alias.de-DE-1901", "de-1901");
pref("intl.hyphenation-alias.de-CH-*", "de-CH");




pref("intl.hyphenation-alias.sr", "sh");
pref("intl.hyphenation-alias.bs", "sh");
pref("intl.hyphenation-alias.sh-*", "sh");
pref("intl.hyphenation-alias.sr-*", "sh");
pref("intl.hyphenation-alias.bs-*", "sh");



pref("intl.hyphenation-alias.no", "nb");
pref("intl.hyphenation-alias.no-*", "nb");
pref("intl.hyphenation-alias.nb-*", "nb");
pref("intl.hyphenation-alias.nn-*", "nn");

pref("font.mathfont-family", "Latin Modern Math, XITS Math, STIX Math, Cambria Math, Asana Math, TeX Gyre Bonum Math, TeX Gyre Pagella Math, TeX Gyre Termes Math, Neo Euler, Lucida Bright Math, MathJax_Main, STIXNonUnicode, STIXSizeOneSym, STIXGeneral, Standard Symbols L, DejaVu Sans");



pref("font.blacklist.underline_offset", "FangSong,Gulim,GulimChe,MingLiU,MingLiU-ExtB,MingLiU_HKSCS,MingLiU-HKSCS-ExtB,MS Gothic,MS Mincho,MS PGothic,MS PMincho,MS UI Gothic,PMingLiU,PMingLiU-ExtB,SimHei,SimSun,SimSun-ExtB,Hei,Kai,Apple LiGothic,Apple LiSung,Osaka");

#ifdef MOZ_B2G




pref("font.whitelist.skip_default_features_space_check", "Fira Sans,Fira Mono");
#endif

pref("images.dither", "auto");
pref("security.directory",              "");

pref("signed.applets.codebase_principal_support", false);
pref("security.checkloaduri", true);
pref("security.xpconnect.plugin.unrestricted", true);

pref("security.dialog_enable_delay", 1000);
pref("security.notification_enable_delay", 500);

pref("security.csp.enable", true);
pref("security.csp.debug", false);
pref("security.csp.experimentalEnabled", false);


pref("security.apps.privileged.CSP.default", "default-src * data: blob:; script-src 'self'; object-src 'none'; style-src 'self' 'unsafe-inline'");


pref("security.mixed_content.block_active_content", false);
pref("security.mixed_content.block_display_content", false);


pref("security.cert_pinning.enforcement_level", 0);



pref("security.cert_pinning.process_headers_from_non_builtin_roots", false);




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




pref("mousewheel.acceleration.start", -1);

pref("mousewheel.acceleration.factor", 10);










pref("mousewheel.system_scroll_override_on_root_content.vertical.factor", 200);
pref("mousewheel.system_scroll_override_on_root_content.horizontal.factor", 200);








pref("mousewheel.default.action", 1);
pref("mousewheel.with_alt.action", 2);
pref("mousewheel.with_control.action", 3);
pref("mousewheel.with_meta.action", 1);  
pref("mousewheel.with_shift.action", 1);
pref("mousewheel.with_win.action", 1);





pref("mousewheel.default.action.override_x", -1);
pref("mousewheel.with_alt.action.override_x", -1);
pref("mousewheel.with_control.action.override_x", -1);
pref("mousewheel.with_meta.action.override_x", -1);  
pref("mousewheel.with_shift.action.override_x", -1);
pref("mousewheel.with_win.action.override_x", -1);





pref("mousewheel.default.delta_multiplier_x", 100);
pref("mousewheel.default.delta_multiplier_y", 100);
pref("mousewheel.default.delta_multiplier_z", 100);
pref("mousewheel.with_alt.delta_multiplier_x", 100);
pref("mousewheel.with_alt.delta_multiplier_y", 100);
pref("mousewheel.with_alt.delta_multiplier_z", 100);
pref("mousewheel.with_control.delta_multiplier_x", 100);
pref("mousewheel.with_control.delta_multiplier_y", 100);
pref("mousewheel.with_control.delta_multiplier_z", 100);
pref("mousewheel.with_meta.delta_multiplier_x", 100);  
pref("mousewheel.with_meta.delta_multiplier_y", 100);  
pref("mousewheel.with_meta.delta_multiplier_z", 100);  
pref("mousewheel.with_shift.delta_multiplier_x", 100);
pref("mousewheel.with_shift.delta_multiplier_y", 100);
pref("mousewheel.with_shift.delta_multiplier_z", 100);
pref("mousewheel.with_win.delta_multiplier_x", 100);
pref("mousewheel.with_win.delta_multiplier_y", 100);
pref("mousewheel.with_win.delta_multiplier_z", 100);



pref("mousewheel.min_line_scroll_amount", 5);









pref("general.smoothScroll.mouseWheel.durationMinMS", 200);
pref("general.smoothScroll.mouseWheel.durationMaxMS", 400);
pref("general.smoothScroll.pixels.durationMinMS", 150);
pref("general.smoothScroll.pixels.durationMaxMS", 150);
pref("general.smoothScroll.lines.durationMinMS", 150);
pref("general.smoothScroll.lines.durationMaxMS", 150);
pref("general.smoothScroll.pages.durationMinMS", 150);
pref("general.smoothScroll.pages.durationMaxMS", 150);
pref("general.smoothScroll.scrollbars.durationMinMS", 150);
pref("general.smoothScroll.scrollbars.durationMaxMS", 150);
pref("general.smoothScroll.other.durationMinMS", 150);
pref("general.smoothScroll.other.durationMaxMS", 150);

pref("general.smoothScroll.mouseWheel", true);
pref("general.smoothScroll.pixels", true);
pref("general.smoothScroll.lines", true);
pref("general.smoothScroll.pages", true);
pref("general.smoothScroll.scrollbars", true);
pref("general.smoothScroll.other", true);





pref("general.smoothScroll.durationToIntervalRatio", 200);

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



pref("bidi.edit.delete_immediately", true);





pref("bidi.edit.caret_movement_style", 2);




pref("bidi.browser.ui", false);


pref("layout.word_select.eat_space_to_next_word", false);
pref("layout.word_select.stop_at_punctuation", true);










pref("layout.selection.caret_style", 0);


pref("layout.frames.force_resizability", false);


pref("layout.css.report_errors", true);


pref("layout.css.visited_links_enabled", true);





pref("layout.css.dpi", -1);





pref("layout.css.devPixelsPerPx", "-1.0");


pref("layout.css.masking.enabled", true);


pref("layout.css.mix-blend-mode.enabled", true);


pref("layout.css.isolation.enabled", true);


pref("layout.css.filters.enabled", true);



pref("layout.css.scroll-snap.proximity-threshold", 200);



pref("layout.css.scroll-snap.prediction-max-velocity", 2000);





pref("layout.css.scroll-snap.prediction-sensitivity", "0.750");


pref("layout.css.clip-path-shapes.enabled", false);


pref("layout.css.sticky.enabled", true);


pref("layout.css.will-change.enabled", true);


pref("layout.css.DOMPoint.enabled", true);


pref("layout.css.DOMQuad.enabled", true);


pref("layout.css.DOMMatrix.enabled", true);


#ifdef RELEASE_BUILD
pref("layout.css.getBoxQuads.enabled", false);
#else
pref("layout.css.getBoxQuads.enabled", true);
#endif


#ifdef RELEASE_BUILD
pref("layout.css.convertFromNode.enabled", false);
#else
pref("layout.css.convertFromNode.enabled", true);
#endif


#ifdef RELEASE_BUILD
pref("layout.css.unicode-range.enabled", false);
#else
pref("layout.css.unicode-range.enabled", true);
#endif


pref("layout.css.text-align-true-value.enabled", false);


pref("layout.css.image-orientation.enabled", true);


pref("layout.css.prefixes.border-image", true);
pref("layout.css.prefixes.transforms", true);
pref("layout.css.prefixes.transitions", true);
pref("layout.css.prefixes.animations", true);
pref("layout.css.prefixes.box-sizing", true);
pref("layout.css.prefixes.font-features", true);



pref("layout.css.unprefixing-service.enabled", true);


pref("layout.css.scope-pseudo.enabled", true);


pref("layout.css.background-blend-mode.enabled", true);


#ifdef RELEASE_BUILD
pref("layout.css.vertical-text.enabled", false);
#else
pref("layout.css.vertical-text.enabled", true);
#endif


pref("layout.css.object-fit-and-position.enabled", true);



#ifdef XP_MACOSX
pref("layout.css.osx-font-smoothing.enabled", true);
#else
pref("layout.css.osx-font-smoothing.enabled", false);
#endif


pref("layout.css.unset-value.enabled", true);


pref("layout.css.all-shorthand.enabled", true);


pref("layout.css.variables.enabled", true);


pref("layout.css.overflow-clip-box.enabled", false);


pref("layout.css.grid.enabled", false);







pref("layout.css.ruby.enabled", true);


pref("layout.css.display-contents.enabled", true);


pref("layout.css.box-decoration-break.enabled", true);


pref("layout.css.outline-style-auto.enabled", false);


pref("layout.css.scroll-behavior.enabled", true);


pref("layout.css.scroll-behavior.property-enabled", true);




pref("layout.css.scroll-behavior.spring-constant", "250.0");










pref("layout.css.scroll-behavior.damping-ratio", "1.0");


pref("layout.css.scroll-snap.enabled", true);


#ifdef RELEASE_BUILD
pref("layout.css.font-loading-api.enabled", false);
#else
pref("layout.css.font-loading-api.enabled", true);
#endif






pref("layout.scrollbar.side", 0);


pref("layout.testing.overlay-scrollbars.always-visible", false);




pref("layout.interruptible-reflow.enabled", true);




pref("layout.frame_rate", -1);


pref("layout.display-list.dump", false);











pref("layout.frame_rate.precise", false);


pref("layout.spammy_warnings.enabled", true);


pref("layout.float-fragments-inside-column.enabled", true);


#ifdef RELEASE_BUILD
pref("dom.animations-api.core.enabled", false);
#else
pref("dom.animations-api.core.enabled", true);
#endif


pref("capability.policy.default.SOAPCall.invokeVerifySourceHeader", "allAccess");


pref("plugin.override_internal_types", false);



pref("browser.popups.showPopupBlocker", true);



pref("viewmanager.do_doublebuffering", true);


pref("gestures.enable_single_finger_input", true);

pref("editor.resizing.preserve_ratio",       true);
pref("editor.positioning.offset",            0);

pref("dom.use_watchdog", true);
pref("dom.max_chrome_script_run_time", 20);
pref("dom.max_child_script_run_time", 10);
pref("dom.max_script_run_time", 10);


pref("dom.archivereader.enabled", false);





pref("hangmonitor.timeout", 0);

pref("plugins.load_appdir_plugins", false);

pref("plugins.click_to_play", false);







pref("plugins.enumerable_names", "*");


pref("plugin.default.state", 2);




pref("plugin.java.mime", "application/x-java-vm");



pref("plugin.sessionPermissionNow.intervalInMinutes", 60);


pref("plugin.persistentPermissionAlways.intervalInDays", 90);

#ifndef DEBUG


pref("dom.ipc.plugins.timeoutSecs", 45);



pref("dom.ipc.plugins.parentTimeoutSecs", 0);


pref("dom.ipc.plugins.contentTimeoutSecs", 45);


pref("dom.ipc.plugins.processLaunchTimeoutSecs", 45);
#ifdef XP_WIN


pref("dom.ipc.plugins.hangUITimeoutSecs", 11);

pref("dom.ipc.plugins.hangUIMinDisplaySecs", 10);
#endif



pref("dom.ipc.tabs.shutdownTimeoutSecs", 5);
#else

pref("dom.ipc.plugins.timeoutSecs", 0);
pref("dom.ipc.plugins.contentTimeoutSecs", 0);
pref("dom.ipc.plugins.processLaunchTimeoutSecs", 0);
pref("dom.ipc.plugins.parentTimeoutSecs", 0);
#ifdef XP_WIN
pref("dom.ipc.plugins.hangUITimeoutSecs", 0);
pref("dom.ipc.plugins.hangUIMinDisplaySecs", 0);
#endif
pref("dom.ipc.tabs.shutdownTimeoutSecs", 0);
#endif

#ifdef XP_WIN


pref("dom.ipc.plugins.java.enabled", false);
#endif

pref("dom.ipc.plugins.flash.disable-protected-mode", false);

pref("dom.ipc.plugins.flash.subprocess.crashreporter.enabled", true);
pref("dom.ipc.plugins.reportCrashURL", true);



pref("dom.ipc.plugins.unloadTimeoutSecs", 30);



#ifdef NIGHTLY_BUILD
pref("dom.ipc.plugins.asyncInit", false);
#else
pref("dom.ipc.plugins.asyncInit", true);
#endif

pref("dom.ipc.processCount", 1);


pref("svg.path-caching.enabled", true);


pref("svg.display-lists.hit-testing.enabled", true);
pref("svg.display-lists.painting.enabled", true);


pref("svg.paint-order.enabled", true);


pref("svg.marker-improvements.enabled", true);



pref("svg.new-getBBox.enabled", false);


pref("font.default.ar", "sans-serif");
pref("font.minimum-size.ar", 0);
pref("font.size.variable.ar", 16);
pref("font.size.fixed.ar", 13);

pref("font.default.el", "serif");
pref("font.minimum-size.el", 0);
pref("font.size.variable.el", 16);
pref("font.size.fixed.el", 13);

pref("font.default.he", "sans-serif");
pref("font.minimum-size.he", 0);
pref("font.size.variable.he", 16);
pref("font.size.fixed.he", 13);

pref("font.default.ja", "sans-serif");
pref("font.minimum-size.ja", 0);
pref("font.size.variable.ja", 16);
pref("font.size.fixed.ja", 16);

pref("font.default.ko", "sans-serif");
pref("font.minimum-size.ko", 0);
pref("font.size.variable.ko", 16);
pref("font.size.fixed.ko", 16);

pref("font.default.th", "serif");
pref("font.minimum-size.th", 0);
pref("font.size.variable.th", 16);
pref("font.size.fixed.th", 13);

pref("font.default.x-cyrillic", "serif");
pref("font.minimum-size.x-cyrillic", 0);
pref("font.size.variable.x-cyrillic", 16);
pref("font.size.fixed.x-cyrillic", 13);

pref("font.default.x-devanagari", "serif");
pref("font.minimum-size.x-devanagari", 0);
pref("font.size.variable.x-devanagari", 16);
pref("font.size.fixed.x-devanagari", 13);

pref("font.default.x-tamil", "serif");
pref("font.minimum-size.x-tamil", 0);
pref("font.size.variable.x-tamil", 16);
pref("font.size.fixed.x-tamil", 13);

pref("font.default.x-armn", "serif");
pref("font.minimum-size.x-armn", 0);
pref("font.size.variable.x-armn", 16);
pref("font.size.fixed.x-armn", 13);

pref("font.default.x-beng", "serif");
pref("font.minimum-size.x-beng", 0);
pref("font.size.variable.x-beng", 16);
pref("font.size.fixed.x-beng", 13);

pref("font.default.x-cans", "serif");
pref("font.minimum-size.x-cans", 0);
pref("font.size.variable.x-cans", 16);
pref("font.size.fixed.x-cans", 13);

pref("font.default.x-ethi", "serif");
pref("font.minimum-size.x-ethi", 0);
pref("font.size.variable.x-ethi", 16);
pref("font.size.fixed.x-ethi", 13);

pref("font.default.x-geor", "serif");
pref("font.minimum-size.x-geor", 0);
pref("font.size.variable.x-geor", 16);
pref("font.size.fixed.x-geor", 13);

pref("font.default.x-gujr", "serif");
pref("font.minimum-size.x-gujr", 0);
pref("font.size.variable.x-gujr", 16);
pref("font.size.fixed.x-gujr", 13);

pref("font.default.x-guru", "serif");
pref("font.minimum-size.x-guru", 0);
pref("font.size.variable.x-guru", 16);
pref("font.size.fixed.x-guru", 13);

pref("font.default.x-khmr", "serif");
pref("font.minimum-size.x-khmr", 0);
pref("font.size.variable.x-khmr", 16);
pref("font.size.fixed.x-khmr", 13);

pref("font.default.x-mlym", "serif");
pref("font.minimum-size.x-mlym", 0);
pref("font.size.variable.x-mlym", 16);
pref("font.size.fixed.x-mlym", 13);

pref("font.default.x-orya", "serif");
pref("font.minimum-size.x-orya", 0);
pref("font.size.variable.x-orya", 16);
pref("font.size.fixed.x-orya", 13);

pref("font.default.x-telu", "serif");
pref("font.minimum-size.x-telu", 0);
pref("font.size.variable.x-telu", 16);
pref("font.size.fixed.x-telu", 13);

pref("font.default.x-knda", "serif");
pref("font.minimum-size.x-knda", 0);
pref("font.size.variable.x-knda", 16);
pref("font.size.fixed.x-knda", 13);

pref("font.default.x-sinh", "serif");
pref("font.minimum-size.x-sinh", 0);
pref("font.size.variable.x-sinh", 16);
pref("font.size.fixed.x-sinh", 13);

pref("font.default.x-tibt", "serif");
pref("font.minimum-size.x-tibt", 0);
pref("font.size.variable.x-tibt", 16);
pref("font.size.fixed.x-tibt", 13);

pref("font.default.x-unicode", "serif");
pref("font.minimum-size.x-unicode", 0);
pref("font.size.variable.x-unicode", 16);
pref("font.size.fixed.x-unicode", 13);

pref("font.default.x-western", "serif");
pref("font.minimum-size.x-western", 0);
pref("font.size.variable.x-western", 16);
pref("font.size.fixed.x-western", 13);

pref("font.default.zh-CN", "sans-serif");
pref("font.minimum-size.zh-CN", 0);
pref("font.size.variable.zh-CN", 16);
pref("font.size.fixed.zh-CN", 16);

pref("font.default.zh-HK", "sans-serif");
pref("font.minimum-size.zh-HK", 0);
pref("font.size.variable.zh-HK", 16);
pref("font.size.fixed.zh-HK", 16);

pref("font.default.zh-TW", "sans-serif");
pref("font.minimum-size.zh-TW", 0);
pref("font.size.variable.zh-TW", 16);
pref("font.size.fixed.zh-TW", 16);











pref("font.size.inflation.emPerLine", 0);










pref("font.size.inflation.minTwips", 0);










pref("font.size.inflation.forceEnabled", false);









pref("font.size.inflation.disabledInMasterProcess", false);





















pref("font.size.inflation.lineThreshold", 400);






















pref("font.size.inflation.mappingIntercept", 1);











pref("font.size.inflation.maxRatio", 0);










pref("ui.touch.radius.enabled", false);
pref("ui.touch.radius.leftmm", 8);
pref("ui.touch.radius.topmm", 12);
pref("ui.touch.radius.rightmm", 8);
pref("ui.touch.radius.bottommm", 4);
pref("ui.touch.radius.visitedWeight", 120);

pref("ui.mouse.radius.enabled", false);
pref("ui.mouse.radius.leftmm", 8);
pref("ui.mouse.radius.topmm", 12);
pref("ui.mouse.radius.rightmm", 8);
pref("ui.mouse.radius.bottommm", 4);
pref("ui.mouse.radius.visitedWeight", 120);



pref("ui.mouse.radius.inputSource.touchOnly", true);

#ifdef XP_WIN

pref("font.name.serif.ar", "Times New Roman");
pref("font.name.sans-serif.ar", "Segoe UI");
pref("font.name-list.sans-serif.ar", "Segoe UI, Tahoma, Arial");
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

pref("font.name.serif.ja", "MS PMincho");
pref("font.name.sans-serif.ja", "MS PGothic");
pref("font.name.monospace.ja", "MS Gothic");
pref("font.name-list.serif.ja", "MS PMincho, MS Mincho, MS PGothic, MS Gothic,Meiryo");
pref("font.name-list.sans-serif.ja", "MS PGothic, MS Gothic, MS PMincho, MS Mincho,Meiryo");
pref("font.name-list.monospace.ja", "MS Gothic, MS Mincho, MS PGothic, MS PMincho,Meiryo");

pref("font.name.serif.ko", "Batang");
pref("font.name.sans-serif.ko", "Gulim");
pref("font.name.monospace.ko", "GulimChe");
pref("font.name.cursive.ko", "Gungsuh");

pref("font.name-list.serif.ko", "Batang, Gulim");
pref("font.name-list.sans-serif.ko", "Gulim");
pref("font.name-list.monospace.ko", "GulimChe");
pref("font.name-list.cursive.ko", "Gungsuh");

pref("font.name.serif.th", "Tahoma");
pref("font.name.sans-serif.th", "Tahoma");
pref("font.name.monospace.th", "Tahoma");
pref("font.name.cursive.th", "Tahoma");

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

pref("font.name.serif.zh-CN", "SimSun");
pref("font.name.sans-serif.zh-CN", "Microsoft YaHei");
pref("font.name.monospace.zh-CN", "SimSun");
pref("font.name.cursive.zh-CN", "KaiTi");
pref("font.name-list.serif.zh-CN", "MS Song, SimSun, SimSun-ExtB");
pref("font.name-list.sans-serif.zh-CN", "Microsoft YaHei, SimHei, Arial Unicode MS");
pref("font.name-list.monospace.zh-CN", "MS Song, SimSun, SimSun-ExtB");



pref("font.name.serif.zh-TW", "Times New Roman");
pref("font.name.sans-serif.zh-TW", "Arial");
pref("font.name.monospace.zh-TW", "MingLiU");
pref("font.name-list.serif.zh-TW", "PMingLiu, MingLiU, MingLiU-ExtB");
pref("font.name-list.sans-serif.zh-TW", "PMingLiU, MingLiU, MingLiU-ExtB");
pref("font.name-list.monospace.zh-TW", "MingLiU, MingLiU-ExtB");



pref("font.name.serif.zh-HK", "Times New Roman");
pref("font.name.sans-serif.zh-HK", "Arial");
pref("font.name.monospace.zh-HK", "MingLiu_HKSCS");
pref("font.name-list.serif.zh-HK", "MingLiu_HKSCS, Ming(for ISO10646), MingLiU, MingLiU_HKSCS-ExtB");
pref("font.name-list.sans-serif.zh-HK", "MingLiU_HKSCS, Ming(for ISO10646), MingLiU, MingLiU_HKSCS-ExtB");
pref("font.name-list.monospace.zh-HK", "MingLiU_HKSCS, Ming(for ISO10646), MingLiU, MingLiU_HKSCS-ExtB");

pref("font.name.serif.x-devanagari", "Kokila");
pref("font.name.sans-serif.x-devanagari", "Nirmala UI");
pref("font.name.monospace.x-devanagari", "Mangal");
pref("font.name-list.serif.x-devanagari", "Kokila, Raghindi");
pref("font.name-list.sans-serif.x-devanagari", "Nirmala UI, Mangal");
pref("font.name-list.monospace.x-devanagari", "Mangal, Nirmala UI");

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

pref("font.minimum-size.th", 10);

pref("font.default.x-devanagari", "sans-serif");

pref("font.mathfont-family", "MathJax_Main, STIXNonUnicode, STIXSizeOneSym, STIXGeneral, Asana Math, Symbol, DejaVu Sans, Cambria Math");




pref("gfx.font_rendering.cleartype.use_for_downloadable_fonts", true);


pref("gfx.font_rendering.cleartype.always_use_for_content", false);




























pref("gfx.font_rendering.cleartype_params.gamma", -1);
pref("gfx.font_rendering.cleartype_params.enhanced_contrast", -1);
pref("gfx.font_rendering.cleartype_params.cleartype_level", -1);
pref("gfx.font_rendering.cleartype_params.pixel_structure", -1);
pref("gfx.font_rendering.cleartype_params.rendering_mode", -1);






pref("gfx.font_rendering.cleartype_params.force_gdi_classic_for_families",
     "Arial,Consolas,Courier New,Microsoft Sans Serif,Segoe UI,Tahoma,Trebuchet MS,Verdana");


pref("gfx.font_rendering.cleartype_params.force_gdi_classic_max_size", 15);

pref("ui.key.menuAccessKeyFocuses", true);


pref("layout.word_select.eat_space_to_next_word", true);


pref("slider.snapMultiplier", 6);



pref("print.print_extra_margin", 90); 


pref("print.extend_native_print_dialog", true);


pref("plugin.scan.Acrobat", "5.0");


pref("plugin.scan.Quicktime", "5.0");


pref("plugin.scan.WindowsMediaPlayer", "7.0");



pref("plugin.scan.plid.all", true);


pref("plugin.allow.asyncdrawing", false);



pref("network.autodial-helper.enabled", true);


pref("intl.keyboard.per_window_layout", false);

#ifdef NS_ENABLE_TSF

pref("intl.tsf.enable", true);



pref("intl.tsf.force_enable", false);


pref("intl.tsf.support_imm", true);




pref("intl.tsf.hack.atok.create_native_caret", true);




pref("intl.tsf.hack.free_chang_jie.do_not_return_no_layout_error", true);

pref("intl.tsf.hack.easy_changjei.do_not_return_no_layout_error", true);



pref("intl.tsf.hack.google_ja_input.do_not_return_no_layout_error_at_first_char", true);



pref("intl.tsf.hack.google_ja_input.do_not_return_no_layout_error_at_caret", true);
#endif


pref("ui.panel.default_level_parent", false);

pref("mousewheel.system_scroll_override_on_root_content.enabled", true);


pref("mousewheel.enable_pixel_scrolling", true);



pref("mousewheel.emulate_at_wm_scroll", false);



pref("ui.trackpoint_hack.enabled", -1);




pref("ui.window_class_override", "");




pref("ui.elantech_gesture_hacks.enabled", -1);

# XP_WIN
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

pref("font.name.serif.el", "Times");
pref("font.name.sans-serif.el", "Helvetica");
pref("font.name.monospace.el", "Courier New");
pref("font.name.cursive.el", "Lucida Grande");
pref("font.name.fantasy.el", "Lucida Grande");
pref("font.name-list.serif.el", "Times,Times New Roman");
pref("font.name-list.sans-serif.el", "Helvetica,Lucida Grande");
pref("font.name-list.monospace.el", "Courier New,Lucida Grande");
pref("font.name-list.cursive.el", "Times,Lucida Grande");
pref("font.name-list.fantasy.el", "Times,Lucida Grande");

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

pref("font.name.serif.ja", "Hiragino Mincho ProN");
pref("font.name.sans-serif.ja", "Hiragino Kaku Gothic ProN");
pref("font.name.monospace.ja", "Osaka-Mono");
pref("font.name-list.serif.ja", "Hiragino Mincho ProN,Hiragino Mincho Pro");
pref("font.name-list.sans-serif.ja", "Hiragino Kaku Gothic ProN,Hiragino Kaku Gothic Pro");
pref("font.name-list.monospace.ja", "Osaka-Mono");

pref("font.name.serif.ko", "AppleMyungjo");
pref("font.name.sans-serif.ko", "Apple SD Gothic Neo");
pref("font.name.monospace.ko", "Apple SD Gothic Neo");
pref("font.name-list.serif.ko", "AppleMyungjo");
pref("font.name-list.sans-serif.ko", "Apple SD Gothic Neo,AppleGothic");
pref("font.name-list.monospace.ko", "Apple SD Gothic Neo,AppleGothic");

pref("font.name.serif.th", "Thonburi");
pref("font.name.sans-serif.th", "Thonburi");
pref("font.name.monospace.th", "Ayuthaya");
pref("font.name-list.serif.th", "Thonburi");
pref("font.name-list.sans-serif.th", "Thonburi");
pref("font.name-list.monospace.th", "Ayuthaya");

pref("font.name.serif.x-armn", "Mshtakan");
pref("font.name.sans-serif.x-armn", "Mshtakan");
pref("font.name.monospace.x-armn", "Mshtakan");
pref("font.name-list.serif.x-armn", "Mshtakan");
pref("font.name-list.sans-serif.x-armn", "Mshtakan");
pref("font.name-list.monospace.x-armn", "Mshtakan");


pref("font.name.serif.x-beng", "Bangla MN");
pref("font.name.sans-serif.x-beng", "Bangla Sangam MN");
pref("font.name.monospace.x-beng", "Bangla Sangam MN");
pref("font.name-list.serif.x-beng", "Bangla MN");
pref("font.name-list.sans-serif.x-beng", "Bangla Sangam MN");
pref("font.name-list.monospace.x-beng", "Bangla Sangam MN");

pref("font.name.serif.x-cans", "Euphemia UCAS");
pref("font.name.sans-serif.x-cans", "Euphemia UCAS");
pref("font.name.monospace.x-cans", "Euphemia UCAS");
pref("font.name-list.serif.x-cans", "Euphemia UCAS");
pref("font.name-list.sans-serif.x-cans", "Euphemia UCAS");
pref("font.name-list.monospace.x-cans", "Euphemia UCAS");

pref("font.name.serif.x-cyrillic", "Times");
pref("font.name.sans-serif.x-cyrillic", "Helvetica");
pref("font.name.monospace.x-cyrillic", "Monaco");
pref("font.name.cursive.x-cyrillic", "Geneva");
pref("font.name.fantasy.x-cyrillic", "Charcoal CY");
pref("font.name-list.serif.x-cyrillic", "Times,Times New Roman");
pref("font.name-list.sans-serif.x-cyrillic", "Helvetica,Arial");
pref("font.name-list.monospace.x-cyrillic", "Monaco,Courier New");
pref("font.name-list.cursive.x-cyrillic", "Geneva");
pref("font.name-list.fantasy.x-cyrillic", "Charcoal CY");

pref("font.name.serif.x-devanagari", "Devanagari MT");
pref("font.name.sans-serif.x-devanagari", "Devanagari Sangam MN");
pref("font.name.monospace.x-devanagari", "Devanagari Sangam MN");
pref("font.name-list.serif.x-devanagari", "Devanagari MT");
pref("font.name-list.sans-serif.x-devanagari", "Devanagari Sangam MN,Devanagari MT");
pref("font.name-list.monospace.x-devanagari", "Devanagari Sangam MN,Devanagari MT");


pref("font.name.serif.x-ethi", "Kefa");
pref("font.name.sans-serif.x-ethi", "Kefa");
pref("font.name.monospace.x-ethi", "Kefa");
pref("font.name-list.serif.x-ethi", "Kefa,Abyssinica SIL");
pref("font.name-list.sans-serif.x-ethi", "Kefa,Abyssinica SIL");
pref("font.name-list.monospace.x-ethi", "Kefa,Abyssinica SIL");





pref("font.name.serif.x-geor", "TITUS Cyberbit Basic");
pref("font.name.sans-serif.x-geor", "Zuzumbo");
pref("font.name.monospace.x-geor", "Zuzumbo");
pref("font.name-list.serif.x-geor", "TITUS Cyberbit Basic");
pref("font.name-list.sans-serif.x-geor", "Zuzumbo");
pref("font.name-list.monospace.x-geor", "Zuzumbo");

pref("font.name.serif.x-gujr", "Gujarati MT");
pref("font.name.sans-serif.x-gujr", "Gujarati Sangam MN");
pref("font.name.monospace.x-gujr", "Gujarati Sangam MN");
pref("font.name-list.serif.x-gujr", "Gujarati MT");
pref("font.name-list.sans-serif.x-gujr", "Gujarati Sangam MN,Gujarati MT");
pref("font.name-list.monospace.x-gujr", "Gujarati Sangam MN,Gujarati MT");

pref("font.name.serif.x-guru", "Gurmukhi MT");
pref("font.name.sans-serif.x-guru", "Gurmukhi MT");
pref("font.name.monospace.x-guru", "Gurmukhi MT");
pref("font.name-list.serif.x-guru", "Gurmukhi MT");
pref("font.name-list.sans-serif.x-guru", "Gurmukhi MT");
pref("font.name-list.monospace.x-guru", "Gurmukhi MT");

pref("font.name.serif.x-khmr", "Khmer MN");
pref("font.name.sans-serif.x-khmr", "Khmer Sangam MN");
pref("font.name.monospace.x-khmr", "Khmer Sangam MN");
pref("font.name-list.serif.x-khmr", "Khmer MN");
pref("font.name-list.sans-serif.x-khmr", "Khmer Sangam MN");
pref("font.name-list.monospace.x-khmr", "Khmer Sangam MN");

pref("font.name.serif.x-mlym", "Malayalam MN");
pref("font.name.sans-serif.x-mlym", "Malayalam Sangam MN");
pref("font.name.monospace.x-mlym", "Malayalam Sangam MN");
pref("font.name-list.serif.x-mlym", "Malayalam MN");
pref("font.name-list.sans-serif.x-mlym", "Malayalam Sangam MN");
pref("font.name-list.monospace.x-mlym", "Malayalam Sangam MN");

pref("font.name.serif.x-orya", "Oriya MN");
pref("font.name.sans-serif.x-orya", "Oriya Sangam MN");
pref("font.name.monospace.x-orya", "Oriya Sangam MN");
pref("font.name-list.serif.x-orya", "Oriya MN");
pref("font.name-list.sans-serif.x-orya", "Oriya Sangam MN");
pref("font.name-list.monospace.x-orya", "Oriya Sangam MN");


pref("font.name.serif.x-telu", "Telugu MN");
pref("font.name.sans-serif.x-telu", "Telugu Sangam MN");
pref("font.name.monospace.x-telu", "Telugu Sangam MN");
pref("font.name-list.serif.x-telu", "Telugu MN,Pothana");
pref("font.name-list.sans-serif.x-telu", "Telugu Sangam MN,Pothana");
pref("font.name-list.monospace.x-telu", "Telugu Sangam MN,Pothana");


pref("font.name.serif.x-knda", "Kannada MN");
pref("font.name.sans-serif.x-knda", "Kannada Sangam MN");
pref("font.name.monospace.x-knda", "Kannada Sangam MN");
pref("font.name-list.serif.x-knda", "Kannada MN,Kedage");
pref("font.name-list.sans-serif.x-knda", "Kannada Sangam MN,Kedage");
pref("font.name-list.monospace.x-knda", "Kannada Sangam MN,Kedage");

pref("font.name.serif.x-sinh", "Sinhala MN");
pref("font.name.sans-serif.x-sinh", "Sinhala Sangam MN");
pref("font.name.monospace.x-sinh", "Sinhala Sangam MN");
pref("font.name-list.serif.x-sinh", "Sinhala MN");
pref("font.name-list.sans-serif.x-sinh", "Sinhala Sangam MN");
pref("font.name-list.monospace.x-sinh", "Sinhala Sangam MN");

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
pref("font.name-list.serif.x-western", "Times,Times New Roman");
pref("font.name-list.sans-serif.x-western", "Helvetica,Arial");
pref("font.name-list.monospace.x-western", "Courier,Courier New");
pref("font.name-list.cursive.x-western", "Apple Chancery");
pref("font.name-list.fantasy.x-western", "Papyrus");

pref("font.name.serif.zh-CN", "Times");
pref("font.name.sans-serif.zh-CN", "Helvetica");
pref("font.name.monospace.zh-CN", "Courier");
pref("font.name-list.serif.zh-CN", "Times,STSong,Heiti SC");
pref("font.name-list.sans-serif.zh-CN", "Helvetica,STHeiti,Heiti SC");
pref("font.name-list.monospace.zh-CN", "Courier,STHeiti,Heiti SC");

pref("font.name.serif.zh-TW", "Times");
pref("font.name.sans-serif.zh-TW", "Helvetica");
pref("font.name.monospace.zh-TW", "Courier");
pref("font.name-list.serif.zh-TW", "Times,LiSong Pro,Heiti TC");
pref("font.name-list.sans-serif.zh-TW", "Helvetica,Heiti TC,LiHei Pro");
pref("font.name-list.monospace.zh-TW", "Courier,Heiti TC,LiHei Pro");

pref("font.name.serif.zh-HK", "Times");
pref("font.name.sans-serif.zh-HK", "Helvetica");
pref("font.name.monospace.zh-HK", "Courier");
pref("font.name-list.serif.zh-HK", "Times,LiSong Pro,Heiti TC");
pref("font.name-list.sans-serif.zh-HK", "Helvetica,Heiti TC,LiHei Pro");
pref("font.name-list.monospace.zh-HK", "Courier,Heiti TC,LiHei Pro");


pref("font.minimum-size.th", 10);
pref("font.size.variable.zh-CN", 15);
pref("font.size.variable.zh-HK", 15);
pref("font.size.variable.zh-TW", 15);


pref("font.mathfont-family", "Latin Modern Math, XITS Math, STIX Math, Cambria Math, Asana Math, TeX Gyre Bonum Math, TeX Gyre Pagella Math, TeX Gyre Termes Math, Neo Euler, Lucida Bright Math, MathJax_Main, STIXNonUnicode, STIXSizeOneSym, STIXGeneral, Symbol, DejaVu Sans");



pref("font.single-face-list", "Osaka-Mono");



pref("font.preload-names-list", "Hiragino Kaku Gothic ProN,Hiragino Mincho ProN,STSong");





pref("font.weight-override.AppleSDGothicNeo-Thin", 100); 
pref("font.weight-override.AppleSDGothicNeo-UltraLight", 200);
pref("font.weight-override.AppleSDGothicNeo-Light", 300);
pref("font.weight-override.AppleSDGothicNeo-Heavy", 900); 

pref("font.weight-override.Avenir-Book", 300); 
pref("font.weight-override.Avenir-BookOblique", 300);
pref("font.weight-override.Avenir-MediumOblique", 500); 
pref("font.weight-override.Avenir-Black", 900); 
pref("font.weight-override.Avenir-BlackOblique", 900);

pref("font.weight-override.AvenirNext-MediumItalic", 500); 
pref("font.weight-override.AvenirNextCondensed-MediumItalic", 500);

pref("font.weight-override.HelveticaNeue-Light", 300); 
pref("font.weight-override.HelveticaNeue-LightItalic", 300);
pref("font.weight-override.HelveticaNeue-MediumItalic", 500); 



pref("ui.key.menuAccessKey", 0);
pref("ui.key.accelKey", 224);




pref("ui.key.generalAccessKey", -1);




pref("ui.key.chromeAccess", 2);
pref("ui.key.contentAccess", 6);



pref("print.print_extra_margin", 90); 


pref("ui.panel.default_level_parent", false);

pref("ui.plugin.cancel_composition_at_input_source_changed", false);

pref("mousewheel.system_scroll_override_on_root_content.enabled", false);


pref("mousewheel.enable_pixel_scrolling", true);

# XP_MACOSX
#endif

#ifdef ANDROID

pref("network.protocol-handler.warn-external.file", false);
pref("browser.drag_out_of_frame_style", 1);


pref("middlemouse.paste", true);
pref("middlemouse.contentLoadURL", true);
pref("middlemouse.openNewWindow", true);
pref("middlemouse.scrollbarPosition", true);

pref("browser.urlbar.clickSelectsAll", false);







pref("autocomplete.grab_during_popup", true);
pref("autocomplete.ungrab_during_mode_switch", true);



pref("ui.allow_platform_file_picker", true);

pref("helpers.global_mime_types_file", "/etc/mime.types");
pref("helpers.global_mailcap_file", "/etc/mailcap");
pref("helpers.private_mime_types_file", "~/.mime.types");
pref("helpers.private_mailcap_file", "~/.mailcap");
pref("print.print_command", "lpr ${MOZ_PRINTER_NAME:+-P\"$MOZ_PRINTER_NAME\"}");
pref("print.printer_list", ""); 
pref("print.print_reversed", false);
pref("print.print_color", true);
pref("print.print_landscape", false);
pref("print.print_paper_size", 0);



pref("print.print_extra_margin", 0); 


pref("layout.css.scroll-behavior.enabled", false);
pref("layout.css.scroll-behavior.property-enabled", false);


pref("layout.css.scroll-snap.enabled", false);



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
pref("print.print_command", "lpr ${MOZ_PRINTER_NAME:+-P\"$MOZ_PRINTER_NAME\"}");
pref("print.printer_list", ""); 
pref("print.print_reversed", false);
pref("print.print_color", true);
pref("print.print_landscape", false);
pref("print.print_paper_size", 0);



pref("print.print_extra_margin", 0); 



pref("font.size.fixed.ar", 12);

pref("font.name.serif.el", "serif");
pref("font.name.sans-serif.el", "sans-serif");
pref("font.name.monospace.el", "monospace");
pref("font.size.fixed.el", 12);

pref("font.name.serif.he", "serif");
pref("font.name.sans-serif.he", "sans-serif");
pref("font.name.monospace.he", "monospace");
pref("font.size.fixed.he", 12);

pref("font.name.serif.ja", "serif");
pref("font.name.sans-serif.ja", "sans-serif");
pref("font.name.monospace.ja", "monospace");

pref("font.name.serif.ko", "serif");
pref("font.name.sans-serif.ko", "sans-serif");
pref("font.name.monospace.ko", "monospace");

pref("font.name.serif.th", "serif");
pref("font.name.sans-serif.th", "sans-serif");
pref("font.minimum-size.th", 13);
pref("font.name.monospace.th", "monospace");

pref("font.name.serif.x-cyrillic", "serif");
pref("font.name.sans-serif.x-cyrillic", "sans-serif");
pref("font.name.monospace.x-cyrillic", "monospace");
pref("font.size.fixed.x-cyrillic", 12);

pref("font.name.serif.x-unicode", "serif");
pref("font.name.sans-serif.x-unicode", "sans-serif");
pref("font.name.monospace.x-unicode", "monospace");
pref("font.size.fixed.x-unicode", 12);

pref("font.name.serif.x-western", "serif");
pref("font.name.sans-serif.x-western", "sans-serif");
pref("font.name.monospace.x-western", "monospace");
pref("font.size.fixed.x-western", 12);

pref("font.name.serif.zh-CN", "serif");
pref("font.name.sans-serif.zh-CN", "sans-serif");
pref("font.name.monospace.zh-CN", "monospace");



pref("font.name.serif.zh-HK", "serif");
pref("font.name.sans-serif.zh-HK", "sans-serif");
pref("font.name.monospace.zh-HK", "monospace");





pref("print.postscript.paper_size",    "letter");
pref("print.postscript.orientation",   "portrait");
pref("print.postscript.print_command", "lpr ${MOZ_PRINTER_NAME:+-P\"$MOZ_PRINTER_NAME\"}");










pref("ui.panel.default_level_parent", true);

pref("mousewheel.system_scroll_override_on_root_content.enabled", false);

#if MOZ_WIDGET_GTK == 2
pref("intl.ime.use_simple_context_on_password_field", true);
#else
pref("intl.ime.use_simple_context_on_password_field", false);
#endif

# XP_UNIX
#endif
#endif
#endif

#if defined(ANDROID) || defined(MOZ_B2G)

pref("font.size.fixed.ar", 12);

pref("font.default.el", "sans-serif");
pref("font.size.fixed.el", 12);

pref("font.size.fixed.he", 12);

pref("font.default.x-cyrillic", "sans-serif");
pref("font.size.fixed.x-cyrillic", 12);

pref("font.default.x-unicode", "sans-serif");
pref("font.size.fixed.x-unicode", 12);

pref("font.default.x-western", "sans-serif");
pref("font.size.fixed.x-western", 12);

# ANDROID || MOZ_B2G
#endif

#if defined(MOZ_B2G)






pref("font.name.serif.el", "Droid Serif"); 
pref("font.name.sans-serif.el", "Fira Sans");
pref("font.name.monospace.el", "Fira Mono");

pref("font.name.serif.he", "Charis SIL Compact");
pref("font.name.sans-serif.he", "Fira Sans");
pref("font.name.monospace.he", "Fira Mono");
pref("font.name-list.sans-serif.he", "Droid Sans Hebrew, Fira Sans");

pref("font.name.serif.ja", "Charis SIL Compact");
pref("font.name.sans-serif.ja", "Fira Sans");
pref("font.name.monospace.ja", "MotoyaLMaru");
pref("font.name-list.sans-serif.ja", "Fira Sans, MotoyaLMaru, MotoyaLCedar, Droid Sans Japanese");
pref("font.name-list.monospace.ja", "MotoyaLMaru, MotoyaLCedar, Fira Mono");

pref("font.name.serif.ko", "Charis SIL Compact");
pref("font.name.sans-serif.ko", "Fira Sans");
pref("font.name.monospace.ko", "Fira Mono");

pref("font.name.serif.th", "Charis SIL Compact");
pref("font.name.sans-serif.th", "Fira Sans");
pref("font.name.monospace.th", "Fira Mono");
pref("font.name-list.sans-serif.th", "Fira Sans, Noto Sans Thai, Droid Sans Thai");

pref("font.name.serif.x-cyrillic", "Charis SIL Compact");
pref("font.name.sans-serif.x-cyrillic", "Fira Sans");
pref("font.name.monospace.x-cyrillic", "Fira Mono");

pref("font.name.serif.x-unicode", "Charis SIL Compact");
pref("font.name.sans-serif.x-unicode", "Fira Sans");
pref("font.name.monospace.x-unicode", "Fira Mono");

pref("font.name.serif.x-western", "Charis SIL Compact");
pref("font.name.sans-serif.x-western", "Fira Sans");
pref("font.name.monospace.x-western", "Fira Mono");

pref("font.name.serif.zh-CN", "Charis SIL Compact");
pref("font.name.sans-serif.zh-CN", "Fira Sans");
pref("font.name.monospace.zh-CN", "Fira Mono");
pref("font.name-list.sans-serif.zh-CN", "Fira Sans,Droid Sans Fallback");

pref("font.name.serif.zh-HK", "Charis SIL Compact");
pref("font.name.sans-serif.zh-HK", "Fira Sans");
pref("font.name.monospace.zh-HK", "Fira Mono");
pref("font.name-list.sans-serif.zh-HK", "Fira Sans,Droid Sans Fallback");

pref("font.name.serif.zh-TW", "Charis SIL Compact");
pref("font.name.sans-serif.zh-TW", "Fira Sans");
pref("font.name.monospace.zh-TW", "Fira Mono");
pref("font.name-list.sans-serif.zh-TW", "Fira Sans,Droid Sans Fallback");

#elif defined(ANDROID)




pref("font.name.serif.el", "Droid Serif"); 
pref("font.name.sans-serif.el", "Clear Sans");
pref("font.name.monospace.el", "Droid Sans Mono");
pref("font.name-list.sans-serif.el", "Clear Sans, Roboto, Droid Sans");

pref("font.name.serif.he", "Droid Serif");
pref("font.name.sans-serif.he", "Clear Sans");
pref("font.name.monospace.he", "Droid Sans Mono");
pref("font.name-list.sans-serif.he", "Droid Sans Hebrew, Clear Sans, Droid Sans");

pref("font.name.serif.ja", "Charis SIL Compact");
pref("font.name.sans-serif.ja", "Clear Sans");
pref("font.name.monospace.ja", "MotoyaLMaru");
pref("font.name-list.serif.ja", "Droid Serif");
pref("font.name-list.sans-serif.ja", "Clear Sans, Roboto, Droid Sans, MotoyaLMaru, MotoyaLCedar, Noto Sans JP, Droid Sans Japanese");
pref("font.name-list.monospace.ja", "MotoyaLMaru, MotoyaLCedar, Droid Sans Mono");

pref("font.name.serif.ko", "Charis SIL Compact");
pref("font.name.sans-serif.ko", "Clear Sans");
pref("font.name.monospace.ko", "Droid Sans Mono");
pref("font.name-list.serif.ko", "Droid Serif, HYSerif");
pref("font.name-list.sans-serif.ko", "SmartGothic, NanumGothic, Noto Sans KR, DroidSansFallback, Droid Sans Fallback");

pref("font.name.serif.th", "Charis SIL Compact");
pref("font.name.sans-serif.th", "Clear Sans");
pref("font.name.monospace.th", "Droid Sans Mono");
pref("font.name-list.serif.th", "Droid Serif");
pref("font.name-list.sans-serif.th", "Droid Sans Thai, Clear Sans, Droid Sans");

pref("font.name.serif.x-cyrillic", "Charis SIL Compact");
pref("font.name.sans-serif.x-cyrillic", "Clear Sans");
pref("font.name.monospace.x-cyrillic", "Droid Sans Mono");
pref("font.name-list.serif.x-cyrillic", "Droid Serif");
pref("font.name-list.sans-serif.x-cyrillic", "Clear Sans, Roboto, Droid Sans");

pref("font.name.serif.x-unicode", "Charis SIL Compact");
pref("font.name.sans-serif.x-unicode", "Clear Sans");
pref("font.name.monospace.x-unicode", "Droid Sans Mono");
pref("font.name-list.serif.x-unicode", "Droid Serif");
pref("font.name-list.sans-serif.x-unicode", "Clear Sans, Roboto, Droid Sans");

pref("font.name.serif.x-western", "Charis SIL Compact");
pref("font.name.sans-serif.x-western", "Clear Sans");
pref("font.name.monospace.x-western", "Droid Sans Mono");
pref("font.name-list.serif.x-western", "Droid Serif");
pref("font.name-list.sans-serif.x-western", "Clear Sans, Roboto, Droid Sans");

pref("font.name.serif.zh-CN", "Charis SIL Compact");
pref("font.name.sans-serif.zh-CN", "Clear Sans");
pref("font.name.monospace.zh-CN", "Droid Sans Mono");
pref("font.name-list.serif.zh-CN", "Droid Serif, Droid Sans Fallback");
pref("font.name-list.sans-serif.zh-CN", "Roboto, Droid Sans, Noto Sans SC, Droid Sans Fallback");
pref("font.name-list.monospace.zh-CN", "Droid Sans Fallback");

pref("font.name.serif.zh-HK", "Charis SIL Compact");
pref("font.name.sans-serif.zh-HK", "Clear Sans");
pref("font.name.monospace.zh-HK", "Droid Sans Mono");
pref("font.name-list.serif.zh-HK", "Droid Serif, Droid Sans Fallback");
pref("font.name-list.sans-serif.zh-HK", "Roboto, Droid Sans, Noto Sans TC, Noto Sans SC, Droid Sans Fallback");
pref("font.name-list.monospace.zh-HK", "Droid Sans Fallback");

pref("font.name.serif.zh-TW", "Charis SIL Compact");
pref("font.name.sans-serif.zh-TW", "Clear Sans");
pref("font.name.monospace.zh-TW", "Droid Sans Mono");
pref("font.name-list.serif.zh-TW", "Droid Serif, Droid Sans Fallback");
pref("font.name-list.sans-serif.zh-TW", "Roboto, Droid Sans, Noto Sans TC, Noto Sans SC, Droid Sans Fallback");
pref("font.name-list.monospace.zh-TW", "Droid Sans Fallback");

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
pref("signon.autofillForms",                true);
pref("signon.autologin.proxy",              false);
pref("signon.storeWhenAutocompleteOff",     true);
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












pref("browser.zoom.reflowOnZoom", false);






pref("browser.zoom.reflowZoom.reflowTimeout", 500);










pref("browser.zoom.reflowZoom.reflowTextOnPageLoad", true);






pref("image.cache.size", 5242880);



pref("image.cache.timeweight", 500);



pref("image.decode-only-on-draw.enabled", true);



pref("image.decode-immediately.enabled", false);


pref("image.downscale-during-decode.enabled", false);


pref("image.http.accept", "image/png,image/*;q=0.8,*/*;q=0.5");

pref("image.high_quality_downscaling.enabled", true);



pref("image.high_quality_downscaling.min_factor", 1000);



pref("image.high_quality_upscaling.max_size", 20971520);


pref("image.single-color-optimization.enabled", true);







pref("image.mem.discardable", true);


pref("image.mem.allow_locking_in_content_processes", true);


pref("image.mem.decode_bytes_at_a_time", 16384);



pref("image.mem.surfacecache.min_expiration_ms", 60000); 


pref("image.mem.surfacecache.max_size_kb", 1048576); 





pref("image.mem.surfacecache.size_factor", 4);







pref("image.mem.surfacecache.discard_factor", 1);



pref("image.multithreaded_decoding.limit", -1);



pref("canvas.image.cache.limit", 0);


pref("image.onload.decode.limit", 0);


#ifdef ANDROID

pref("gl.msaa-level", 0);
#else
pref("gl.msaa-level", 2);
#endif
pref("webgl.force-enabled", false);
pref("webgl.disabled", false);
pref("webgl.disable-angle", false);
pref("webgl.min_capability_mode", false);
pref("webgl.disable-extensions", false);
pref("webgl.msaa-force", false);
pref("webgl.prefer-16bpp", false);
pref("webgl.default-no-alpha", false);
pref("webgl.force-layers-readback", false);
pref("webgl.lose-context-on-memory-pressure", false);
pref("webgl.can-lose-context-in-foreground", true);
pref("webgl.restore-context-when-visible", true);
pref("webgl.max-warnings-per-context", 32);
pref("webgl.enable-draft-extensions", false);
pref("webgl.enable-privileged-extensions", false);
pref("webgl.bypass-shader-validation", false);
pref("webgl.enable-prototype-webgl2", false);
pref("gl.require-hardware", false);

#ifdef XP_WIN
pref("webgl.angle.try-d3d11", true);
pref("webgl.angle.force-d3d11", false);
#endif

#ifdef MOZ_WIDGET_GONK
pref("gfx.gralloc.fence-with-readpixels", false);
#endif


pref("stagefright.force-enabled", false);
pref("stagefright.disabled", false);

#ifdef XP_WIN

pref("network.tcp.sendbuffer", 131072);
#endif

pref("network.tcp.keepalive.enabled", true);



pref("network.tcp.keepalive.idle_time", 600); 


#if defined(XP_UNIX) && !defined(XP_MACOSX) || defined(XP_WIN)
pref("network.tcp.keepalive.retry_interval", 1); 
#endif


#ifdef XP_UNIX && !defined(XP_MACOSX)
pref("network.tcp.keepalive.probe_count", 4);
#endif


pref("layers.acceleration.disabled", false);


pref("layers.bench.enabled", false);


#ifdef ANDROID





pref("layers.acceleration.force-enabled", true);
#else
pref("layers.acceleration.force-enabled", false);
#endif

pref("layers.acceleration.draw-fps", false);

pref("layers.dump", false);
#ifdef MOZ_DUMP_PAINTING

pref("layers.dump-texture", false);
pref("layers.dump-decision", false);
pref("layers.dump-client-layers", false);
pref("layers.dump-host-layers", false);
#endif
pref("layers.draw-borders", false);
pref("layers.draw-tile-borders", false);
pref("layers.draw-bigimage-borders", false);
pref("layers.frame-counter", false);
pref("layers.enable-tiles", false);
pref("layers.tiled-drawtarget.enabled", false);
pref("layers.low-precision-buffer", false);
pref("layers.progressive-paint", false);
pref("layers.tile-width", 256);
pref("layers.tile-height", 256);

pref("layers.max-active", -1);





pref("layers.tiles.adjust", true);


pref("layers.offmainthreadcomposition.enabled", true);




pref("layers.offmainthreadcomposition.frame-rate", -1);



pref("layers.async-video.enabled", true);
pref("layers.async-video-oop.enabled",true);

#ifdef XP_MACOSX
pref("layers.enable-tiles", true);
pref("layers.tiled-drawtarget.enabled", true);
#endif



pref("layers.offmainthreadcomposition.testing.enabled", false);


pref("layers.offmainthreadcomposition.force-basic", false);


#ifdef RELEASE_BUILD
pref("layers.offmainthreadcomposition.async-animations", false);
#else
#if defined(MOZ_X11)
pref("layers.offmainthreadcomposition.async-animations", false);
#else
pref("layers.offmainthreadcomposition.async-animations", true);
#endif
#endif


pref("layers.offmainthreadcomposition.log-animations", false);

pref("layers.bufferrotation.enabled", true);

pref("layers.componentalpha.enabled", true);

#ifdef ANDROID
pref("gfx.apitrace.enabled",false);
#endif

#ifdef MOZ_X11
#ifdef MOZ_WIDGET_GTK
pref("gfx.xrender.enabled",true);
#endif
#endif

#ifdef XP_WIN

pref("gfx.direct2d.disabled", false);
pref("gfx.direct2d.use1_1", true);



pref("gfx.direct2d.force-enabled", false);

pref("layers.prefer-opengl", false);
pref("layers.prefer-d3d9", false);
pref("layers.d3d11.force-warp", false);
pref("layers.d3d11.disable-warp", false);
#endif


pref("layers.force-active", false);



pref("layers.gralloc.disable", false);


pref("geo.enabled", true);


pref("device.sensors.enabled", true);


pref("device.storage.enabled", false);


pref("html5.offmainthread", true);



pref("html5.flushtimer.initialdelay", 120);


pref("html5.flushtimer.subsequentdelay", 120);


pref("browser.history.allowPushState", true);
pref("browser.history.allowReplaceState", true);
pref("browser.history.allowPopState", true);
pref("browser.history.maxStateObjectSize", 655360);


pref("xpinstall.whitelist.required", true);
pref("extensions.alwaysUnpack", false);
pref("extensions.minCompatiblePlatformVersion", "2.0");

pref("network.buffer.cache.count", 24);
pref("network.buffer.cache.size",  32768);


pref("notification.feature.enabled", false);


pref("dom.webnotifications.enabled", true);


pref("alerts.disableSlidingEffect", false);


pref("full-screen-api.enabled", false);
pref("full-screen-api.allow-trusted-requests-only", true);
pref("full-screen-api.content-only", false);
pref("full-screen-api.pointer-lock.enabled", true);


pref("dom.idle-observers-api.enabled", true);



pref("dom.event.handling-user-input-time-limit", 1000);


pref("layout.animated-image-layers.enabled", false);

pref("dom.vibrator.enabled", true);
pref("dom.vibrator.max_vibrate_ms", 10000);
pref("dom.vibrator.max_vibrate_list_len", 128);


pref("dom.battery.enabled", true);


pref("dom.image.srcset.enabled", true);


pref("dom.image.picture.enabled", true);


pref("dom.sms.enabled", false);


pref("dom.sms.strict7BitEncoding", false);
pref("dom.sms.requestStatusReport", true);


pref("dom.sms.defaultServiceId", 0);





pref("dom.sms.maxReadAheadEntries", 0);


pref("dom.mozContacts.enabled", false);


pref("dom.mozAlarms.enabled", false);


pref("dom.push.enabled", false);

pref("dom.push.debug", false);
pref("dom.push.serverURL", "wss://push.services.mozilla.com/");
pref("dom.push.userAgentID", "");



pref("dom.push.connection.enabled", true);



pref("dom.push.retryBaseInterval", 5000);




pref("dom.push.pingInterval", 1800000); 


pref("dom.push.requestTimeout", 10000);
pref("dom.push.pingInterval.default", 180000);
pref("dom.push.pingInterval.mobile", 180000); 
pref("dom.push.pingInterval.wifi", 180000);  


pref("dom.push.adaptive.enabled", false);
pref("dom.push.adaptive.lastGoodPingInterval", 180000);
pref("dom.push.adaptive.lastGoodPingInterval.mobile", 180000);
pref("dom.push.adaptive.lastGoodPingInterval.wifi", 180000);

pref("dom.push.adaptive.gap", 60000); 

pref("dom.push.adaptive.upperLimit", 1740000); 


pref("dom.push.udp.wakeupEnabled", false);


pref("dom.mozNetworkStats.enabled", false);


pref("dom.mozSettings.enabled", false);
pref("dom.mozPermissionSettings.enabled", false);



#ifdef XP_WIN
pref("dom.w3c_touch_events.enabled", 2);
#endif


pref("dom.w3c_pointer_events.enabled", false);


pref("dom.imagecapture.enabled", false);


pref("layout.css.touch_action.enabled", false);




pref("layout.css.expensive-style-struct-assertions.enabled", false);


pref("browser.dom.window.dump.enabled", false);

#if defined(MOZ_WIDGET_GONK) || defined(MOZ_WIDGET_ANDROID)

pref("dom.netinfo.enabled", true);
#else
pref("dom.netinfo.enabled", false);
#endif

#ifdef XP_WIN


pref("memory.low_virtual_memory_threshold_mb", 128);



pref("memory.low_commit_space_threshold_mb", 128);



pref("memory.low_physical_memory_threshold_mb", 0);




pref("memory.low_memory_notification_interval_ms", 10000);
#endif




pref("memory.ghost_window_timeout_seconds", 60);


pref("memory.free_dirty_pages", false);


#ifdef XP_LINUX
pref("memory.system_memory_reporter", false);
#endif


pref("memory.dump_reports_on_oom", false);


pref("memory.blob_report.stack_frames", 0);



pref("social.whitelist", "https://mozsocial.cliqz.com");


pref("social.directories", "https://activations.cdn.mozilla.net");



pref("social.remote-install.enabled", true);
pref("social.toast-notifications.enabled", true);



pref("dom.idle-observers-api.fuzz_time.disabled", true);


pref("dom.mozApps.maxLocalId", 1000);


pref("dom.apps.reset-permissions", false);











pref("dom.mozApps.signed_apps_installable_from", "https://marketplace.firefox.com");





pref("dom.mozApps.debug", false);





pref("network.activity.blipIntervalMilliseconds", 0);





pref("jsloader.reuseGlobal", false);



pref("dom.browserElement.maxScreenshotDelayMS", 2000);


pref("dom.placeholder.show_on_focus", true);

pref("dom.vr.enabled", false);

pref("dom.vr.add-test-devices", 1);


pref("wap.UAProf.url", "");
pref("wap.UAProf.tagname", "x-wap-profile");




pref("dom.mms.version", 19);

pref("dom.mms.requestStatusReport", true);






pref("dom.mms.retrieval_mode", "manual");

pref("dom.mms.sendRetryCount", 3);
pref("dom.mms.sendRetryInterval", "10000,60000,180000");

pref("dom.mms.retrievalRetryCount", 4);
pref("dom.mms.retrievalRetryIntervals", "60000,300000,600000,1800000");


pref("dom.mms.defaultServiceId", 0);

pref("mms.debugging.enabled", false);


pref("dom.mms.requestReadReport", true);


pref("ril.numRadioInterfaces", 0);





pref("ui.touch_activation.delay_ms", 100);




pref("ui.touch_activation.duration_ms", 10);



pref("memory_info_dumper.watch_fifo.enabled", false);

#ifdef MOZ_CAPTIVEDETECT
pref("captivedetect.maxWaitingTime", 5000);
pref("captivedetect.pollingTime", 3000);
pref("captivedetect.maxRetryCount", 5);
#endif

#ifdef RELEASE_BUILD
pref("dom.forms.inputmode", false);
#else
pref("dom.forms.inputmode", true);
#endif


pref("dom.mozInputMethod.enabled", false);


pref("dom.datastore.enabled", false);


#ifdef MOZ_B2G_RIL
pref("dom.telephony.enabled", true);
#else
pref("dom.telephony.enabled", false);
#endif


pref("dom.telephony.defaultServiceId", 0);


#ifdef MOZ_B2G_RIL
pref("dom.cellbroadcast.enabled", true);
#else
pref("dom.cellbroadcast.enabled", false);
#endif


#ifdef MOZ_B2G_RIL
pref("dom.icc.enabled", true);
#else
pref("dom.icc.enabled", false);
#endif


#ifdef MOZ_B2G_RIL
pref("dom.mobileconnection.enabled", true);
#else
pref("dom.mobileconnection.enabled", false);
#endif


#ifdef MOZ_B2G_RIL
pref("dom.voicemail.enabled", true);
#else
pref("dom.voicemail.enabled", false);
#endif


pref("dom.voicemail.defaultServiceId", 0);


pref("dom.broadcastChannel.enabled", true);


pref("dom.inter-app-communication-api.enabled", false);


pref("dom.mapped_arraybuffer.enabled", false);


pref("urlclassifier.malwareTable", "goog-malware-shavar,test-malware-simple");
pref("urlclassifier.phishTable", "goog-phish-shavar,test-phish-simple");
pref("urlclassifier.downloadBlockTable", "");
pref("urlclassifier.downloadAllowTable", "");
pref("urlclassifier.disallow_completions", "test-malware-simple,test-phish-simple,goog-downloadwhite-digest256,mozpub-track-digest256");



pref("urlclassifier.trackingTable", "mozpub-track-digest256");
pref("browser.trackingprotection.updateURL", "https://tracking.services.mozilla.com/downloads?client=SAFEBROWSING_ID&appver=%VERSION%&pver=2.2");
pref("browser.trackingprotection.gethashURL", "https://tracking.services.mozilla.com/gethash?client=SAFEBROWSING_ID&appver=%VERSION%&pver=2.2");


pref("snav.enabled", false);


pref("touchcaret.enabled", false);



pref("touchcaret.inflatesize.threshold", 40);




pref("touchcaret.expiration.time", 3000);


pref("selectioncaret.enabled", false);



pref("selectioncaret.inflatesize.threshold", 40);


pref("selectioncaret.detects.longtap", true);


pref("dom.wakelock.enabled", false);


pref("identity.fxaccounts.auth.uri", "https://api.accounts.firefox.com/v1");


pref("image.mozsamplesize.enabled", false);



#ifndef MOZ_WIDGET_GONK
pref("beacon.enabled", true);
#endif


pref("camera.control.face_detection.enabled", true);



#ifdef RELEASE_BUILD
pref("dom.caches.enabled", false);
#else
pref("dom.caches.enabled", true);
#endif 

#ifdef MOZ_WIDGET_GONK





pref("camera.control.low_memory_thresholdMB", 404);
#endif


pref("dom.udpsocket.enabled", false);


pref("dom.beforeAfterKeyboardEvent.enabled", false);


pref("dom.presentation.enabled", false);
pref("dom.presentation.tcp_server.debug", false);


#ifdef XP_MACOSX
pref("intl.collation.mac.use_icu", true);
#endif


pref("dom.meta-viewport.enabled", false);


pref("dom.mozSettings.SettingsDB.debug.enabled", false);
pref("dom.mozSettings.SettingsManager.debug.enabled", false);
pref("dom.mozSettings.SettingsRequestManager.debug.enabled", false);
pref("dom.mozSettings.SettingsService.debug.enabled", false);


pref("dom.mozSettings.SettingsDB.verbose.enabled", false);
pref("dom.mozSettings.SettingsManager.verbose.enabled", false);
pref("dom.mozSettings.SettingsRequestManager.verbose.enabled", false);
pref("dom.mozSettings.SettingsService.verbose.enabled", false);




pref("dom.mozSettings.allowForceReadOnly", false);


#ifdef NIGHTLY_BUILD
pref("browser.addon-watch.interval", 15000);
#else
pref("browser.addon-watch.interval", -1);
#endif
pref("browser.addon-watch.ignore", "[\"mochikit@mozilla.org\",\"special-powers@mozilla.org\",\"fxdevtools-adapters@mozilla.org\",\"fx-devtools\"]");

pref("browser.addon-watch.percentage-limit", 5);


pref("dom.requestSync.enabled", false);


pref("browser.search.log", false);
pref("browser.search.update", true);
pref("browser.search.update.log", false);
pref("browser.search.update.interval", 21600);
pref("browser.search.suggest.enabled", true);
pref("browser.search.geoSpecificDefaults", false);
pref("browser.search.geoip.url", "https://location.services.mozilla.com/v1/country?key=%MOZILLA_API_KEY%");


pref("browser.search.geoip.timeout", 2000);

#ifdef MOZ_OFFICIAL_BRANDING

pref("browser.search.official", true);
#endif

#ifndef MOZ_WIDGET_GONK






pref("media.gmp-manager.url", "https://aus4.mozilla.org/update/3/GMP/%VERSION%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/%OS_VERSION%/%DISTRIBUTION%/%DISTRIBUTION_VERSION%/update.xml");





pref("media.gmp-manager.cert.requireBuiltIn", true);

















pref("media.gmp-manager.cert.checkAttributes", true);
pref("media.gmp-manager.certs.1.issuerName", "CN=DigiCert Secure Server CA,O=DigiCert Inc,C=US");
pref("media.gmp-manager.certs.1.commonName", "aus4.mozilla.org");
pref("media.gmp-manager.certs.2.issuerName", "CN=Thawte SSL CA,O=\"Thawte, Inc.\",C=US");
pref("media.gmp-manager.certs.2.commonName", "aus4.mozilla.org");
#endif



pref("reader.parse-on-load.enabled", true);



pref("reader.parse-on-load.force-enabled", false);


pref("reader.font_size", 5);




pref("reader.color_scheme", "light");


pref("reader.color_scheme.values", "[\"light\",\"dark\",\"sepia\"]");


pref("reader.font_type", "sans-serif");



pref("reader.has_used_toolbar", false);


pref("reader.toolbar.vertical", true);

#if defined(XP_LINUX) && defined(MOZ_GMP_SANDBOX)



pref("media.gmp.insecure.allow", false);
#endif




#if defined(XP_MACOSX) || defined(XP_WIN) || defined(XP_LINUX)
pref("gfx.vsync.hw-vsync.enabled", true);
pref("gfx.vsync.compositor", true);
pref("gfx.vsync.refreshdriver", true);
#endif


#ifdef MOZ_SECUREELEMENT
pref("dom.secureelement.enabled", false);
#endif
