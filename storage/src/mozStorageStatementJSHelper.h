





#ifndef MOZSTORAGESTATEMENTJSHELPER_H
#define MOZSTORAGESTATEMENTJSHELPER_H

#include "nsIXPCScriptable.h"

class Statement;

namespace mozilla {
namespace storage {

class StatementJSHelper : public nsIXPCScriptable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCSCRIPTABLE

private:
  nsresult getRow(Statement *, JSContext *, JSObject *, JS::Value *);
  nsresult getParams(Statement *, JSContext *, JSObject *, JS::Value *);
};






class StatementJSObjectHolder : public nsIXPConnectJSObjectHolder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIXPCONNECTJSOBJECTHOLDER

  explicit StatementJSObjectHolder(nsIXPConnectJSObjectHolder* aHolder);

protected:
  virtual ~StatementJSObjectHolder() {};
  nsCOMPtr<nsIXPConnectJSObjectHolder> mHolder;
};

class StatementParamsHolder final: public StatementJSObjectHolder {
public:
  explicit StatementParamsHolder(nsIXPConnectJSObjectHolder* aHolder)
    : StatementJSObjectHolder(aHolder) {
  }

private:
  virtual ~StatementParamsHolder();
};

class StatementRowHolder final: public StatementJSObjectHolder {
public:
  explicit StatementRowHolder(nsIXPConnectJSObjectHolder* aHolder)
    : StatementJSObjectHolder(aHolder) {
  }

private:
  virtual ~StatementRowHolder();
};

} 
} 

#endif 
