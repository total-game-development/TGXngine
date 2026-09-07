#include "Lexer.h"

namespace TGX::Shell
{
namespace
{
const Map<String, TokenType> &Keywords()
{
	static const Map<String, TokenType> keywords = {
		{"let", TokenType::Let},
		{"const", TokenType::Const},
		{"print", TokenType::Print},
		{"toggle", TokenType::Toggle},
		{"split", TokenType::Split},
		{"length", TokenType::Length},
		{"read", TokenType::Read},
		{"write", TokenType::Write},
		{"exec", TokenType::Exec},
		{"spawn", TokenType::Spawn},
		{"if", TokenType::If},
		{"else", TokenType::Else},
		{"while", TokenType::While},
		{"for", TokenType::For},
		{"function", TokenType::Function},
		{"return", TokenType::Return},
		{"break", TokenType::Break},
		{"continue", TokenType::Continue}};

	return keywords;
}

bool IsAlpha(char character)
{
	return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z');
}

bool IsInt(char character)
{
	return character >= '0' && character <= '9';
}

bool IsSkippable(char character)
{
	return character == ' ' || character == '\t' || character == '\n' || character == '\r' || character == '\f' || character == '\v';
}

void Push(Vector<Token> &tokens, const String &value, TokenType type)
{
	tokens.push_back(Token{value, type});
}
} // namespace

bool IsKeyword(const String &identifier)
{
	return Keywords().find(identifier) != Keywords().end();
}

Vector<Token> Tokenise(const String &sourceCode, Vector<String> &errors)
{
	Vector<Token> tokens;

	const std::size_t length = sourceCode.size();
	std::size_t cursor = 0;

	bool inString = false;
	String currentString;

	while (cursor < length)
	{
		const char character = sourceCode[cursor];
		++cursor;

		if (inString)
		{
			if (character == '"')
			{
				Push(tokens, currentString, TokenType::String_);
				currentString.clear();
				inString = false;
			}
			else
			{
				currentString += character;
			}

			continue;
		}

		const char next = cursor < length ? sourceCode[cursor] : '\0';

		switch (character)
		{
			case '(':
				Push(tokens, "(", TokenType::OpenParen);
				break;
			case ')':
				Push(tokens, ")", TokenType::CloseParen);
				break;
			case '{':
				Push(tokens, "{", TokenType::OpenBrace);
				break;
			case '}':
				Push(tokens, "}", TokenType::CloseBrace);
				break;
			case '[':
				Push(tokens, "[", TokenType::OpenBracket);
				break;
			case ']':
				Push(tokens, "]", TokenType::CloseBracket);
				break;

			case '+':
			case '*':
			case '%':
				Push(tokens, String(1, character), TokenType::BinaryOperator);
				break;

			case '-':
				Push(tokens, "-", TokenType::BinaryOperator);
				break;

			case '/':
				if (next == '/')
				{
					while (cursor < length && sourceCode[cursor] != '\n')
					{
						++cursor;
					}

					if (cursor < length && sourceCode[cursor] == '\n')
					{
						++cursor;
					}
				}
				else if (next == '*')
				{
					++cursor;

					while (cursor < length)
					{
						if (sourceCode[cursor] == '*' && (cursor + 1) < length && sourceCode[cursor + 1] == '/')
						{
							cursor += 2;
							break;
						}

						++cursor;
					}
				}
				else
				{
					Push(tokens, "/", TokenType::BinaryOperator);
				}
				break;

			case '!':
				if (next == '=')
				{
					Push(tokens, "!=", TokenType::LogicalOperator);
					++cursor;
					break;
				}

				Push(tokens, "!", TokenType::Bang);
				break;

			case '=':
				if (next == '=')
				{
					Push(tokens, "==", TokenType::LogicalOperator);
					++cursor;
					break;
				}

				Push(tokens, "=", TokenType::Equals);
				break;

			case '>':
				if (next == '=')
				{
					Push(tokens, ">=", TokenType::LogicalOperator);
					++cursor;
					break;
				}

				Push(tokens, ">", TokenType::LogicalOperator);
				break;

			case '<':
				if (next == '=')
				{
					Push(tokens, "<=", TokenType::LogicalOperator);
					++cursor;
					break;
				}

				Push(tokens, "<", TokenType::LogicalOperator);
				break;

			case ';':
				Push(tokens, ";", TokenType::Semicolon);
				break;
			case ':':
				Push(tokens, ":", TokenType::Colon);
				break;
			case ',':
				Push(tokens, ",", TokenType::Comma);
				break;
			case '.':
				Push(tokens, ".", TokenType::Dot);
				break;

			case '"':
				inString = true;
				break;

			default:
				if (IsInt(character))
				{
					String number(1, character);

					while (cursor < length && IsInt(sourceCode[cursor]))
					{
						number += sourceCode[cursor];
						++cursor;
					}

					Push(tokens, number, TokenType::Number);
				}
				else if (IsAlpha(character))
				{
					String identifier(1, character);

					while (cursor < length && IsAlpha(sourceCode[cursor]))
					{
						identifier += sourceCode[cursor];
						++cursor;
					}

					const auto reserved = Keywords().find(identifier);

					if (reserved == Keywords().end())
					{
						Push(tokens, identifier, TokenType::Identifier);
					}
					else
					{
						Push(tokens, identifier, reserved->second);
					}
				}
				else if (!IsSkippable(character))
				{
					errors.push_back(String("Unrecognized character found in source: ") + character);
				}
				break;
		}
	}

	if (inString)
	{
		errors.push_back("Undetermined string literal");
	}

	Push(tokens, "EndOfFile", TokenType::EndOfFile);

	return tokens;
}
} // namespace TGX::Shell
