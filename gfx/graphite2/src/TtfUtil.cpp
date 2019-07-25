










































#include <cassert>
#include <cstddef>
#include <cstring>
#include <climits>
#include <cwchar>



#include "TtfUtil.h"
#include "TtfTypes.h"
#include "Endian.h"








namespace 
{
	
	const int kMaxGlyphComponents = 8;

	template <int R, typename T>
	inline float fixed_to_float(const T f) {
		return float(f)/float(2^R);
	}




#ifdef ALL_TTFUTILS
	const int kcPostNames = 258;

	const char * rgPostName[kcPostNames] = {
		".notdef", ".null", "nonmarkingreturn", "space", "exclam", "quotedbl", "numbersign", 
		"dollar", "percent", "ampersand", "quotesingle", "parenleft", 
		"parenright", "asterisk", "plus", "comma", "hyphen", "period", "slash", 
		"zero", "one", "two", "three", "four", "five", "six", "seven", "eight", 
		"nine", "colon", "semicolon", "less", "equal", "greater", "question", 
		"at", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", 
		"N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", 
		"bracketleft", "backslash", "bracketright", "asciicircum", 
		"underscore", "grave", "a", "b", "c", "d", "e", "f", "g", "h", "i", 
		"j", "k", "l", "m", "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", 
		"x", "y", "z", "braceleft", "bar", "braceright", "asciitilde", 
		"Adieresis", "Aring", "Ccedilla", "Eacute", "Ntilde", "Odieresis", 
		"Udieresis", "aacute", "agrave", "acircumflex", "adieresis", "atilde", 
		"aring", "ccedilla", "eacute", "egrave", "ecircumflex", "edieresis", 
		"iacute", "igrave", "icircumflex", "idieresis", "ntilde", "oacute", 
		"ograve", "ocircumflex", "odieresis", "otilde", "uacute", "ugrave", 
		"ucircumflex", "udieresis", "dagger", "degree", "cent", "sterling", 
		"section", "bullet", "paragraph", "germandbls", "registered", 
		"copyright", "trademark", "acute", "dieresis", "notequal", "AE", 
		"Oslash", "infinity", "plusminus", "lessequal", "greaterequal", "yen", 
		"mu", "partialdiff", "summation", "product", "pi", "integral", 
		"ordfeminine", "ordmasculine", "Omega", "ae", "oslash", "questiondown", 
		"exclamdown", "logicalnot", "radical", "florin", "approxequal", 
		"Delta", "guillemotleft", "guillemotright", "ellipsis", "nonbreakingspace", 
		"Agrave", "Atilde", "Otilde", "OE", "oe", "endash", "emdash", 
		"quotedblleft", "quotedblright", "quoteleft", "quoteright", "divide", 
		"lozenge", "ydieresis", "Ydieresis", "fraction", "currency", 
		"guilsinglleft", "guilsinglright", "fi", "fl", "daggerdbl", "periodcentered", 
		"quotesinglbase", "quotedblbase", "perthousand", "Acircumflex", 
		"Ecircumflex", "Aacute", "Edieresis", "Egrave", "Iacute", 
		"Icircumflex", "Idieresis", "Igrave", "Oacute", "Ocircumflex", 
		"apple", "Ograve", "Uacute", "Ucircumflex", "Ugrave", "dotlessi", 
		"circumflex", "tilde", "macron", "breve", "dotaccent", "ring", 
		"cedilla", "hungarumlaut", "ogonek", "caron", "Lslash", "lslash", 
		"Scaron", "scaron", "Zcaron", "zcaron", "brokenbar", "Eth", "eth", 
		"Yacute", "yacute", "Thorn", "thorn", "minus", "multiply", 
		"onesuperior", "twosuperior", "threesuperior", "onehalf", "onequarter", 
		"threequarters", "franc", "Gbreve", "gbreve", "Idotaccent", "Scedilla", 
		"scedilla", "Cacute", "cacute", "Ccaron", "ccaron", 
		"dcroat" };
#endif

} 




















