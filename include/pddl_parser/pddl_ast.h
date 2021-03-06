
/***************************************************************************
 *  pddlast.h
 *
 *  Created: Fri 19 May 2017 14:07:13 CEST
 *  Copyright  2017  Matthias Loebach
 *                   Till Hofmann
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

#ifndef PLUGINS_PDDL_AST_H_
#define PLUGINS_PDDL_AST_H_

#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/std_pair.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/support_line_pos_iterator.hpp>
#include <string>
#include <vector>

namespace pddl_parser {
namespace qi    = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

typedef std::pair<std::string, std::string> pair_type;
typedef std::vector<pair_type>              pairs_type;

typedef std::vector<std::string>                                      type_list;
typedef std::pair<type_list, std::string>                             pair_multi_const;
typedef std::vector<pair_multi_const>                                 pairs_multi_consts;
typedef std::pair<std::vector<std::string>, std::vector<std::string>> pair_strings_type;

typedef std::pair<std::string, std::string>       string_pair_type;
typedef std::vector<string_pair_type>             string_pairs_type;
typedef std::pair<std::string, string_pairs_type> predicate_type;

typedef boost::spirit::line_pos_iterator<std::string::const_iterator> iterator_type;

using Atom = std::string;

struct Predicate;
struct QuantifiedFormula;

enum ExpressionType {
	BOOL,
	NUMERIC_COMP,
	PREDICATE,
	NUMERIC,
	NUMERIC_CHANGE,
	VALUE,
	ATOM,
	DURATIVE,
	QUANTIFIED,
	COND_EFFECT,
	UNKNOWN
};

typedef boost::
  variant<Atom, boost::recursive_wrapper<Predicate>, boost::recursive_wrapper<QuantifiedFormula>>
    expression_t;

/** @class Expression
   * A PDDL Expression.
   */
struct Expression
{
	/** The type of the expression, determined at parsing time. */
	ExpressionType type;
	/** The expression formula */
	expression_t expression;
};

/** @class QuantifiedFormula
   * A PDDL quantified formula.
   */
struct QuantifiedFormula
{
	/** The name of the quantifier ('exists' or 'forall') */
	Atom quantifier;

	/** args that are bound by the quantifier */
	string_pairs_type args;

	/** Sub-expression that is quantified over */
	Expression sub_expr;
};

/** @class Predicate
   * A PDDL formula (either part of a precondition or an effect(.
   * Note that this is NOT necesarily a PDDL predicate, but may also be a
   * compound formula. For a conjunction, the function would be 'and', and the
   * arguments would be the subformulae.
   */
struct Predicate
{
	/** The name of the predicate for atomic formulae, 'and' for a conjunction,
     * 'or' for a disjunction, 'not' for a negation.
     */
	Atom function;
	/** The arguments of the predicate or the subformulae of the compound
     * formula.
     */
	std::vector<Expression> arguments;
};

/** @class Function
   * A structured representation of a PDDL function.
   */
struct Function
{
	/** The name of the function. */
	std::string name;
	/** A typed list of function parameters. */
	string_pairs_type object_params;
};

/** @class Action
   * A structured representation of a PDDL action.
   */
struct Action
{
	/** The name of the action. */
	std::string name;
	/** A typed list of action parameters. */
	string_pairs_type action_params;
	/** The action duration in temporal domains. */
	Expression duration;
	/** The precondition of an action. May be a compound. */
	Expression precondition;
	/** The effect of an action. May be a compound. */
	Expression effect;
	/** Used by the STN generator to determine conditional break points in the
     * STN.
     */
	Expression cond_breakup;
	/** Used by the STN generator to determine temporal break points in the STN.
     */
	Expression temp_breakup;
};

/** @class Domain
   * A structured representation of a PDDL domain.
   */
struct Domain
{
	/** The name of the domain. */
	std::string name;
	/** A list of PDDL features required by the domain. */
	std::vector<std::string> requirements;
	/** A list of types with their super types. */
	pairs_type types;
	/** A typed list of constants defined in the domain. */
	pairs_multi_consts constants;
	/** A list of predicate names in the domain, including the types of their
     * arguments.
     */
	std::vector<predicate_type> predicates;
	/** A list of numeric functions in the domain. */
	std::vector<Function> functions;
	/** A list of actions defined in the domain. */
	std::vector<Action> actions;
};

/** @class Problem
   * A structured representation of a PDDL problem.
   */
struct Problem
{
	/** The name of the problem. */
	std::string name;
	/** The name of the domain this problem belongs to. */
	std::string domain_name;
	/** A typed list of objects in the domain. */
	pairs_multi_consts objects;
	/** A list of facts that are initially true. */
	std::vector<Expression> init;
	/** The goal of the problem. */
	Expression goal;
};

} // namespace pddl_parser

BOOST_FUSION_ADAPT_STRUCT(pddl_parser::Domain,
                          name,
                          requirements,
                          types,
                          constants,
                          predicates,
                          functions,
                          actions)

BOOST_FUSION_ADAPT_STRUCT(pddl_parser::Problem, name, domain_name, objects, init, goal)

BOOST_FUSION_ADAPT_STRUCT(pddl_parser::Action,
                          name,
                          action_params,
                          duration,
                          precondition,
                          effect,
                          cond_breakup,
                          temp_breakup)

BOOST_FUSION_ADAPT_STRUCT(pddl_parser::Predicate, function, arguments)
BOOST_FUSION_ADAPT_STRUCT(pddl_parser::QuantifiedFormula, quantifier, args, sub_expr)

BOOST_FUSION_ADAPT_STRUCT(pddl_parser::Function, name, object_params)
BOOST_FUSION_ADAPT_STRUCT(pddl_parser::Expression, type, expression)

#endif
