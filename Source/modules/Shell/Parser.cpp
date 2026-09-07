#include "Parser.h"
#include <cstdlib>

namespace TGX::Shell
{
namespace
{
const char *TokenName(TokenType type)
{
	switch (type)
	{
		case TokenType::Number: return "Number";
		case TokenType::Identifier: return "Identifier";
		case TokenType::Equals: return "Equals";
		case TokenType::Semicolon: return "Semicolon";
		case TokenType::Colon: return "Colon";
		case TokenType::Comma: return "Comma";
		case TokenType::Dot: return "Dot";
		case TokenType::OpenParen: return "OpenParen";
		case TokenType::CloseParen: return "CloseParen";
		case TokenType::OpenBrace: return "OpenBrace";
		case TokenType::CloseBrace: return "CloseBrace";
		case TokenType::OpenBracket: return "OpenBracket";
		case TokenType::CloseBracket: return "CloseBracket";
		case TokenType::BinaryOperator: return "BinaryOperator";
		case TokenType::LogicalOperator: return "LogicalOperator";
		case TokenType::Bang: return "Bang";
		case TokenType::Length: return "Length";
		case TokenType::Let: return "Let";
		case TokenType::Const: return "Const";
		case TokenType::Break: return "Break";
		case TokenType::Continue: return "Continue";
		case TokenType::Function: return "Function";
		case TokenType::Return: return "Return";
		case TokenType::Print: return "Print";
		case TokenType::Toggle: return "Toggle";
		case TokenType::Split: return "Split";
		case TokenType::Read: return "Read";
		case TokenType::Write: return "Write";
		case TokenType::Exec: return "Exec";
		case TokenType::Spawn: return "Spawn";
		case TokenType::If: return "If";
		case TokenType::Else: return "Else";
		case TokenType::While: return "While";
		case TokenType::For: return "For";
		case TokenType::String_: return "String";
		case TokenType::EndOfFile: return "EndOfFile";
	}

	return "Unknown";
}
} // namespace

const Token &Parser::At() const
{
	return tokens[cursor];
}

Token Parser::Advance()
{
	Token token = tokens[cursor];

	if (cursor + 1 < tokens.size())
	{
		++cursor;
	}

	return token;
}

Token Parser::Expect(TokenType type, const String &message)
{
	Token token = Advance();

	if (token.type != type)
	{
		errors.push_back(
			"Parser Error: " + message + " Got: " + TokenName(token.type) + " Expected: " + TokenName(type));
	}

	return token;
}

bool Parser::NotEndOfFile() const
{
	return tokens[cursor].type != TokenType::EndOfFile;
}

const Vector<String> &Parser::GetErrors() const
{
	return errors;
}

NodeRef Parser::Produce(const String &sourceCode)
{
	errors.clear();
	cursor = 0;
	tokens = Tokenise(sourceCode, errors);

	NodeRef program = MakeNode(NodeKind::Program);

	while (NotEndOfFile() && errors.empty())
	{
		program->body.push_back(ParseStatement());
	}

	return program;
}

NodeRef Parser::ParseStatement()
{
	switch (At().type)
	{
		case TokenType::Print:
			return ParsePrintStatement();
		case TokenType::Toggle:
			return ParseToggleStatement();
		case TokenType::Split:
			return ParseSplitStatement();
		case TokenType::Write:
			return ParseWriteStatement();
		case TokenType::Return:
			return ParseReturnStatement();
		case TokenType::Exec:
			return ParseExecStatement();
		case TokenType::Spawn:
			return ParseSpawnStatement();
		case TokenType::If:
			return ParseIfStatement();
		case TokenType::While:
			return ParseWhileStatement();
		case TokenType::For:
			return ParseForStatement();
		case TokenType::Let:
		case TokenType::Const:
			return ParseVariableExpression();
		case TokenType::Function:
			return ParseFunctionDeclaration();
		case TokenType::Break:
			return ParseBreakStatement();
		case TokenType::Continue:
			return ParseContinueStatement();
		default:
			return ParseExpression();
	}
}

NodeRef Parser::ParseExpression()
{
	return ParseAssignmentExpression();
}

NodeRef Parser::ParseAssignmentExpression()
{
	NodeRef left = ParseArrayExpression();

	if (At().type == TokenType::Equals)
	{
		Advance();

		NodeRef assignment = MakeNode(NodeKind::AssignmentExpression);
		assignment->assigne = left;
		assignment->value = ParseAssignmentExpression();

		return assignment;
	}

	return left;
}

NodeRef Parser::ParseArrayExpression()
{
	if (At().type != TokenType::OpenBracket)
	{
		return ParseObjectExpression();
	}

	Advance();

	NodeRef array = MakeNode(NodeKind::ArrayLiteral);

	while (NotEndOfFile() && At().type != TokenType::CloseBracket)
	{
		array->elements.push_back(ParseExpression());

		if (At().type != TokenType::CloseBracket)
		{
			Expect(TokenType::Comma, "Expected comma or closing bracket in array literal");
		}
	}

	Expect(TokenType::CloseBracket, "Expected closing bracket after array elements");

	return array;
}

NodeRef Parser::ParseObjectExpression()
{
	if (At().type != TokenType::OpenBrace)
	{
		return ParseLogicalExpression();
	}

	Advance();

	NodeRef object = MakeNode(NodeKind::ObjectLiteral);

	while (NotEndOfFile() && At().type != TokenType::CloseBrace)
	{
		const String key = Expect(TokenType::Identifier, "Object literal key expected").value;

		if (At().type == TokenType::Comma)
		{
			Advance();
			object->properties.push_back(Property{key, nullptr});
			continue;
		}

		if (At().type == TokenType::CloseBrace)
		{
			object->properties.push_back(Property{key, nullptr});
			continue;
		}

		Expect(TokenType::Colon, "Missing colon following identifier in Object Expression");

		object->properties.push_back(Property{key, ParseExpression()});

		if (At().type != TokenType::CloseBrace)
		{
			Expect(TokenType::Comma, "Expected comma or closing brace following property");
		}
	}

	Expect(TokenType::CloseBrace, "Object literal missing closing brace");

	return object;
}

NodeRef Parser::ParseLogicalExpression()
{
	NodeRef left = ParseAdditiveExpression();

	while (At().type == TokenType::LogicalOperator)
	{
		NodeRef logical = MakeNode(NodeKind::LogicalExpression);
		logical->op = Advance().value;
		logical->left = left;
		logical->right = ParseAdditiveExpression();

		left = logical;
	}

	return left;
}

NodeRef Parser::ParseAdditiveExpression()
{
	NodeRef left = ParseMultiplicativeExpression();

	while (At().type == TokenType::BinaryOperator && (At().value == "+" || At().value == "-"))
	{
		NodeRef binary = MakeNode(NodeKind::BinaryExpression);
		binary->op = Advance().value;
		binary->left = left;
		binary->right = ParseMultiplicativeExpression();

		left = binary;
	}

	return left;
}

NodeRef Parser::ParseMultiplicativeExpression()
{
	NodeRef left = ParseUnaryExpression();

	while (At().type == TokenType::BinaryOperator && (At().value == "*" || At().value == "/" || At().value == "%"))
	{
		NodeRef binary = MakeNode(NodeKind::BinaryExpression);
		binary->op = Advance().value;
		binary->left = left;
		binary->right = ParseUnaryExpression();

		left = binary;
	}

	return left;
}

NodeRef Parser::ParseUnaryExpression()
{
	if (At().type == TokenType::Bang || (At().type == TokenType::BinaryOperator && At().value == "-"))
	{
		NodeRef unary = MakeNode(NodeKind::UnaryExpression);
		unary->op = Advance().value;
		unary->right = ParseUnaryExpression();

		return unary;
	}

	return ParseCallMemberExpression();
}

NodeRef Parser::ParseCallMemberExpression()
{
	NodeRef member = ParseMemberExpression();

	if (At().type == TokenType::OpenParen)
	{
		return ParseCallExpression(member);
	}

	return member;
}

NodeRef Parser::ParseMemberExpression()
{
	NodeRef object = ParsePrimaryExpression();

	while (At().type == TokenType::Dot || At().type == TokenType::OpenBracket)
	{
		const Token op = Advance();

		NodeRef member = MakeNode(NodeKind::MemberExpression);
		member->object = object;

		if (op.type == TokenType::Dot)
		{
			member->computed = false;
			member->property = ParsePrimaryExpression();

			if (!member->property || member->property->kind != NodeKind::Identifier)
			{
				errors.push_back("Cannot use dot operator without right hand side being a identifier.");
			}
		}
		else
		{
			member->computed = true;
			member->property = ParseExpression();

			Expect(TokenType::CloseBracket, "Missing closing bracket in computed value.");
		}

		object = member;
	}

	return object;
}

NodeRef Parser::ParseCallExpression(NodeRef caller)
{
	NodeRef call = MakeNode(NodeKind::CallExpression);
	call->caller = std::move(caller);
	call->args = ParseArgsExpression();

	if (At().type == TokenType::OpenParen)
	{
		return ParseCallExpression(call);
	}

	return call;
}

Vector<NodeRef> Parser::ParseArgsExpression()
{
	Expect(TokenType::OpenParen, "Expected open parenthesis");

	Vector<NodeRef> args;

	if (At().type != TokenType::CloseParen)
	{
		args = ParseArgumentsListExpression();
	}

	Expect(TokenType::CloseParen, "Missing closing parenthesis inside argument list");

	return args;
}

Vector<NodeRef> Parser::ParseArgumentsListExpression()
{
	Vector<NodeRef> args;
	args.push_back(ParseAssignmentExpression());

	while (At().type == TokenType::Comma)
	{
		Advance();
		args.push_back(ParseAssignmentExpression());
	}

	return args;
}

NodeRef Parser::ParseFunctionDeclaration()
{
	Advance();

	NodeRef function = MakeNode(NodeKind::FunctionDeclaration);
	function->identifier = Expect(TokenType::Identifier, "Expected function name following function keyword").value;

	const Vector<NodeRef> args = ParseArgsExpression();

	for (const NodeRef &arg : args)
	{
		if (!arg || arg->kind != NodeKind::Identifier)
		{
			errors.push_back("Inside function declaration expected parameters to be of type string.");
			continue;
		}

		function->parameters.push_back(arg->symbol);
	}

	Expect(TokenType::OpenBrace, "Expected function body following declaration");

	while (NotEndOfFile() && At().type != TokenType::CloseBrace)
	{
		function->body.push_back(ParseStatement());
	}

	Expect(TokenType::CloseBrace, "Expected closing brace at the end of the function body");

	return function;
}

NodeRef Parser::ParseVariableExpression()
{
	const bool isConstant = Advance().type == TokenType::Const;

	NodeRef declaration = MakeNode(NodeKind::VariableDeclaration);
	declaration->identifier = Expect(TokenType::Identifier, "Expected identifier name following let | const keywords.").value;
	declaration->constant = isConstant;

	if (At().type == TokenType::Semicolon)
	{
		Advance();

		if (isConstant)
		{
			errors.push_back("Must assign value to constant expression. No value provided.");
		}

		return declaration;
	}

	Expect(TokenType::Equals, "Expected equals token following identifier in var declaration.");

	declaration->value = ParseExpression();

	return declaration;
}

NodeRef Parser::ParsePrintStatement()
{
	Expect(TokenType::Print, "Expected print keyword.");
	Expect(TokenType::OpenParen, "Expected ( after print.");

	NodeRef statement = MakeNode(NodeKind::PrintStatement);
	statement->value = ParseExpression();

	Expect(TokenType::CloseParen, "Expected ) after print value.");

	return statement;
}

NodeRef Parser::ParseToggleStatement()
{
	Expect(TokenType::Toggle, "Expected toggle keyword.");
	Expect(TokenType::OpenParen, "Expected ( after toggle.");

	NodeRef statement = MakeNode(NodeKind::ToggleStatement);
	statement->name = ParseExpression();

	Expect(TokenType::Comma, "Expected , after toggle name.");

	statement->value = ParseExpression();

	Expect(TokenType::Comma, "Expected , after toggle value.");

	statement->active = ParseExpression();

	Expect(TokenType::CloseParen, "Expected ) after toggle value.");

	return statement;
}

NodeRef Parser::ParseSplitStatement()
{
	Expect(TokenType::Split, "Expected split keyword.");
	Expect(TokenType::OpenParen, "Expected ( after split.");

	NodeRef statement = MakeNode(NodeKind::SplitStatement);
	statement->value = ParseExpression();

	Expect(TokenType::Comma, "Expected , after split value.");

	statement->delimiter = ParseExpression();

	Expect(TokenType::Comma, "Expected , after delimiter value.");

	statement->property = ParseExpression();

	Expect(TokenType::CloseParen, "Expected ) after split value.");

	return statement;
}

NodeRef Parser::ParseLengthStatement()
{
	Expect(TokenType::Length, "Expected length keyword.");
	Expect(TokenType::OpenParen, "Expected ( after length.");

	NodeRef statement = MakeNode(NodeKind::LengthStatement);
	statement->value = ParseExpression();

	Expect(TokenType::CloseParen, "Expected ) after length value.");

	return statement;
}

NodeRef Parser::ParseReadStatement()
{
	Expect(TokenType::Read, "Expected read keyword.");
	Expect(TokenType::OpenParen, "Expected ( after read.");

	NodeRef statement = MakeNode(NodeKind::ReadStatement);
	statement->value = ParseExpression();

	Expect(TokenType::CloseParen, "Expected ) after read value.");

	return statement;
}

NodeRef Parser::ParseWriteStatement()
{
	Expect(TokenType::Write, "Expected write keyword.");
	Expect(TokenType::OpenParen, "Expected ( after write.");

	NodeRef statement = MakeNode(NodeKind::WriteStatement);
	statement->file = ParseExpression();

	Expect(TokenType::Comma, "Expected , after write file.");

	statement->source = ParseExpression();

	Expect(TokenType::CloseParen, "Expected ) after write value.");

	return statement;
}

NodeRef Parser::ParseReturnStatement()
{
	Expect(TokenType::Return, "Expected return keyword.");

	NodeRef statement = MakeNode(NodeKind::ReturnStatement);
	statement->value = ParseExpression();

	return statement;
}

NodeRef Parser::ParseExecStatement()
{
	Expect(TokenType::Exec, "Expected exec keyword.");
	Expect(TokenType::OpenParen, "Expected ( after exec.");

	NodeRef statement = MakeNode(NodeKind::ExecStatement);
	statement->value = ParseExpression();

	Expect(TokenType::CloseParen, "Expected ) after exec value.");

	return statement;
}

NodeRef Parser::ParseSpawnStatement()
{
	Expect(TokenType::Spawn, "Expected spawn keyword.");
	Expect(TokenType::OpenParen, "Expected ( after spawn.");

	NodeRef statement = MakeNode(NodeKind::SpawnStatement);
	statement->value = ParseExpression();

	Expect(TokenType::CloseParen, "Expected ) after spawn value.");

	return statement;
}

NodeRef Parser::ParseIfStatement()
{
	Advance();

	Expect(TokenType::OpenParen, "Expected opening parenthesis after if");

	NodeRef statement = MakeNode(NodeKind::IfStatement);
	statement->condition = ParseLogicalExpression();

	Expect(TokenType::CloseParen, "Expected closing parenthesis after condition");
	Expect(TokenType::OpenBrace, "Expected opening brace before then branch");

	while (NotEndOfFile() && At().type != TokenType::CloseBrace)
	{
		statement->thenBranch.push_back(ParseStatement());
	}

	Expect(TokenType::CloseBrace, "Expected closing brace after then branch");

	if (At().type == TokenType::Else)
	{
		Advance();

		Expect(TokenType::OpenBrace, "Expected opening brace before else branch");

		while (NotEndOfFile() && At().type != TokenType::CloseBrace)
		{
			statement->elseBranch.push_back(ParseStatement());
		}

		Expect(TokenType::CloseBrace, "Expected closing brace after else branch");
	}

	return statement;
}

NodeRef Parser::ParseWhileStatement()
{
	Advance();

	Expect(TokenType::OpenParen, "Expected opening parenthesis after while");

	NodeRef statement = MakeNode(NodeKind::WhileStatement);
	statement->condition = ParseLogicalExpression();

	Expect(TokenType::CloseParen, "Expected closing parenthesis after condition");
	Expect(TokenType::OpenBrace, "Expected opening brace before body branch");

	while (NotEndOfFile() && At().type != TokenType::CloseBrace)
	{
		statement->body.push_back(ParseStatement());
	}

	Expect(TokenType::CloseBrace, "Expected closing brace after body branch");

	return statement;
}

NodeRef Parser::ParseForStatement()
{
	Advance();

	Expect(TokenType::OpenParen, "Expected opening parenthesis after for");

	NodeRef statement = MakeNode(NodeKind::ForStatement);
	statement->initializer = ParseStatement();

	Expect(TokenType::Semicolon, "Expected semicolon after initializer");

	statement->condition = ParseLogicalExpression();

	Expect(TokenType::Semicolon, "Expected semicolon after condition");

	statement->increment = ParseExpression();

	Expect(TokenType::CloseParen, "Expected closing parenthesis after increment");
	Expect(TokenType::OpenBrace, "Expected opening brace before body");

	while (NotEndOfFile() && At().type != TokenType::CloseBrace)
	{
		statement->body.push_back(ParseStatement());
	}

	Expect(TokenType::CloseBrace, "Expected closing brace after body");

	return statement;
}

NodeRef Parser::ParseBreakStatement()
{
	Advance();
	return MakeNode(NodeKind::BreakStatement);
}

NodeRef Parser::ParseContinueStatement()
{
	Advance();
	return MakeNode(NodeKind::ContinueStatement);
}

NodeRef Parser::ParsePrimaryExpression()
{
	switch (At().type)
	{
		case TokenType::Identifier:
			{
				NodeRef node = MakeNode(NodeKind::Identifier);
				node->symbol = Advance().value;
				return node;
			}

		case TokenType::Number:
			{
				NodeRef node = MakeNode(NodeKind::NumericLiteral);
				node->number = std::strtod(Advance().value.c_str(), nullptr);
				return node;
			}

		case TokenType::String_:
			{
				NodeRef node = MakeNode(NodeKind::StringLiteral);
				node->symbol = Advance().value;
				return node;
			}

		case TokenType::OpenParen:
			{
				Advance();
				NodeRef value = ParseExpression();
				Expect(
					TokenType::CloseParen,
					"Unexpected token found inside parenthesised expression. Expected closing parenthesis.");
				return value;
			}

		case TokenType::Length:
			return ParseLengthStatement();

		case TokenType::Read:
			return ParseReadStatement();

		case TokenType::EndOfFile:
			return MakeNode(NodeKind::Empty);

		default:
			errors.push_back(String("Unexpected token found during parsing: ") + TokenName(At().type));
			Advance();
			return MakeNode(NodeKind::Empty);
	}
}
} // namespace TGX::Shell
