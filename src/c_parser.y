%code requires{
	#include <stdio.h>
	#include <string.h>

	#include <vector>
	#include <string>

	#include "includes.hpp"

	//! This is to fix problems when generating C++
    int yylex(void);
    void yyerror(const char *);

    extern Node* ast_root;
}

%union{
  char* s;
  char c;
  unsigned int i;
  float f;
  double d;
  std::vector<std::string>* strvector;
  std::vector<Expression*>* expvector;
  TypeSuffix* typesuffix;
  enumerator_entry* enumentry;
  EnumType* enumlist;
  Node* ast_node;
}

%token TVOID TLONG TSHORT TCHAR TINT TFLOAT TDOUBLE UNSIGNED SIGNED
%token AUTO CONST EXTERN REGISTER STATIC VOLATILE
%token STRUCT TYPEDEF UNION ENUM
%token IF ELSE WHILE DO FOR
%token GOTO BREAK CONTINUE RETURN
%token SWITCH CASE DEFAULT

%token SIZEOF
%token '.' ARROW ',' ';' ELLIPSIS '(' ')' '[' ']' OPENBRACE CLOSEBRACE
%token '+' '-' '*' '/' '%' INCREMENT DECREMENT
%token '!' LOGICALAND LOGICALOR
%token '&' '|' '^' '~' SHIFTLEFT SHIFTRIGHT
%token '?' ':'
%token ASSIGN ASSIGNPLUS ASSIGNMINUS ASSIGNSTAR ASSIGNSLASH
%token ASSIGNMODULO ASSIGNAMPERSAND ASSIGNPIPE ASSIGNCARET
%token ASSIGNLEFTSHIFT ASSIGNRIGHTSHIFT
%token LESSTHAN GREATERTHAN EQUALS NOTEQUALS LESSOREQUAL MOREOREQUAL

%token OCTINT DECINT HEXINT FLOATINGPOINT FLOATINGDOUBLE
%token CHARLITERAL STRINGLITERAL IDENTIFIER TYPE_ALIAS

%type <s> STRINGLITERAL
%type <c> CHARLITERAL
%type <i> DECINT OCTINT HEXINT
%type <f> FLOATINGPOINT
%type <d> FLOATINGDOUBLE

%type <s> IDENTIFIER TYPE_ALIAS TypeSpecifier
%type <c> AssignmentOperator UnaryOperator
%type <i> Pointer EnumValue
%type <typesuffix> TypeSuffix
%type <enumentry> Enumerator
%type <enumlist> EnumeratorList
%type <s> StructureDeclaration EnumDeclaration
%type <strvector> DeclarationSpecifiers
%type <expvector> FunctionArgumentList
%type <ast_node> ProgramRoot TopLevelDeclaration DeclarationList Declaration Declarator Initialiser InitialiserList Function FunctionParameterList Value
%type <ast_node> InnerDeclarationBlock StatementBlock Statement CompoundStatement
%type <ast_node> ExpressionStatement LabeledStatement SwitchStatement WhileStatement ForStatement ReturnStatement BreakStatement GotoStatement IfStatement
%type <ast_node> Expression AssignmentExpression TernaryExpression LogicalOrExpression LogicalAndExpression BitwiseOrExpression BitwiseXorExpression BitwiseAndExpression
%type <ast_node> EqualityExpression ComparisonExpression ShiftExpression AdditiveExpression MultiplicativeExpression
%type <ast_node> CastExpression UnaryExpression PostfixExpression AtomicExpression

%start ProgramRoot

%%

ProgramRoot : TopLevelDeclaration {
				$$ = new ProgramRoot();
				ast_root = $$;
				dynamic_cast<ProgramRoot*>$$->add($1);
			}
			| ProgramRoot TopLevelDeclaration {
				dynamic_cast<ProgramRoot*>$$->add($2);
			}

