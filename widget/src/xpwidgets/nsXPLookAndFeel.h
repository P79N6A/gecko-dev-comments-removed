




































#ifndef __nsXPLookAndFeel
#define __nsXPLookAndFeel

#include "nsILookAndFeel.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "nsIPrefBranch.h"

#ifdef NS_DEBUG
struct nsSize;
#endif

typedef enum {
  nsLookAndFeelTypeInt,
  nsLookAndFeelTypeFloat,
  nsLookAndFeelTypeColor
} nsLookAndFeelType;

struct nsLookAndFeelIntPref
{
  const char* name;
  nsILookAndFeel::nsMetricID id;
  PRPackedBool isSet;
  nsLookAndFeelType type;
  PRInt32 intVar;
};

struct nsLookAndFeelFloatPref
{
  const char* name;
  nsILookAndFeel::nsMetricFloatID id;
  PRPackedBool isSet;
  nsLookAndFeelType type;
  float floatVar;
};

#define CACHE_BLOCK(x)     ((x) >> 5)
#define CACHE_BIT(x)       (1 << ((x) & 31))

#define COLOR_CACHE_SIZE   (CACHE_BLOCK(nsILookAndFeel::eColor_LAST_COLOR) + 1)
#define IS_COLOR_CACHED(x) (CACHE_BIT(x) & nsXPLookAndFeel::sCachedColorBits[CACHE_BLOCK(x)])
#define CACHE_COLOR(x, y)  nsXPLookAndFeel::sCachedColors[(x)] = y; \
              nsXPLookAndFeel::sCachedColorBits[CACHE_BLOCK(x)] |= CACHE_BIT(x);

class nsXPLookAndFeel: public nsILookAndFeel, public nsIObserver
{
public:
  nsXPLookAndFeel();
  virtual ~nsXPLookAndFeel();

  NS_DECL_ISUPPORTS

  NS_DECL_NSIOBSERVER

  void Init();

  
  
  
  
  
  
  NS_IMETHOD GetColor(const nsColorID aID, nscolor &aColor);
  NS_IMETHOD GetMetric(const nsMetricID aID, PRInt32 & aMetric);
  NS_IMETHOD GetMetric(const nsMetricFloatID aID, float & aMetric);

  NS_IMETHOD LookAndFeelChanged();

#ifdef NS_DEBUG
  NS_IMETHOD GetNavSize(const nsMetricNavWidgetID aWidgetID,
                        const nsMetricNavFontID   aFontID, 
                        const PRInt32             aFontSize, 
                        nsSize &aSize);
#endif

protected:
  void IntPrefChanged(nsLookAndFeelIntPref *data);
  void FloatPrefChanged(nsLookAndFeelFloatPref *data);
  void ColorPrefChanged(unsigned int index, const char *prefName);
  void InitFromPref(nsLookAndFeelIntPref* aPref, nsIPrefBranch* aPrefBranch);
  void InitFromPref(nsLookAndFeelFloatPref* aPref, nsIPrefBranch* aPrefBranch);
  void InitColorFromPref(PRInt32 aIndex, nsIPrefBranch* aPrefBranch);
  virtual nsresult NativeGetColor(const nsColorID aID, nscolor& aColor) = 0;

  static PRBool sInitialized;
  static nsLookAndFeelIntPref sIntPrefs[];
  static nsLookAndFeelFloatPref sFloatPrefs[];
  


  static const char sColorPrefs[][38];
  static PRInt32 sCachedColors[nsILookAndFeel::eColor_LAST_COLOR];
  static PRInt32 sCachedColorBits[COLOR_CACHE_SIZE];
};

extern nsresult NS_NewXPLookAndFeel(nsILookAndFeel**);

#endif
