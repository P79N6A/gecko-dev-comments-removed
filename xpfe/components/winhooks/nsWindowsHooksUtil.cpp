





































#include <windows.h>
#include <stdlib.h>
#include <string.h>

#include "nsString.h"
#include "nsIStringBundle.h"
#include "nsDirectoryService.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsNativeCharsetUtils.h"
#include "nsWindowsHooksUtil.h"

#ifdef MOZ_XUL_APP
#include "nsNativeAppSupportWin.h"
#else
#include "nsINativeAppSupportWin.h"
#include "nsICmdLineHandler.h"
#endif

#define MOZ_HWND_BROADCAST_MSG_TIMEOUT 5000
#define MOZ_CLIENT_BROWSER_KEY "Software\\Clients\\StartMenuInternet"


nsCString RegistryEntry::fullName() const {
    nsCString result;
    if ( baseKey == HKEY_CURRENT_USER ) {
        result = "HKEY_CURRENT_USER\\";
    } else if ( baseKey == HKEY_LOCAL_MACHINE ) {
        result = "HKEY_LOCAL_MACHINE\\";
    } else {
        result = "\\";
    }
    result += keyName;
    if ( !valueName.IsEmpty() ) {
        result += "[";
        result += valueName;
        result += "]";
    }
    return result;
}


PRBool RegistryEntry::isAlreadySet() const {
    PRBool result = FALSE;

    nsCAutoString current( currentSetting() );

    result = ( current == setting );

    return result;
}


nsresult RegistryEntry::set() {
#ifdef DEBUG_law
if ( isNull && setting.IsEmpty() ) printf( "Deleting %s\n", fullName().get() );
else printf( "Setting %s=%s\n", fullName().get(), setting.get() );
#endif
    nsresult result = NS_ERROR_FAILURE;

    HKEY   key;
    LONG   rc = ::RegOpenKey( baseKey, keyName.get(), &key );

    
    if ( rc == ERROR_FILE_NOT_FOUND ) {
        rc = ::RegCreateKey( baseKey, keyName.get(), &key );
    }
    if ( rc == ERROR_SUCCESS ) {
        if ( isNull && setting.IsEmpty() ) {
            
            
            rc = ::RegDeleteValue( key, valueNameArg() );
            if ( rc == ERROR_SUCCESS ) {
                result = NS_OK;
            }
        } else {
            NS_ConvertUTF8toUTF16 utf16Setting(setting);
            
            PRUnichar buffer[4096] = { 0 };
            DWORD len = sizeof buffer;
            NS_ConvertASCIItoUTF16 wValueName(valueNameArg());
            rc = ::RegQueryValueExW( key, wValueName.get(), NULL,
                 NULL, (LPBYTE)buffer, &len );
            if ( rc != ERROR_SUCCESS || !utf16Setting.Equals(buffer) ) {
                rc = ::RegSetValueExW( key, wValueName.get(), 0, REG_SZ,
                     (LPBYTE) (utf16Setting.get()),
                     utf16Setting.Length() * 2);
                if ( rc == ERROR_SUCCESS ) {
                   result = NS_OK;
                }
            } else {
                
                result = NS_OK;
            }
        }
        ::RegCloseKey( key );
    } else {
#ifdef DEBUG_law
NS_ASSERTION( rc == ERROR_SUCCESS, fullName().get() );
#endif
    }
    return result;
}


nsresult SavedRegistryEntry::set() {
    nsresult rv = NS_OK;
    PRBool   currentlyUndefined = PR_TRUE;
    nsCAutoString prev( currentSetting( &currentlyUndefined ) );
    
    
    
    if ( setting != prev || ( !currentlyUndefined && isNull ) ) {
        
        rv = RegistryEntry::set();
        if ( NS_SUCCEEDED( rv ) ) {
            
            RegistryEntry tmp( HKEY_LOCAL_MACHINE, "Software\\Mozilla\\Desktop", fullName().get(), prev.get() );
            tmp.set();
        }
    }
    return rv;
}