TopLevelDeclaration : DeclarationSpecifiers DeclarationList ';' {
						$$ = $2;
						// add in the specifiers for each declaration in the list
						for(int i = 0; i < dynamic_cast<Scope*>$$->declarations.size(); i++) {
							dynamic_cast<Scope*>$$->declarations[i]->var_type = Type(*$1, dynamic_cast<Scope*>$$->declarations[i]->var_type.pointer_depth);
						}
					}
					| DeclarationSpecifiers Declarator '(' ')' ';' {
						// function declaration
						$$ = new Function();
						dynamic_cast<Function*>$$->prototype_only = true;

						// give the function a name
						dynamic_cast<Function*>$$->function_name = dynamic_cast<Declaration*>$2->identifier;

						// set the type
						dynamic_cast<Function*>$$->return_type = Type(*$1, dynamic_cast<Declaration*>$2->var_type.pointer_depth);
					}
					| DeclarationSpecifiers Declarator '(' FunctionParameterList ')' ';' {
						// function declaration
						$$ = new Function();
						dynamic_cast<Function*>$$->prototype_only = true;

						// give the function a name
						dynamic_cast<Function*>$$->function_name = dynamic_cast<Declaration*>$2->identifier;

						// set the type
						dynamic_cast<Function*>$$->return_type = Type(*$1, dynamic_cast<Declaration*>$2->var_type.pointer_depth);

						// transfer the parameter list
						dynamic_cast<Function*>$$->merge_parameters(dynamic_cast<Scope*>$4);
					}
					| DeclarationSpecifiers Declarator '(' FunctionParameterList ',' ELLIPSIS ')' ';' {
						// function declaration
						$$ = new Function();
						dynamic_cast<Function*>$$->prototype_only = true;
						dynamic_cast<Function*>$$->has_ellipsis = true;

						// give the function a name
						dynamic_cast<Function*>$$->function_name = dynamic_cast<Declaration*>$2->identifier;

						// set the type
						dynamic_cast<Function*>$$->return_type = Type(*$1, dynamic_cast<Declaration*>$2->var_type.pointer_depth);

						// transfer the parameter list
						dynamic_cast<Function*>$$->merge_parameters(dynamic_cast<Scope*>$4);
					}
					| DeclarationSpecifiers Function {
						// function definition
						$$ = $2;
						// add in the specifiers for the function type
						dynamic_cast<Function*>$$->return_type = Type(*$1, dynamic_cast<Function*>$$->return_type.pointer_depth);
					}
					| DeclarationSpecifiers ';' {
						// this is for the benefit of named structs/unions/enums with no instances
						if($1->at(0).substr(0, 7) == "struct " || $1->at(0).substr(0, 6) == "union " || $1->at(0).substr(0, 5) == "enum ") {
							$$ = NULL;
						} else {
							yyerror("syntax error: must name an identifier after a type");
						}
					}
					| TYPEDEF DeclarationSpecifiers IDENTIFIER ';' {
						// add definition
						typedefs_define($3, Type(*$2, 0));
						$$ = NULL;
					}
					| TYPEDEF DeclarationSpecifiers TypeSuffix IDENTIFIER ';' {
						// add definition
						typedefs_define($4, Type(*$2, $3->depth));
						$$ = NULL;
					}

DeclarationSpecifiers : TypeSpecifier {
					  	  $$ = new std::vector<std::string>;
						  if($1 != NULL) $$->push_back($1);
					  }
					  | DeclarationSpecifiers TypeSpecifier { if($2 != NULL) $$->push_back($2); }

TypeSpecifier	: TVOID { $$ = strdup("void"); }
				| TLONG { $$ = strdup("long"); }
				| TSHORT { $$ = strdup("short"); }
				| TCHAR { $$ = strdup("char"); }
				| TINT { $$ = strdup("int"); }
				| TFLOAT { $$ = strdup("float"); }
				| TDOUBLE { $$ = strdup("double"); }
				| SIGNED { $$ = strdup("signed"); }
				| UNSIGNED { $$ = strdup("unsigned"); }
				| CONST { $$ = NULL; }
				| VOLATILE { $$ = NULL; }
				| EXTERN { $$ = NULL; }
				| STATIC { $$ = NULL; }
				| AUTO { $$ = NULL; }
				| REGISTER { $$ = NULL; }
				| StructureDeclaration { $$ = $1; }
				| EnumDeclaration { $$ = $1; }
				| TYPE_ALIAS { $$ = $1; }


TypeSuffix	: Pointer { $$ = new TypeSuffix(1, $1); }
			| TypeSuffix Pointer  { $$->depth += 1; $$->elements *= $2; }

