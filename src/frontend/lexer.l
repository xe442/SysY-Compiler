%{

	#include <iostream>
	#include "parser.tab.h"
	
	/* #define DEBUG */
	#include "dbg.h"

	using yytoken = yy::parser::token;

	/*
	 * A dumb but effective way to track the locations of each token: simply
	 * set two global variables to keep the line and column number.
	 */
	extern int yylineno; /* = 1, which is pre-defined by flex. */
	int yycolumn = 1; /* Our column number counter. */

	/* This macro sets the location, called by flex when a token is matched. */
	#define YY_USER_ACTION \
		do\
		{\
			yylloc->begin = yy::position(nullptr, yylineno, yycolumn);\
			for(int i = 0; yytext[i] != '\0'; i++)\
				if(yytext[i] == '\n')\
				{\
					yylineno++;\
					yycolumn = 1;\
				}\
				else\
					yycolumn++;\
			yy::position tok_end_pos(nullptr, yylineno, yycolumn);\
			yylloc->end = yy::position(nullptr, yylineno, yycolumn);\
		}while(0);
	
	int _text_to_int(const char *text);
%}

%option bison-bridge
%option bison-locations

white-space				[ \t\n]

identifier-nondigit		[A-Za-z_]
identifier-digit		[0-9]
identifier				{identifier-nondigit}({identifier-nondigit}|{identifier-digit})*

digit					[0-9]
nonzero-digit			[1-9]
octal-digit				[0-7]
octal-prefix			0
hexadecimal-digit		[0-9A-Fa-f]
hexadecimal-prefix		0[Xx]
decimal-uinteger		0|{nonzero-digit}{digit}*
octal-uinteger			{octal-prefix}{octal-digit}+
hexadecimal-uinteger	{hexadecimal-prefix}{hexadecimal-digit}+
uinteger				{decimal-uinteger}|{octal-uinteger}|{hexadecimal-uinteger}

line-comment			\/\/.*
bcomment-start			\/\*
bcomment-end			\*\/
any-char				(.|\n)

 /* declare BCOMMENT state, for recognizing block comment */
%x BCOMMENT

%%

{white-space}	; /* white space skipped */

 /* keywords */
"const"			{ DBG(std::cout << "CONST" << std::endl); return yytoken::CONST; }
"int"			{ DBG(std::cout << "INT" << std::endl); return yytoken::INT; }
"void"			{ DBG(std::cout << "VOID" << std::endl); return yytoken::VOID; }
"if"			{ DBG(std::cout << "IF" << std::endl); return yytoken::IF; }
"else"			{ DBG(std::cout << "ELSE" << std::endl); return yytoken::ELSE; }
"while"			{ DBG(std::cout << "WHILE" << std::endl); return yytoken::WHILE; }
"break"			{ DBG(std::cout << "BREAK" << std::endl); return yytoken::BREAK; }
"continue"		{ DBG(std::cout << "CONT" << std::endl); return yytoken::CONT; }
"return"		{ DBG(std::cout << "RET" << std::endl); return yytoken::RET; }

 /* delimiters */
"{"				{ DBG(std::cout << "LCBRKT" << std::endl); return yytoken::LCBRKT; }
"}"				{ DBG(std::cout << "RCBRKT" << std::endl); return yytoken::RCBRKT; }
";"				{ DBG(std::cout << "SEMI" << std::endl); return yytoken::SEMI; }
","				{ DBG(std::cout << "COMMA" << std::endl); return yytoken::COMMA; }

 /* operators */
"+"				{ DBG(std::cout << "ADD" << std::endl); return yytoken::ADD; }
"-"				{ DBG(std::cout << "SUB" << std::endl); return yytoken::SUB; }
"*"				{ DBG(std::cout << "MUL" << std::endl); return yytoken::MUL; }
"/"				{ DBG(std::cout << "DIV" << std::endl); return yytoken::DIV; }
"%"				{ DBG(std::cout << "MOD" << std::endl); return yytoken::MOD; }
"!"				{ DBG(std::cout << "NOT" << std::endl); return yytoken::NOT; }
">"				{ DBG(std::cout << "GT" << std::endl); return yytoken::GT; }
"<"				{ DBG(std::cout << "LT" << std::endl); return yytoken::LT; }
"="				{ DBG(std::cout << "ASSIGN" << std::endl); return yytoken::ASSIGN; }
"=="			{ DBG(std::cout << "EQ" << std::endl); return yytoken::EQ; }
">="			{ DBG(std::cout << "GE" << std::endl); return yytoken::GE; }
"<="			{ DBG(std::cout << "LE" << std::endl); return yytoken::LE; }
"!="			{ DBG(std::cout << "NE" << std::endl); return yytoken::NE; }
"&&"			{ DBG(std::cout << "AND" << std::endl); return yytoken::AND; }
"||"			{ DBG(std::cout << "OR" << std::endl); return yytoken::OR; }
"("				{ DBG(std::cout << "LBRKT" << std::endl); return yytoken::LBRKT; }
")"				{ DBG(std::cout << "RBRKT" << std::endl); return yytoken::RBRKT; }
"["				{ DBG(std::cout << "LSBRKT" << std::endl); return yytoken::LSBRKT; }
"]"				{ DBG(std::cout << "RSBRKT" << std::endl); return yytoken::RSBRKT; }

 /* comments */
{line-comment}	{ DBG(std::cout << "in line comment!" << std::endl); }
{bcomment-start} {
					DBG(std::cout << "block comment start!" << std::endl);
					BEGIN BCOMMENT; /* set the lexer state to BCOMMENT */
				}
<BCOMMENT> /* defines the behavior in BCOMMENT state */
{
	{bcomment-end}	{
						DBG(std::cout << "block comment ended!" << std::endl);
						BEGIN INITIAL; /* reset the lexer state */
					}
	{any-char}		; /* skip the other characters in the comment */
}


{identifier} 	{
					DBG(std::cout << "ID " << yytext << std::endl);
					yylval->emplace<std::string>() = std::string(yytext);
					return yytoken::ID;
				}
{uinteger}		{
					DBG(std::cout << "UINT " << yytext << std::endl);
					yylval->emplace<int>() = _text_to_int(yytext);
					return yytoken::UINT;
				}

%%

 /* The general text -> int converision function. Never checks input correctness. */
int _text_to_int(const char *text)
{
	int base = 10;
	if(*text == '0') /* For inpue text = "0", the base would be incorrectly set
					    to 8, but that does not matter. */
	{
		text++;
		if(*text == 'X' || *text == 'x')
		{
			text++;
			base = 16;
		}
		else
			base = 8;
	}

	int res = 0;
	for(; *text != '\0'; text++)
	{
		res *= base;

		if('0' <= *text && *text <='9')
			res += *text - '0';
		else
			res += *text - 'A' + 10;
	}
	return res;
}