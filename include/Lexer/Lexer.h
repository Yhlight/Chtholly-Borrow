#ifndef CHTHOLLY_LEXER_H
#define CHTHOLLY_LEXER_H

#include <string>
#include <vector>
#include <string_view>
#include <cstdint>

#include "Token.h"

namespace chtholly
{

	class Lexer
	{
	public:
		Lexer(std::string_view source);
		Token nextToken();               // 获取下一个Token，并消耗
		Token peekToken();               // 获取下一个Token，但不消耗
		char peek(int offset = 0) const; // 获取offset位置的字符，但不消耗

	private:
		char advance();            // 消耗并获取下一个字符
		bool match(char expected); // 是否是预期的字符
		void skipWhitespace();     // 跳过空白与换行
		bool isAtEnd() const;
		Token makeToken(TokenType type, int line, int col); // 工厂函数，创建一个Token

		Token scanIdentifierOrKeyword(int line, int col); // 是否是标识符或关键字
		Token scanNumber(int line, int col);              // 是否是数字
		Token scanString(int line, int col);              // 是否是字符串
		Token scanCharOrLifetime(int line, int col);      // 是否是字符或生命周期

		std::string_view currentText() const; // 获取当前Token对应的文本切片

		// Helpers
		bool isDigit(char c) const;
		bool isAlpha(char c) const;
		bool isAlphaNumeric(char c) const;
		bool isHexDigit(char c) const;

		std::string_view m_source;
		size_t m_pos = 0;
		int m_line = 1;
		int m_column = 1;
		size_t start_pos = 0;
	};

} // namespace chtholly

#endif // CHTHOLLY_LEXER_H