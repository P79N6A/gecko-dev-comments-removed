






#ifndef SkOTUtils_DEFINED
#define SkOTUtils_DEFINED

#include "SkOTTableTypes.h"
#include "SkOTTable_name.h"
#include "SkTypeface.h"

class SkData;
class SkStream;

struct SkOTUtils {
    


    static uint32_t CalcTableChecksum(SK_OT_ULONG *data, size_t length);

    












    static SkData* RenameFont(SkStream* fontData, const char* fontName, int fontNameLen);

    
    class LocalizedStrings_NameTable : public SkTypeface::LocalizedStrings {
    public:
        
        LocalizedStrings_NameTable(SkOTTableName* nameTableData,
                                   SkOTTableName::Record::NameID::Predefined::Value types[],
                                   int typesCount)
            : fTypes(types), fTypesCount(typesCount), fTypesIndex(0)
            , fNameTableData(nameTableData), fFamilyNameIter(*nameTableData, fTypes[fTypesIndex])
        { }

        


        static LocalizedStrings_NameTable* CreateForFamilyNames(const SkTypeface& typeface);

        virtual bool next(SkTypeface::LocalizedString* localizedString) SK_OVERRIDE;
    private:
        static SkOTTableName::Record::NameID::Predefined::Value familyNameTypes[3];

        SkOTTableName::Record::NameID::Predefined::Value* fTypes;
        int fTypesCount;
        int fTypesIndex;
        SkAutoTDeleteArray<SkOTTableName> fNameTableData;
        SkOTTableName::Iterator fFamilyNameIter;
    };

    
    class LocalizedStrings_SingleName : public SkTypeface::LocalizedStrings {
    public:
        LocalizedStrings_SingleName(SkString name, SkString language)
            : fName(name), fLanguage(language), fHasNext(true)
        { }

        virtual bool next(SkTypeface::LocalizedString* localizedString) SK_OVERRIDE {
            localizedString->fString = fName;
            localizedString->fLanguage = fLanguage;

            bool hadNext = fHasNext;
            fHasNext = false;
            return hadNext;
        }

    private:
        SkString fName;
        SkString fLanguage;
        bool fHasNext;
    };
};

#endif
