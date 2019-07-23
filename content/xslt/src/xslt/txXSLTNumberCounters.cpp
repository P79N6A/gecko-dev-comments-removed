





































#include "txXSLTNumber.h"
#include "nsReadableUtils.h"
#include "txCore.h"

class txDecimalCounter : public txFormattedCounter {
public:
    txDecimalCounter() : mMinLength(1), mGroupSize(50)
    {
    }
    
    txDecimalCounter(PRInt32 aMinLength, PRInt32 aGroupSize,
                     const nsAString& mGroupSeparator);
    
    virtual void appendNumber(PRInt32 aNumber, nsAString& aDest);

private:
    PRInt32 mMinLength;
    PRInt32 mGroupSize;
    nsString mGroupSeparator;
};

class txAlphaCounter : public txFormattedCounter {
public:
    txAlphaCounter(PRUnichar aOffset) : mOffset(aOffset)
    {
    }

    virtual void appendNumber(PRInt32 aNumber, nsAString& aDest);
    
private:
    PRUnichar mOffset;
};

class txRomanCounter : public txFormattedCounter {
public:
    txRomanCounter(MBool aUpper) : mTableOffset(aUpper ? 30 : 0)
    {
    }

    void appendNumber(PRInt32 aNumber, nsAString& aDest);

private:
    PRInt32 mTableOffset;
};


nsresult
txFormattedCounter::getCounterFor(const nsAFlatString& aToken,
                                  PRInt32 aGroupSize,
                                  const nsAString& aGroupSeparator,
                                  txFormattedCounter*& aCounter)
{
    PRInt32 length = aToken.Length();
    NS_ASSERTION(length, "getting counter for empty token");
    aCounter = 0;
    
    if (length == 1) {
        PRUnichar ch = aToken.CharAt(0);
        switch (ch) {

            case 'i':
            case 'I':
                aCounter = new txRomanCounter(ch == 'I');
                break;
            
            case 'a':
            case 'A':
                aCounter = new txAlphaCounter(ch);
                break;
            
            case '1':
            default:
                
                aCounter = new txDecimalCounter(1, aGroupSize,
                                                aGroupSeparator);
                break;
        }
        return aCounter ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
    }
    
    
    PRInt32 i;
    for (i = 0; i < length-1; ++i) {
        if (aToken.CharAt(i) != '0')
            break;
    }
    if (i == length-1 && aToken.CharAt(i) == '1') {
        aCounter = new txDecimalCounter(length, aGroupSize, aGroupSeparator);
    }
    else {
        
        aCounter = new txDecimalCounter(1, aGroupSize, aGroupSeparator);
    }

    return aCounter ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


txDecimalCounter::txDecimalCounter(PRInt32 aMinLength, PRInt32 aGroupSize,
                                   const nsAString& aGroupSeparator)
    : mMinLength(aMinLength), mGroupSize(aGroupSize),
      mGroupSeparator(aGroupSeparator)
{
    if (mGroupSize <= 0) {
        mGroupSize = aMinLength + 10;
    }
}

void txDecimalCounter::appendNumber(PRInt32 aNumber, nsAString& aDest)
{
    const PRInt32 bufsize = 10; 
    PRUnichar buf[bufsize];
    PRInt32 pos = bufsize;
    while (aNumber > 0) {
        PRInt32 ch = aNumber % 10;
        aNumber /= 10;
        buf[--pos] = ch + '0';
    }

    
    PRInt32 end  = (bufsize > mMinLength) ? bufsize - mMinLength : 0;
    while (pos > end) {
        buf[--pos] = '0';
    }
    
    
    
    
    
    PRInt32 extraPos = mMinLength;
    while (extraPos > bufsize) {
        aDest.Append(PRUnichar('0'));
        --extraPos;
        if (extraPos % mGroupSize == 0) {
            aDest.Append(mGroupSeparator);
        }
    }

    
    if (mGroupSize >= bufsize - pos) {
        
        aDest.Append(buf + pos, (PRUint32)(bufsize - pos));
    }
    else {
        
        PRInt32 len = ((bufsize - pos - 1) % mGroupSize) + 1;
        aDest.Append(buf + pos, len);
        pos += len;
        while (bufsize - pos > 0) {
            aDest.Append(mGroupSeparator);
            aDest.Append(buf + pos, mGroupSize);
            pos += mGroupSize;
        }
        NS_ASSERTION(bufsize == pos, "error while grouping");
    }
}


void txAlphaCounter::appendNumber(PRInt32 aNumber, nsAString& aDest)
{
    PRUnichar buf[12];
    buf[11] = 0;
    PRInt32 pos = 11;
    while (aNumber > 0) {
        --aNumber;
        PRInt32 ch = aNumber % 26;
        aNumber /= 26;
        buf[--pos] = ch + mOffset;
    }
    
    aDest.Append(buf + pos, (PRUint32)(11 - pos));
}


const char* const kTxRomanNumbers[] =
    {"", "c", "cc", "ccc", "cd", "d", "dc", "dcc", "dccc", "cm",
     "", "x", "xx", "xxx", "xl", "l", "lx", "lxx", "lxxx", "xc",
     "", "i", "ii", "iii", "iv", "v", "vi", "vii", "viii", "ix",
     "", "C", "CC", "CCC", "CD", "D", "DC", "DCC", "DCCC", "CM",
     "", "X", "XX", "XXX", "XL", "L", "LX", "LXX", "LXXX", "XC",
     "", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX"};

void txRomanCounter::appendNumber(PRInt32 aNumber, nsAString& aDest)
{
    
    if (aNumber >= 4000) {
        txDecimalCounter().appendNumber(aNumber, aDest);
        return;
    }

    while (aNumber >= 1000) {
        aDest.Append(!mTableOffset ? PRUnichar('m') : PRUnichar('M'));
        aNumber -= 1000;
    }

    PRInt32 posValue;
    
    
    posValue = aNumber / 100;
    aNumber %= 100;
    AppendASCIItoUTF16(kTxRomanNumbers[posValue + mTableOffset], aDest);
    
    posValue = aNumber / 10;
    aNumber %= 10;
    AppendASCIItoUTF16(kTxRomanNumbers[10 + posValue + mTableOffset], aDest);
    
    AppendASCIItoUTF16(kTxRomanNumbers[20 + aNumber + mTableOffset], aDest);
}
