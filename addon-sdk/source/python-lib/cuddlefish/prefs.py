



DEFAULT_COMMON_PREFS = {
    
    
    
    'browser.dom.window.dump.enabled': True,
    
    'javascript.options.showInConsole': True,

    
    'devtools.debugger.remote-enabled' : True,

    'extensions.sdk.console.logLevel': 'info',

    'extensions.checkCompatibility.nightly' : False,

    
    'extensions.update.enabled' : False,
    'lightweightThemes.update.enabled' : False,
    'extensions.update.notifyUser' : False,

    
    
    
    
    'extensions.enabledScopes' : 5,
    
    'extensions.getAddons.cache.enabled' : False,
    
    'extensions.installDistroAddons' : False,
    
    'extensions.autoDisableScopes' : 10,

    
    'app.releaseNotesURL': 'http://localhost/app-dummy/',
    'app.vendorURL': 'http://localhost/app-dummy/',

    
    'browser.displayedE10SPrompt.1': 5
}

DEFAULT_NO_CONNECTIONS_PREFS = {
    'toolkit.telemetry.enabled': False,
    'toolkit.telemetry.server': 'https://localhost/telemetry-dummy/',
    'app.update.auto' : False,
    'app.update.url': 'http://localhost/app-dummy/update',
    
    'media.gmp-gmpopenh264.autoupdate' : False,
    'media.gmp-manager.cert.checkAttributes' : False,
    'media.gmp-manager.cert.requireBuiltIn' : False,
    'media.gmp-manager.url' : 'http://localhost/media-dummy/gmpmanager',
    'media.gmp-manager.url.override': 'http://localhost/dummy-gmp-manager.xml',
    'browser.aboutHomeSnippets.updateUrl': 'https://localhost/snippet-dummy',
    'browser.newtab.url' : 'about:blank',
    'browser.search.update': False,
    'browser.search.suggest.enabled' : False,
    'browser.safebrowsing.enabled' : False,
    'browser.safebrowsing.updateURL': 'http://localhost/safebrowsing-dummy/update',
    'browser.safebrowsing.gethashURL': 'http://localhost/safebrowsing-dummy/gethash',
    'browser.safebrowsing.reportURL': 'http://localhost/safebrowsing-dummy/report',
    'browser.safebrowsing.malware.reportURL': 'http://localhost/safebrowsing-dummy/malwarereport',
    'browser.selfsupport.url': 'https://localhost/selfsupport-dummy',
    'browser.trackingprotection.gethashURL': 'http://localhost/safebrowsing-dummy/gethash',
    'browser.trackingprotection.updateURL': 'http://localhost/safebrowsing-dummy/update',

    
    'app.update.enabled' : False,
    'app.update.staging.enabled': False,

    
    'browser.newtabpage.directory.source': 'data:application/json,{"jetpack":1}',
    'browser.newtabpage.directory.ping': '',

    
    'extensions.update.url' : 'http://localhost/extensions-dummy/updateURL',
    'extensions.update.background.url': 'http://localhost/extensions-dummy/updateBackgroundURL',
    'extensions.blocklist.url' : 'http://localhost/extensions-dummy/blocklistURL',
    
    'extensions.webservice.discoverURL' : 'http://localhost/extensions-dummy/discoveryURL',
    'extensions.getAddons.maxResults': 0,

    
    'browser.webapps.checkForUpdates': 0,

    
    'geo.wifi.uri': 'http://localhost/location-dummy/locationURL',
    'browser.search.geoip.url': 'http://localhost/location-dummy/locationURL',

    
    
    'browser.search.isUS' : True,
    'browser.search.countryCode' : 'US',

    'geo.wifi.uri' : 'http://localhost/extensions-dummy/geowifiURL',
    'geo.wifi.scan' : False,

    
    
    
    'identity.fxaccounts.auth.uri': 'http://localhost/fxa-dummy/'
}

DEFAULT_FENNEC_PREFS = {
  'browser.console.showInPanel': True,
  'browser.firstrun.show.uidiscovery': False
}


DEFAULT_FIREFOX_PREFS = {
    'browser.startup.homepage' : 'about:blank',
    'startup.homepage_welcome_url' : 'about:blank',
    'devtools.browsertoolbox.panel': 'jsdebugger',
    'devtools.errorconsole.enabled' : True,
    'devtools.chrome.enabled' : True,

    
    
    
    'urlclassifier.updateinterval' : 172800,
    
    'browser.safebrowsing.provider.0.gethashURL' : 'http://localhost/safebrowsing-dummy/gethash',
    'browser.safebrowsing.provider.0.updateURL' : 'http://localhost/safebrowsing-dummy/update',
}




