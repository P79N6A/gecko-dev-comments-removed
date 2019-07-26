




int CalculatePerformExpression(char* expr,
                               int significantDigits,
                               int flags,
                               char* answer);

int main() {
  char buffer[1024];
  return CalculatePerformExpression("42", 1, 0, buffer);
}