Pointer	: '[' ']' { $$ = 0; }
		| '[' DECINT ']' { $$ = $2; }
		| '[' OCTINT ']' { $$ = $2; }
		| '[' HEXINT ']' { $$ = $2; }
		| '*' { $$ = 0; }

DeclarationList : Declaration {
					$$ = new Scope();
					dynamic_cast<Scope*>$$->declare(dynamic_cast<Declaration*>$1);
				}
				| DeclarationList ',' Declaration {
					dynamic_cast<Scope*>$$->declare(dynamic_cast<Declaration*>$3);
				}

Declaration : Declarator {
				$$ = $1;
			}
			| Declarator '=' Initialiser {
				$$ = $1;
				dynamic_cast<Declaration*>$$->initialiser = dynamic_cast<Expression*>$3;
			}

Declarator : IDENTIFIER {
				$$ = new Declaration();
				dynamic_cast<Declaration*>$$->identifier = $1;
			}
			| TypeSuffix IDENTIFIER {
				$$ = new Declaration();
				dynamic_cast<Declaration*>$$->var_type.pointer_depth = $1->depth;
				dynamic_cast<Declaration*>$$->array_elements = $1->elements;
				dynamic_cast<Declaration*>$$->identifier = $2;
			}
			| IDENTIFIER TypeSuffix {
				$$ = new Declaration();
				dynamic_cast<Declaration*>$$->var_type.pointer_depth = $2->depth;
				dynamic_cast<Declaration*>$$->array_elements = $2->elements;
				dynamic_cast<Declaration*>$$->identifier = $1;
			}
			| TypeSuffix IDENTIFIER TypeSuffix {
				$$ = new Declaration();
				dynamic_cast<Declaration*>$$->var_type.pointer_depth = $1->depth + $3->depth;
				dynamic_cast<Declaration*>$$->array_elements = ($1->elements ? $1->elements : 1) * $3->elements;
				dynamic_cast<Declaration*>$$->identifier = $2;
			}

Initialiser : AssignmentExpression
			| OPENBRACE InitialiserList CLOSEBRACE { $$ = $2; }
			| OPENBRACE InitialiserList ',' CLOSEBRACE { $$ = $2; }

InitialiserList	: AssignmentExpression {
					$$ = new InitialiserList();
					dynamic_cast<InitialiserList*>$$->add(dynamic_cast<Expression*>$1);
				}
				| InitialiserList ',' AssignmentExpression {
					$$ = $1;
					dynamic_cast<InitialiserList*>$$->add(dynamic_cast<Expression*>$3);
				}

StructureDeclaration	: StructOrUnion IDENTIFIER OPENBRACE InnerDeclarationBlock CLOSEBRACE {
							StructureType s;
							std::string name = $2;
							s.add_members(dynamic_cast<Scope*>$4);
							structures().add(name, s);
							$$ = strdup(((std::string)"struct " + name).c_str());
						}
						| StructOrUnion OPENBRACE InnerDeclarationBlock CLOSEBRACE {
							StructureType s;
							// invent a name for this
							std::string name = unique("anonymous");
							s.add_members(dynamic_cast<Scope*>$3);
							structures().add(name, s);
							$$ = strdup(((std::string)"struct " + name).c_str());
						}
						| StructOrUnion IDENTIFIER {
							$$ = strdup(((std::string)"struct " + $2).c_str());
						}

StructOrUnion	: STRUCT
				| UNION

EnumDeclaration	: ENUM OPENBRACE EnumeratorList CLOSEBRACE {
					// invent a name for this
					std::string name = unique("anonymous");
					enums().add(name, *$3);
					delete $3;
					$$ = strdup(((std::string)"enum " + name).c_str());
				}
				| ENUM IDENTIFIER OPENBRACE EnumeratorList CLOSEBRACE {
					std::string name = $2;
					enums().add(name, *$4);
					delete $4;
					$$ = strdup(((std::string)"enum " + name).c_str());
				}
				| ENUM IDENTIFIER {
					$$ = strdup(((std::string)"enum " + $2).c_str());
				}

EnumeratorList	: Enumerator {
					$$ = new EnumType();
					if($1->with_val) {
						$$->add($1->name, $1->val);
					} else {
						$$->add($1->name);
					}
					delete $1;
				}
				| EnumeratorList ',' Enumerator {
					if($3->with_val) {
						$$->add($3->name, $3->val);
					} else {
						$$->add($3->name);
					}
					delete $3;
				}