DEFAULT_THUNDERBIRD_PREFS = {
    
    'dom.max_chrome_script_run_time': 200,
    'dom.max_script_run_time': 0,
    
    'mail.shell.checkDefaultClient': False,
    
    'mail.winsearch.enable': False,
    'mail.winsearch.firstRunDone': True,
    'mail.spotlight.enable': False,
    'mail.spotlight.firstRunDone': True,
    
    'ldap_2.servers.osx.position': 0,
    'ldap_2.servers.oe.position': 0,
    
    'mailnews.ui.junk.firstuse': False,
    
    
    'mail.account.account1.server' :  "server1",
    'mail.account.account2.identities' :  "id1",
    'mail.account.account2.server' :  "server2",
    'mail.accountmanager.accounts' :  "account1,account2",
    'mail.accountmanager.defaultaccount' :  "account2",
    'mail.accountmanager.localfoldersserver' :  "server1",
    'mail.identity.id1.fullName' :  "Tinderbox",
    'mail.identity.id1.smtpServer' :  "smtp1",
    'mail.identity.id1.useremail' :  "tinderbox@invalid.com",
    'mail.identity.id1.valid' :  True,
    'mail.root.none-rel' :  "[ProfD]Mail",
    'mail.root.pop3-rel' :  "[ProfD]Mail",
    'mail.server.server1.directory-rel' :  "[ProfD]Mail/Local Folders",
    'mail.server.server1.hostname' :  "Local Folders",
    'mail.server.server1.name' :  "Local Folders",
    'mail.server.server1.type' :  "none",
    'mail.server.server1.userName' :  "nobody",
    'mail.server.server2.check_new_mail' :  False,
    'mail.server.server2.directory-rel' :  "[ProfD]Mail/tinderbox",
    'mail.server.server2.download_on_biff' :  True,
    'mail.server.server2.hostname' :  "tinderbox",
    'mail.server.server2.login_at_startup' :  False,
    'mail.server.server2.name' :  "tinderbox@invalid.com",
    'mail.server.server2.type' :  "pop3",
    'mail.server.server2.userName' :  "tinderbox",
    'mail.smtp.defaultserver' :  "smtp1",
    'mail.smtpserver.smtp1.hostname' :  "tinderbox",
    'mail.smtpserver.smtp1.username' :  "tinderbox",
    'mail.smtpservers' :  "smtp1",
    'mail.startup.enabledMailCheckOnce' :  True,
    'mailnews.start_page_override.mstone' :  "ignore",
}

DEFAULT_TEST_PREFS = {
    'browser.console.showInPanel': True,
    'browser.startup.page': 0,
    'browser.firstrun.show.localepicker': False,
    'browser.firstrun.show.uidiscovery': False,
    'browser.ui.layout.tablet': 0,
    'dom.disable_open_during_load': False,
    'dom.experimental_forms': True,
    'dom.forms.number': True,
    'dom.forms.color': True,
    'dom.max_script_run_time': 0,
    'hangmonitor.timeout': 0,
    'dom.max_chrome_script_run_time': 0,
    'dom.popup_maximum': -1,
    'dom.send_after_paint_to_content': True,
    'dom.successive_dialog_time_limit': 0,
    'browser.shell.checkDefaultBrowser': False,
    'shell.checkDefaultClient': False,
    'browser.warnOnQuit': False,
    'accessibility.typeaheadfind.autostart': False,
    'browser.EULA.override': True,
    'gfx.color_management.force_srgb': True,
    'network.manage-offline-status': False,
    
    'network.http.speculative-parallel-limit': 0,
    'test.mousescroll': True,
    
    'security.default_personal_cert': 'Select Automatically',
    'network.http.prompt-temp-redirect': False,
    'security.warn_viewing_mixed': False,
    'browser.panorama.experienced_first_run': True,
    
    'toolkit.telemetry.prompted': 999,
    'toolkit.telemetry.notifiedOptOut': 999,
    'extensions.defaultProviders.enabled': True,
    'datareporting.policy.dataSubmissionPolicyBypassNotification': True,
    'layout.css.report_errors': True,
    'layout.css.grid.enabled': True,
    'layout.spammy_warnings.enabled': False,
    'dom.mozSettings.enabled': True,
    
    'network.http.bypass-cachelock-threshold': 200000,
    
    
    'geo.provider.testing': True,
    
    
    'browser.pagethumbnails.capturing_disabled': True,
    
    
    'browser.download.panel.shown': True,
    
    
    'browser.newtabpage.introShown': True,
    
    'general.useragent.updates.enabled': False,
    'dom.mozApps.debug': True,
    'dom.apps.customization.enabled': True,
    'media.eme.enabled': True,
    'media.eme.apiVisible': True,
    
    'dom.ipc.tabs.shutdownTimeoutSecs': 0,
    'general.useragent.locale': "en-US",
    'intl.locale.matchOS': "en-US",
    'dom.indexedDB.experimental': True
}
