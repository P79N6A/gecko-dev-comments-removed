






































#include <windows.h>


const char * const mozillaKeyName = "Software\\Mozilla\\Desktop";

static const char shortcutSuffix[] = " -url \"%1\"";
static const char chromeSuffix[] = " -chrome \"%1\"";
static const char iconSuffix[] = ",0";


static nsCString thisApplication() {
    static nsCAutoString result;

    if ( result.IsEmpty() ) {
        char buffer[MAX_PATH] = { 0 };
    	DWORD len = ::GetModuleFileName( NULL, buffer, sizeof buffer );
        len = ::GetShortPathName( buffer, buffer, sizeof buffer );
    
        result = buffer;
        ToUpperCase(result);
    }

    return result;
}



static nsCString shortAppName() {
    static nsCAutoString result;
    
    if ( result.IsEmpty() ) { 
        
        nsCAutoString thisApp( thisApplication() );
        PRInt32 n = thisApp.RFind( "\\" );
        if ( n != kNotFound ) {
            
            result = (const char*)thisApp.get() + n + 1;
        } else {
            
            result = thisApp;
        }
    }

    return result;
}








struct RegistryEntry {
    HKEY        baseKey;   
    PRBool      isNull;    
    nsCString   keyName;   
    nsCString   valueName; 
    nsCString   setting;   
                           
                           

    RegistryEntry( HKEY baseKey, const char* keyName, const char* valueName, const char* setting )
        : baseKey( baseKey ), isNull( setting == 0 ), keyName( keyName ), valueName( valueName ), setting( setting ? setting : "" ) {
    }

    PRBool     isAlreadySet() const;
    nsresult   set();
    nsresult   reset();
    nsCString  currentSetting( PRBool *currentUndefined = 0 ) const;

    
    
    const char* valueNameArg() const {
        return valueName.IsEmpty() ? NULL : valueName.get();
    }

    nsCString  fullName() const;
};







struct BoolRegistryEntry : public RegistryEntry {
    BoolRegistryEntry( const char *name )
        : RegistryEntry( HKEY_LOCAL_MACHINE, mozillaKeyName, name, "1" ) {
    }
    operator PRBool();
};





struct SavedRegistryEntry : public RegistryEntry {
    SavedRegistryEntry( HKEY baseKey, const char *keyName, const char *valueName, const char *setting )
        : RegistryEntry( baseKey, keyName, valueName, setting ) {
    }
    nsresult set();
    nsresult reset();
};








struct ProtocolRegistryEntry : public SavedRegistryEntry {
    nsCString protocol;
    ProtocolRegistryEntry( const char* protocol )
        : SavedRegistryEntry( HKEY_LOCAL_MACHINE, "", "", thisApplication().get() ),
          protocol( protocol ) {
        keyName = "Software\\Classes\\";
        keyName += protocol;
        keyName += "\\shell\\open\\command";

        
        if ( this->protocol.Equals( "chrome" ) || this->protocol.Equals( "MozillaXUL" ) ) {
            
            setting += chromeSuffix;
        } else {
            
            setting += shortcutSuffix;
        }
    }
    nsresult set();
    nsresult reset();
};







struct ProtocolIconRegistryEntry : public SavedRegistryEntry {
    nsCString protocol;
    ProtocolIconRegistryEntry( const char* aprotocol )
        : SavedRegistryEntry( HKEY_LOCAL_MACHINE, "", "", thisApplication().get() ),
          protocol( aprotocol ) {
        keyName = NS_LITERAL_CSTRING("Software\\Classes\\") + protocol + NS_LITERAL_CSTRING("\\DefaultIcon");

        
        setting += iconSuffix;
    }
};
















struct DDERegistryEntry : public SavedRegistryEntry {
    DDERegistryEntry( const char *protocol )
        : SavedRegistryEntry( HKEY_LOCAL_MACHINE, "", "", 0 ),
          activate( HKEY_LOCAL_MACHINE, "", "NoActivateHandler", 0 ),
          app( HKEY_LOCAL_MACHINE, "", "", 0 ),
          topic( HKEY_LOCAL_MACHINE, "", "", 0 ) {
        
        keyName = "Software\\Classes\\";
        keyName += protocol;
        keyName += "\\shell\\open\\ddeexec";
        
        activate.keyName = keyName;
        app.keyName = keyName;
        app.keyName += "\\Application";
        topic.keyName = keyName;
        topic.keyName += "\\Topic";
    }
    nsresult set();
    nsresult reset();
    SavedRegistryEntry activate, app, topic;
};








struct FileTypeRegistryEntry : public ProtocolRegistryEntry {
    nsCString fileType;
    const char **ext;
    nsCString desc;
    nsCString defDescKey;
    nsCString iconFile;
    FileTypeRegistryEntry ( const char **ext, const char *fileType, 
        const char *desc, const char *defDescKey, const char *iconFile )
        : ProtocolRegistryEntry( fileType ),
          fileType( fileType ),
          ext( ext ),
          desc( desc ),
          defDescKey(defDescKey),
          iconFile(iconFile) {
    }
    nsresult set();
    nsresult reset();
};




struct EditableFileTypeRegistryEntry : public FileTypeRegistryEntry {
    EditableFileTypeRegistryEntry( const char **ext, const char *fileType, 
        const char *desc, const char *defDescKey, const char *iconFile )
        : FileTypeRegistryEntry( ext, fileType, desc, defDescKey, iconFile ) {
    }
    nsresult set();
};