Enumerator	: IDENTIFIER { $$ = new enumerator_entry($1, 0, false); }
			| IDENTIFIER '=' EnumValue { $$ = new enumerator_entry($1, $3, true); }

EnumValue	: DECINT { $$ = $1; }
			| HEXINT { $$ = $1; }
			| OCTINT { $$ = $1; }
			| '-' DECINT { $$ = -$2; }
			| '-' HEXINT { $$ = -$2; }
			| '-' OCTINT { $$ = -$2; }
			| '+' DECINT { $$ = $2; }
			| '+' HEXINT { $$ = $2; }
			| '+' OCTINT { $$ = $2; }

Function : Declarator '(' ')' CompoundStatement {
				$$ = new Function();

				// give the function a name
				dynamic_cast<Function*>$$->function_name = dynamic_cast<Declaration*>$1->identifier;

				// transfer the pointer depth
				dynamic_cast<Function*>$$->return_type.pointer_depth = dynamic_cast<Declaration*>$1->var_type.pointer_depth;

				// transfer the declarations and statements from the compound statement
				dynamic_cast<Function*>$$->merge_declarations(dynamic_cast<Scope*>$4);
				dynamic_cast<Function*>$$->merge_statements(dynamic_cast<Scope*>$4);
			}
			| Declarator '(' FunctionParameterList ')' CompoundStatement {
				$$ = new Function();

				// give the function a name
				dynamic_cast<Function*>$$->function_name = dynamic_cast<Declaration*>$1->identifier;

				// transfer the pointer depth
				dynamic_cast<Function*>$$->return_type.pointer_depth = dynamic_cast<Declaration*>$1->var_type.pointer_depth;

				// transfer the parameter list
				dynamic_cast<Function*>$$->merge_parameters(dynamic_cast<Scope*>$3);

				// transfer the declarations and statements from the compound statement
				dynamic_cast<Function*>$$->merge_declarations(dynamic_cast<Scope*>$5);
				dynamic_cast<Function*>$$->merge_statements(dynamic_cast<Scope*>$5);
			}

FunctionParameterList	: DeclarationSpecifiers Declarator {
							$$ = new Scope();
							dynamic_cast<Declaration*>$2->var_type.set_specifiers(*$1);
							dynamic_cast<Scope*>$$->declare(dynamic_cast<Declaration*>$2);
						}
						| FunctionParameterList ',' DeclarationSpecifiers Declarator {
							$$ = $1;
							dynamic_cast<Declaration*>$4->var_type.set_specifiers(*$3);
							dynamic_cast<Scope*>$$->declare(dynamic_cast<Declaration*>$4);
						}

CompoundStatement : OPENBRACE CLOSEBRACE { $$ = new Scope(); }
				  | OPENBRACE InnerDeclarationBlock CLOSEBRACE { $$ = $2; }
			  	  | OPENBRACE StatementBlock CLOSEBRACE { $$ = $2; }
			  	  | OPENBRACE InnerDeclarationBlock StatementBlock CLOSEBRACE { $$ = $2; dynamic_cast<Scope*>$$->merge_statements(dynamic_cast<Scope*>$3); }

InnerDeclarationBlock : DeclarationSpecifiers DeclarationList ';' {
						  // get the list of declarations
						  Scope* d = dynamic_cast<Scope*>$2;

						  // add in the specifiers for each declaration in the list
						  for(int i = 0; i < d->declarations.size(); i++) {
							  d->declarations[i]->var_type.set_specifiers(*$1);
						  }

						  // set the new declarationblock to be the declaration list
						  $$ = d;
					  }
					  | InnerDeclarationBlock DeclarationSpecifiers DeclarationList ';' {
  						  // get the list of declarations
  						  Scope* d = dynamic_cast<Scope*>$3;

						  // set the specifiers for the new declarations
						  for(int i = 0; i < d->declarations.size(); i++) {
							  d->declarations[i]->var_type.set_specifiers(*$2);
						  }

						  // add the new declarations into the existing list
						  Scope* block = dynamic_cast<Scope*>$1;
						  block->merge_declarations(d);

						  $$ = $1;
					  }
					  | DeclarationSpecifiers ';' {
						  // this is for the benefit of named structs/unions/enums with no instances
  						  if($1->at(0).substr(0, 7) == "struct " || $1->at(0).substr(0, 6) == "union " || $1->at(0).substr(0, 5) == "enum ") {
  							$$ = new Scope;
  						  } else {
  							yyerror("syntax error: must name an identifier after a type");
  						  }
					  }
					  | InnerDeclarationBlock DeclarationSpecifiers ';' {
						  // this is for the benefit of named structs/unions/enums with no instances
  						  if($2->at(0).substr(0, 7) == "struct " || $2->at(0).substr(0, 6) == "union " || $2->at(0).substr(0, 5) == "enum ") {
  							$$ = $1;
  						  } else {
  							yyerror("syntax error: must name an identifier after a type");
  						  }
					  }

