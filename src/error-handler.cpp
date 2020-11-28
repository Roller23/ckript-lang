#include "error-handler.hpp"

#include <string>
#include <iostream>
#include <cstdlib>
#include <cstdint>

void ErrorHandler::throw_generic_error(const std::string &cause, std::uint32_t line) {
  std::cout << cause;
  if (line != 0) {
    std::cout << " (line " << line << ")";
  }
  std::cout << std::endl;
  std::exit(EXIT_FAILURE);
}

void ErrorHandler::throw_syntax_error(const std::string &cause, std::uint32_t line) {
  throw_generic_error("Syntax error: " + cause, line);
}

void ErrorHandler::throw_runtime_error(const std::string &cause, std::uint32_t line) {
  throw_generic_error("Runtime error: " + cause, line);
}

void ErrorHandler::throw_file_error(const std::string &cause) {
  throw_generic_error("File error: " + cause);
}