namespace graphite2
{
namespace TtfUtil
{







bool GetHeaderInfo(size_t & lOffset, size_t & lSize)
{
	lOffset = 0;
	lSize   = offsetof(Sfnt::OffsetSubTable, table_directory);
	assert(sizeof(uint32) + 4*sizeof (uint16) == lSize);
	return true;
}





bool CheckHeader(const void * pHdr)
{
	const Sfnt::OffsetSubTable * pOffsetTable  
		= reinterpret_cast<const Sfnt::OffsetSubTable *>(pHdr);

	return be::swap(pOffsetTable->scaler_type) == Sfnt::OffsetSubTable::TrueTypeWin;
}





bool GetTableDirInfo(const void * pHdr, size_t & lOffset, size_t & lSize)
{
	const Sfnt::OffsetSubTable * pOffsetTable
		= reinterpret_cast<const Sfnt::OffsetSubTable *>(pHdr);

	lOffset = offsetof(Sfnt::OffsetSubTable, table_directory);
	lSize   = be::swap(pOffsetTable->num_tables)
		* sizeof(Sfnt::OffsetSubTable::Entry);
	
	return true;
}






bool GetTableInfo(const Tag TableTag, const void * pHdr, const void * pTableDir,
						   size_t & lOffset, size_t & lSize)
{
	const Sfnt::OffsetSubTable * pOffsetTable 
		= reinterpret_cast<const Sfnt::OffsetSubTable *>(pHdr);
	const Sfnt::OffsetSubTable::Entry 
		* entry_itr = reinterpret_cast<const Sfnt::OffsetSubTable::Entry *>(
			pTableDir),
		* const  dir_end = entry_itr + be::swap(pOffsetTable->num_tables);

	if (be::swap(pOffsetTable->num_tables) > 40)
		return false;

	for (;entry_itr != dir_end; ++entry_itr) 
	{
		if (be::swap(entry_itr->tag) == TableTag)
		{
			lOffset = be::swap(entry_itr->offset);
			lSize   = be::swap(entry_itr->length);
			return true;
		}
	}

	return false;
}





bool CheckTable(const Tag TableId, const void * pTable, size_t lTableSize)
{
	using namespace Sfnt;
	
	if (pTable == 0) return false;

	switch(TableId)
	{
	case Tag::cmap: 
	{
		const Sfnt::CharacterCodeMap * const pCmap 
			= reinterpret_cast<const Sfnt::CharacterCodeMap *>(pTable);
		return be::swap(pCmap->version) == 0;
	}

	case Tag::head: 
	{
		const Sfnt::FontHeader * const pHead 
			= reinterpret_cast<const Sfnt::FontHeader *>(pTable);
		bool r = be::swap(pHead->version) == OneFix
			&& be::swap(pHead->magic_number) == FontHeader::MagicNumber
			&& be::swap(pHead->glyph_data_format)
					== FontHeader::GlypDataFormat 
			&& (be::swap(pHead->index_to_loc_format)
					== FontHeader::ShortIndexLocFormat 
				|| be::swap(pHead->index_to_loc_format)
					== FontHeader::LongIndexLocFormat) 
			&& sizeof(FontHeader) <= lTableSize;
		return r;
	}

	case Tag::post: 
	{
		const Sfnt::PostScriptGlyphName * const pPost 
			= reinterpret_cast<const Sfnt::PostScriptGlyphName *>(pTable);
		const fixed format = be::swap(pPost->format);
		bool r = format == PostScriptGlyphName::Format1 
			|| format == PostScriptGlyphName::Format2 
			|| format == PostScriptGlyphName::Format3 
			|| format == PostScriptGlyphName::Format25;
		return r;
	}

	case Tag::hhea: 
	{
		const Sfnt::HorizontalHeader * pHhea = 
			reinterpret_cast<const Sfnt::HorizontalHeader *>(pTable);
		bool r = be::swap(pHhea->version) == OneFix
			&& be::swap(pHhea->metric_data_format) == 0
			&& sizeof (Sfnt::HorizontalHeader) <= lTableSize;
		return r;
	}

	case Tag::maxp: 
	{
		const Sfnt::MaximumProfile * pMaxp = 
			reinterpret_cast<const Sfnt::MaximumProfile *>(pTable);
		bool r = be::swap(pMaxp->version) == OneFix
			&& sizeof(Sfnt::MaximumProfile) <= lTableSize;
		return r;
	}

	case Tag::OS_2: 
	{
		const Sfnt::Compatibility * pOs2 
			= reinterpret_cast<const Sfnt::Compatibility *>(pTable);
		if (be::swap(pOs2->version) == 0)
		{ 



			if (sizeof(Sfnt::Compatibility0) <= lTableSize)
				return true;
		}
		else if (be::swap(pOs2->version) == 1)
		{ 



			if (sizeof(Sfnt::Compatibility1) <= lTableSize)
				return true;
		}
		else if (be::swap(pOs2->version) == 2)
		{ 
			if (sizeof(Sfnt::Compatibility2) <= lTableSize)
				return true;
		}
		else if (be::swap(pOs2->version) == 3 || be::swap(pOs2->version) == 4)
		{ 
			if (sizeof(Sfnt::Compatibility3) <= lTableSize)
				return true;
		}
		else
			return false;
		break;
	}

	case Tag::name:
	{
		const Sfnt::FontNames * pName 
			= reinterpret_cast<const Sfnt::FontNames *>(pTable);
		return be::swap(pName->format) == 0;
	}

	default:
		break;
	}

	return true;
}






size_t GlyphCount(const void * pMaxp)
{
	const Sfnt::MaximumProfile * pTable = 
			reinterpret_cast<const Sfnt::MaximumProfile *>(pMaxp);
	return be::swap(pTable->num_glyphs);
}

#ifdef ALL_TTFUTILS





size_t  MaxCompositeComponentCount(const void * pMaxp)
{
	const Sfnt::MaximumProfile * pTable = 
			reinterpret_cast<const Sfnt::MaximumProfile *>(pMaxp);
	return be::swap(pTable->max_component_elements);
}








size_t  MaxCompositeLevelCount(const void * pMaxp)
{
	const Sfnt::MaximumProfile * pTable = 
			reinterpret_cast<const Sfnt::MaximumProfile *>(pMaxp);
	return be::swap(pTable->max_component_depth);
}







size_t LocaGlyphCount(size_t lLocaSize, const void * pHead) 
{

	const Sfnt::FontHeader * pTable 
		= reinterpret_cast<const Sfnt::FontHeader *>(pHead);

	if (be::swap(pTable->index_to_loc_format)
		== Sfnt::FontHeader::ShortIndexLocFormat)
	
		return (lLocaSize >> 1) - 1;
	
	if (be::swap(pTable->index_to_loc_format)
		== Sfnt::FontHeader::LongIndexLocFormat)
	 
		return (lLocaSize >> 2) - 1;

	return -1;
	
}
#endif




int DesignUnits(const void * pHead)
{
	const Sfnt::FontHeader * pTable = 
			reinterpret_cast<const Sfnt::FontHeader *>(pHead);
	
	return be::swap(pTable->units_per_em);
}

#ifdef ALL_TTFUTILS



int HeadTableCheckSum(const void * pHead)
{
	const Sfnt::FontHeader * pTable = 
			reinterpret_cast<const Sfnt::FontHeader *>(pHead);
	
	return be::swap(pTable->check_sum_adjustment);
}







void HeadTableCreateTime(const void * pHead,
	unsigned int * pnDateBC, unsigned int * pnDateAD)
{
	const Sfnt::FontHeader * pTable = 
			reinterpret_cast<const Sfnt::FontHeader *>(pHead);
	
	*pnDateBC = be::swap(pTable->created[0]);
	*pnDateAD = be::swap(pTable->created[1]);
}







void HeadTableModifyTime(const void * pHead,
	unsigned int * pnDateBC, unsigned int *pnDateAD)
{
	const Sfnt::FontHeader * pTable = 
			reinterpret_cast<const Sfnt::FontHeader *>(pHead);
	
	*pnDateBC = be::swap(pTable->modified[0]);
	*pnDateAD = be::swap(pTable->modified[1]);
}




bool IsItalic(const void * pHead)
{
	const Sfnt::FontHeader * pTable = 
			reinterpret_cast<const Sfnt::FontHeader *>(pHead);

	return ((be::swap(pTable->mac_style) & 0x00000002) != 0);
}




int FontAscent(const void * pOs2)
{
	const Sfnt::Compatibility * pTable = reinterpret_cast<const Sfnt::Compatibility *>(pOs2);

	return be::swap(pTable->win_ascent);
}




int FontDescent(const void * pOs2)
{
	const Sfnt::Compatibility * pTable = reinterpret_cast<const Sfnt::Compatibility *>(pOs2);

	return be::swap(pTable->win_descent);
}








bool FontOs2Style(const void *pOs2, bool & fBold, bool & fItalic)
{
	const Sfnt::Compatibility * pTable = reinterpret_cast<const Sfnt::Compatibility *>(pOs2);

	fBold = (be::swap(pTable->fs_selection) & Sfnt::Compatibility::Bold) != 0;
	fItalic = (be::swap(pTable->fs_selection) & Sfnt::Compatibility::Italic) != 0;
	
	return true;
}
#endif




bool GetNameInfo(const void * pName, int nPlatformId, int nEncodingId,
		int nLangId, int nNameId, size_t & lOffset, size_t & lSize)
{
	lOffset = 0;
	lSize = 0;

	const Sfnt::FontNames * pTable = reinterpret_cast<const Sfnt::FontNames *>(pName);
	uint16 cRecord = be::swap(pTable->count);
	uint16 nRecordOffset = be::swap(pTable->string_offset);
	const Sfnt::NameRecord * pRecord = reinterpret_cast<const Sfnt::NameRecord *>(pTable + 1);

	for (int i = 0; i < cRecord; ++i)
	{
		if (be::swap(pRecord->platform_id) == nPlatformId &&
			be::swap(pRecord->platform_specific_id) == nEncodingId &&
			be::swap(pRecord->language_id) == nLangId &&
			be::swap(pRecord->name_id) == nNameId)
		{
			lOffset = be::swap(pRecord->offset) + nRecordOffset;
			lSize = be::swap(pRecord->length);
			return true;
		}
		pRecord++;
	}

	return false;
}

#ifdef ALL_TTFUTILS





int GetLangsForNames(const void * pName, int nPlatformId, int nEncodingId,
		int * nameIdList, int cNameIds, short * langIdList)
{
	const Sfnt::FontNames * pTable = reinterpret_cast<const Sfnt::FontNames *>(pName);
        int cLangIds = 0;
	uint16 cRecord = be::swap(pTable->count);
        if (cRecord > 127) return cLangIds;
	
	const Sfnt::NameRecord * pRecord = reinterpret_cast<const Sfnt::NameRecord *>(pTable + 1);

	for (int i = 0; i < cRecord; ++i)
	{
		if (be::swap(pRecord->platform_id) == nPlatformId &&
			be::swap(pRecord->platform_specific_id) == nEncodingId)
		{
			bool fNameFound = false;
			int nLangId = be::swap(pRecord->language_id);
			int nNameId = be::swap(pRecord->name_id);
			for (int j = 0; j < cNameIds; j++)
			{
				if (nNameId == nameIdList[j])
				{
					fNameFound = true;
					break;
				}
			}
			if (fNameFound)
			{
				
				int ilang;
				for (ilang = 0; ilang < cLangIds; ilang++)
					if (langIdList[ilang] == nLangId)
						break;
				if (ilang >= cLangIds)
				{
					langIdList[cLangIds] = short(nLangId);
					cLangIds++;
				}
				if (cLangIds == 128)
					return cLangIds;
			}
		}
		pRecord++;
	}

	return cLangIds;
}






bool Get31EngFamilyInfo(const void * pName, size_t & lOffset, size_t & lSize)
{
	return GetNameInfo(pName, Sfnt::NameRecord::Microsoft, 1, 1033, 
		Sfnt::NameRecord::Family, lOffset, lSize);
}








bool Get31EngFullFontInfo(const void * pName, size_t & lOffset, size_t & lSize)
{
	return GetNameInfo(pName, Sfnt::NameRecord::Microsoft, 1, 1033, 
		Sfnt::NameRecord::Fullname, lOffset, lSize);
}






bool Get30EngFamilyInfo(const void * pName, size_t & lOffset, size_t & lSize)
{
	return GetNameInfo(pName, Sfnt::NameRecord::Microsoft, 0, 1033, 
		Sfnt::NameRecord::Family, lOffset, lSize);
}








bool Get30EngFullFontInfo(const void * pName, size_t & lOffset, size_t & lSize)
{
	return GetNameInfo(pName, Sfnt::NameRecord::Microsoft, 0, 1033, 
		Sfnt::NameRecord::Fullname, lOffset, lSize);
}












int PostLookup(const void * pPost, size_t lPostSize, const void * pMaxp, 
						const char * pPostName)
{
	using namespace Sfnt;
	
	const Sfnt::PostScriptGlyphName * pTable 
		= reinterpret_cast<const Sfnt::PostScriptGlyphName *>(pPost);
	fixed format = be::swap(pTable->format);

	if (format == PostScriptGlyphName::Format3)
	{ 
		return -2;
	}

	
	int iPostName = -1; 
	for (int i = 0; i < kcPostNames; i++)
	{
		if (!strcmp(pPostName, rgPostName[i]))
		{
			iPostName = i;
			break;
		}
	}

	if (format == PostScriptGlyphName::Format1)
	{ 
		return iPostName;
	}
	
	if (format == PostScriptGlyphName::Format25)
	{ 
		if (iPostName == -1)
			return -1;
		
		const PostScriptGlyphName25 * pTable25 
			= static_cast<const PostScriptGlyphName25 *>(pTable);
		int cnGlyphs = GlyphCount(pMaxp);
		for (gid16 nGlyphId = 0; nGlyphId < cnGlyphs && nGlyphId < kcPostNames; 
				nGlyphId++)
		{ 
		  
			if (nGlyphId + pTable25->offset[nGlyphId] == iPostName)
				return nGlyphId;
		}
	}

	if (format == PostScriptGlyphName::Format2)
	{ 
		const PostScriptGlyphName2 * pTable2 
			= static_cast<const PostScriptGlyphName2 *>(pTable);
		
		int cnGlyphs = be::swap(pTable2->number_of_glyphs);

		if (iPostName != -1)
		{ 
			for (gid16 nGlyphId = 0; nGlyphId < cnGlyphs; nGlyphId++)
			{
				if (be::swap(pTable2->glyph_name_index[nGlyphId]) == iPostName)
					return nGlyphId;
			}
			return -1; 
		}

		else
		{ 
			size_t nStrSizeGoal = strlen(pPostName);
			const char * pFirstGlyphName = reinterpret_cast<const char *>(
				&pTable2->glyph_name_index[0] + cnGlyphs);
			const char * pGlyphName = pFirstGlyphName;
			int iInNames = 0; 
			bool fFound = false;
			const char * const endOfTable 
				= reinterpret_cast<const char *>(pTable2) + lPostSize;
			while (pGlyphName < endOfTable && !fFound) 
			{ 
				size_t nStringSize = size_t(*pGlyphName);
				if (nStrSizeGoal != nStringSize ||
					strncmp(pGlyphName + 1, pPostName, nStringSize))
				{ 
					++iInNames;
					pGlyphName += nStringSize + 1;
				}
				else
				{ 
					fFound = true;
				}
			}
			if (!fFound)
				return -1; 

			iInNames += kcPostNames;
			for (gid16 nGlyphId = 0; nGlyphId < cnGlyphs; nGlyphId++)
			{ 
				if (be::swap(pTable2->glyph_name_index[nGlyphId]) == iInNames)
					return nGlyphId;
			}
			return -1; 
		}
	}

	return -3;
}









void SwapWString(void * pWStr, size_t nSize ) 
{
	if (pWStr == 0)
	{

        return;
	}

	uint16 * pStr = reinterpret_cast<uint16 *>(pWStr);
	uint16 * const pStrEnd = pStr + (nSize == 0 ? wcslen((const wchar_t*)pStr) : nSize);

        for (; pStr != pStrEnd; ++pStr)
          *pStr = be::swap(*pStr);






}
#endif





bool HorMetrics(gid16 nGlyphId, const void * pHmtx, size_t lHmtxSize, const void * pHhea, 
						 int & nLsb, unsigned int & nAdvWid)
{
	const Sfnt::HorizontalMetric * phmtx = 
		reinterpret_cast<const Sfnt::HorizontalMetric *>(pHmtx);

	const Sfnt::HorizontalHeader * phhea = 
		reinterpret_cast<const Sfnt::HorizontalHeader *>(pHhea);

	size_t cLongHorMetrics = be::swap(phhea->num_long_hor_metrics);
	if (nGlyphId < cLongHorMetrics) 
	{	
                if (nGlyphId * sizeof(Sfnt::HorizontalMetric) > lHmtxSize) return false;
		nAdvWid = be::swap(phmtx[nGlyphId].advance_width);
		nLsb = be::swap(phmtx[nGlyphId].left_side_bearing);
	}
	else
	{
		
		size_t lLsbOffset = sizeof(Sfnt::HorizontalMetric) * cLongHorMetrics +
			sizeof(int16) * (nGlyphId - cLongHorMetrics); 
		if (lLsbOffset + 1 >= lHmtxSize) 
		{
			nLsb = 0;
			return false;
		}
                nAdvWid = be::swap(phmtx[cLongHorMetrics - 1].advance_width);
		const int16 * pLsb = reinterpret_cast<const int16 *>(phmtx) + 
			lLsbOffset / sizeof(int16);
		nLsb = be::swap(*pLsb);
	}

	return true;
}






const void * FindCmapSubtable(const void * pCmap, int nPlatformId,  int nEncodingId,  size_t length)
{
    const Sfnt::CharacterCodeMap * pTable = reinterpret_cast<const Sfnt::CharacterCodeMap *>(pCmap);
    uint16 csuPlatforms = be::swap(pTable->num_subtables);
    if (length && (sizeof(Sfnt::CharacterCodeMap) + 8 * (csuPlatforms - 1) > length))
        return NULL;
    for (int i = 0; i < csuPlatforms; i++)
    {
        if (be::swap(pTable->encoding[i].platform_id) == nPlatformId &&
                (nEncodingId == -1 || be::swap(pTable->encoding[i].platform_specific_id) == nEncodingId))
        {
            uint32 offset = be::swap(pTable->encoding[i].offset);
            const uint8 * pRtn = reinterpret_cast<const uint8 *>(pCmap) + offset;
            if (length)
            {
                if (offset > length) return NULL;
                uint16 format = be::swap(*reinterpret_cast<const uint16*>(pRtn));
                if (format == 4)
                {
                    uint16 subTableLength = be::swap(*reinterpret_cast<const uint16*>(pRtn + 2));
                    if (i + 1 == csuPlatforms)
                    {
                        if (subTableLength > length - offset)
                            return NULL;
                    }
                    else if (subTableLength > be::swap(pTable->encoding[i+1].offset))
                        return NULL;
                }
                if (format == 12)
                {
                    uint32 subTableLength = be::swap(*reinterpret_cast<const uint32*>(pRtn + 2));
                    if (i + 1 == csuPlatforms)
                    {
                        if (subTableLength > length - offset)
                            return NULL;
                    }
                    else if (subTableLength > be::swap(pTable->encoding[i+1].offset))
                        return NULL;
                }
            }
            return const_cast<void *>(reinterpret_cast<const void *>(pRtn));
        }
    }

    return 0;
}




bool CheckCmap31Subtable(const void * pCmap31)
{
	const Sfnt::CmapSubTable * pTable = reinterpret_cast<const Sfnt::CmapSubTable *>(pCmap31);
	
	
    if (be::swap(pTable->format) != 4) return false;
    const Sfnt::CmapSubTableFormat4 * pTable4 = reinterpret_cast<const Sfnt::CmapSubTableFormat4 *>(pCmap31);
    uint16 length = be::swap(pTable4->length);
    if (length < sizeof(Sfnt::CmapSubTableFormat4))
        return false;
    uint16 nRanges = be::swap(pTable4->seg_count_x2) >> 1;
    if (length < sizeof(Sfnt::CmapSubTableFormat4) + 4 * nRanges * sizeof(uint16))
        return false;
    
    uint16 chEnd = be::swap(pTable4->end_code[nRanges-1]);
    return (chEnd == 0xFFFF);
}






gid16 Cmap31Lookup(const void * pCmap31, int nUnicodeId, int rangeKey)
{
	const Sfnt::CmapSubTableFormat4 * pTable = reinterpret_cast<const Sfnt::CmapSubTableFormat4 *>(pCmap31);

	uint16 nSeg = be::swap(pTable->seg_count_x2) >> 1;
  
	uint16 n;
    	const uint16 * pLeft, * pMid;
	uint16 cMid, chStart, chEnd;

    if (rangeKey)
    {
        pMid = &(pTable->end_code[rangeKey]);
        chEnd = be::swap(*pMid);
        n = rangeKey;
    }
    else
    {
        
        pLeft = &(pTable->end_code[0]);
        n = nSeg;
        while (n > 0)
        {
            cMid = n >> 1;           
            pMid = pLeft + cMid;
            chEnd = be::swap(*pMid);
            if (nUnicodeId <= chEnd)
            {
                if (cMid == 0 || nUnicodeId > be::swap(pMid[-1]))
                        break;          
                n = cMid;            
            }
            else
            {
                pLeft = pMid + 1;    
                n -= (cMid + 1);
            }
        }

        if (!n)
        return 0;
    }

    
    

    chStart = be::swap(*(pMid += nSeg + 1));
    if (chEnd >= nUnicodeId && nUnicodeId >= chStart)
    {
        
        int16 idDelta = be::swap(*(pMid += nSeg));
        uint16 idRangeOffset = be::swap(*(pMid += nSeg));

        if (idRangeOffset == 0)
            return (uint16)(idDelta + nUnicodeId); 

        
        size_t offset = (nUnicodeId - chStart) + (idRangeOffset >> 1) +
                (reinterpret_cast<const uint16 *>(pMid) - reinterpret_cast<const uint16 *>(pTable));
        if (offset * 2 >= pTable->length)
            return 0;
        gid16 nGlyphId = be::swap(*(pMid + (nUnicodeId - chStart) + (idRangeOffset >> 1)));
        
        return nGlyphId ? nGlyphId + idDelta : 0;
    }

    return 0;
}







unsigned int Cmap31NextCodepoint(const void *pCmap31, unsigned int nUnicodeId, int * pRangeKey)
{
	const Sfnt::CmapSubTableFormat4 * pTable = reinterpret_cast<const Sfnt::CmapSubTableFormat4 *>(pCmap31);

	uint16 nRange = be::swap(pTable->seg_count_x2) >> 1;

	uint32 nUnicodePrev = (uint32)nUnicodeId;

	const uint16 * pStartCode = &(pTable->end_code[0])
		+ nRange 
		+ 1;   

	if (nUnicodePrev == 0)
	{
		
		if (pRangeKey)
			*pRangeKey = 0;
		return be::swap(pStartCode[0]);
	}
	else if (nUnicodePrev >= 0xFFFF)
	{
		if (pRangeKey)
			*pRangeKey = nRange - 1;
		return 0xFFFF;
	}

	int iRange = (pRangeKey) ? *pRangeKey : 0;
	
	while (iRange > 0 && be::swap(pStartCode[iRange]) > nUnicodePrev)
		iRange--;
	while (be::swap(pTable->end_code[iRange]) < nUnicodePrev)
		iRange++;

	
	unsigned int nStartCode = be::swap(pStartCode[iRange]);
	unsigned int nEndCode = be::swap(pTable->end_code[iRange]);

	if (nStartCode > nUnicodePrev)
		
		
		nUnicodePrev = nStartCode - 1;

	if (nEndCode > nUnicodePrev)
	{
		
		if (pRangeKey)
			*pRangeKey = iRange;
		return nUnicodePrev + 1;
	}

	
	
	
	if (pRangeKey)
		*pRangeKey = iRange + 1;
	return be::swap(pStartCode[iRange + 1]);
}




bool CheckCmap310Subtable(const void *pCmap310)
{
	const Sfnt::CmapSubTable * pTable = reinterpret_cast<const Sfnt::CmapSubTable *>(pCmap310);
    if (be::swap(pTable->format) != 12)
        return false;
    const Sfnt::CmapSubTableFormat12 * pTable12 = reinterpret_cast<const Sfnt::CmapSubTableFormat12 *>(pCmap310);
    uint32 length = be::swap(pTable12->length);
    if (length < sizeof(Sfnt::CmapSubTableFormat12))
        return false;
    
	return (length == (sizeof(Sfnt::CmapSubTableFormat12) + (be::swap(pTable12->num_groups) - 1)
        * sizeof(uint32) * 3));
}






gid16 Cmap310Lookup(const void * pCmap310, unsigned int uUnicodeId, int rangeKey)
{
	const Sfnt::CmapSubTableFormat12 * pTable = reinterpret_cast<const Sfnt::CmapSubTableFormat12 *>(pCmap310);

	
	uint32 ucGroups = be::swap(pTable->num_groups);

	for (unsigned int i = rangeKey; i < ucGroups; i++)
	{
		uint32 uStartCode = be::swap(pTable->group[i].start_char_code);
		uint32 uEndCode = be::swap(pTable->group[i].end_char_code);
		if (uUnicodeId >= uStartCode && uUnicodeId <= uEndCode)
		{
			uint32 uDiff = uUnicodeId - uStartCode;
			uint32 uStartGid = be::swap(pTable->group[i].start_glyph_id);
			return static_cast<gid16>(uStartGid + uDiff);
		}
	}

	return 0;
}







unsigned int Cmap310NextCodepoint(const void *pCmap310, unsigned int nUnicodeId, int * pRangeKey)
{
	const Sfnt::CmapSubTableFormat12 * pTable = reinterpret_cast<const Sfnt::CmapSubTableFormat12 *>(pCmap310);

	int nRange = be::swap(pTable->num_groups);

	uint32 nUnicodePrev = (uint32)nUnicodeId;

	if (nUnicodePrev == 0)
	{
		
		if (pRangeKey)
			*pRangeKey = 0;
		return be::swap(pTable->group[0].start_char_code);
	}
	else if (nUnicodePrev >= 0x10FFFF)
	{
		if (pRangeKey)
			*pRangeKey = nRange;
		return 0x10FFFF;
	}

	int iRange = (pRangeKey) ? *pRangeKey : 0;
	
	while (iRange > 0 && be::swap(pTable->group[iRange].start_char_code) > nUnicodePrev)
		iRange--;
	while (be::swap(pTable->group[iRange].end_char_code) < nUnicodePrev)
		iRange++;

	

	unsigned int nStartCode = be::swap(pTable->group[iRange].start_char_code);
	unsigned int nEndCode = be::swap(pTable->group[iRange].end_char_code);

	if (nStartCode > nUnicodePrev)
		
		
		nUnicodePrev = nStartCode - 1;

	if (nEndCode > nUnicodePrev)
	{
		
		if (pRangeKey)
			*pRangeKey = iRange;
		return nUnicodePrev + 1;
	}

	
	if (pRangeKey)
		*pRangeKey = iRange + 1;
	return (iRange + 1 >= nRange) ? 0x10FFFF : be::swap(pTable->group[iRange + 1].start_char_code);
}








size_t LocaLookup(gid16 nGlyphId, 
		const void * pLoca, size_t lLocaSize, 
		const void * pHead) 
{
	const Sfnt::FontHeader * pTable = reinterpret_cast<const Sfnt::FontHeader *>(pHead);

	
	if (be::swap(pTable->index_to_loc_format) == Sfnt::FontHeader::ShortIndexLocFormat)
	{ 
		if (nGlyphId <= (lLocaSize >> 1) - 1) 
		{
			const uint16 * pShortTable = reinterpret_cast<const uint16 *>(pLoca);
			return (be::swap(pShortTable[nGlyphId]) << 1);
		}
	}
	
	if (be::swap(pTable->index_to_loc_format) == Sfnt::FontHeader::LongIndexLocFormat)
	{ 
		if (nGlyphId <= (lLocaSize >> 2) - 1)
		{
			const uint32 * pLongTable = reinterpret_cast<const uint32 *>(pLoca);
			return be::swap(pLongTable[nGlyphId]);
		}
	}

	
	return -1;
	
}





void * GlyfLookup(const void * pGlyf, size_t nGlyfOffset, size_t nTableLen)
{
	const uint8 * pByte = reinterpret_cast<const uint8 *>(pGlyf);
        if (nGlyfOffset == size_t(-1) || nGlyfOffset > nTableLen)
            return NULL;
	return const_cast<uint8 *>(pByte + nGlyfOffset);
}





bool GlyfBox(const void * pSimpleGlyf, int & xMin, int & yMin, 
					  int & xMax, int & yMax)
{
	const Sfnt::Glyph * pGlyph = reinterpret_cast<const Sfnt::Glyph *>(pSimpleGlyf);

	xMin = be::swap(pGlyph->x_min);
	yMin = be::swap(pGlyph->y_min);
	xMax = be::swap(pGlyph->x_max);
	yMax = be::swap(pGlyph->y_max);

	return true;
}

#ifdef ALL_TTFUTILS




int GlyfContourCount(const void * pSimpleGlyf)
{
	const Sfnt::Glyph * pGlyph = reinterpret_cast<const Sfnt::Glyph *>(pSimpleGlyf);
	return be::swap(pGlyph->number_of_contours); 
}










bool GlyfContourEndPoints(const void * pSimpleGlyf, int * prgnContourEndPoint, 
								   int cnPointsTotal, int & cnPoints)
{
	const Sfnt::SimpleGlyph * pGlyph = reinterpret_cast<const Sfnt::SimpleGlyph *>(pSimpleGlyf);

	int cContours = be::swap(pGlyph->number_of_contours);
	if (cContours < 0)
		return false; 

	for (int i = 0; i < cContours && i < cnPointsTotal; i++)
	{
		prgnContourEndPoint[i] = be::swap(pGlyph->end_pts_of_contours[i]);
	}

	cnPoints = cContours;
	return true;
}













bool GlyfPoints(const void * pSimpleGlyf, int * prgnX, int * prgnY, 
		char * prgbFlag, int cnPointsTotal, int & cnPoints)
{
	using namespace Sfnt;
	
	const Sfnt::SimpleGlyph * pGlyph = reinterpret_cast<const Sfnt::SimpleGlyph *>(pSimpleGlyf);
	int cContours = be::swap(pGlyph->number_of_contours);
	
	if (cContours <= 0)
		return false;
	int cPts = be::swap(pGlyph->end_pts_of_contours[cContours - 1]) + 1;
	if (cPts > cnPointsTotal)
		return false;

	
	const uint8 * pbGlyph = reinterpret_cast<const uint8 *>
		(&pGlyph->end_pts_of_contours[cContours]);
	
	
	int cbHints = be::swap(*(uint16 *)pbGlyph);
	pbGlyph += sizeof(uint16);
	pbGlyph += cbHints;

	
	int iFlag = 0;
	while (iFlag < cPts)
	{
		if (!(*pbGlyph & SimpleGlyph::Repeat))
		{ 
			prgbFlag[iFlag] = (char)*pbGlyph;
			pbGlyph++;
			iFlag++;
		}
		else
		{ 
			char chFlag = (char)*pbGlyph;
			pbGlyph++;
			int cFlags = (int)*pbGlyph;
			pbGlyph++;
			prgbFlag[iFlag] = chFlag;
			iFlag++;
			for (int i = 0; i < cFlags; i++)
			{
				prgbFlag[iFlag + i] = chFlag;
			}
			iFlag += cFlags;
		}
	}
	if (iFlag != cPts)
		return false;

	
	iFlag = 0;
	while (iFlag < cPts)
	{
		if (prgbFlag[iFlag] & SimpleGlyph::XShort)
		{
			prgnX[iFlag] = *pbGlyph;
			if (!(prgbFlag[iFlag] & SimpleGlyph::XIsPos))
			{
				prgnX[iFlag] = -prgnX[iFlag];
			}
			pbGlyph++;
		}
		else
		{
			if (prgbFlag[iFlag] & SimpleGlyph::XIsSame)
			{
				prgnX[iFlag] = 0;
				
			}
			else
			{
				prgnX[iFlag] = be::swap(*(int16 *)pbGlyph);
				pbGlyph += sizeof(int16);
			}
		}
		iFlag++;
	}
		
	
	iFlag = 0;
	while (iFlag < cPts)
	{
		if (prgbFlag[iFlag] & SimpleGlyph::YShort)
		{
			prgnY[iFlag] = *pbGlyph;
			if (!(prgbFlag[iFlag] & SimpleGlyph::YIsPos))
			{
				prgnY[iFlag] = -prgnY[iFlag];
			}
			pbGlyph++;
		}
		else
		{
			if (prgbFlag[iFlag] & SimpleGlyph::YIsSame)
			{
				prgnY[iFlag] = 0;
				
			}
			else
			{
				prgnY[iFlag] = be::swap(*(int16 *)pbGlyph);
				pbGlyph += sizeof(int16);
			}
		}
		iFlag++;
	}
		
	cnPoints = cPts;
	return true;
}










bool GetComponentGlyphIds(const void * pSimpleGlyf, int * prgnCompId, 
		size_t cnCompIdTotal, size_t & cnCompId)
{
	using namespace Sfnt;
	
	if (GlyfContourCount(pSimpleGlyf) >= 0)
		return false;

	const Sfnt::SimpleGlyph * pGlyph = reinterpret_cast<const Sfnt::SimpleGlyph *>(pSimpleGlyf);
	
	const uint8 * pbGlyph = reinterpret_cast<const uint8 *>(&pGlyph->end_pts_of_contours[0]);

	uint16 GlyphFlags;
	size_t iCurrentComp = 0;
	do 
	{
		GlyphFlags = be::swap(*((uint16 *)pbGlyph));
		pbGlyph += sizeof(uint16);
		prgnCompId[iCurrentComp++] = be::swap(*((uint16 *)pbGlyph));
		pbGlyph += sizeof(uint16);
		if (iCurrentComp >= cnCompIdTotal) 
			return false;
		int nOffset = 0;
		nOffset += GlyphFlags & CompoundGlyph::Arg1Arg2Words ? 4 : 2;
		nOffset += GlyphFlags & CompoundGlyph::HaveScale ? 2 : 0;
		nOffset += GlyphFlags & CompoundGlyph::HaveXAndYScale  ? 4 : 0;
		nOffset += GlyphFlags & CompoundGlyph::HaveTwoByTwo  ? 8 :  0;
		pbGlyph += nOffset;
	} while (GlyphFlags & CompoundGlyph::MoreComponents);

	cnCompId = iCurrentComp;

	return true;
}











bool GetComponentPlacement(const void * pSimpleGlyf, int nCompId,
									bool fOffset, int & a, int & b)
{
	using namespace Sfnt;
	
	if (GlyfContourCount(pSimpleGlyf) >= 0)
		return false;

	const Sfnt::SimpleGlyph * pGlyph = reinterpret_cast<const Sfnt::SimpleGlyph *>(pSimpleGlyf);
	
	const uint8 * pbGlyph = reinterpret_cast<const uint8 *>(&pGlyph->end_pts_of_contours[0]);

	uint16 GlyphFlags;
	do 
	{
		GlyphFlags = be::swap(*((uint16 *)pbGlyph));
		pbGlyph += sizeof(uint16);
		if (be::swap(*((uint16 *)pbGlyph)) == nCompId)
		{
			pbGlyph += sizeof(uint16); 
			fOffset = (GlyphFlags & CompoundGlyph::ArgsAreXYValues) == CompoundGlyph::ArgsAreXYValues;

			if (GlyphFlags & CompoundGlyph::Arg1Arg2Words )
			{
				a = be::swap(*(int16 *)pbGlyph);
				pbGlyph += sizeof(int16);
				b = be::swap(*(int16 *)pbGlyph);
				pbGlyph += sizeof(int16);
			}
			else
			{ 
				a = *pbGlyph++;
				b = *pbGlyph++;
			}
			return true;
		}
		pbGlyph += sizeof(uint16); 
		int nOffset = 0;
		nOffset += GlyphFlags & CompoundGlyph::Arg1Arg2Words  ? 4 : 2;
		nOffset += GlyphFlags & CompoundGlyph::HaveScale ? 2 : 0;
		nOffset += GlyphFlags & CompoundGlyph::HaveXAndYScale  ? 4 : 0;
		nOffset += GlyphFlags & CompoundGlyph::HaveTwoByTwo  ? 8 :  0;
		pbGlyph += nOffset;
	} while (GlyphFlags & CompoundGlyph::MoreComponents);

	
	fOffset = true;
	a = 0;
	b = 0;
	return false;
}













bool GetComponentTransform(const void * pSimpleGlyf, int nCompId, 
									float & flt11, float & flt12, float & flt21, float & flt22, 
									bool & fTransOffset)
{
	using namespace Sfnt;
	
	if (GlyfContourCount(pSimpleGlyf) >= 0)
		return false;

	const Sfnt::SimpleGlyph * pGlyph = reinterpret_cast<const Sfnt::SimpleGlyph *>(pSimpleGlyf);
	
	const uint8 * pbGlyph = reinterpret_cast<const uint8 *>(&pGlyph->end_pts_of_contours[0]);

	uint16 GlyphFlags;
	do 
	{
		GlyphFlags = be::swap(*((uint16 *)pbGlyph));
		pbGlyph += sizeof(uint16);
		if (be::swap(*((uint16 *)pbGlyph)) == nCompId)
		{
			pbGlyph += sizeof(uint16); 
			pbGlyph += GlyphFlags & CompoundGlyph::Arg1Arg2Words  ? 4 : 2; 

			if (fTransOffset) 
				fTransOffset = !(GlyphFlags & CompoundGlyph::UnscaledOffset); 
			else 
				fTransOffset = (GlyphFlags & CompoundGlyph::ScaledOffset) != 0;

			if (GlyphFlags & CompoundGlyph::HaveScale)
			{
				flt11 = fixed_to_float<14>(be::swap(*(uint16 *)pbGlyph));
				pbGlyph += sizeof(uint16);
				flt12 = 0;
				flt21 = 0;
				flt22 = flt11;
			}
			else if (GlyphFlags & CompoundGlyph::HaveXAndYScale)
			{
				flt11 = fixed_to_float<14>(be::swap(*(uint16 *)pbGlyph));
				pbGlyph += sizeof(uint16);
				flt12 = 0;
				flt21 = 0;
				flt22 = fixed_to_float<14>(be::swap(*(uint16 *)pbGlyph));
				pbGlyph += sizeof(uint16);
			}
			else if (GlyphFlags & CompoundGlyph::HaveTwoByTwo)
			{
				flt11 = fixed_to_float<14>(be::swap(*(uint16 *)pbGlyph));
				pbGlyph += sizeof(uint16);
				flt12 = fixed_to_float<14>(be::swap(*(uint16 *)pbGlyph));
				pbGlyph += sizeof(uint16);
				flt21 = fixed_to_float<14>(be::swap(*(uint16 *)pbGlyph));
				pbGlyph += sizeof(uint16);
				flt22 = fixed_to_float<14>(be::swap(*(uint16 *)pbGlyph));
				pbGlyph += sizeof(uint16);
			}
			else
			{ 
				flt11 = 1.0;
				flt12 = 0.0;
				flt21 = 0.0;
				flt22 = 1.0;
			}
			return true;
		}
		pbGlyph += sizeof(uint16); 
		int nOffset = 0;
		nOffset += GlyphFlags & CompoundGlyph::Arg1Arg2Words  ? 4 : 2;
		nOffset += GlyphFlags & CompoundGlyph::HaveScale ? 2 : 0;
		nOffset += GlyphFlags & CompoundGlyph::HaveXAndYScale  ? 4 : 0;
		nOffset += GlyphFlags & CompoundGlyph::HaveTwoByTwo  ? 8 :  0;
		pbGlyph += nOffset;
	} while (GlyphFlags & CompoundGlyph::MoreComponents);

	
	fTransOffset = false;
	flt11 = 1;
	flt12 = 0;
	flt21 = 0;
	flt22 = 1;
	return false;
}
#endif






void * GlyfLookup(gid16 nGlyphId, const void * pGlyf, const void * pLoca, 
						   size_t lGlyfSize, size_t lLocaSize, const void * pHead)
{
	
	
	
	const Sfnt::FontHeader * pTable 
		= reinterpret_cast<const Sfnt::FontHeader *>(pHead);

	if (be::swap(pTable->index_to_loc_format) == Sfnt::FontHeader::ShortIndexLocFormat)
	{ 
		if (nGlyphId >= (lLocaSize >> 1) - 1) 
		{

            return NULL;
		}
	}
	if (be::swap(pTable->index_to_loc_format) == Sfnt::FontHeader::LongIndexLocFormat)
	{ 
		if (nGlyphId >= (lLocaSize >> 2) - 1)
		{

            return NULL;
		}
	}

	long lGlyfOffset = LocaLookup(nGlyphId, pLoca, lLocaSize, pHead);
	void * pSimpleGlyf = GlyfLookup(pGlyf, lGlyfOffset, lGlyfSize); 
	return pSimpleGlyf;
}

#ifdef ALL_TTFUTILS




bool IsSpace(gid16 nGlyphId, const void * pLoca, size_t lLocaSize, const void * pHead)
{
	size_t lGlyfOffset = LocaLookup(nGlyphId, pLoca, lLocaSize, pHead);
	
	
	size_t lNextGlyfOffset = LocaLookup(nGlyphId + 1, pLoca, lLocaSize, pHead);

	return (lNextGlyfOffset - lGlyfOffset) == 0;
}




bool IsDeepComposite(gid16 nGlyphId, const void * pGlyf, const void * pLoca, 
					size_t lGlyfSize, long lLocaSize, const void * pHead)
{
	if (IsSpace(nGlyphId, pLoca, lLocaSize, pHead)) {return false;}

	void * pSimpleGlyf = GlyfLookup(nGlyphId, pGlyf, pLoca, lGlyfSize, lLocaSize, pHead);
	if (pSimpleGlyf == NULL)
		return false; 

	if (GlyfContourCount(pSimpleGlyf) >= 0)
		return false;

	int rgnCompId[kMaxGlyphComponents]; 
	size_t cCompIdTotal = kMaxGlyphComponents;
	size_t cCompId = 0;

	if (!GetComponentGlyphIds(pSimpleGlyf, rgnCompId, cCompIdTotal, cCompId))
		return false;

	for (size_t i = 0; i < cCompId; i++)
	{
		pSimpleGlyf = GlyfLookup(static_cast<gid16>(rgnCompId[i]), 
							pGlyf, pLoca, lGlyfSize, lLocaSize, pHead);
		if (pSimpleGlyf == NULL) {return false;}

		if (GlyfContourCount(pSimpleGlyf) < 0)
			return true;
	}

	return false;
}







bool GlyfBox(gid16  nGlyphId, const void * pGlyf, const void * pLoca, 
		size_t lGlyfSize, size_t lLocaSize, const void * pHead, int & xMin, int & yMin, int & xMax, int & yMax)
{
	xMin = yMin = xMax = yMax = INT_MIN;

	if (IsSpace(nGlyphId, pLoca, lLocaSize, pHead)) {return false;}

	void * pSimpleGlyf = GlyfLookup(nGlyphId, pGlyf, pLoca, lGlyfSize, lLocaSize, pHead);
	if (pSimpleGlyf == NULL) {return false;}

	return GlyfBox(pSimpleGlyf, xMin, yMin, xMax, yMax);
}







bool GlyfContourCount(gid16 nGlyphId, const void * pGlyf, const void * pLoca, 
	size_t lGlyfSize, size_t lLocaSize, const void * pHead, size_t & cnContours)
{
	cnContours = static_cast<size_t>(INT_MIN);

	if (IsSpace(nGlyphId, pLoca, lLocaSize, pHead)) {return false;}

	void * pSimpleGlyf = GlyfLookup(nGlyphId, pGlyf, pLoca, lGlyfSize, lLocaSize, pHead);
	if (pSimpleGlyf == NULL) {return false;}

	int cRtnContours = GlyfContourCount(pSimpleGlyf);
	if (cRtnContours >= 0)
	{
		cnContours = size_t(cRtnContours);
		return true;
	}
		
	

	int rgnCompId[kMaxGlyphComponents]; 
	size_t cCompIdTotal = kMaxGlyphComponents;
	size_t cCompId = 0;

	if (!GetComponentGlyphIds(pSimpleGlyf, rgnCompId, cCompIdTotal, cCompId))
		return false;

	cRtnContours = 0;
	int cTmp = 0;
	for (size_t i = 0; i < cCompId; i++)
	{
		if (IsSpace(static_cast<gid16>(rgnCompId[i]), pLoca, lLocaSize, pHead)) {return false;}
		pSimpleGlyf = GlyfLookup(static_cast<gid16>(rgnCompId[i]), 
		                         pGlyf, pLoca, lGlyfSize, lLocaSize, pHead);
		if (pSimpleGlyf == 0) {return false;}
		
		if ((cTmp = GlyfContourCount(pSimpleGlyf)) < 0) 
			return false;
		cRtnContours += cTmp;
	}

	cnContours = size_t(cRtnContours);
	return true;
}










bool GlyfContourEndPoints(gid16 nGlyphId, const void * pGlyf, const void * pLoca, 
	size_t lGlyfSize, size_t lLocaSize, const void * pHead,
	int * prgnContourEndPoint, size_t cnPoints)
{
        memset(prgnContourEndPoint, 0xFF, cnPoints * sizeof(int));
	

	if (IsSpace(nGlyphId, pLoca, lLocaSize, pHead)) {return false;}

	void * pSimpleGlyf = GlyfLookup(nGlyphId, pGlyf, pLoca, lGlyfSize, lLocaSize, pHead);
	if (pSimpleGlyf == NULL) {return false;}

	int cContours = GlyfContourCount(pSimpleGlyf);
	int cActualPts = 0;
	if (cContours > 0)
		return GlyfContourEndPoints(pSimpleGlyf, prgnContourEndPoint, cnPoints, cActualPts);
	
	
	
	int rgnCompId[kMaxGlyphComponents]; 
	size_t cCompIdTotal = kMaxGlyphComponents;
	size_t cCompId = 0;

	if (!GetComponentGlyphIds(pSimpleGlyf, rgnCompId, cCompIdTotal, cCompId))
		return false;

	int * prgnCurrentEndPoint = prgnContourEndPoint;
	int cCurrentPoints = cnPoints;
	int nPrevPt = 0;
	for (size_t i = 0; i < cCompId; i++)
	{
		if (IsSpace(static_cast<gid16>(rgnCompId[i]), pLoca, lLocaSize, pHead)) {return false;}
		pSimpleGlyf = GlyfLookup(static_cast<gid16>(rgnCompId[i]), pGlyf, pLoca, lGlyfSize, lLocaSize, pHead);
		if (pSimpleGlyf == NULL) {return false;}
		
		if (!GlyfContourEndPoints(pSimpleGlyf, prgnCurrentEndPoint, cCurrentPoints, cActualPts))
			return false;
		
		
		for (int j = 0; j < cActualPts; j++)
			prgnCurrentEndPoint[j] += nPrevPt;
		nPrevPt = prgnCurrentEndPoint[cActualPts - 1] + 1;

		prgnCurrentEndPoint += cActualPts;
		cCurrentPoints -= cActualPts;
	}

	return true;
}























bool GlyfPoints(gid16 nGlyphId, const void * pGlyf,
		const void * pLoca, size_t lGlyfSize, size_t lLocaSize, const void * pHead,
		const int * , size_t ,
		int * prgnX, int * prgnY, bool * prgfOnCurve, size_t cnPoints)
{
        memset(prgnX, 0x7F, cnPoints * sizeof(int));
        memset(prgnY, 0x7F, cnPoints * sizeof(int));

	if (IsSpace(nGlyphId, pLoca, lLocaSize, pHead)) 
		return false;

	void * pSimpleGlyf = GlyfLookup(nGlyphId, pGlyf, pLoca, lGlyfSize, lLocaSize, pHead);
	if (pSimpleGlyf == NULL)
		return false;

	int cContours = GlyfContourCount(pSimpleGlyf);
	int cActualPts;
	if (cContours > 0)
	{
		if (!GlyfPoints(pSimpleGlyf, prgnX, prgnY, (char *)prgfOnCurve, cnPoints, cActualPts))
			return false;
		CalcAbsolutePoints(prgnX, prgnY, cnPoints);
		SimplifyFlags((char *)prgfOnCurve, cnPoints);
		return true;
	}

	
	int rgnCompId[kMaxGlyphComponents]; 
	size_t cCompIdTotal = kMaxGlyphComponents;
	size_t cCompId = 0;

	
	if (!GetComponentGlyphIds(pSimpleGlyf, rgnCompId, cCompIdTotal, cCompId))
		return false;

	int * prgnCurrentX = prgnX;
	int * prgnCurrentY = prgnY;
	char * prgbCurrentFlag = (char *)prgfOnCurve; 
	int cCurrentPoints = cnPoints;
	bool fOffset = true, fTransOff = true;
	int a, b;
	float flt11, flt12, flt21, flt22;
	
	
	for (size_t i = 0; i < cCompId; i++)
	{
		if (IsSpace(static_cast<gid16>(rgnCompId[i]), pLoca, lLocaSize, pHead)) {return false;}
		void * pCompGlyf = GlyfLookup(static_cast<gid16>(rgnCompId[i]), pGlyf, pLoca, lGlyfSize, lLocaSize, pHead);
		if (pCompGlyf == NULL) {return false;}
		
		if (!GlyfPoints(pCompGlyf, prgnCurrentX, prgnCurrentY, prgbCurrentFlag, 
			cCurrentPoints, cActualPts))
			return false; 
		if (!GetComponentPlacement(pSimpleGlyf, rgnCompId[i], fOffset, a, b))
			return false;
		if (!GetComponentTransform(pSimpleGlyf, rgnCompId[i], 
			flt11, flt12, flt21, flt22, fTransOff))
			return false;
		bool fIdTrans = flt11 == 1.0 && flt12 == 0.0 && flt21 == 0.0 && flt22 == 1.0;

		
		
		CalcAbsolutePoints(prgnCurrentX, prgnCurrentY, cActualPts);

		
		
		if (!fIdTrans)
			for (int j = 0; j < cActualPts; j++)
			{
				int x = prgnCurrentX[j]; 
				int y = prgnCurrentY[j];
				prgnCurrentX[j] = (int)(x * flt11 + y * flt12);
				prgnCurrentY[j] = (int)(x * flt21 + y * flt22);
			}
			
		
		int nXOff, nYOff;
		if (fOffset) 
		{ 
			





 
			{ 
				nXOff = a;
				nYOff = b;
			}
		}
		else  
		{	
			
			
			
			nXOff = prgnX[a] - prgnCurrentX[b];
			nYOff = prgnY[a] - prgnCurrentY[b];
		}
		for (int j = 0; j < cActualPts; j++)
		{
			prgnCurrentX[j] += nXOff;
			prgnCurrentY[j] += nYOff;
		}

		
		
		prgnCurrentX += cActualPts;
		prgnCurrentY += cActualPts;
		prgbCurrentFlag += cActualPts;
		cCurrentPoints -= cActualPts;
	}

	SimplifyFlags((char *)prgfOnCurve, cnPoints);

	return true;
}




bool SimplifyFlags(char * prgbFlags, int cnPoints)
{
	for (int i = 0; i < cnPoints; i++)
		prgbFlags[i] = static_cast<char>(prgbFlags[i] & Sfnt::SimpleGlyph::OnCurve);
	return true;
}






bool CalcAbsolutePoints(int * prgnX, int * prgnY, int cnPoints)
{
	int nX = prgnX[0];
	int nY = prgnY[0];
	for (int i = 1; i < cnPoints; i++)
	{
		prgnX[i] += nX;
		nX = prgnX[i];
		prgnY[i] += nY;
		nY = prgnY[i];
	}

	return true;
}
#endif





#if 0
size_t NameTableLength(const byte * pTable)
{
	byte * pb = (const_cast<byte *>(pTable)) + 2; 
	size_t cRecords = *pb++ << 8; cRecords += *pb++;
	int dbStringOffset0 = (*pb++) << 8; dbStringOffset0 += *pb++;
	int dbMaxStringOffset = 0;
	for (size_t irec = 0; irec < cRecords; irec++)
	{
		int nPlatform = (*pb++) << 8; nPlatform += *pb++;
		int nEncoding = (*pb++) << 8; nEncoding += *pb++;
		int nLanguage = (*pb++) << 8; nLanguage += *pb++;
		int nName = (*pb++) << 8; nName += *pb++;
		int cbStringLen = (*pb++) << 8; cbStringLen += *pb++;
		int dbStringOffset = (*pb++) << 8; dbStringOffset += *pb++;
		if (dbMaxStringOffset < dbStringOffset + cbStringLen)
			dbMaxStringOffset = dbStringOffset + cbStringLen;
	}
	return dbStringOffset0 + dbMaxStringOffset;
}
#endif

} 
} 
