function dumpPathOfProperty(prop)
{
    dump(prop +' = ');

    try
    {
        var file = dsprops.get(prop, Components.interfaces.nsIFile);
        dump(file.path + '\n');
    }
    catch (ar)
    {
        dump('undefined or error \n');
    }
}

var dscontractid = "@mozilla.org/file/directory_service;1";  
var ds = Components.classes[dscontractid].getService();
var dsprops = ds.QueryInterface(Components.interfaces.nsIProperties);

dump("xpcom locations::\n");

    dumpPathOfProperty ("xpcom.currentProcess");
    dumpPathOfProperty ("xpcom.currentProcess.componentRegistry");
    dumpPathOfProperty ("xpcom.currentProcess.componentDirectory");
    dumpPathOfProperty ("system.OS_DriveDirectory");
    dumpPathOfProperty ("system.OS_TemporaryDirectory");
    dumpPathOfProperty ("system.OS_CurrentProcessDirectory");
    dumpPathOfProperty ("system.OS_CurrentWorkingDirectory");

dump("Mac locations::\n");

    dumpPathOfProperty ("system.SystemDirectory");
    dumpPathOfProperty ("system.DesktopDirectory");
    dumpPathOfProperty ("system.TrashDirectory");
    dumpPathOfProperty ("system.StartupDirectory");
    dumpPathOfProperty ("system.ShutdownDirectory");
    dumpPathOfProperty ("system.AppleMenuDirectory");
    dumpPathOfProperty ("system.ControlPanelDirectory");
    dumpPathOfProperty ("system.ExtensionDirectory");
    dumpPathOfProperty ("system.FontsDirectory");
    dumpPathOfProperty ("system.PreferencesDirectory");
    dumpPathOfProperty ("system.DocumentsDirectory");
    dumpPathOfProperty ("system.InternetSearchDirectory");

dump("PC locations::\n");

    dumpPathOfProperty ("system.SystemDirectory");
    dumpPathOfProperty ("system.WindowsDirectory");
    dumpPathOfProperty ("system.HomeDirectory");
    dumpPathOfProperty ("system.Desktop");
    dumpPathOfProperty ("system.Programs");
    dumpPathOfProperty ("system.Controls");
    dumpPathOfProperty ("system.Printers");
    dumpPathOfProperty ("system.Personal");
    dumpPathOfProperty ("system.Favorites");
    dumpPathOfProperty ("system.Startup");
    dumpPathOfProperty ("system.Recent");
    dumpPathOfProperty ("system.Sendto");
    dumpPathOfProperty ("system.Bitbucket");
    dumpPathOfProperty ("system.Startmenu");
    dumpPathOfProperty ("system.Desktopdirectory");
    dumpPathOfProperty ("system.Drives");
    dumpPathOfProperty ("system.Network");
    dumpPathOfProperty ("system.Nethood");
    dumpPathOfProperty ("system.Fonts");
    dumpPathOfProperty ("system.Templates");
    dumpPathOfProperty ("system.Common_Startmenu");
    dumpPathOfProperty ("system.Common_Programs");
    dumpPathOfProperty ("system.Common_Startup");
    dumpPathOfProperty ("system.Common_Desktopdirectory");
    dumpPathOfProperty ("system.Appdata");
    dumpPathOfProperty ("system.Printhood");

dump("Unix locations::\n");


    dumpPathOfProperty ("system.LocalDirectory");
    dumpPathOfProperty ("system.LibDirectory");
    dumpPathOfProperty ("system.HomeDirectory");

dump("Beos locations::\n");


    dumpPathOfProperty ("system.SettingsDirectory");
    dumpPathOfProperty ("system.HomeDirectory");
    dumpPathOfProperty ("system.DesktopDirectory");
    dumpPathOfProperty ("system.SystemDirectory");

dump("OS2 locations::\n");

    dumpPathOfProperty ("system.SystemDirectory");
    dumpPathOfProperty ("system.OS2Directory");
    dumpPathOfProperty ("system.DesktopDirectory");






    dumpPathOfProperty ("app.res.directory");
    dumpPathOfProperty ("app.defaults.directory");
    dumpPathOfProperty ("app.chrome.directory");
    dumpPathOfProperty ("app.chrome.user.directory");
    dumpPathOfProperty ("app.plugins.directory");



    dumpPathOfProperty ("app.registry.file.4");
    dumpPathOfProperty ("app.registry.file.5");
    dumpPathOfProperty ("app.local.store.file.5");
    dumpPathOfProperty ("app.history.file.5");
    dumpPathOfProperty ("app.user.panels.5");





    dumpPathOfProperty ("app.prefs.directory.5");
    dumpPathOfProperty ("app.pref.default.directory.5");
    


    dumpPathOfProperty ("app.prefs.file.5");





    dumpPathOfProperty ("app.profile.user.directory.5");


    dumpPathOfProperty ("app.profile.default.user.directory.5");


    dumpPathOfProperty ("app.profile.defaults.directory.5");







    dumpPathOfProperty ("app.bookmark.file.5");


    dumpPathOfProperty ("app.search.file.5");
    dumpPathOfProperty ("app.search.directory.5");
    



    dumpPathOfProperty ("app.mail.directory.5");
    dumpPathOfProperty ("app.mail.imap.directory.5");
    dumpPathOfProperty ("app.mail.news.directory.5");
    dumpPathOfProperty ("app.mail.messenger.cache.directory.5");
