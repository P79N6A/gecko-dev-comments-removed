



































#ifndef PROCESSOR_POSTFIX_EVALUATOR_INL_H__
#define PROCESSOR_POSTFIX_EVALUATOR_INL_H__


#include <cstdio>
#include <sstream>

#include "processor/postfix_evaluator.h"
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
bool PostfixEvaluator<ValueType>::Evaluate(const string &expression,
                                           DictionaryValidityType *assigned) {
  
  AutoStackClearer clearer(&stack_);

  
  istringstream stream(expression);
  string token;
  while (stream >> token) {
    
    
    
    enum BinaryOperation {
      BINARY_OP_NONE = 0,
      BINARY_OP_ADD,
      BINARY_OP_SUBTRACT,
      BINARY_OP_MULTIPLY,
      BINARY_OP_DIVIDE_QUOTIENT,
      BINARY_OP_DIVIDE_MODULUS
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

    if (operation != BINARY_OP_NONE) {
      
      ValueType operand1, operand2;
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
        BPLOG(ERROR) << "Could not PopValue to get value to assign: " <<
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
  }

  
  
  
  BPLOG_IF(ERROR, !stack_.empty()) << "Incomplete execution: " << expression;
  return stack_.empty();
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
  ValueType literal;
  if (token_stream >> literal && token_stream.peek() == EOF) {
    if (value) {
      *value = literal;
    }
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
  ValueType literal;
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
      
      
      BPLOG(ERROR) << "Identifier " << token << " not in dictionary";
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
