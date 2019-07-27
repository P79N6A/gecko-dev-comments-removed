


























from .euctwfreq import (EUCTWCharToFreqOrder, EUCTW_TABLE_SIZE,
                        EUCTW_TYPICAL_DISTRIBUTION_RATIO)
from .euckrfreq import (EUCKRCharToFreqOrder, EUCKR_TABLE_SIZE,
                        EUCKR_TYPICAL_DISTRIBUTION_RATIO)
from .gb2312freq import (GB2312CharToFreqOrder, GB2312_TABLE_SIZE,
                         GB2312_TYPICAL_DISTRIBUTION_RATIO)
from .big5freq import (Big5CharToFreqOrder, BIG5_TABLE_SIZE,
                       BIG5_TYPICAL_DISTRIBUTION_RATIO)
from .jisfreq import (JISCharToFreqOrder, JIS_TABLE_SIZE,
                      JIS_TYPICAL_DISTRIBUTION_RATIO)
from .compat import wrap_ord

ENOUGH_DATA_THRESHOLD = 1024
SURE_YES = 0.99
SURE_NO = 0.01
MINIMUM_DATA_THRESHOLD = 3


class CharDistributionAnalysis:
    def __init__(self):
        
        
        self._mCharToFreqOrder = None
        self._mTableSize = None  
        
        
        
        
        self._mTypicalDistributionRatio = None
        self.reset()

    def reset(self):
        """reset analyser, clear any state"""
        
        
        self._mDone = False
        self._mTotalChars = 0  
        
        self._mFreqChars = 0

    def feed(self, aBuf, aCharLen):
        """feed a character with known length"""
        if aCharLen == 2:
            
            order = self.get_order(aBuf)
        else:
            order = -1
        if order >= 0:
            self._mTotalChars += 1
            
            if order < self._mTableSize:
                if 512 > self._mCharToFreqOrder[order]:
                    self._mFreqChars += 1

    def get_confidence(self):
        """return confidence based on existing data"""
        
        
        if self._mTotalChars <= 0 or self._mFreqChars <= MINIMUM_DATA_THRESHOLD:
            return SURE_NO

        if self._mTotalChars != self._mFreqChars:
            r = (self._mFreqChars / ((self._mTotalChars - self._mFreqChars)
                 * self._mTypicalDistributionRatio))
            if r < SURE_YES:
                return r

        
        return SURE_YES

    def got_enough_data(self):
        
        
        return self._mTotalChars > ENOUGH_DATA_THRESHOLD

    def get_order(self, aBuf):
        
        
        
        
        return -1


class EUCTWDistributionAnalysis(CharDistributionAnalysis):
    def __init__(self):
        CharDistributionAnalysis.__init__(self)
        self._mCharToFreqOrder = EUCTWCharToFreqOrder
        self._mTableSize = EUCTW_TABLE_SIZE
        self._mTypicalDistributionRatio = EUCTW_TYPICAL_DISTRIBUTION_RATIO

    def get_order(self, aBuf):
        
        
        
        
        first_char = wrap_ord(aBuf[0])
        if first_char >= 0xC4:
            return 94 * (first_char - 0xC4) + wrap_ord(aBuf[1]) - 0xA1
        else:
            return -1


class EUCKRDistributionAnalysis(CharDistributionAnalysis):
    def __init__(self):
        CharDistributionAnalysis.__init__(self)
        self._mCharToFreqOrder = EUCKRCharToFreqOrder
        self._mTableSize = EUCKR_TABLE_SIZE
        self._mTypicalDistributionRatio = EUCKR_TYPICAL_DISTRIBUTION_RATIO

    def get_order(self, aBuf):
        
        
        
        
        first_char = wrap_ord(aBuf[0])
        if first_char >= 0xB0:
            return 94 * (first_char - 0xB0) + wrap_ord(aBuf[1]) - 0xA1
        else:
            return -1


class GB2312DistributionAnalysis(CharDistributionAnalysis):
    def __init__(self):
        CharDistributionAnalysis.__init__(self)
        self._mCharToFreqOrder = GB2312CharToFreqOrder
        self._mTableSize = GB2312_TABLE_SIZE
        self._mTypicalDistributionRatio = GB2312_TYPICAL_DISTRIBUTION_RATIO

    def get_order(self, aBuf):
        
        
        
        
        first_char, second_char = wrap_ord(aBuf[0]), wrap_ord(aBuf[1])
        if (first_char >= 0xB0) and (second_char >= 0xA1):
            return 94 * (first_char - 0xB0) + second_char - 0xA1
        else:
            return -1


class Big5DistributionAnalysis(CharDistributionAnalysis):
    def __init__(self):
        CharDistributionAnalysis.__init__(self)
        self._mCharToFreqOrder = Big5CharToFreqOrder
        self._mTableSize = BIG5_TABLE_SIZE
        self._mTypicalDistributionRatio = BIG5_TYPICAL_DISTRIBUTION_RATIO

    def get_order(self, aBuf):
        
        
        
        
        first_char, second_char = wrap_ord(aBuf[0]), wrap_ord(aBuf[1])
        if first_char >= 0xA4:
            if second_char >= 0xA1:
                return 157 * (first_char - 0xA4) + second_char - 0xA1 + 63
            else:
                return 157 * (first_char - 0xA4) + second_char - 0x40
        else:
            return -1


class SJISDistributionAnalysis(CharDistributionAnalysis):
    def __init__(self):
        CharDistributionAnalysis.__init__(self)
        self._mCharToFreqOrder = JISCharToFreqOrder
        self._mTableSize = JIS_TABLE_SIZE
        self._mTypicalDistributionRatio = JIS_TYPICAL_DISTRIBUTION_RATIO

    def get_order(self, aBuf):
        
        
        
        
        first_char, second_char = wrap_ord(aBuf[0]), wrap_ord(aBuf[1])
        if (first_char >= 0x81) and (first_char <= 0x9F):
            order = 188 * (first_char - 0x81)
        elif (first_char >= 0xE0) and (first_char <= 0xEF):
            order = 188 * (first_char - 0xE0 + 31)
        else:
            return -1
        order = order + second_char - 0x40
        if second_char > 0x7F:
            order = -1
        return order


class EUCJPDistributionAnalysis(CharDistributionAnalysis):
    def __init__(self):
        CharDistributionAnalysis.__init__(self)
        self._mCharToFreqOrder = JISCharToFreqOrder
        self._mTableSize = JIS_TABLE_SIZE
        self._mTypicalDistributionRatio = JIS_TYPICAL_DISTRIBUTION_RATIO

    def get_order(self, aBuf):
        
        
        
        
        char = wrap_ord(aBuf[0])
        if char >= 0xA0:
            return 94 * (char - 0xA1) + wrap_ord(aBuf[1]) - 0xa1
        else:
            return -1
