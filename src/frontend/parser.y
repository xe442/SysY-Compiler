%require "3.2" // Requires a higher verision of Bison here so that useless files
			   // would not be generated.
%language "c++"

%defines "parser.tab.h" // Generate a header file with specific filename.
%locations // Track token locations.

%define parse.trace // enable parser trace

%code requires
{
	#include <memory>
	#include "ast_node.h"
	#include "type.h"
}

%code provides
{
	// Bison does not define YYSTYPE and YYLTYPE in a C++ parser, so we have to
	// define them for C lexer explicitly.
	typedef yy::parser::semantic_type YYSTYPE;
	typedef yy::parser::location_type YYLTYPE;

	// Call yylex in C style, since it comes from lib flex.
	extern "C"
		int yylex(YYSTYPE *yylval,YYLTYPE *yylloc);
}

%define api.value.type variant // Set the type of lvals.

// These lval types are not used.
%token<int> CONST INT VOID IF ELSE WHILE BREAK CONT RET
%token<int> LCBRKT RCBRKT SEMI COMMA
%token<int> ADD SUB MUL DIV MOD NOT GT LT ASSIGN EQ GE LE NE AND OR LBRKT RBRKT
			LSBRKT RSBRKT

// The used token lval type.
%token<int> UINT
%token<std::string> ID

%nterm<compiler::define::AstPtr> 
		program var_decl single_var_decl array_initializer func_def func_def_params
		func_def_param arr_param_decl block block_item statement assign_statement
		if_statement while_statement return_statement break_statement cont_statement
		expr array_access func_call func_args
%nterm<compiler::define::AstPtrVec>
		var_decl_list array_initializer_list block_item_list
%nterm<compiler::define::TypePtr> base_type

%precedence IF
%precedence ELSE // Make the precedence of ELSE statement higher than IF.
%left OR
%left AND
%left GT LT GE LE EQ NE
%left ADD SUB
%left MUL DIV MOD
%precedence NOT NEG

%%

program: // ProgramNode
  %empty { $$ = std::make_unique<compiler::define::ProgramNode>(); }
| program[subprogram] var_decl
		{
			$$ = std::move($subprogram);
			$$->push_back(std::move($var_decl));
		}
| program[subprogram] func_def
		{
			$$ = std::move($subprogram);
			$$->push_back(std::move($func_def));
		}
;

// e.g. "const int a = 3, b[3] = {1, 2, 3}, c;"
var_decl: // VarDeclNode
  base_type var_decl_list SEMI
		{
			$$ = compiler::define::VarDeclNode(
				std::move($base_type), std::move($var_decl_list)
			);
		}
;

// e.g. "a = 3, b[3] = {1, 2, 3}, c"
var_decl_list: // vector<SingleVarDeclNode>
  var_decl_list[sublist] COMMA single_var_decl
		{
			$$ = std::move($sublist);
			$$.push_back(std::move($single_var_decl));
		}
| single_var_decl
		{
			$$ = compiler::define::AstPtrVec();
			$$.push_back(std::move($single_var_decl));
		}
;


// e.g. "a = b" or "b[3] = {a, 2 * a}"
single_var_decl: // SingleVarDeclNode
  ID	{
  			auto id_node = std::make_unique<compiler::define::IdNode>($ID);
  			$$ = std::make_unique<compiler::define::SingleVarDeclNode>(id_node);
  		}
| ID ASSIGN expr
		{
			auto id_node = std::make_unique<compiler::define::IdNode>($ID);
			$$ = std::make_unique<compiler::define::SingleVarDeclNode>(
				std::move(id_node), std::move($expr)
			);
		}
| array_access
		{
			$$ = std::make_unique<compiler::define::SingleVarDeclNode>($array_access);
		}
| array_access ASSIGN array_initializer
		{
			$$ = std::make_unique<compiler::define::SingleVarDeclNode>(
				std::move($array_access), std::move($array_initializer)
			);
		}
;

// e.g. "{{1, 2}, {3}, {}, 4, {{5,6}, {7}}}"
array_initializer: // InitializerNode
  LCBRKT array_initializer_list RCBRKT
  		{
  			$$ = compiler::define::InitializerNode(
  				std::move($array_initializer_list)
  			);
  		}
