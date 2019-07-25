





#ifndef COMPILER_PREPROCESSOR_LEXER_GLUE_H_
#define COMPILER_PREPROCESSOR_LEXER_GLUE_H_

struct InputSrc;

InputSrc* LexerInputSrc(int count, const char* const string[], const int length[]);

#endif  

