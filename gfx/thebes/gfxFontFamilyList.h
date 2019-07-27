




#ifndef GFX_FONT_FAMILY_LIST_H
#define GFX_FONT_FAMILY_LIST_H

#include "nsDebug.h"
#include "nsISupportsImpl.h"
#include "nsString.h"
#include "nsTArray.h"
#include "mozilla/MemoryReporting.h"

namespace mozilla {





 

enum FontFamilyType {
  eFamily_none = 0,  

  
  eFamily_named,
  eFamily_named_quoted,

  
  eFamily_serif,
  eFamily_sans_serif,
  eFamily_monospace,
  eFamily_cursive,
  eFamily_fantasy,

  
  eFamily_moz_variable,
  eFamily_moz_fixed
};

enum QuotedName { eQuotedName, eUnquotedName };






struct FontFamilyName MOZ_FINAL {
    FontFamilyName()
        : mType(eFamily_named)
    {}

    
    FontFamilyName(const nsAString& aFamilyName,
                   QuotedName aQuoted = eUnquotedName) {
        mType = (aQuoted == eQuotedName) ? eFamily_named_quoted : eFamily_named;
        mName = aFamilyName;
    }

    
    FontFamilyName(FontFamilyType aType) {
        NS_ASSERTION(aType != eFamily_named &&
                     aType != eFamily_named_quoted &&
                     aType != eFamily_none,
                     "expected a generic font type");
        mName.Truncate();
        mType = aType;
    }

    FontFamilyName(const FontFamilyName& aCopy) {
        mType = aCopy.mType;
        mName = aCopy.mName;
    }

    bool IsNamed() const {
        return mType == eFamily_named || mType == eFamily_named_quoted;
    }

    bool IsGeneric() const {
        return !IsNamed();
    }

    void AppendToString(nsAString& aFamilyList, bool aQuotes = true) const {
        switch (mType) {
            case eFamily_named:
                aFamilyList.Append(mName);
                break;
            case eFamily_named_quoted:
                if (aQuotes) {
                    aFamilyList.Append('"');
                }
                aFamilyList.Append(mName);
                if (aQuotes) {
                    aFamilyList.Append('"');
                }
                break;
            case eFamily_serif:
                aFamilyList.AppendLiteral("serif");
                break;
            case eFamily_sans_serif:
                aFamilyList.AppendLiteral("sans-serif");
                break;
            case eFamily_monospace:
                aFamilyList.AppendLiteral("monospace");
                break;
            case eFamily_cursive:
                aFamilyList.AppendLiteral("cursive");
                break;
            case eFamily_fantasy:
                aFamilyList.AppendLiteral("fantasy");
                break;
            case eFamily_moz_fixed:
                aFamilyList.AppendLiteral("-moz-fixed");
                break;
            default:
                break;
        }
    }

    
    static FontFamilyName
    Convert(const nsAString& aFamilyOrGenericName) {
        
        
        
        NS_ASSERTION(aFamilyOrGenericName.FindChar(',') == -1,
                     "Convert method should only be passed a single family name");

        FontFamilyType genericType = eFamily_none;
        if (aFamilyOrGenericName.LowerCaseEqualsLiteral("serif")) {
            genericType = eFamily_serif;
        } else if (aFamilyOrGenericName.LowerCaseEqualsLiteral("sans-serif")) {
            genericType = eFamily_sans_serif;
        } else if (aFamilyOrGenericName.LowerCaseEqualsLiteral("monospace")) {
            genericType = eFamily_monospace;
        } else if (aFamilyOrGenericName.LowerCaseEqualsLiteral("cursive")) {
            genericType = eFamily_cursive;
        } else if (aFamilyOrGenericName.LowerCaseEqualsLiteral("fantasy")) {
            genericType = eFamily_fantasy;
        } else if (aFamilyOrGenericName.LowerCaseEqualsLiteral("-moz-fixed")) {
            genericType = eFamily_moz_fixed;
        } else {
            return FontFamilyName(aFamilyOrGenericName, eUnquotedName);
        }

        return FontFamilyName(genericType);
    }

    
    size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const {
        return mName.SizeOfExcludingThisIfUnshared(aMallocSizeOf);
    }

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const {
        return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
    }