static void setWindowsXP() {
    
    
    
    HKEY key;
    nsCAutoString baseKey( NS_LITERAL_CSTRING( "Software\\Clients\\StartMenuInternet" ) );
    LONG rc = ::RegOpenKey( HKEY_LOCAL_MACHINE, baseKey.get(), &key );
    if ( rc == ERROR_SUCCESS ) {
        
        
        
        
        ::RegCloseKey(key);
        
        nsCAutoString subkey( baseKey + NS_LITERAL_CSTRING( "\\" ) + shortAppName() );
        
        
        
        
        char buffer[ _MAX_PATH + 8 ]; 
        _snprintf( buffer, sizeof buffer, "@%s,-%d", thisApplication().get(), IDS_STARTMENU_APPNAME );
        RegistryEntry tmp_entry1( HKEY_LOCAL_MACHINE, 
                       subkey.get(),
                       "LocalizedString", 
                       buffer );
        tmp_entry1.set();
        
        RegistryEntry tmp_entry2( HKEY_LOCAL_MACHINE, 
                       nsCAutoString( subkey + NS_LITERAL_CSTRING( "\\DefaultIcon" ) ).get(),
                       "", 
                       nsCAutoString( thisApplication() + NS_LITERAL_CSTRING( ",0" ) ).get() );
        tmp_entry2.set();
        
        RegistryEntry tmp_entry3( HKEY_LOCAL_MACHINE,
                       nsCAutoString( subkey + NS_LITERAL_CSTRING( "\\shell\\open\\command" ) ).get(),
                       "", 
                       thisApplication().get() );
        tmp_entry3.set();
        
        
        
        nsCOMPtr<nsIStringBundleService> bundleService( do_GetService( "@mozilla.org/intl/stringbundle;1" ) );
        nsCOMPtr<nsIStringBundle> bundle;
        nsXPIDLString label;
        if ( bundleService &&
             NS_SUCCEEDED( bundleService->CreateBundle( "chrome://global-platform/locale/nsWindowsHooks.properties",
                                                   getter_AddRefs( bundle ) ) ) &&
             NS_SUCCEEDED( bundle->GetStringFromName( NS_LITERAL_STRING( "prefsLabel" ).get(), getter_Copies( label ) ) ) ) {
            
            RegistryEntry tmp_entry4( HKEY_LOCAL_MACHINE,
                           nsCAutoString( subkey + NS_LITERAL_CSTRING( "\\shell\\properties" ) ).get(),
                           "", 
                           NS_ConvertUTF16toUTF8( label ).get() );
            tmp_entry4.set();
        }
        RegistryEntry tmp_entry5( HKEY_LOCAL_MACHINE,
                       nsCAutoString( subkey + NS_LITERAL_CSTRING( "\\shell\\properties\\command" ) ).get(),
                       "", nsCAutoString( thisApplication() + 
                                           NS_LITERAL_CSTRING(" -chrome \"chrome://communicator/content/pref/pref.xul\"") ).get()
                      );
        tmp_entry5.set();

        
        
        
        
        SavedRegistryEntry hklmAppEntry( HKEY_LOCAL_MACHINE, baseKey.get(), "", shortAppName().get() );
        hklmAppEntry.set();
        
        if ( hklmAppEntry.currentSetting() == hklmAppEntry.setting ) {
            
            SavedRegistryEntry tmp_entry6( HKEY_CURRENT_USER, baseKey.get(), "", 0 );
            tmp_entry6.set();
        } else {
            
            SavedRegistryEntry tmp_entry7( HKEY_CURRENT_USER, baseKey.get(), "", shortAppName().get() );
        }
        
        ::SendMessageTimeout( HWND_BROADCAST,
                              WM_SETTINGCHANGE,
                              0,
                              (LPARAM)MOZ_CLIENT_BROWSER_KEY,
                              SMTO_NORMAL|SMTO_ABORTIFHUNG,
                              MOZ_HWND_BROADCAST_MSG_TIMEOUT,
                              NULL);
    }
}



nsresult ProtocolRegistryEntry::set() {
    
    
    
    
    if ( protocol.EqualsLiteral( "http" ) ) {
        setWindowsXP();
    }

    
    nsresult rv = SavedRegistryEntry::set();

    
    DDERegistryEntry( protocol.get() ).set();
    
    ProtocolIconRegistryEntry( protocol.get() ).set();

    return rv;
}


nsresult RegistryEntry::reset() {
    HKEY key;
    LONG rc = ::RegOpenKey( baseKey, keyName.get(), &key );
    if ( rc == ERROR_SUCCESS ) {
        rc = ::RegDeleteValue( key, valueNameArg() );
        ::RegCloseKey(key);
#ifdef DEBUG_law
if ( rc == ERROR_SUCCESS ) printf( "Deleting key=%s\n", (const char*)fullName().get() );
#endif
    }
    return NS_OK;
}




