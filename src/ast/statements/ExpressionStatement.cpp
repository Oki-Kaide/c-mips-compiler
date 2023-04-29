#include "ExpressionStatement.hpp"

/* ************************* */

ExpressionStatement::ExpressionStatement() : Statement(), expression(NULL) {}

ExpressionStatement::ExpressionStatement(Expression* e) : Statement(), expression(e) {}

void ExpressionStatement::Debug(std::ostream& dst, int indent) const {
	dst << std::endl << spaces(indent) << "Statement:" << sourceLine << ": ";
	if(expression) {
		expression->Debug(dst, indent);
	} else {
		dst << "null expression";
	}
}

void ExpressionStatement::MakeIR(VariableMap const& bindings, FunctionStack& stack, IRVector& out) const {
	if(expression) {
		expression->MakeIR(bindings, stack, out);
	}
}
