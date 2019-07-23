




































#include <stdio.h>

#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsCOMPtr.h"
#include "nsIRegistry.h"
#include "nsIEnumerator.h"
#include "nsILocalFile.h"
#include "nsDependentString.h"
#include "prmem.h"
#include "plstr.h"
#include "nsMemory.h"

static void display( nsIRegistry *reg, nsRegistryKey root, const char *name );
static void displayValues( nsIRegistry *reg, nsRegistryKey root );
static void printString( const char *value, int indent );

int main( int argc, char *argv[] ) {


#ifdef __MWERKS__
    
    
    argc = 1;
    const char* myArgs[] =
    {
        "regExport"
    };
    argv = const_cast<char**>(myArgs);
#endif

    nsresult rv;

    
    nsIServiceManager *servMgr = NULL;
    rv = NS_InitXPCOM2(&servMgr, NULL, NULL);
    if (NS_FAILED(rv))
    {
        
        printf("Cannot initialize XPCOM. Exit. [rv=0x%08X]\n", (int)rv);
        exit(-1);
    }
    {
        
        static NS_DEFINE_CID(kComponentManagerCID, NS_COMPONENTMANAGER_CID);
        nsCOMPtr<nsIComponentManager> compMgr = do_GetService(kComponentManagerCID, &rv);
        if (NS_FAILED(rv))
        {
            
            printf("Cannot get component manager from service manager.. Exit. [rv=0x%08X]\n", (int)rv);
            exit(-1);
        }

        nsIRegistry *reg;

        if (argc>1) {
            
            rv = compMgr->CreateInstanceByContractID(NS_REGISTRY_CONTRACTID, NULL,
                                                 NS_GET_IID(nsIRegistry),
                                                 (void **) &reg);
            
            if ( NS_FAILED(rv) )
    	    {   
                printf( "Error opening registry file %s, rv=0x%08X\n", argv[1] , (int)rv );
                return rv;
		    }
            
            nsCOMPtr<nsILocalFile> regFile;
            rv = NS_NewNativeLocalFile( nsDependentCString(argv[1]), PR_FALSE, getter_AddRefs(regFile) );
            if ( NS_FAILED(rv) ) {
                printf( "Error instantiating local file for %s, rv=0x%08X\n", argv[1], (int)rv );
                return rv;
            }

            rv = reg->Open( regFile );
    
            if ( rv == NS_OK ) 
            {
                printf( "Registry %s opened OK.\n", argv[1] );
            
                
                display( reg, nsIRegistry::Common, "nsRegistry::Common" );
                display( reg, nsIRegistry::Users, "nsRegistry::Users" );
            }
            NS_RELEASE(reg);
        }
        else
        {
            
            
            
            rv = compMgr->CreateInstanceByContractID(NS_REGISTRY_CONTRACTID, NULL,
                                                 NS_GET_IID(nsIRegistry),
                                                 (void **) &reg);

            
            if ( NS_FAILED(rv) )
            {   
                printf( "Error opening creating registry instance, rv=0x%08X\n", (int)rv );
                return rv;
            }
            rv = reg->OpenWellKnownRegistry(nsIRegistry::ApplicationComponentRegistry);
            if ( rv == NS_ERROR_REG_BADTYPE ) {
                printf( "\n\n\nThere is no <Application Component Registry>\n" );
            }
            else if ( rv == NS_OK ) {

                printf( "\n\n\nRegistry %s opened OK.\n", "<Application Component Registry>\n" );
            
                
                display( reg, nsIRegistry::Common, "nsRegistry::Common" );
                display( reg, nsIRegistry::Users, "nsRegistry::Users" );
            }
            NS_RELEASE(reg);
        }
    }
    NS_ShutdownXPCOM( servMgr );

    return rv;
}

