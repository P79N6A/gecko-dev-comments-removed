






#ifndef SemanticAnalysis_h__
#define SemanticAnalysis_h__

namespace js {

struct Parser;
class StackFrame;

namespace frontend {






bool
AnalyzeFunctions(Parser *parser, StackFrame *callerFrame);

} 
} 

#endif 