| LCBRKT RCBRKT
		{
			$$ = compiler::define::InitializerNode();
		}
;

// a list contained by array_initializers or exprs, separated by comma
// e.g. "{1, 2}, {3}, {}, 4, {{5,6}, {7}}"
array_initializer_list: // vector<IdNode|ConstIntNode|UnaryOpNode|BinaryOpNode|FuncCallNode|InitializerNode>
  array_initializer_list[sublist] COMMA array_initializer
  		{
  			$$ = std::move($sublist);
  			$$.push_back($array_initializer);
  		}
| array_initializer_list[sublist] COMMA expr
		{
			$$ = std::move($sublist);
			$$.push_back($expr);
		}
| array_initializer
		{
			$$ = compiler::define::AstPtrVec();
			$$.push_back($array_initializer);
		}
| expr
		{
			$$ = compiler::define::AstPtrVec();
			$$.push_back($expr);
		}
;

// e.g. "void func(int len, int arr[][100]) { ... }"
func_def: // FuncDefNode
  base_type ID LBRKT func_def_params RBRKT block
  		{
  			auto id_node = std::make_unique<compiler::define::IdNode>($ID);
  			$$ = std::make_unique<compiler::define::FuncDefNode>(
  				$base_type, std::move(id_node), std::move($func_def_params),
  				std::move($block)
  			);
  		}
| base_type ID LBRKT RBRKT block
		{
			auto id_node = std::make_unique<compiler::define::IdNode>($ID);
			$$ = std::make_unique<compiler::define::FuncDefNode>(
				$base_type, std::move(id_node), std::move($block)
			);
		}
;

// e.g. "int len, int arr[][100]"
func_def_params: // FuncParamsNode
  func_def_params[sublist] COMMA func_def_param
  		{
  			$$ = std::move(sublist);
  			$$->push_back($func_def_param);
  		}
| func_def_param
		{
			$$ = std::make_unique<compiler::define::FuncParamsNode>();
			$$->push_back(std::move($func_def_param));
		}
;

// e.g. "int len" or "int arr[][100]"
func_def_param: // SingleFuncParamNode
  base_type ID
  		{
  			auto id_node = std::make_unique<compiler::define::IdNode>($ID);
  			$$ = std::make_unique<compiler::define::SingleFuncParamNode>(
  				$base_type, std::move(id_node)
  			);
  		}
| base_type arr_param_decl
		{
			$$ = std::make_unique<compiler::define::SingleFuncParamNode>(
				$base_type, std::move($arr_param_decl)
			);
		}
;

// e.g. "arr[]" or "arr[][100]"
arr_param_decl: // BinaryOpNode(ACCESS only)|UnaryOpNode(POINTER only)
  ID LSBRKT RSBRKT
  		{
  			auto id_node = std::make_unique<compiler::define::IdNode>($ID);
  			$$ = make_unique<compiler::define::UnaryOpNode>(
  				compiler::define::UnaryOpNode::POINTER, std::move(id_node);
  			);
  		}
| arr_param_decl[subdecl] LSBRKT expr RSBRKT
		{
			$$ = make_unique<compiler::define::BinaryOpNode>(
				compiler::define::BinaryOpNode::ACCESS,
				std::move($subdecl), std::move($expr)
			);
		}
;

// e.g. "{ int a = 3; return a; }" or "{}"
block: // BlockNode
  LCBRKT block_item_list RCBRKT
  		{ $$ = std::make_unique<compiler::define::compiler::define::BlockNode>(std::move($block_item_list)); }
| LCBRKT RCBRKT
		{ $$ = std::make_unique<compiler::define::compiler::define::BlockNode>(); }
;

// e.g. "int a = 3; return a;"
block_item_list: // vector<VarDeclNode|IdNode|ConstIntNode|UnaryOpNode|BinaryOpNode|FuncCallNode|IfNode|WhileNode|BreakNode|ContNode|RetNode|BlockNode>
  block_item_list[sublist] block_item
  		{
  			$$ = std::move($sublist);
  			$$.push_back($block_item);
  		}
