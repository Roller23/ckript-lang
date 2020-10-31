#include "error-handler.hpp"

#include <string>
#include <iostream>
#include <cstdlib>

void ErrorHandler::thow_generic_error(const std::string &cause, int line) {
  std::cout << cause;
  if (line != -1) {
    std::cout << " on line " << line;
  }
  std::cout << std::endl;
  std::exit(EXIT_FAILURE);
}

void ErrorHandler::thow_syntax_error(const std::string &cause, int line) {
  std::cout << "Syntax error: ";
  thow_generic_error(cause, line);
}

void ErrorHandler::throw_file_error(const std::string &cause) {
  std::cout << "File error: ";
  thow_generic_error(cause);
}