StatementBlock	: Statement {
					// get the statement
					Statement* statement = dynamic_cast<Statement*>$1;

					// create a scope and add in the statement
					Scope* scope = new Scope();
					scope->add_statement(statement);

					// set the block to the new scope
					$$ = scope;
				}
				| StatementBlock Statement {
					// get the statement
					Statement* statement = dynamic_cast<Statement*>$2;

					// add statement to existing scope
					dynamic_cast<Scope*>$1->add_statement(statement);
					$$ = $1;
				}

Statement	: CompoundStatement {
				// wrap the scope in a compound statement class
				$$ = new CompoundStatement(dynamic_cast<Scope*>$1);
			}
			| LabeledStatement
			| SwitchStatement
			| WhileStatement
			| ForStatement
			| ReturnStatement
			| BreakStatement
			| GotoStatement
			| IfStatement
			| ExpressionStatement


LabeledStatement	: IDENTIFIER ':' Statement { $$ = new LabeledStatement($1, dynamic_cast<Statement*>$3); }
					| CASE TernaryExpression ':' Statement { $$ = new CaseStatement(dynamic_cast<Expression*>$2, dynamic_cast<Statement*>$4); }
					| DEFAULT ':' Statement { $$ = new CaseStatement(NULL, dynamic_cast<Statement*>$3); }

SwitchStatement : SWITCH '(' Expression ')' Statement { $$ = new SwitchStatement(dynamic_cast<Expression*>$3, dynamic_cast<Statement*>$5); }

WhileStatement	: WHILE '(' Expression ')' Statement { $$ = new WhileStatement(dynamic_cast<Expression*>$3, dynamic_cast<Statement*>$5, false); }
				| DO Statement WHILE '(' Expression ')' ';' { $$ = new WhileStatement(dynamic_cast<Expression*>$5, dynamic_cast<Statement*>$2, true); }

ForStatement	: FOR '(' ExpressionStatement ExpressionStatement ')' Statement {
					$$ = new ForStatement(dynamic_cast<ExpressionStatement*>$3->expression, dynamic_cast<ExpressionStatement*>$4->expression, NULL, dynamic_cast<Statement*>$6);
				}
				| FOR '(' ExpressionStatement ExpressionStatement Expression ')' Statement {
					$$ = new ForStatement(dynamic_cast<ExpressionStatement*>$3->expression, dynamic_cast<ExpressionStatement*>$4->expression, dynamic_cast<Expression*>$5, dynamic_cast<Statement*>$7);
				}
				| FOR '(' DeclarationSpecifiers DeclarationList ';' ExpressionStatement ')' Statement {
					$$ = new ForStatement(NULL, dynamic_cast<ExpressionStatement*>$6->expression, NULL, dynamic_cast<Statement*>$8);
					Scope* s = dynamic_cast<Scope*>$4;
					// add in the specifiers for each declaration in the list
					for(int i = 0; i < s->declarations.size(); i++) {
						s->declarations[i]->var_type.set_specifiers(*$3);
					}
					dynamic_cast<ForStatement*>$$->merge_declarations(s);
				}
				| FOR '(' DeclarationSpecifiers DeclarationList ';' ExpressionStatement Expression ')' Statement {
					$$ = new ForStatement(NULL, dynamic_cast<ExpressionStatement*>$6->expression, dynamic_cast<Expression*>$7, dynamic_cast<Statement*>$9);
					Scope* s = dynamic_cast<Scope*>$4;
					// add in the specifiers for each declaration in the list
					for(int i = 0; i < s->declarations.size(); i++) {
						s->declarations[i]->var_type.set_specifiers(*$3);
					}
					dynamic_cast<ForStatement*>$$->merge_declarations(s);
				}

