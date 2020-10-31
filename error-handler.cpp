#include "error-handler.hpp"

#include <string>
#include <iostream>
#include <cstdlib>

void ErrorHandler::thow_syntax_error(const std::string &cause, int line) {
  std::cout << "Syntax error: " << cause << std::endl;
  std::exit(EXIT_FAILURE);
}

void ErrorHandler::throw_file_error(const std::string &cause) {
  std::cout << "File error: " << cause << std::endl;
  std::exit(EXIT_FAILURE);
}