| block_item
		{
			$$ = compiler::define::AstPtrVec();
			$$.push_back($block_item);
		}
;

// e.g. "int a = 3;" or "return a;"
block_item: // VarDeclNode|IdNode|ConstIntNode|UnaryOpNode|BinaryOpNode|FuncCallNode|IfNode|WhileNode|BreakNode|ContNode|RetNode
  var_decl
  		{ $$ = std::move($var_decl); }
| statement
		{ $$ = std::move($statement); }
;

// a single statement or a block
statement: // IdNode|ConstIntNode|UnaryOpNode|BinaryOpNode|FuncCallNode|IfNode|WhileNode|BreakNode|ContNode|RetNode|BlockNode
  assign_statement
		{ $$ = std::move($assign_statement); }
| if_statement
		{ $$ = std::move($if_statement); }
| while_statement
		{ $$ = std::move($while_statement); }
| return_statement
		{ $$ = std::move($return_statement); }
| break_statement
		{ $$ = std::move($break_statement); }
| cont_statement
		{ $$ = std::move($cont_statement); }
| expr SEMI
		{ $$ = std::move($expr); }
| block
		{ $$ = std::move($block); }

assign_statement: // BinaryOpNode
  ID ASSIGN expr SEMI
  		{
  			auto id_node = std::make_unique<compiler::define::IdNode>($ID);
  			$$ = std::make_unique<compiler::define::BinaryOpNode>(
  				compiler::define::BinaryOpNode::ASSIGN,
  				std::move(id_node), std::move($expr)
  			);
  		}
;

if_statement: // IfNode
  /*
   * Set the precedence of statement the same as IF, or lower than ELSE.
   * This lets an "else" be paired with the previous "if".
   */
  IF LBRKT expr RBRKT statement %prec IF
  		{
  			$$ = std::make_unique<compiler::define::IfNode>(
  				std::move($expr), std::move($statement)
  			);
  		}
| IF LBRKT expr RBRKT statement[statement1] ELSE statement[statement2]
		{
			$$ = std::make_unique<compiler::define::IfNode>(
				std::move($expr), std::move($statement1), std::move($statement2)
			);
		}
;

while_statement: // WhileNode
  WHILE LBRKT expr RBRKT statement
  		{
  			$$ = std::make_unique<compiler::define::WhileNode>(
  				std::move($expr), std::move($statement)
  			);
  		}
;

return_statement: // RetNode
  RET expr SEMI
  		{ $$ = std::make_unique<compiler::define::RetNode>(std::move($expr)); }
;

break_statement: // BreakNode
  BREAK SEMI
  		{ $$ = std::make_unique<compiler::define::BreakNode>(); }
;

cont_statement: // ContNode
  CONT SEMI
  		{ $$ = std::make_unique<compiler::define::ContNode>(); }
;

base_type: // TypePtr
  INT
  		{ return compiler::define::make_int(/* is_const */false); }
| CONST INT
		{ return compiler::define::make_int(/* is_const */true); }
| VOID
		{ return compiler::define::make_void(); }
;

expr: // IdNode|ConstIntNode|UnaryOpNode|BinaryOpNode|FuncCallNode
  expr[opr1] ADD expr[opr2]
		{
			$$ = std::make_unique<compiler::define::BinaryOpNode>(
				compiler::define::BinaryOpNode::ADD,$opr1, $opr2
			);
		}
| expr[opr1] SUB expr[opr2]
		{
			$$ = std::make_unique<compiler::define::BinaryOpNode>(
				compiler::define::BinaryOpNode::SUB, $opr1, $opr2
			);
		}
| expr[opr1] MUL expr[opr2]
		{
			$$ = std::make_unique<compiler::define::BinaryOpNode>(
				compiler::define::BinaryOpNode::MUL, $opr1, $opr2
			);
		}
| expr[opr1] DIV expr[opr2]
		{
			$$ = std::make_unique<compiler::define::BinaryOpNode>(
				compiler::define::BinaryOpNode::DIV, $opr1, $opr2
			);
		}
| expr[opr1] MOD expr[opr2]
		{
			$$ = std::make_unique<compiler::define::BinaryOpNode>(
				compiler::define::BinaryOpNode::MOD, $opr1, $opr2
			);
		}
