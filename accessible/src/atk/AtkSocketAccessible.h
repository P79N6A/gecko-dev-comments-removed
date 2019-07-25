






































#ifndef _AtkSocketAccessible_H_
#define _AtkSocketAccessible_H_

#include "nsAccessibleWrap.h"



#ifdef __ATK_H__
typedef void (*AtkSocketEmbedType) (AtkSocket*, gchar*);
#else
typedef void (*AtkSocketEmbedType) (void*, void*);
#endif





class AtkSocketAccessible: public nsAccessibleWrap
{
public:

  
  static AtkSocketEmbedType g_atk_socket_embed;
#ifdef __ATK_H__
  static GType g_atk_socket_type;
#endif
  static const char* sATKSocketEmbedSymbol;
  static const char* sATKSocketGetTypeSymbol;

  



  static bool gCanEmbed;

  AtkSocketAccessible(nsIContent* aContent, nsIWeakReference* aShell,
                      const nsCString& aPlugId);

  
  virtual void Shutdown();

  
  NS_IMETHODIMP GetNativeInterface(void** aOutAccessible);
};

#endif