nsresult SavedRegistryEntry::reset() {
    nsresult result = NS_OK;

    
    nsCAutoString current( currentSetting() );

    
    if ( current == setting ) {
        
        PRBool noSavedValue = PR_TRUE;
        RegistryEntry saved = RegistryEntry( HKEY_LOCAL_MACHINE, mozillaKeyName, fullName().get(), "" );
        
        
        
        
        
        
        setting = saved.currentSetting( &noSavedValue );
        if ( !setting.IsEmpty() || !noSavedValue ) {
            
            isNull = PR_FALSE; 
                               
            result = RegistryEntry::set();
            
            saved.reset();
        } else {
            
            result = RegistryEntry::reset();
        }
    }

    return result;
}








static void resetWindowsXP() {
    NS_NAMED_LITERAL_CSTRING( baseKey, "Software\\Clients\\StartMenuInternet" );
    
    
    SavedRegistryEntry tmp_entry8( HKEY_LOCAL_MACHINE, baseKey.get(), "", shortAppName().get() );
    tmp_entry8.reset();

    
    
    
    SavedRegistryEntry tmp_entry9( HKEY_CURRENT_USER, baseKey.get(), "", shortAppName().get() );
    tmp_entry9.reset();
    
    
    SavedRegistryEntry tmp_entry10( HKEY_CURRENT_USER, baseKey.get(), "", 0 );
    tmp_entry10.reset();

    
    ::SendMessageTimeout( HWND_BROADCAST,
                          WM_SETTINGCHANGE,
                          0,
                          (LPARAM)MOZ_CLIENT_BROWSER_KEY,
                          SMTO_NORMAL|SMTO_ABORTIFHUNG,
                          MOZ_HWND_BROADCAST_MSG_TIMEOUT,
                          NULL);
}


nsresult ProtocolRegistryEntry::reset() {
    
    nsresult rv = SavedRegistryEntry::reset();

    
    DDERegistryEntry( protocol.get() ).reset();
    
    ProtocolIconRegistryEntry( protocol.get() ).reset();

    
    if ( protocol.EqualsLiteral( "http" ) ) {
        resetWindowsXP();
    }

    return rv;
}

static DWORD deleteKey( HKEY baseKey, const char *keyName ) {
    
    DWORD rc;
    if ( keyName && ::strlen(keyName) ) {
        
        HKEY key;
        rc = ::RegOpenKeyEx( baseKey,
                             keyName,
                             0,
                             KEY_ENUMERATE_SUB_KEYS | DELETE,
                             &key );
        
        while ( rc == ERROR_SUCCESS ) {
            char subkeyName[_MAX_PATH];
            DWORD len = sizeof subkeyName;
            
            
            
            rc = ::RegEnumKeyEx( key,
                                 0,
                                 subkeyName,
                                 &len,
                                 0,
                                 0,
                                 0,
                                 0 );
            if ( rc == ERROR_NO_MORE_ITEMS ) {
                
                rc = ::RegDeleteKey( baseKey, keyName );
                break;
            } else if ( rc == ERROR_SUCCESS ) {
                
                rc = deleteKey( key, subkeyName );
            }
        }
        
        ::RegCloseKey( key );
    } else {
        rc = ERROR_BADKEY;
    }
    return rc;
}





nsresult DDERegistryEntry::set() {
    nsresult rv = SavedRegistryEntry::set();
    rv = activate.set();
    rv = app.set();
    rv = topic.set();
    
    
    
    
    if ( deleteKey( baseKey, keyName.get() ) != ERROR_SUCCESS ) {
        rv = NS_ERROR_FAILURE;
    }
    return rv;
}





nsresult DDERegistryEntry::reset() {
    nsresult rv = SavedRegistryEntry::reset();
    rv = activate.reset();
    rv = app.reset();
    rv = topic.reset();
    return rv;
}






nsCString RegistryEntry::currentSetting( PRBool *currentlyUndefined ) const {
    nsCString result;

    if ( currentlyUndefined ) {
        *currentlyUndefined = PR_TRUE;
    }

    HKEY   key;
    LONG   rc = ::RegOpenKey( baseKey, keyName.get(), &key );
    if ( rc == ERROR_SUCCESS ) {
        PRUnichar buffer[4096] = { 0 };
        DWORD len = sizeof buffer;
        rc = ::RegQueryValueExW( key,
             NS_ConvertASCIItoUTF16(valueNameArg()).get(), NULL, NULL,
             (LPBYTE)buffer, &len );
        if ( rc == ERROR_SUCCESS ) {
            CopyUTF16toUTF8(buffer, result);
            if ( currentlyUndefined ) {
                *currentlyUndefined = PR_FALSE; 
            }
        }
        ::RegCloseKey( key );
    }

    return result;
}



