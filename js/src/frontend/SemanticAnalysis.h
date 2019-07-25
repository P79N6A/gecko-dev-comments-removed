






#ifndef SemanticAnalysis_h__
#define SemanticAnalysis_h__

namespace js {

class StackFrame;

namespace frontend {

class Parser;






bool
AnalyzeFunctions(Parser *parser, StackFrame *callerFrame);

} 
} 

#endif 