    FontFamilyType mType;
    nsString       mName; 
};

inline bool
operator==(const FontFamilyName& a, const FontFamilyName& b) {
    return a.mType == b.mType && a.mName == b.mName;
}





 

class FontFamilyList {
public:
    FontFamilyList()
        : mDefaultFontType(eFamily_none)
    {
    }

    FontFamilyList(FontFamilyType aGenericType)
        : mDefaultFontType(eFamily_none)
    {
        Append(FontFamilyName(aGenericType));
    }

    FontFamilyList(const nsAString& aFamilyName,
                   QuotedName aQuoted)
        : mDefaultFontType(eFamily_none)
    {
        Append(FontFamilyName(aFamilyName, aQuoted));
    }

    FontFamilyList(const FontFamilyList& aOther)
        : mFontlist(aOther.mFontlist)
        , mDefaultFontType(aOther.mDefaultFontType)
    {
    }

    void Append(const FontFamilyName& aFamilyName) {
        mFontlist.AppendElement(aFamilyName);
    }

    void Append(const nsTArray<nsString>& aFamilyNameList) {
        uint32_t len = aFamilyNameList.Length();
        for (uint32_t i = 0; i < len; i++) {
            mFontlist.AppendElement(FontFamilyName(aFamilyNameList[i],
                                                   eUnquotedName));
        }
    }

    void Clear() {
        mFontlist.Clear();
    }

    uint32_t Length() const {
        return mFontlist.Length();
    }

    bool IsEmpty() const {
      return mFontlist.IsEmpty();
    }

    const nsTArray<FontFamilyName>& GetFontlist() const {
        return mFontlist;
    }

    bool Equals(const FontFamilyList& aFontlist) const {
        return mFontlist == aFontlist.mFontlist &&
               mDefaultFontType == aFontlist.mDefaultFontType;
    }

    FontFamilyType FirstGeneric() const {
        uint32_t len = mFontlist.Length();
        for (uint32_t i = 0; i < len; i++) {
            const FontFamilyName& name = mFontlist[i];
            if (name.IsGeneric()) {
                return name.mType;
            }
        }
        return eFamily_none;
    }

    bool HasGeneric() const {
        return FirstGeneric() != eFamily_none;
    }

    bool HasDefaultGeneric() const {
        uint32_t len = mFontlist.Length();
        for (uint32_t i = 0; i < len; i++) {
            const FontFamilyName& name = mFontlist[i];
            if (name.mType == mDefaultFontType) {
                return true;
            }
        }
        return false;
    }

    void ToString(nsAString& aFamilyList,
                  bool aQuotes = true,
                  bool aIncludeDefault = false) const {
        aFamilyList.Truncate();
        uint32_t len = mFontlist.Length();
        for (uint32_t i = 0; i < len; i++) {
            if (i != 0) {
                aFamilyList.Append(',');
            }
            const FontFamilyName& name = mFontlist[i];
            name.AppendToString(aFamilyList, aQuotes);
        }
        if (aIncludeDefault && mDefaultFontType != eFamily_none) {
            if (!aFamilyList.IsEmpty()) {
                aFamilyList.Append(',');
            }
            if (mDefaultFontType == eFamily_serif) {
                aFamilyList.AppendLiteral("serif");
            } else {
                aFamilyList.AppendLiteral("sans-serif");
            }
        }
    }

    FontFamilyType GetDefaultFontType() const { return mDefaultFontType; }
    void SetDefaultFontType(FontFamilyType aType) {
        NS_ASSERTION(aType == eFamily_none || aType == eFamily_serif ||
                     aType == eFamily_sans_serif,
                     "default font type must be either serif or sans-serif");
        mDefaultFontType = aType;
    }

    
    size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const {
        return mFontlist.SizeOfExcludingThis(aMallocSizeOf);
    }

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const {
        return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
    }

private:
    nsTArray<FontFamilyName>   mFontlist;
    FontFamilyType             mDefaultFontType; 
};

inline bool
operator==(const FontFamilyList& a, const FontFamilyList& b) {
    return a.Equals(b);
}

} 

#endif 
