





































#ifndef PROCESSOR_POSTFIX_EVALUATOR_INL_H__
#define PROCESSOR_POSTFIX_EVALUATOR_INL_H__

#include "processor/postfix_evaluator.h"

#include <stdio.h>

#include <sstream>

#include "google_breakpad/processor/memory_region.h"
#include "processor/logging.h"

namespace google_breakpad {

using std::istringstream;
using std::ostringstream;




class AutoStackClearer {
 public:
  explicit AutoStackClearer(vector<string> *stack) : stack_(stack) {}
  ~AutoStackClearer() { stack_->clear(); }

 private:
  vector<string> *stack_;
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
      BPLOG(ERROR) << "Could not PopValue to get value to derefence: " <<
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

    
    
    
    string identifier;
    if (PopValueOrIdentifier(NULL, &identifier) != POP_RESULT_IDENTIFIER) {
      BPLOG(ERROR) << "PopValueOrIdentifier returned a value, but an "
                      "identifier is needed to assign " <<
                      HexString(value) << ": " << expression;
      return false;
    }
    if (identifier.empty() || identifier[0] != '$') {
      BPLOG(ERROR) << "Can't assign " << HexString(value) << " to " <<
                      identifier << ": " << expression;
      return false;
    }

    (*dictionary_)[identifier] = value;
    if (assigned)
      (*assigned)[identifier] = true;
  } else {
    
    
    
    
    stack_.push_back(token);
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
bool PostfixEvaluator<ValueType>::Evaluate(const string &expression,
                                           DictionaryValidityType *assigned) {
  
  AutoStackClearer clearer(&stack_);

  if (!EvaluateInternal(expression, assigned))
    return false;

  
  
  
  if (stack_.empty())
    return true;

  BPLOG(ERROR) << "Incomplete execution: " << expression;
  return false;
}

template<typename ValueType>
bool PostfixEvaluator<ValueType>::EvaluateForValue(const string &expression,
                                                   ValueType *result) {
  
  AutoStackClearer clearer(&stack_);

  if (!EvaluateInternal(expression, NULL))
    return false;

  
  if (stack_.size() != 1) {
    BPLOG(ERROR) << "Expression yielded bad number of results: "
                 << "'" << expression << "'";
    return false;
  }

  return PopValue(result);
}

template<typename ValueType>
typename PostfixEvaluator<ValueType>::PopResult
PostfixEvaluator<ValueType>::PopValueOrIdentifier(
    ValueType *value, string *identifier) {
  
  if (!stack_.size())
    return POP_RESULT_FAIL;

  string token = stack_.back();
  stack_.pop_back();

  
  
  
  
  
  
  
  
  
  istringstream token_stream(token);
  ValueType literal = ValueType();
  bool negative;
  if (token_stream.peek() == '-') {
    negative = true;
    token_stream.get();
  } else {
    negative = false;
  }
  if (token_stream >> literal && token_stream.peek() == EOF) {
    if (value) {
      *value = literal;
    }
    if (negative)
      *value = -*value;
    return POP_RESULT_VALUE;
  } else {
    if (identifier) {
      *identifier = token;
    }
    return POP_RESULT_IDENTIFIER;
  }
}


template<typename ValueType>
bool PostfixEvaluator<ValueType>::PopValue(ValueType *value) {
  ValueType literal = ValueType();
  string token;
  PopResult result;
  if ((result = PopValueOrIdentifier(&literal, &token)) == POP_RESULT_FAIL) {
    return false;
  } else if (result == POP_RESULT_VALUE) {
    
    *value = literal;
  } else {  
    
    
    typename DictionaryType::const_iterator iterator =
        dictionary_->find(token);
    if (iterator == dictionary_->end()) {
      
      
      BPLOG(INFO) << "Identifier " << token << " not in dictionary";
      return false;
    }

    *value = iterator->second;
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
  ostringstream token_stream;
  token_stream << value;
  stack_.push_back(token_stream.str());
}


}  


#endif  
