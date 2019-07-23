





































#pragma once

#include "DotNETEmbed.h"

using namespace System;

namespace Mozilla 
{
  namespace Embedding
  {
    
    public __gc class ProfileManager
    {
    public:
      
      static UInt32 SHUTDOWN_PERSIST = 1;
      static UInt32 SHUTDOWN_CLEANSE = 2;

      __property static Int32 get_ProfileCount();
      static String *GetProfileList()[];
      static bool ProfileExists(String *aProfileName);
      __property static String* get_CurrentProfile();
      __property static void set_CurrentProfile(String* aCurrentProfile);
      static void ShutDownCurrentProfile(UInt32 shutDownType);
      static void CreateNewProfile(String* aProfileName,
                                   String *aNativeProfileDir,
                                   String* aLangcode, bool useExistingDir);
      static void RenameProfile(String* aOldName, String* aNewName);
      static void DeleteProfile(String* aName, bool aCanDeleteFiles);
      static void CloneProfile(String* aProfileName);

    private:
      static nsIProfile *sProfileService = 0; 

      static void EnsureProfileService();
    }; 
  } 
}
