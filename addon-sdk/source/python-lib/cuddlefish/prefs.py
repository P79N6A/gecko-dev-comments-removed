



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

}

DEFAULT_NO_CONNECTIONS_PREFS = {
    'toolkit.telemetry.enabled': False,
    'app.update.auto' : False,
    'app.update.url': 'http://localhost/app-dummy/update',
    'media.gmp-gmpopenh264.autoupdate' : False,
    'media.gmp-manager.cert.checkAttributes' : False,
    'media.gmp-manager.cert.requireBuiltIn' : False,
    'media.gmp-manager.url' : 'http://localhost/media-dummy/gmpmanager',
    'browser.newtab.url' : 'about:blank',
    'browser.search.update': False,
    'browser.safebrowsing.enabled' : False,
    'browser.safebrowsing.updateURL': 'http://localhost/safebrowsing-dummy/update',
    'browser.safebrowsing.gethashURL': 'http://localhost/safebrowsing-dummy/gethash',
    'browser.safebrowsing.reportURL': 'http://localhost/safebrowsing-dummy/report',
    'browser.safebrowsing.malware.reportURL': 'http://localhost/safebrowsing-dummy/malwarereport',

    
    'app.update.enabled' : False,

    
    'browser.newtabpage.directory.source': 'data:application/json,{"jetpack":1}',
    'browser.newtabpage.directory.ping': '',

    
    'extensions.update.url' : 'http://localhost/extensions-dummy/updateURL',
    'extensions.blocklist.url' : 'http://localhost/extensions-dummy/blocklistURL',
    
    'extensions.webservice.discoverURL' : 'http://localhost/extensions-dummy/discoveryURL'
}

DEFAULT_FENNEC_PREFS = {
  'browser.console.showInPanel': True,
  'browser.firstrun.show.uidiscovery': False
}


DEFAULT_FIREFOX_PREFS = {
    'browser.startup.homepage' : 'about:blank',
    'startup.homepage_welcome_url' : 'about:blank',
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
