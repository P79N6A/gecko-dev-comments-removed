





































#ifndef nsGUIEventIPC_h__
#define nsGUIEventIPC_h__

#include "IPC/IPCMessageUtils.h"
#include "nsGUIEvent.h"

namespace IPC
{

template<>
struct ParamTraits<nsEvent>
{
  typedef nsEvent paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.eventStructType);
    WriteParam(aMsg, aParam.message);
    WriteParam(aMsg, aParam.refPoint);
    WriteParam(aMsg, aParam.time);
    WriteParam(aMsg, aParam.flags);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    return ReadParam(aMsg, aIter, &aResult->eventStructType) &&
           ReadParam(aMsg, aIter, &aResult->message) &&
           ReadParam(aMsg, aIter, &aResult->refPoint) &&
           ReadParam(aMsg, aIter, &aResult->time) &&
           ReadParam(aMsg, aIter, &aResult->flags);
  }
};

template<>
struct ParamTraits<nsGUIEvent>
{
  typedef nsGUIEvent paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, static_cast<nsEvent>(aParam));
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    return ReadParam(aMsg, aIter, static_cast<nsEvent*>(aResult));
  }
};

template<>
struct ParamTraits<nsInputEvent>
{
  typedef nsInputEvent paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, static_cast<nsGUIEvent>(aParam));
    WriteParam(aMsg, aParam.isShift);
    WriteParam(aMsg, aParam.isControl);
    WriteParam(aMsg, aParam.isAlt);
    WriteParam(aMsg, aParam.isMeta);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    return ReadParam(aMsg, aIter, static_cast<nsGUIEvent*>(aResult)) &&
           ReadParam(aMsg, aIter, &aResult->isShift) &&
           ReadParam(aMsg, aIter, &aResult->isControl) &&
           ReadParam(aMsg, aIter, &aResult->isAlt) &&
           ReadParam(aMsg, aIter, &aResult->isMeta);
  }
};

template<>
struct ParamTraits<nsTextRangeStyle>
{
  typedef nsTextRangeStyle paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.mDefinedStyles);
    WriteParam(aMsg, aParam.mLineStyle);
    WriteParam(aMsg, aParam.mIsBoldLine);
    WriteParam(aMsg, aParam.mForegroundColor);
    WriteParam(aMsg, aParam.mBackgroundColor);
    WriteParam(aMsg, aParam.mUnderlineColor);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    return ReadParam(aMsg, aIter, &aResult->mDefinedStyles) &&
           ReadParam(aMsg, aIter, &aResult->mLineStyle) &&
           ReadParam(aMsg, aIter, &aResult->mIsBoldLine) &&
           ReadParam(aMsg, aIter, &aResult->mForegroundColor) &&
           ReadParam(aMsg, aIter, &aResult->mBackgroundColor) &&
           ReadParam(aMsg, aIter, &aResult->mUnderlineColor);
  }
};

template<>
struct ParamTraits<nsTextRange>
{
  typedef nsTextRange paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, aParam.mStartOffset);
    WriteParam(aMsg, aParam.mEndOffset);
    WriteParam(aMsg, aParam.mRangeType);
    WriteParam(aMsg, aParam.mRangeStyle);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    return ReadParam(aMsg, aIter, &aResult->mStartOffset) &&
           ReadParam(aMsg, aIter, &aResult->mEndOffset) &&
           ReadParam(aMsg, aIter, &aResult->mRangeType) &&
           ReadParam(aMsg, aIter, &aResult->mRangeStyle);
  }
};

