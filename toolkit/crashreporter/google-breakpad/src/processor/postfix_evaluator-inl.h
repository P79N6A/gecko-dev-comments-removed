





































#ifndef PROCESSOR_POSTFIX_EVALUATOR_INL_H__
#define PROCESSOR_POSTFIX_EVALUATOR_INL_H__

#include "processor/postfix_evaluator.h"

#include <stdio.h>

#include <sstream>

#include "google_breakpad/processor/memory_region.h"
#include "common/logging.h"

namespace google_breakpad {

using std::istringstream;
using std::ostringstream;




template<typename ValueType>
class AutoStackClearer {
 public:
  explicit AutoStackClearer(vector<StackElem<ValueType> > *stack)
    : stack_(stack) {}
  ~AutoStackClearer() { stack_->clear(); }

 private:
  vector<StackElem<ValueType> > *stack_;
};


template<typename ValueType>
bool PostfixEvaluator<ValueType>::EvaluateToken(
    const string &token,
    const string &expression,
    DictionaryValidityType *assigned) {
  
  
  
  enum BinaryOperation {
    BINARY_OP_NONE = 0,
    BINARY_OP_ADD,
    BINARY_OP_SUBTRACT,
    BINARY_OP_MULTIPLY,
    BINARY_OP_DIVIDE_QUOTIENT,
    BINARY_OP_DIVIDE_MODULUS,
    BINARY_OP_ALIGN
  };

  BinaryOperation operation = BINARY_OP_NONE;
  if (token == "+")
    operation = BINARY_OP_ADD;
  else if (token == "-")
    operation = BINARY_OP_SUBTRACT;
  else if (token == "*")
    operation = BINARY_OP_MULTIPLY;
  else if (token == "/")
    operation = BINARY_OP_DIVIDE_QUOTIENT;
  else if (token == "%")
    operation = BINARY_OP_DIVIDE_MODULUS;
  else if (token == "@")
    operation = BINARY_OP_ALIGN;

  if (operation != BINARY_OP_NONE) {
    
    ValueType operand1 = ValueType();
    ValueType operand2 = ValueType();
    if (!PopValues(&operand1, &operand2)) {
      BPLOG(ERROR) << "Could not PopValues to get two values for binary "
                      "operation " << token << ": " << expression;
      return false;
    }

    
    ValueType result;
    switch (operation) {
      case BINARY_OP_ADD:
        result = operand1 + operand2;
        break;
      case BINARY_OP_SUBTRACT:
        result = operand1 - operand2;
        break;
      case BINARY_OP_MULTIPLY:
        result = operand1 * operand2;
        break;
      case BINARY_OP_DIVIDE_QUOTIENT:
        result = operand1 / operand2;
        break;
      case BINARY_OP_DIVIDE_MODULUS:
        result = operand1 % operand2;
        break;
      case BINARY_OP_ALIGN:
	result =
	  operand1 & (static_cast<ValueType>(-1) ^ (operand2 - 1));
        break;
      case BINARY_OP_NONE:
        
        
        BPLOG(ERROR) << "Not reached!";
        return false;
        break;
    }

    
    PushValue(result);
  } else if (token == "^") {
    
    if (!memory_) {
      BPLOG(ERROR) << "Attempt to dereference without memory: " <<
                      expression;
      return false;
    }

    ValueType address;
    if (!PopValue(&address)) {
      BPLOG(ERROR) << "Could not PopValue to get value to dereference: " <<
                      expression;
      return false;
    }

    ValueType value;
    if (!memory_->GetMemoryAtAddress(address, &value)) {
      BPLOG(ERROR) << "Could not dereference memory at address " <<
                      HexString(address) << ": " << expression;
      return false;
    }

    PushValue(value);
  } else if (token == "=") {
    
    ValueType value;
    if (!PopValue(&value)) {
      BPLOG(INFO) << "Could not PopValue to get value to assign: " <<
                     expression;
      return false;
    }

    
    
    
    const UniqueString* identifier;
    if (PopValueOrIdentifier(NULL, &identifier) != POP_RESULT_IDENTIFIER) {
      BPLOG(ERROR) << "PopValueOrIdentifier returned a value, but an "
                      "identifier is needed to assign " <<
                      HexString(value) << ": " << expression;
      return false;
    }
    if (identifier == ustr__empty() || Index(identifier,0) != '$') {
      BPLOG(ERROR) << "Can't assign " << HexString(value) << " to " <<
                      identifier << ": " << expression;
      return false;
    }

    dictionary_->set(identifier, value);
    if (assigned)
      assigned->set(identifier, true);
  } else {
    
    
    
    
    
    
    
    
    
    
    
    
    istringstream token_stream(token);
    ValueType literal = ValueType();
    bool negative = false;
    if (token_stream.peek() == '-') {
      negative = true;
      token_stream.get();
    }
    if (token_stream >> literal && token_stream.peek() == EOF) {
      PushValue(negative ? (-literal) : literal);
    } else {
      PushIdentifier(ToUniqueString(token));
    }
  }
  return true;
}

template<typename ValueType>
bool PostfixEvaluator<ValueType>::EvaluateInternal(
    const string &expression,
    DictionaryValidityType *assigned) {
  
  istringstream stream(expression);
  string token;
  while (stream >> token) {
    
    
    
    
    
    if (token.size() > 1 && token[0] == '=') {
      if (!EvaluateToken("=", expression, assigned)) {
        return false;
      }

      if (!EvaluateToken(token.substr(1), expression, assigned)) {
        return false;
      }
    } else if (!EvaluateToken(token, expression, assigned)) {
      return false;
    }
  }

  return true;
}

template<typename ValueType>
bool PostfixEvaluator<ValueType>::Evaluate(const Module::Expr& expr,
                                           DictionaryValidityType* assigned) {
  
  
  if (expr.how_ != Module::kExprPostfix) {
    BPLOG(ERROR) << "Can't evaluate for side-effects: " << expr;
    return false;
  }

  
  AutoStackClearer<ValueType> clearer(&stack_);

  if (!EvaluateInternal(expr.postfix_, assigned))
    return false;

  
  
  
  if (stack_.empty())
    return true;

  BPLOG(ERROR) << "Incomplete execution: " << expr;
  return false;
}

template<typename ValueType>
bool PostfixEvaluator<ValueType>::EvaluateForValue(const Module::Expr& expr,
                                                   ValueType* result) {
  switch (expr.how_) {

    
    
    case Module::kExprPostfix: {
      
      AutoStackClearer<ValueType> clearer(&stack_);

      if (!EvaluateInternal(expr.postfix_, NULL))
        return false;

      
      if (stack_.size() != 1) {
        BPLOG(ERROR) << "Expression yielded bad number of results: "
                     << "'" << expr << "'";
        return false;
      }

      return PopValue(result);
    }

    
    case Module::kExprSimple:
    case Module::kExprSimpleMem: {
      
      bool found = false;
      ValueType v = dictionary_->get(&found, expr.ident_);
      if (!found) {
        
        
        static uint64_t n_complaints = 0; 
        n_complaints++;
        if (is_power_of_2(n_complaints)) {
          BPLOG(INFO) << "Identifier " << FromUniqueString(expr.ident_)
                      << " not in dictionary (kExprSimple{Mem})"
                      << " (shown " << n_complaints << " times)";
        }
        return false;
      }

      
      ValueType sum = v + (int64_t)expr.offset_;

      
      if (expr.how_ == Module::kExprSimpleMem) {
        ValueType derefd;
        if (!memory_ || !memory_->GetMemoryAtAddress(sum, &derefd)) {
          return false;
        }
        *result = derefd;
      } else {
        *result = sum;
      }
      return true;
    }

    default:
      return false;
  }
}


template<typename ValueType>
typename PostfixEvaluator<ValueType>::PopResult
PostfixEvaluator<ValueType>::PopValueOrIdentifier(
    ValueType *value, const UniqueString** identifier) {
  
  if (!stack_.size())
    return POP_RESULT_FAIL;

  StackElem<ValueType> el = stack_.back();
  stack_.pop_back();

  if (el.isValue) {
    if (value)
      *value = el.u.val;
    return POP_RESULT_VALUE;
  } else {
    if (identifier)
      *identifier = el.u.ustr;
    return POP_RESULT_IDENTIFIER;
  }
}


template<typename ValueType>
bool PostfixEvaluator<ValueType>::PopValue(ValueType *value) {
  ValueType literal = ValueType();
  const UniqueString* token;
  PopResult result;
  if ((result = PopValueOrIdentifier(&literal, &token)) == POP_RESULT_FAIL) {
    return false;
  } else if (result == POP_RESULT_VALUE) {
    
    *value = literal;
  } else {  
    
    
    bool found = false;
    ValueType v = dictionary_->get(&found, token);
    if (!found) {
      
      
      BPLOG(INFO) << "Identifier " << FromUniqueString(token)
                  << " not in dictionary";
      return false;
    }

    *value = v;
  }

  return true;
}


template<typename ValueType>
bool PostfixEvaluator<ValueType>::PopValues(ValueType *value1,
                                            ValueType *value2) {
  return PopValue(value2) && PopValue(value1);
}


template<typename ValueType>
void PostfixEvaluator<ValueType>::PushValue(const ValueType &value) {
  StackElem<ValueType> el(value);
  stack_.push_back(el);
}

template<typename ValueType>
void PostfixEvaluator<ValueType>::PushIdentifier(const UniqueString* str) {
  StackElem<ValueType> el(str);
  stack_.push_back(el);
}


}  


#endif  
