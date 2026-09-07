#pragma once

#include <cstdint>
#include "Core.h"

namespace TGX::Shell
{
enum class TokenType : std::uint8_t
{
	Number,
	Identifier,
	Equals,
	Semicolon,
	Colon,
	Comma,
	Dot,
	OpenParen,
	CloseParen,
	OpenBrace,
	CloseBrace,
	OpenBracket,
	CloseBracket,
	BinaryOperator,
	LogicalOperator,
	Bang,
	Length,
	Let,
	Const,
	Break,
	Continue,
	Function,
	Return,
	Print,
	Toggle,
	Split,
	Read,
	Write,
	Exec,
	Spawn,
	If,
	Else,
	While,
	For,
	String_,
	EndOfFile
};

struct Token
{
	String value;
	TokenType type = TokenType::EndOfFile;
};

Vector<Token> Tokenise(const String &sourceCode, Vector<String> &errors);
} // namespace TGX::Shell
