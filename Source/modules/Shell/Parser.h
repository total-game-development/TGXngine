#pragma once

#include "Ast.h"
#include "Lexer.h"

namespace TGX::Shell
{
class Parser
{
private:
	Vector<Token> tokens;
	std::size_t cursor = 0;
	Vector<String> errors;

	const Token &At() const;
	Token Advance();
	Token Expect(TokenType type, const String &message);
	bool NotEndOfFile() const;

	NodeRef ParseStatement();
	NodeRef ParseExpression();
	NodeRef ParseAssignmentExpression();
	NodeRef ParseArrayExpression();
	NodeRef ParseObjectExpression();
	NodeRef ParseLogicalExpression();
	NodeRef ParseAdditiveExpression();
	NodeRef ParseMultiplicativeExpression();
	NodeRef ParseUnaryExpression();
	NodeRef ParseCallMemberExpression();
	NodeRef ParseMemberExpression();
	NodeRef ParseCallExpression(NodeRef caller);
	NodeRef ParsePrimaryExpression();

	Vector<NodeRef> ParseArgsExpression();
	Vector<NodeRef> ParseArgumentsListExpression();

	NodeRef ParseFunctionDeclaration();
	NodeRef ParseVariableExpression();
	NodeRef ParsePrintStatement();
	NodeRef ParseToggleStatement();
	NodeRef ParseSplitStatement();
	NodeRef ParseLengthStatement();
	NodeRef ParseReadStatement();
	NodeRef ParseWriteStatement();
	NodeRef ParseReturnStatement();
	NodeRef ParseExecStatement();
	NodeRef ParseSpawnStatement();
	NodeRef ParseIfStatement();
	NodeRef ParseWhileStatement();
	NodeRef ParseForStatement();
	NodeRef ParseBreakStatement();
	NodeRef ParseContinueStatement();

public:
	NodeRef Produce(const String &sourceCode);
	const Vector<String> &GetErrors() const;
};
} // namespace TGX::Shell
