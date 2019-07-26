

user_pref("browser.console.showInPanel", true);
user_pref("browser.dom.window.dump.enabled", true);
user_pref("browser.firstrun.show.localepicker", false);
user_pref("browser.firstrun.show.uidiscovery", false);
user_pref("browser.startup.page", 0); 
user_pref("browser.ui.layout.tablet", 0); 
user_pref("dom.allow_scripts_to_close_windows", true);
user_pref("dom.disable_open_during_load", false);
user_pref("dom.experimental_forms", true); 
user_pref("dom.forms.number", true); 
user_pref("dom.forms.color", true); 
user_pref("dom.max_script_run_time", 0); 
user_pref("hangmonitor.timeout", 0); 
user_pref("dom.max_chrome_script_run_time", 0);
user_pref("dom.popup_maximum", -1);
user_pref("dom.send_after_paint_to_content", true);
user_pref("dom.successive_dialog_time_limit", 0);
user_pref("signed.applets.codebase_principal_support", true);
user_pref("browser.shell.checkDefaultBrowser", false);
user_pref("shell.checkDefaultClient", false);
user_pref("browser.warnOnQuit", false);
user_pref("accessibility.typeaheadfind.autostart", false);
user_pref("javascript.options.showInConsole", true);
user_pref("devtools.errorconsole.enabled", true);
user_pref("devtools.debugger.remote-port", 6023);
user_pref("layout.debug.enable_data_xbl", true);
user_pref("browser.EULA.override", true);
user_pref("gfx.color_management.force_srgb", true);
user_pref("network.manage-offline-status", false);

user_pref("network.http.speculative-parallel-limit", 0);
user_pref("dom.min_background_timeout_value", 1000);
user_pref("test.mousescroll", true);
user_pref("security.default_personal_cert", "Select Automatically"); 
user_pref("network.http.prompt-temp-redirect", false);
user_pref("media.cache_size", 100);
user_pref("media.volume_scale", "0.01");
user_pref("security.warn_viewing_mixed", false);
user_pref("app.update.enabled", false);
user_pref("app.update.staging.enabled", false);
user_pref("browser.panorama.experienced_first_run", true); 
user_pref("dom.w3c_touch_events.enabled", 1);
user_pref("dom.undo_manager.enabled", true);
user_pref("dom.webcomponents.enabled", true);
user_pref("dom.animations-api.core.enabled", true);

user_pref("toolkit.telemetry.prompted", 999);
user_pref("toolkit.telemetry.notifiedOptOut", 999);

user_pref("font.size.inflation.emPerLine", 0);
user_pref("font.size.inflation.minTwips", 0);


user_pref("experiments.supported", true);
user_pref("experiments.logging.level", "Trace");
user_pref("experiments.logging.dump", true);


user_pref("experiments.manifest.uri", "http://%(server)s/experiments-dummy/manifest");



user_pref("extensions.enabledScopes", 5);

user_pref("extensions.getAddons.cache.enabled", false);

user_pref("extensions.installDistroAddons", false);

user_pref("extensions.defaultProviders.enabled", true);

user_pref("geo.wifi.uri", "http://%(server)s/tests/dom/tests/mochitest/geolocation/network_geolocation.sjs");
user_pref("geo.wifi.timeToWaitBeforeSending", 200);
user_pref("geo.wifi.scan", false);
user_pref("geo.wifi.logging.enabled", true);

user_pref("camino.warn_when_closing", false); 


user_pref("urlclassifier.updateinterval", 172800);

user_pref("browser.safebrowsing.gethashURL", "http://%(server)s/safebrowsing-dummy/gethash");
user_pref("browser.safebrowsing.updateURL", "http://%(server)s/safebrowsing-dummy/update");

user_pref("extensions.update.url", "http://%(server)s/extensions-dummy/updateURL");
user_pref("extensions.update.background.url", "http://%(server)s/extensions-dummy/updateBackgroundURL");
user_pref("extensions.blocklist.url", "http://%(server)s/extensions-dummy/blocklistURL");
user_pref("extensions.hotfix.url", "http://%(server)s/extensions-dummy/hotfixURL");

user_pref("extensions.update.enabled", false);

user_pref("extensions.webservice.discoverURL", "http://%(server)s/extensions-dummy/discoveryURL");

user_pref("extensions.getAddons.maxResults", 0);
user_pref("extensions.getAddons.get.url", "http://%(server)s/extensions-dummy/repositoryGetURL");
user_pref("extensions.getAddons.getWithPerformance.url", "http://%(server)s/extensions-dummy/repositoryGetWithPerformanceURL");
user_pref("extensions.getAddons.search.browseURL", "http://%(server)s/extensions-dummy/repositoryBrowseURL");
user_pref("extensions.getAddons.search.url", "http://%(server)s/extensions-dummy/repositorySearchURL");

user_pref("plugins.update.url", "http://%(server)s/plugins-dummy/updateCheckURL");


user_pref("security.notification_enable_delay", 0);


user_pref("security.turn_off_all_security_so_that_viruses_can_take_over_this_computer", true);







user_pref("dom.use_xbl_scopes_for_remote_xul", true);


user_pref("network.activity.blipIntervalMilliseconds", 250);


user_pref("datareporting.policy.dataSubmissionPolicyBypassAcceptance", true);



user_pref("datareporting.healthreport.documentServerURI", "http://%(server)s/healthreport/");


user_pref("layout.css.report_errors", true);


user_pref("layout.css.grid.enabled", true);


user_pref("dom.mozContacts.enabled", true);
user_pref("dom.navigator-property.disable.mozContacts", false);
user_pref("dom.global-constructor.disable.mozContact", false);


user_pref("dom.mozSettings.enabled", true);


user_pref("network.http.bypass-cachelock-threshold", 200000);


user_pref("dom.gamepad.enabled", true);
user_pref("dom.gamepad.non_standard_events.enabled", true);



user_pref("geo.provider.testing", true);



user_pref("browser.pagethumbnails.capturing_disabled", true);



user_pref("browser.download.panel.shown", true);



user_pref("browser.firstrun.count", 0);


user_pref("pbackground.testing", true);


user_pref("browser.webapps.testing", true);


user_pref("browser.snippets.enabled", false);
user_pref("browser.snippets.syncPromo.enabled", false);


user_pref("browser.cache.use_new_backend_temp", false);



user_pref('browser.contentHandlers.types.0.uri', 'http://test1.example.org/rss?url=%%s')
user_pref('browser.contentHandlers.types.1.uri', 'http://test1.example.org/rss?url=%%s')
user_pref('browser.contentHandlers.types.2.uri', 'http://test1.example.org/rss?url=%%s')
user_pref('browser.contentHandlers.types.3.uri', 'http://test1.example.org/rss?url=%%s')
user_pref('browser.contentHandlers.types.4.uri', 'http://test1.example.org/rss?url=%%s')
user_pref('browser.contentHandlers.types.5.uri', 'http://test1.example.org/rss?url=%%s')


user_pref('toolkit.telemetry.server', 'https://%(server)s/telemetry-dummy/');




user_pref('identity.fxaccounts.auth.uri', 'https://%(server)s/fxa-dummy/');


user_pref('apz.test.logging_enabled', true);
