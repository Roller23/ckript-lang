#include "lexer.hpp"
#include <iterator>
#include <fstream>

void Lexer::tokenize(const std::string &code) {

}

void Lexer::process_file(const std::string &filename) {
  std::ifstream file(filename);
  if (!file) {
    this->last_error = FILE_ERROR;
    return;
  }
  std::string buffer(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{});
  tokenize(buffer);
}