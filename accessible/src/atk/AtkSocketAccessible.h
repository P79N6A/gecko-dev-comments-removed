





#ifndef _AtkSocketAccessible_H_
#define _AtkSocketAccessible_H_

#include "AccessibleWrap.h"



#ifdef __ATK_H__
extern "C" typedef void (*AtkSocketEmbedType) (AtkSocket*, gchar*);
#else
extern "C" typedef void (*AtkSocketEmbedType) (void*, void*);
#endif

namespace mozilla {
namespace a11y {





class AtkSocketAccessible : public AccessibleWrap
{
public:

  
  static AtkSocketEmbedType g_atk_socket_embed;
#ifdef __ATK_H__
  static GType g_atk_socket_type;
#endif
  static const char* sATKSocketEmbedSymbol;
  static const char* sATKSocketGetTypeSymbol;

  



  static bool gCanEmbed;

  AtkSocketAccessible(nsIContent* aContent, DocAccessible* aDoc,
                      const nsCString& aPlugId);

  
  virtual void Shutdown();

  
  NS_IMETHODIMP GetNativeInterface(void** aOutAccessible);
};

} 
} 

#endif
