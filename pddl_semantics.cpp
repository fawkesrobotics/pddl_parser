/***************************************************************************
 *  pddl_semantics.cpp semantic checks during parsing
 *
 *  Created: Thursday 15 October 2020
 *  Copyright  2020  Tarik Viehmann
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
 *  Read the full text in the LICENSE.GPL file in the doc directory.
 */

#include <pddl_parser/pddl_exception.h>
#include <pddl_parser/pddl_semantics.h>

#include <algorithm>

namespace pddl_parser {

namespace semantics_utils {
bool
typing_required(const Domain &d)
{
	auto typing_reqs = {"typing", "adl", "ucpop"};
	for (const auto &type_req : typing_reqs) {
		if (d.requirements.end() != std::find(d.requirements.begin(), d.requirements.end(), type_req)) {
			return true;
		}
	}
	return false;
}

void
check_type_vs_requirement(const iterator_type &where, bool typing_required, const std::string &type)
{
	if ((type == "") && typing_required) {
		throw PddlTypeException(std::string("Missing type."), where);
	}
	if ((type != "") && !typing_required) {
		throw PddlTypeException(std::string("Requirement typing disabled, unexpected type found."),
		                        where);
	}
}

} // end namespace semantics_utils

pair_type
TypeSemantics::operator()(const iterator_type &where,
                          const pair_type &    parsed,
                          const Domain &       domain) const
{
	if (!semantics_utils::typing_required(domain)) {
		throw PddlTypeException(std::string("Requirement typing disabled, unexpected type found."),
		                        where);
	}
	return parsed;
}

pair_type
ParamTransformer::operator()(const iterator_type &,
                             const pair_strings_type &parsed,
                             string_pairs_type &      target) const
{
	if (parsed.second.empty()) {
		std::transform(parsed.first.begin(),
		               parsed.first.end(),
		               std::back_inserter(target),
		               [](const std::string &s) { return std::make_pair(s, ""); });
	} else {
		for (const auto &variant_type : parsed.second) {
			std::transform(parsed.first.begin(),
			               parsed.first.end(),
			               std::back_inserter(target),
			               [variant_type](const std::string &s) {
				               return std::make_pair(s, variant_type);
			               });
		}
	}
	pair_type res = target.back();
	target.pop_back();
	return res;
}

pair_multi_const
ConstantSemantics::operator()(const iterator_type &     where,
                              const pair_multi_const &  parsed,
                              const Domain &            domain,
                              std::vector<std::string> &warnings) const
{
	// typing test:
	bool typing_enabled = semantics_utils::typing_required(domain);
	if (typing_enabled) {
		auto search =
		  std::find_if(domain.types.begin(), domain.types.end(), [parsed](const pair_type &p) {
			  return p.first == parsed.second || p.second == parsed.second;
		  });
		if (search == domain.types.end()) {
			throw PddlTypeException(std::string("Unknown type: ") + parsed.second, where);
		}
	}
	semantics_utils::check_type_vs_requirement(where, typing_enabled, parsed.second);
	for (const auto &constant : parsed.first) {
		for (const auto &dom_constants : domain.constants) {
			std::for_each(dom_constants.first.begin(),
			              dom_constants.first.end(),
			              [&parsed, &constant, &dom_constants, &warnings](const auto &c) mutable {
				              if (c == constant && parsed.second != dom_constants.second) {
					              warnings.push_back(std::string("Ambiguous type: ") + constant + " type "
					                                 + parsed.second + " and " + dom_constants.second);
				              }
			              });
		}
	}
	return parsed;
}

Action
ActionSemantics::operator()(const iterator_type &where,
                            const Action &       parsed,
                            const Domain &       domain) const
{
	bool typing_enabled = semantics_utils::typing_required(domain);
	for (const auto &action_param : parsed.action_params) {
		if (typing_enabled) {
			auto search =
			  std::find_if(domain.types.begin(), domain.types.end(), [action_param](const pair_type &p) {
				  return p.first == action_param.second || p.second == action_param.second;
			  });
			if (search == domain.types.end()) {
				throw PddlTypeException(std::string("Unknown type: ") + action_param.first + " - "
				                          + action_param.second,
				                        where);
			}
		}
		semantics_utils::check_type_vs_requirement(where, typing_enabled, action_param.second);
	}
	// predicate signature test:
	string_pairs_type bound_vars;
	check_action_condition(where, parsed.precondition, domain, parsed, bound_vars);
	check_action_condition(where, parsed.effect, domain, parsed, bound_vars);

	return parsed;
}

bool
ActionSemantics::check_type(const iterator_type &where,
                            const std::string &  got,
                            const std::string &  expected,
                            const Domain &       domain)
{
	if (got != expected) {
		auto generalized_it = std::find_if(domain.types.begin(),
		                                   domain.types.end(),
		                                   [got](const pair_type &p) { return p.first == got; });
		if (generalized_it == domain.types.end()) {
			return false;
		} else {
			return check_type(where, generalized_it->second, expected, domain);
		}
	} else {
		return true;
	}
}

void
ActionSemantics::check_action_condition(const iterator_type &where,
                                        const Expression &   expr,
                                        const Domain &       domain,
                                        const Action &       curr_action,
                                        string_pairs_type &  bound_vars)
{
	auto curr_obj_type = boost::apply_visitor(ExpressionTypeVisitor(), expr.expression);
	// this function checks conditions, if the expression is an atom, then the action has an invalid structure
	if (curr_obj_type == std::type_index(typeid(Atom))) {
		throw PddlExpressionException(std::string("Unexpected Atom in expression: ")
		                                + boost::get<Atom>(expr.expression),
		                              where);
	}
	if (curr_obj_type == std::type_index(typeid(QuantifiedFormula))) {
		QuantifiedFormula f = boost::get<QuantifiedFormula>(expr.expression);
		bound_vars.insert(bound_vars.end(), f.args.begin(), f.args.end());
		check_action_condition(where, f.sub_expr, domain, curr_action, bound_vars);
	}

	if (curr_obj_type == std::type_index(typeid(Predicate))) {
		return check_action_predicate(
		  where, boost::get<Predicate>(expr.expression), expr.type, domain, curr_action, bound_vars);
	}
}

void
ActionSemantics::check_action_predicate(const iterator_type & where,
                                        const Predicate &     pred,
                                        const ExpressionType &type,
                                        const Domain &        domain,
                                        const Action &        curr_action,
                                        string_pairs_type &   bound_vars)
{
	bool typing_enabled = semantics_utils::typing_required(domain);
	switch (type) {
	case ExpressionType::BOOL: {
		for (const auto &sub_expr : pred.arguments) {
			// recursively check sub expressions of booelean expressions, they all are predicate expressions
			check_action_condition(where, sub_expr, domain, curr_action, bound_vars);
		}
		break;
	}
	case ExpressionType::PREDICATE: {
		auto defined_pred =
		  // check if the predicate name is defined in the domain ...
		  std::find_if(domain.predicates.begin(),
		               domain.predicates.end(),
		               [pred](const predicate_type &p) { return pred.function == p.first; });
		if (defined_pred == domain.predicates.end()) {
			// ... if it is not, then this predicate is invalid
			throw PddlPredicateException(std::string("Unknown predicate: ") + pred.function, where);
		} else {
			// If the predicate is defined, the signature has to match
			if (defined_pred->second.size() != pred.arguments.size()) {
				throw PddlPredicateException(std::string("Predicate argument length missmatch, expected ")
				                               + std::to_string(defined_pred->second.size()) + " but got "
				                               + std::to_string(pred.arguments.size()),
				                             where);
			} else {
				// and all arguments must be atomic expressions
				for (size_t i = 0; i < pred.arguments.size(); i++) {
					if (boost::apply_visitor(ExpressionTypeVisitor(), pred.arguments[i].expression)
					    != std::type_index(typeid(Atom))) {
						throw PddlPredicateException(std::string("Unexpected nested predicate."), where);
					} else {
						Atom        curr_arg      = boost::get<Atom>(pred.arguments[i].expression);
						bool        is_type_error = false;
						std::string arg_type      = "";
						if (curr_arg.front() != '?') {
							// constants need to be known
							bool constant_found = false;
							auto constant_match = std::find_if(
							  domain.constants.begin(),
							  domain.constants.end(),
							  [&curr_arg, &domain, &defined_pred, &i, &where, &constant_found, &arg_type](
							    const pair_multi_const &c) mutable {
								  if (c.first.end() != std::find(c.first.begin(), c.first.end(), curr_arg)) {
									  constant_found = true;
									  arg_type += " " + c.second;
									  return check_type(where, c.second, defined_pred->second[i].second, domain);
								  } else {
									  return false;
								  }
							  });
							if (constant_match == domain.constants.end()) {
								is_type_error = true;
								if (!constant_found) {
									throw PddlConstantException(std::string("Unknown constant ") + curr_arg, where);
								}
							}
						} else {
							auto bound_vars_match =
							  std::find_if(bound_vars.begin(), bound_vars.end(), [curr_arg](const pair_type &c) {
								  return c.first == curr_arg.substr(1, std::string::npos);
							  });
							if (bound_vars_match == bound_vars.end()) {
								auto parameter_match =
								  std::find_if(curr_action.action_params.begin(),
								               curr_action.action_params.end(),
								               [curr_arg](const pair_type &c) {
									               return c.first == curr_arg.substr(1, std::string::npos);
								               });
								if (parameter_match == curr_action.action_params.end()) {
									throw PddlParameterException(std::string("Unknown Parameter ") + curr_arg, where);
								} else {
									arg_type = parameter_match->second;
								}
							} else {
								arg_type = bound_vars_match->second;
							}
							is_type_error = !check_type(where, arg_type, defined_pred->second[i].second, domain);
						}
						// and if typing is required, then the types should match the signature
						if (typing_enabled && is_type_error) {
							throw PddlTypeException(std::string("Type missmatch: Argument ") + std::to_string(i)
							                          + " of " + defined_pred->first + " expects "
							                          + defined_pred->second[i].second + " but got " + arg_type,
							                        where);
						}
					}
				}
			}
		}
		break;
	}
	default: break;
	}
}
} // namespace pddl_parser
