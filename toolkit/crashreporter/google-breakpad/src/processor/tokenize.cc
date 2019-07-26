




























#include <string.h>

#include <string>
#include <vector>

#include "common/using_std_string.h"

namespace google_breakpad {

#ifdef _WIN32
#define strtok_r strtok_s
#endif

using std::vector;

bool Tokenize(char *line,
	      const char *separators,
	      int max_tokens,
	      vector<char*> *tokens) {
  tokens->clear();
  tokens->reserve(max_tokens);

  int remaining = max_tokens;

  
  
  char *save_ptr;
  char *token = strtok_r(line, separators, &save_ptr);
  while (token && --remaining > 0) {
    tokens->push_back(token);
    if (remaining > 1)
      token = strtok_r(NULL, separators, &save_ptr);
  }

  
  if (remaining == 0 && (token = strtok_r(NULL, "\r\n", &save_ptr))) {
    tokens->push_back(token);
  }

  return tokens->size() == static_cast<unsigned int>(max_tokens);
}

void StringToVector(const string &str, vector<char> &vec) {
  vec.resize(str.length() + 1);
  std::copy(str.begin(), str.end(),
	    vec.begin());
  vec[str.length()] = '\0';
}

} 