| expr[opr1] OR expr[opr2]
		{
			$$ = std::make_unique<compiler::define::BinaryOpNode>(
				compiler::define::BinaryOpNode::OR, $opr1, $opr2
			);
		}
| expr[opr1] AND expr[opr2]
		{
			$$ = std::make_unique<compiler::define::BinaryOpNode>(
				compiler::define::BinaryOpNode::AND, $opr1, $opr2
			);
		}
| expr[opr1] GT expr[opr2]
		{
			$$ = std::make_unique<compiler::define::BinaryOpNode>(
				compiler::define::BinaryOpNode::GT, $opr1, $opr2
			);
		}
| expr[opr1] LT expr[opr2]
		{
			$$ = std::make_unique<compiler::define::BinaryOpNode>(
				compiler::define::BinaryOpNode::LT, $opr1, $opr2
			);
		}
| expr[opr1] GE expr[opr2]
		{
			$$ = std::make_unique<compiler::define::BinaryOpNode>(
				compiler::define::BinaryOpNode::GE, $opr1, $opr2
			);
		}
| expr[opr1] LE expr[opr2]
		{
			$$ = std::make_unique<compiler::define::BinaryOpNode>(
				compiler::define::BinaryOpNode::LE, $opr1, $opr2
			);
		}
| expr[opr1] EQ expr[opr2]
		{
			$$ = std::make_unique<compiler::define::BinaryOpNode>(
				compiler::define::BinaryOpNode::EQ, $opr1, $opr2
			);
		}
| expr[opr1] NE expr[opr2]
		{
			$$ = std::make_unique<compiler::define::BinaryOpNode>(
				compiler::define::BinaryOpNode::NE, $opr1, $opr2
			);
		}
| ADD expr[opr] %prec NEG
		{ $$ = std::move($opr); }
| SUB expr[opr] %prec NEG
		{
			$$ = std::make_unique<compiler::define::UnaryOpNode>(
				compiler::define::UnaryOpNode::NEG, $opr
			);
		}
| NOT expr[opr]
		{
			$$ = std::make_unique<compiler::define::UnaryOpNode>(
				compiler::define::UnaryOpNode::NOT, $opr
			);
		}
| LBRKT expr[opr] RBRKT
		{ $$ = std::move($opr); }
| UINT
		{ $$ = std::make_unique<compiler::define::ConstIntNode>($UINT); }
| func_call
		{ $$ = std::move($func_call); }
| ID
		{ $$ = std::make_unique<compiler::define::IdNode>($ID); }
| array_access
		{ $$ = std::move($array_access); }
;

// e.g. "a[2][3 * N]"
array_access: // BinaryOpNode(ACCESS only)
  ID LSBRKT expr RSBRKT
		{
			auto id_node = std::make_unique<compiler::define::IdNode>($ID);
			$$ = std::make_unique<compiler::define::BinaryOpNode>(
				compiler::define::BinaryOpNode::ACCESS,
				std::move(id_node), std::move($expr)
			);
		}
| array_access[subarr] LSBRKT expr RSBRKT
		{
			$$ = std::make_unique<ConstArrayAccessNode>(
				std::move($subarr), std::move($expr)
			);
		}
;

func_call: // FuncCallNode
  ID LBRKT RBRKT
  		{
  			auto id_node = std::make_unique<compiler::define::IdNode>($ID);
  			$$ = std::make_unique<compiler::define::FuncCallNode>(std::move(id_node));
  		}
| ID LBRKT func_args RBRKT
		{
			auto id_node = std::make_unique<compiler::define::IdNode>($ID);
			$$ = std::make_unique<compiler::define::FuncCallNode>(
				std::move(id_node), std::move($func_args)
			);
		}
;

func_args: // FuncArgsNode
  func_args[sublist] COMMA expr
  		{
			$$ = std::move($sublist);
			$$->push_back(std::move($expr));
		}
| expr
		{
			$$ = std::make_unique<compiler::define::FuncArgsNode>();
			$$->push_back(std::move($expr));
		}
;

%%

int main()
{
	auto parser = yy::parser();
	parser.parse();
}