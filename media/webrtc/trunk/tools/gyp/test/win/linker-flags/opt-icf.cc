



void similar_function0(char* x) {
  while (*x) {
    ++x;
  }
}

void similar_function1(char* p) {
  while (*p) {
    ++p;
  }
}

void similar_function2(char* q) {
  while (*q) {
    ++q;
  }
}

int main() {
  char* x = "hello";
  similar_function0(x);
  similar_function1(x);
  similar_function2(x);
  return 0;
}