template<>
struct ParamTraits<nsTextEvent>
{
  typedef nsTextEvent paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, static_cast<nsInputEvent>(aParam));
    WriteParam(aMsg, aParam.theText);
    WriteParam(aMsg, aParam.isChar);
    WriteParam(aMsg, aParam.rangeCount);
    for (PRUint32 index = 0; index < aParam.rangeCount; index++)
      WriteParam(aMsg, aParam.rangeArray[index]);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    if (!ReadParam(aMsg, aIter, static_cast<nsInputEvent*>(aResult)) ||
        !ReadParam(aMsg, aIter, &aResult->theText) ||
        !ReadParam(aMsg, aIter, &aResult->isChar) ||
        !ReadParam(aMsg, aIter, &aResult->rangeCount))
      return false;

    if (!aResult->rangeCount) {
      aResult->rangeArray = nsnull;
      return true;
    }

    aResult->rangeArray = new nsTextRange[aResult->rangeCount];
    if (!aResult->rangeArray)
      return false;

    for (PRUint32 index = 0; index < aResult->rangeCount; index++)
      if (!ReadParam(aMsg, aIter, &aResult->rangeArray[index])) {
        Free(*aResult);
        return false;
      }
    return true;
  }

  static void Free(const paramType& aResult)
  {
    if (aResult.rangeArray)
      delete [] aResult.rangeArray;
  }
};

template<>
struct ParamTraits<nsCompositionEvent>
{
  typedef nsCompositionEvent paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, static_cast<nsInputEvent>(aParam));
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    return ReadParam(aMsg, aIter, static_cast<nsInputEvent*>(aResult));
  }
};

template<>
struct ParamTraits<nsQueryContentEvent>
{
  typedef nsQueryContentEvent paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, static_cast<nsGUIEvent>(aParam));
    WriteParam(aMsg, aParam.mSucceeded);
    WriteParam(aMsg, aParam.mInput.mOffset);
    WriteParam(aMsg, aParam.mInput.mLength);
    WriteParam(aMsg, aParam.mReply.mOffset);
    WriteParam(aMsg, aParam.mReply.mString);
    WriteParam(aMsg, aParam.mReply.mRect);
    WriteParam(aMsg, aParam.mReply.mReversed);
    WriteParam(aMsg, aParam.mReply.mHasSelection);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    aResult->mWasAsync = PR_TRUE;
    return ReadParam(aMsg, aIter, static_cast<nsGUIEvent*>(aResult)) &&
           ReadParam(aMsg, aIter, &aResult->mSucceeded) &&
           ReadParam(aMsg, aIter, &aResult->mInput.mOffset) &&
           ReadParam(aMsg, aIter, &aResult->mInput.mLength) &&
           ReadParam(aMsg, aIter, &aResult->mReply.mOffset) &&
           ReadParam(aMsg, aIter, &aResult->mReply.mString) &&
           ReadParam(aMsg, aIter, &aResult->mReply.mRect) &&
           ReadParam(aMsg, aIter, &aResult->mReply.mReversed) &&
           ReadParam(aMsg, aIter, &aResult->mReply.mHasSelection);
  }
};

template<>
struct ParamTraits<nsSelectionEvent>
{
  typedef nsSelectionEvent paramType;

  static void Write(Message* aMsg, const paramType& aParam)
  {
    WriteParam(aMsg, static_cast<nsGUIEvent>(aParam));
    WriteParam(aMsg, aParam.mOffset);
    WriteParam(aMsg, aParam.mLength);
    WriteParam(aMsg, aParam.mReversed);
    WriteParam(aMsg, aParam.mExpandToClusterBoundary);
    WriteParam(aMsg, aParam.mSucceeded);
  }

  static bool Read(const Message* aMsg, void** aIter, paramType* aResult)
  {
    return ReadParam(aMsg, aIter, static_cast<nsGUIEvent*>(aResult)) &&
           ReadParam(aMsg, aIter, &aResult->mOffset) &&
           ReadParam(aMsg, aIter, &aResult->mLength) &&
           ReadParam(aMsg, aIter, &aResult->mReversed) &&
           ReadParam(aMsg, aIter, &aResult->mExpandToClusterBoundary) &&
           ReadParam(aMsg, aIter, &aResult->mSucceeded);
  }
};

} 

#endif 

