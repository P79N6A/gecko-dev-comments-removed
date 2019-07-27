





#include "compiler/translator/intermediate.h"

#include <set>

class TInfoSinkBase;

class ValidateOutputs : public TIntermTraverser
{
  public:
    ValidateOutputs(TInfoSinkBase& sink, int maxDrawBuffers);

    int numErrors() const { return mNumErrors; }

    virtual void visitSymbol(TIntermSymbol*);

  private:
    TInfoSinkBase& mSink;
    int mMaxDrawBuffers;
    int mNumErrors;
    bool mHasUnspecifiedOutputLocation;

    typedef std::map<int, TIntermSymbol*> OutputMap;
    OutputMap mOutputMap;
    std::set<TString> mVisitedSymbols;

    void error(TSourceLoc loc, const char *reason, const char* token);
};