nsresult FileTypeRegistryEntry::set() {
    nsresult rv = NS_OK;

    
    for ( int i = 0; NS_SUCCEEDED( rv ) && ext[i]; i++ ) {
        nsCAutoString thisExt( "Software\\Classes\\" );
        thisExt += ext[i];
        rv = SavedRegistryEntry( HKEY_LOCAL_MACHINE, thisExt.get(), "", fileType.get() ).set();
    }

    
    if ( NS_SUCCEEDED( rv ) ) {
        rv = ProtocolRegistryEntry::set();

        
        if ( NS_SUCCEEDED( rv ) ) {
            nsCAutoString iconFileToUse( "%1" );
            nsCAutoString descKey( "Software\\Classes\\" );
            descKey += protocol;
            RegistryEntry descEntry( HKEY_LOCAL_MACHINE, descKey.get(), NULL, "" );
            if ( descEntry.currentSetting().IsEmpty() ) {
                nsCAutoString defaultDescKey( "Software\\Classes\\" );
                defaultDescKey += defDescKey;
                RegistryEntry defaultDescEntry( HKEY_LOCAL_MACHINE, defaultDescKey.get(), NULL, "" );

                descEntry.setting = defaultDescEntry.currentSetting();
                if ( descEntry.setting.IsEmpty() )
                    descEntry.setting = desc;
                descEntry.set();
            }
            nsCAutoString iconKey( "Software\\Classes\\" );
            iconKey += protocol;
            iconKey += "\\DefaultIcon";

            if ( !iconFile.Equals(iconFileToUse) ) {
            iconFileToUse = thisApplication() + NS_LITERAL_CSTRING( ",0" );

            
            
            
            if ( !iconFile.IsEmpty() ) {
                nsCOMPtr<nsIFile> iconFileSpec;
                PRBool            flagExists;

                
                
                
                
                rv = NS_GetSpecialDirectory( NS_APP_CHROME_DIR, getter_AddRefs( iconFileSpec ) );
                if ( NS_SUCCEEDED( rv ) ) {
                    iconFileSpec->AppendNative( NS_LITERAL_CSTRING( "icons" ) );
                    iconFileSpec->AppendNative( NS_LITERAL_CSTRING( "default" ) );
                    iconFileSpec->AppendNative( iconFile );

                    
                    iconFileSpec->Exists( &flagExists );
                    if ( flagExists ) {
                        rv = iconFileSpec->GetNativePath( iconFileToUse );
                        if ( NS_SUCCEEDED( rv ) ) {
                            TCHAR buffer[MAX_PATH];
                            DWORD len;

                            
                            
                            len = ::GetShortPathName( iconFileToUse.get(), buffer, sizeof buffer );
                            NS_ASSERTION ( (len > 0) && ( len < sizeof buffer ), "GetShortPathName failed!" );
                            iconFileToUse.Assign( buffer );
                            iconFileToUse.Append( NS_LITERAL_CSTRING( ",0" ) );
                        }
                        }
                    }
                }
            }

            RegistryEntry iconEntry( HKEY_LOCAL_MACHINE, iconKey.get(), NULL, iconFileToUse.get() );
            if( !iconEntry.currentSetting().Equals( iconFileToUse ) )
                iconEntry.set();
        }
    }

    return rv;
}




nsresult FileTypeRegistryEntry::reset() {
    nsresult rv = ProtocolRegistryEntry::reset();

    for ( int i = 0; ext[ i ]; i++ ) {
        nsCAutoString thisExt( "Software\\Classes\\" );
        thisExt += ext[i];
        (void)SavedRegistryEntry( HKEY_LOCAL_MACHINE, thisExt.get(), "", fileType.get() ).reset();
    }

    return rv;
}







nsresult EditableFileTypeRegistryEntry::set() {
    nsresult rv = FileTypeRegistryEntry::set();
#ifndef MOZ_XUL_APP
    if ( NS_SUCCEEDED( rv ) ) {
        
        nsCOMPtr<nsICmdLineHandler> editorService =
            do_GetService( "@mozilla.org/commandlinehandler/general-startup;1?type=edit", &rv );
        if ( NS_SUCCEEDED( rv) ) {
            nsCAutoString editKey( "Software\\Classes\\" );
            editKey += protocol;
            editKey += "\\shell\\edit\\command";
            nsCAutoString editor( thisApplication() );
            editor += " -edit \"%1\"";
            rv = RegistryEntry( HKEY_LOCAL_MACHINE, editKey.get(), "", editor.get() ).set();
        }
    }
#endif
    return rv;
}


BoolRegistryEntry::operator PRBool() {
    return currentSetting().Equals( "1" ) ? PR_TRUE : PR_FALSE;
}
