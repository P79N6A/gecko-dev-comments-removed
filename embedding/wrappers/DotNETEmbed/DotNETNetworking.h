





































#pragma once

#include "DotNETEmbed.h"


class nsIURI;

namespace Mozilla
{
  namespace Embedding
  {
    namespace Networking
    {
      public __gc class URI : public ICloneable,
                              public IDisposable
      {
      public:
        URI(String *aSpec);
        URI(nsIURI *aURI);
        ~URI();

        
        Object *Clone();

        
        void Dispose();

        __property String* get_Spec();
        __property void set_Spec(String *aSpec);
        __property String* get_PrePath();
        __property String* get_Scheme();
        __property void set_Scheme(String *aScheme);
        __property String* get_UserPass();
        __property void set_UserPass(String *aUserPass);
        __property String* get_Username();
        __property void set_Username(String *aUsername);
        __property String* get_Password();
        __property void set_Password(String *aPassword);
        __property String* get_HostPort();
        __property void set_HostPort(String *aHostPort);
        __property String* get_Host();
        __property void set_Host(String *aHost);
        __property Int32 get_Port();
        __property void set_Port(Int32 aPort);
        __property String* get_Path();
        __property void set_Path(String *aPath);
        bool Equals(URI *aOther); 
        bool SchemeIs(String *aScheme); 
        
        String* Resolve(String *aRelativePath); 
        __property String* get_AsciiSpec();
        __property String* get_AsciiHost();
        __property String* get_OriginCharset();

      private:
        nsIURI *mURI; 
      }; 
    }; 
  } 
}
