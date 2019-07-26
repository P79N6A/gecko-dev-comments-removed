






























#ifndef GOOGLE_BREAKPAD_PROCESSOR_TOKENIZE_H_
#define GOOGLE_BREAKPAD_PROCESSOR_TOKENIZE_H_

#include <string>
#include <vector>

namespace google_breakpad {












bool Tokenize(char *line,
	      const char *separators,
	      int max_tokens,
	      std::vector<char*> *tokens);


void StringToVector(const std::string &str, std::vector<char> &vec);

}  

#endif  