ReturnStatement	: RETURN ';' { $$ = new ReturnStatement(); }
				| RETURN Expression ';' { $$ = new ReturnStatement(dynamic_cast<Expression*>$2); }

BreakStatement	: CONTINUE ';' { $$ = new BreakStatement(true); }
				| BREAK ';' { $$ = new BreakStatement(); }

GotoStatement	: GOTO IDENTIFIER ';' { $$ = new GotoStatement($2); }

IfStatement	: IF '(' Expression ')' Statement { $$ = new IfStatement(dynamic_cast<Expression*>$3, dynamic_cast<Statement*>$5, NULL); }
			| IF '(' Expression ')' Statement ELSE Statement { $$ = new IfStatement(dynamic_cast<Expression*>$3, dynamic_cast<Statement*>$5, dynamic_cast<Statement*>$7); }




ExpressionStatement	: ';' { $$ = new ExpressionStatement(); }
					| Expression ';' { $$ = new ExpressionStatement(dynamic_cast<Expression*>$1); }

Expression	: AssignmentExpression { $$ = $1; }
			| Expression ',' AssignmentExpression {
				CompoundExpression* ce = dynamic_cast<CompoundExpression*>$1;
				if(ce) {
					// add expression to compound expression
					$$ = ce;
					ce->add_expression(dynamic_cast<Expression*>$3);
				} else {
					// create a compound expression and add in the new expression
					$$ = new CompoundExpression(dynamic_cast<Expression*>$1);
					dynamic_cast<CompoundExpression*>$$->add_expression(dynamic_cast<Expression*>$3);
				}
			}

AssignmentExpression	: TernaryExpression { $$ = $1; }
						| UnaryExpression AssignmentOperator AssignmentExpression {
							$$ = new AssignmentExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, $2);
						}

AssignmentOperator	: '=' { $$ = '='; }
					| ASSIGNPLUS { $$ = '+'; }
					| ASSIGNMINUS { $$ = '-'; }
					| ASSIGNSTAR { $$ = '*'; }
					| ASSIGNSLASH { $$ = '/'; }
					| ASSIGNMODULO { $$ = '%'; }
					| ASSIGNAMPERSAND { $$ = '&'; }
					| ASSIGNPIPE { $$ = '|'; }
					| ASSIGNCARET { $$ = '^'; }
					| ASSIGNLEFTSHIFT { $$ = '<'; }
					| ASSIGNRIGHTSHIFT { $$ = '>'; }

TernaryExpression	: LogicalOrExpression
					| LogicalOrExpression '?' Expression ':' TernaryExpression { $$ = new TernaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, dynamic_cast<Expression*>$5); }

LogicalOrExpression	: LogicalAndExpression
					| LogicalOrExpression LOGICALOR LogicalAndExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_logicalor); }

LogicalAndExpression	: BitwiseOrExpression
						| LogicalAndExpression LOGICALAND BitwiseOrExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_logicaland); }

BitwiseOrExpression	: BitwiseXorExpression
					| BitwiseOrExpression '|' BitwiseXorExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_bitwiseor); }

BitwiseXorExpression	: BitwiseAndExpression
						| BitwiseXorExpression '^' BitwiseAndExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_bitwisexor); }

BitwiseAndExpression	: EqualityExpression
						| BitwiseAndExpression '&' EqualityExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_bitwiseand); }

EqualityExpression	: ComparisonExpression
					| EqualityExpression EQUALS ComparisonExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_equals); }
					| EqualityExpression NOTEQUALS ComparisonExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_notequals); }

ComparisonExpression	: ShiftExpression
						| ComparisonExpression '<' ShiftExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_lessthan); }
						| ComparisonExpression '>' ShiftExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_morethan); }
						| ComparisonExpression LESSOREQUAL ShiftExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_lessequal); }
						| ComparisonExpression MOREOREQUAL ShiftExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_moreequal); }

ShiftExpression	: AdditiveExpression
				| ShiftExpression SHIFTLEFT AdditiveExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_leftshift); }
				| ShiftExpression SHIFTRIGHT AdditiveExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_rightshift); }