void display( nsIRegistry *reg, nsRegistryKey root, const char *rootName ) {
    
    printf( "%s\n", rootName );

    
    if ( root != nsIRegistry::Common
         &&
         root != nsIRegistry::Users
         &&
         root != nsIRegistry::CurrentUser ) {
        
        displayValues( reg, root );
    }

    
    nsIEnumerator *keys;
    nsresult rv = reg->EnumerateSubtrees( root, &keys );

    
    if ( rv == NS_OK ) {
        
        rv = keys->First();
        
        while( NS_SUCCEEDED( rv ) && (NS_OK != keys->IsDone()) ) {
            nsISupports *base;
            rv = keys->CurrentItem( &base );
            
            if ( rv == NS_OK ) {
                
                nsIRegistryNode *node;
                nsIID nodeIID = NS_IREGISTRYNODE_IID;
                rv = base->QueryInterface( nodeIID, (void**)&node );
                
                if ( rv == NS_OK ) {
                    
                    char *name;
                    rv = node->GetNameUTF8( &name );
                    
                    if ( rv == NS_OK ) {
                        
                        char *fullName = new char[ PL_strlen(rootName) + PL_strlen(name) + 5 ];
                        PL_strcpy( fullName, rootName );
                        PL_strcat( fullName, " -  " );
                        PL_strcat( fullName, name );
                        
                        nsRegistryKey key;
                        rv = reg->GetSubtreeRaw( root, name, &key );
                        if ( rv == NS_OK ) {
                            display( reg, key, fullName );
                            printf( "\n" );
                        } else {
                            printf( "Error getting key, rv=0x%08X\n", (int)rv );
                        }
                        delete [] fullName;
                    } else {
                        printf( "Error getting subtree name, rv=0x%08X\n", (int)rv );
                    }
                    
                    node->Release();
                } else {
                    printf( "Error converting base node ptr to nsIRegistryNode, rv=0x%08X\n", (int)rv );
                }
                
                base->Release();

                
                rv = keys->Next();
                
                if ( NS_SUCCEEDED( rv ) ) {
                } else {
                    printf( "Error advancing enumerator, rv=0x%08X\n", (int)rv );
                }
            } else {
                printf( "Error getting current item, rv=0x%08X\n", (int)rv );
            }
        }
        
        keys->Release();
    } else {
        printf( "Error creating enumerator for %s, root=0x%08X, rv=0x%08X\n",
                rootName, (int)root, (int)rv );
    }
    return;
}

static void displayValues( nsIRegistry *reg, nsRegistryKey root ) {
    
    nsIEnumerator *values;
    nsresult rv = reg->EnumerateValues( root, &values );

    
    if ( rv == NS_OK ) {
        
        rv = values->First();

        
        while( rv == NS_OK && (NS_OK != values->IsDone()) ) {
            nsISupports *base;
            rv = values->CurrentItem( &base );
            
            if ( rv == NS_OK ) {
                
                nsIRegistryValue *value;
                nsIID valueIID = NS_IREGISTRYVALUE_IID;
                rv = base->QueryInterface( valueIID, (void**)&value );
                
                if ( rv == NS_OK ) {
                    
                    char *name;
                    rv = value->GetNameUTF8( &name );
                    
                    if ( rv == NS_OK ) {
                        
                        printf( "\t\t%s", name );
                        
                        PRUint32 type;
                        rv = reg->GetValueType( root, name, &type );
                        if ( rv == NS_OK ) {
                            
                            switch ( type ) {
                                case nsIRegistry::String: {
                                        char *strValue;
                                        rv = reg->GetStringUTF8( root, name, &strValue );
                                        if ( rv == NS_OK ) {
                                            printString( strValue, strlen(name) );
                                            nsMemory::Free( strValue );
                                        } else {
                                            printf( "\t Error getting string value, rv=0x%08X", (int)rv );
                                        }
                                    }
                                    break;

                                case nsIRegistry::Int32:
                                    {
                                        PRInt32 val = 0;
                                        rv = reg->GetInt( root, name, &val );
                                        if (NS_SUCCEEDED(rv)) {
                                            printf( "\t= Int32 [%d, 0x%x]", val, val);
                                        }
                                        else {
                                            printf( "\t Error getting int32 value, rv=%08X", (int)rv);
                                        }
                                    }
                                    break;

                                case nsIRegistry::Bytes:
                                    printf( "\t= Bytes" );
                                    break;

                                case nsIRegistry::File:
                                    printf( "\t= File (?)" );
                                    break;

                                default:
                                    printf( "\t= ? (unknown type=0x%02X)", (int)type );
                                    break;
                            }
                        } else {
                            printf( "\t= ? (error getting value, rv=0x%08X)", (int)rv );
                        }
                        printf("\n");
                        nsMemory::Free( name );
                    } else {
                        printf( "Error getting value name, rv=0x%08X\n", (int)rv );
                    }
                    
                    value->Release();
                } else {
                    printf( "Error converting base node ptr to nsIRegistryNode, rv=0x%08X\n", (int)rv );
                }
                
                base->Release();

                
                rv = values->Next();
                
                if ( NS_SUCCEEDED( rv ) ) {
                } else {
                    printf( "Error advancing enumerator, rv=0x%08X\n", (int)rv );
                    break;
                }
            } else {
                printf( "Error getting current item, rv=0x%08X\n", (int)rv );
                break;
            }
        }

        values->Release();
    } else {
        printf( "\t\tError enumerating values, rv=0x%08X\n", (int)rv );
    }
    return;
}

static void printString( const char *value, int  ) {
    
    printf( "\t = %s", value );
    return;
}
