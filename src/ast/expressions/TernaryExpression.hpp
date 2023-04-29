#ifndef AST_TERNARY_EXPRESSION_H
#define AST_TERNARY_EXPRESSION_H

#include <iostream>

#include "../Expression.hpp"

class TernaryExpression : public Expression {

public:
	Expression* condition;
	Expression* true_branch;
	Expression* false_branch;

	TernaryExpression(Expression* condition, Expression* t, Expression* f);

	virtual void Debug(std::ostream& dst, int indent) const;
	virtual Type GetType(VariableMap const& bindings) const;

	virtual std::string MakeIR(VariableMap const& bindings, FunctionStack& stack, IRVector& out) const;
	virtual std::string MakeIR_lvalue(VariableMap const& bindings, FunctionStack& stack, IRVector& out) const;

	virtual int32_t evaluate_int(VariableMap const& bindings) const;
};

#endif