AdditiveExpression	: MultiplicativeExpression
					| AdditiveExpression '+' MultiplicativeExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_add); }
					| AdditiveExpression '-' MultiplicativeExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_subtract); }

MultiplicativeExpression	: CastExpression
							| MultiplicativeExpression '%' CastExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_modulo); }
							| MultiplicativeExpression '/' CastExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_divide); }
							| MultiplicativeExpression '*' CastExpression { $$ = new BinaryExpression(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3, BinaryExpression::op_multiply); }

CastExpression	: UnaryExpression { $$ = $1; }
				| '(' TypeSpecifier ')' CastExpression {
					$$ = new CastExpression($2, 0, dynamic_cast<Expression*>$4);
				}
				| '(' TypeSpecifier TypeSpecifier ')' CastExpression {
					$$ = new CastExpression($2, $3, 0, dynamic_cast<Expression*>$5);
				}
				| '(' TypeSpecifier TypeSuffix ')' CastExpression {
					$$ = new CastExpression($2, $3->depth, dynamic_cast<Expression*>$5);
				}
				| '(' TypeSpecifier TypeSpecifier TypeSuffix ')' CastExpression {
					$$ = new CastExpression($2, $3, $4->depth, dynamic_cast<Expression*>$6);
				}

UnaryExpression	: PostfixExpression
				| INCREMENT UnaryExpression { $$ = new UnaryExpression(dynamic_cast<Expression*>$2, UnaryExpression::op_preincrement); }
				| DECREMENT UnaryExpression { $$ = new UnaryExpression(dynamic_cast<Expression*>$2, UnaryExpression::op_predecrement); }
				| UnaryOperator CastExpression {
					$$ = new UnaryExpression(dynamic_cast<Expression*>$2, $1);
				}
				| SIZEOF UnaryExpression { $$ = new UnaryExpression(dynamic_cast<Expression*>$2, UnaryExpression::op_sizeof); }
				| SIZEOF '(' DeclarationSpecifiers ')' {
					std::vector<std::string>* specs = $3;
					$$ = UnaryExpression::compute_sizeof_type(specs);
				}
				| SIZEOF '(' DeclarationSpecifiers TypeSuffix ')' {
					$$ = new Value(4);
				}

UnaryOperator	: '+' { $$ = '+'; }
				| '-' { $$ = '-'; }
				| '&' { $$ = '&'; }
				| '*' { $$ = '*'; }
				| '~' { $$ = '~'; }
				| '!' { $$ = '!'; }

PostfixExpression	: AtomicExpression
					| PostfixExpression INCREMENT { $$ = new UnaryExpression(dynamic_cast<Expression*>$1, UnaryExpression::op_postincrement); }
					| PostfixExpression DECREMENT { $$ = new UnaryExpression(dynamic_cast<Expression*>$1, UnaryExpression::op_postdecrement); }
					| IDENTIFIER '(' ')' { $$ = new FunctionCall($1); }
					| IDENTIFIER '(' FunctionArgumentList ')' { $$ = new FunctionCall($1, *$3); }
					| PostfixExpression '[' Expression ']' { $$ = new ArrayAccess(dynamic_cast<Expression*>$1, dynamic_cast<Expression*>$3); }
					| PostfixExpression '.' IDENTIFIER { $$ = new MemberAccess(dynamic_cast<Expression*>$1, $3, false); }
					| PostfixExpression ARROW IDENTIFIER { $$ = new MemberAccess(dynamic_cast<Expression*>$1, $3, true); }

FunctionArgumentList	: AssignmentExpression {
							$$ = new std::vector<Expression*>;
							$$->push_back(dynamic_cast<Expression*>$1);
						}
						| FunctionArgumentList ',' AssignmentExpression {
							$$->push_back(dynamic_cast<Expression*>$3);
						}

AtomicExpression	: Value { $$ = $1; }
					| IDENTIFIER { $$ = new Variable($1); }
					| '(' Expression ')' { $$ = $2; }

Value : DECINT { $$ = new Value($1); }
      | OCTINT { $$ = new Value($1); }
	  | HEXINT { $$ = new Value($1); }
	  | FLOATINGPOINT { $$ = new Value($1); }
	  | FLOATINGDOUBLE { $$ = new Value($1); }
	  | CHARLITERAL { $$ = new Value($1); }
	  | STRINGLITERAL { $$ = new Value($1); }

%%
