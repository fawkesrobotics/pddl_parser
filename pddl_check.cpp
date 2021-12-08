/***************************************************************************
 *  pddl_check.cpp - Check a PDDL domain for syntax errors
 *
 *  Created:   Wed  8 Dec 18:54:25 CET 2021
 *  Copyright  2021  Till Hofmann <hofmann@kbsg.rwth-aachen.de>
 ****************************************************************************/
/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.md file.
 */

#include "pddl_parser/pddl_ast.h"
#include "pddl_parser/pddl_parser.h"
#include <boost/program_options.hpp>
#include <boost/program_options/options_description.hpp>
#include <filesystem>
#include <fstream>
#include <ostream>
#include <spdlog/spdlog.h>

namespace {
std::string read_file(const std::filesystem::path &path) {
  std::ifstream f;
  f.open(path);
  std::ostringstream sstr;
  sstr << f.rdbuf();
  return sstr.str();
}
} // namespace

int main(int argc, const char *const argv[]) {
  boost::program_options::options_description options("Allowed options");
  std::filesystem::path domain_path;
  std::filesystem::path problem_path;
  using boost::program_options::value;
  // clang-format off
  options.add_options()
    ("help,h", "Print help message")
    ("domain", value(&domain_path), "The path to the domain file")
    ("problem", value(&problem_path), "The path to the problem file")
  ;
  // clang-format on

  boost::program_options::variables_map variables;
  boost::program_options::store(
      boost::program_options::parse_command_line(argc, argv, options),
      variables);
  boost::program_options::notify(variables);
  if (variables.count("help")) {
    std::cout << options;
    return 0;
  }
  bool success = true;
  if (!domain_path.empty()) {
    try {
      pddl_parser::PddlParser::parseDomain(read_file(domain_path));
      std::cout << "Successfully parsed domain " << domain_path << '\n';
    } catch (std::exception &e) {
      std::cerr << "Failed to parse domain:\n" << e.what();
      success = false;
    }
  }
  if (!problem_path.empty()) {
    try {
      pddl_parser::PddlParser::parseProblem(read_file(problem_path));
      std::cout << "Successfully parsed problem " << problem_path << '\n';
    } catch (std::exception &e) {
      std::cerr << "Failed to parse problem:\n" << e.what();
      success = false;
    }
  }
  return success ? 0 : 1;
}
