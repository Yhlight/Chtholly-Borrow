#include "Lexer.h"
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <cctype>

namespace chtholly
{

	namespace {
		// 1. 定义一个简单的 constexpr 字符转换函数
		constexpr char charToUpper(char c) {
			if (c >= 'a' && c <= 'z') {
				return c - ('a' - 'A');
			}
			return c;
		}

		enum CharType : uint8_t {
			Digit = 1 << 0,
			Alpha = 1 << 1,
			Hex = 1 << 2,
			Space = 1 << 3
		};

		struct LexerLookupTable {
			uint8_t table[256] = { 0 };

			constexpr LexerLookupTable() {
				// 设置数字
				for (int i = '0'; i <= '9'; ++i) {
					table[static_cast<unsigned char>(i)] |= (Digit | Hex);
				}
				// 设置字母和十六进制
				for (int i = 'a'; i <= 'f'; ++i) {
					table[static_cast<unsigned char>(i)] |= (Alpha | Hex);
					// 使用我们自己定义的 charToUpper
					table[static_cast<unsigned char>(charToUpper(static_cast<char>(i)))] |= (Alpha | Hex);
				}
				// 设置剩余字母
				for (int i = 'g'; i <= 'z'; ++i) {
					table[static_cast<unsigned char>(i)] |= Alpha;
					table[static_cast<unsigned char>(charToUpper(static_cast<char>(i)))] |= Alpha;
				}

				table[static_cast<unsigned char>('_')] |= Alpha;
				table[static_cast<unsigned char>(' ')] |= Space;
				table[static_cast<unsigned char>('\t')] |= Space;
				table[static_cast<unsigned char>('\r')] |= Space;
				table[static_cast<unsigned char>('\n')] |= Space;
			}
		};

		// 此时 LOOKUP 的编译期初始化就能顺利通过了
		static constexpr LexerLookupTable LOOKUP;
	}

	Lexer::Lexer(std::string_view source) : m_source(source) {}
	char Lexer::peek(int offset) const
	{
		if (offset < 0 && static_cast<size_t>(-offset) > m_pos)
		{
			return '\0';
		}

		if (m_pos + offset >= m_source.length())
		{
			return '\0';
		}
		return m_source[m_pos + offset];
	}
	char Lexer::advance()
	{
		if (isAtEnd())
			return '\0';

		char c = m_source[m_pos++];
		if (c == '\n')
		{
			m_line++;
			m_column = 1;
		}
		else
		{
			m_column++;
		}
		return c;
	}
	bool Lexer::match(char expected)
	{
		if (isAtEnd() || peek() != expected)
		{
			return false;
		}
		advance();
		return true;
	}

	void Lexer::skipWhitespace()
	{
		while (!isAtEnd())
		{
			char c = peek();
			switch (c)
			{
			case ' ':
			case '\r':
			case '\t':
			case '\n':
				advance();
				break;
			case '/':
				if (peek(1) == '/')
				{
					while (peek() != '\n' && !isAtEnd())
						advance();
				}
				else if (peek(1) == '*')
				{
					advance();
					advance();
					int nesting = 1;

					while (nesting > 0 && !isAtEnd())
					{
						if (peek() == '/' && peek(1) == '*')
						{
							advance();
							advance();
							nesting++;
						}
						else if (peek() == '*' && peek(1) == '/')
						{
							advance();
							advance();
							nesting--;
						}
						else
						{
							advance();
						}
					}
				}
				else
				{
					return;
				}
				break;
			default:
				return;
			}
		}
	}

	bool Lexer::isAtEnd() const
	{
		return m_pos >= m_source.length();
	}

	Token Lexer::makeToken(TokenType type, int line, int col)
	{
		return { type, currentText(), line, col };
	}

	bool Lexer::isDigit(char c) const
	{
		return LOOKUP.table[static_cast<unsigned char>(c)] & Digit;
	}

	bool Lexer::isAlpha(char c) const
	{
		return LOOKUP.table[static_cast<unsigned char>(c)] & Alpha;
	}

	bool Lexer::isHexDigit(char c) const
	{
		return LOOKUP.table[static_cast<unsigned char>(c)] & Hex;
	}

	bool Lexer::isAlphaNumeric(char c) const
	{
		// 标识符允许 字母、下划线 或 数字
		return LOOKUP.table[static_cast<unsigned char>(c)] & (Alpha | Digit);
	}

	std::string_view Lexer::currentText() const
	{
		// 确保不会发生越界访问
		if (start_pos >= m_source.length())
			return "";

		// 计算当前正在处理的片段长度
		size_t length = (m_pos > start_pos) ? (m_pos - start_pos) : 0;
		return m_source.substr(start_pos, length);
	}

	Token Lexer::scanIdentifierOrKeyword(int line, int col)
	{
		// 此时 start_pos 已经在 nextToken() 中通过 m_pos - 1 设置好了

		// 1. 贪婪扫描标识符字符 (字母、数字、下划线)
		// 利用我们之前讨论的查表法辅助函数
		while (isAlphaNumeric(peek()))
		{
			advance();
		}

		// 2. 获取当前的文本切片 (零拷贝)
		std::string_view text = m_source.substr(start_pos, m_pos - start_pos);

		// 3. 静态关键字映射表 (使用 string_view 提高查找效率)
		static const std::unordered_map<std::string_view, TokenType> keywords = {
			{"fn", TokenType::Fn}, {"let", TokenType::Let}, {"mut", TokenType::Mut}, {"class", TokenType::Class}, {"struct", TokenType::Struct}, {"enum", TokenType::Enum}, {"if", TokenType::If}, {"else", TokenType::Else}, {"switch", TokenType::Switch}, {"case", TokenType::Case}, {"while", TokenType::While}, {"for", TokenType::For}, {"do", TokenType::Do}, {"return", TokenType::Return}, {"import", TokenType::Import}, {"package", TokenType::Package}, {"use", TokenType::Use}, {"pub", TokenType::Pub}, {"as", TokenType::As}, {"break", TokenType::Break}, {"continue", TokenType::Continue}, {"fallthrough", TokenType::Fallthrough}, {"default", TokenType::Default}, {"void", TokenType::Void}, {"char", TokenType::Char}, {"bool", TokenType::Bool}, {"self", TokenType::Self}, {"Self", TokenType::CapitalSelf}, {"extern", TokenType::Extern}, {"true", TokenType::True}, {"false", TokenType::False}, {"nullptr", TokenType::Nullptr}, {"unsafe", TokenType::Unsafe}, {"malloc", TokenType::Malloc}, {"alloca", TokenType::Alloca}, {"free", TokenType::Free}, {"sizeof", TokenType::Sizeof}, {"alignof", TokenType::Alignof}, {"offsetof", TokenType::Offsetof}, {"align", TokenType::Align}, {"packed", TokenType::Packed}, {"_", TokenType::Underscore},
			// 基础类型关键字
			{ "i8", TokenType::I8 },
			{"i16", TokenType::I16},
			{"i32", TokenType::I32},
			{"i64", TokenType::I64},
			{"u8", TokenType::U8},
			{"u16", TokenType::U16},
			{"u32", TokenType::U32},
			{"u64", TokenType::U64},
			{"f32", TokenType::F32},
			{"f64", TokenType::F64} };

		// 4. 匹配关键字
		auto it = keywords.find(text);
		if (it != keywords.end())
		{
			return makeToken(it->second, line, col);
		}

		// 5. 否则返回普通标识符
		return makeToken(TokenType::Identifier, line, col);
	}

	Token Lexer::scanNumber(int line, int col)
	{
		bool isFloat = false;

		// 1. 十六进制处理
		if (m_source[m_pos - 1] == '0' && (peek() == 'x' || peek() == 'X'))
		{
			advance(); // 'x'
			while (isHexDigit(peek()) || peek() == '_') {
				if (peek() == '_') advance(); // 允许 0xFF_FF
				else advance();
			}
		}
		else
		{
			// 2. 十进制整数部分
			while (isDigit(peek()) || peek() == '_') {
				if (peek() == '_') advance(); // 跳过数字间的下划线
				else advance();
			}

			// 3. 小数部分
			if (peek() == '.' && isDigit(peek(1)))
			{
				isFloat = true;
				advance(); // '.'
				while (isDigit(peek()) || peek() == '_') {
					if (peek() == '_') advance();
					else advance();
				}
			}

			// 4. 指数部分 (e.g., 1.2e+10)
			if (peek() == 'e' || peek() == 'E')
			{
				// 先记录当前位置，以便在 e 格式错误时决定是否回退
				size_t ePos = m_pos;
				int eCol = m_column;

				advance(); // 消耗 'e'
				if (peek() == '+' || peek() == '-')
					advance();

				if (!isDigit(peek()))
				{
					// 【改进点】如果 e 后面不是数字，说明这不是一个合法的指数
					// 我们可以选择回退到 e 之前，让 'e' 作为一个标识符被后续处理
					// 或者直接报错。这里我们选择回退，以保证 Lexer 不会崩溃
					m_pos = ePos;
					m_column = eCol;
				}
				else
				{
					isFloat = true;
					while (isDigit(peek()) || peek() == '_') {
						if (peek() == '_') advance();
						else advance();
					}
				}
			}
		}

		// 5. 后缀处理 (i32, u64, f32 等)
		size_t suffixStart = m_pos;
		int suffixCol = m_column;

		if (isAlpha(peek()))
		{
			// 如果是以 '_' 开头，我们记录并跳过它，以便提取纯后缀
			bool hasUnderscore = (peek() == '_');
			if (hasUnderscore) advance();

			size_t nameStart = m_pos; // 纯类型名开始的位置（如 "i32"）
			while (isAlphaNumeric(peek())) {
				advance();
			}

			std::string_view suffix = m_source.substr(nameStart, m_pos - nameStart);

			static const std::unordered_set<std::string_view> validSuffixes = {
				"i8", "i16", "i32", "i64", "u8", "u16", "u32", "u64", "f32", "f64"
			};

			if (validSuffixes.find(suffix) == validSuffixes.end()) {
				// 如果后缀无效（例如 42_abc），回退到下划线或字母开始前
				m_pos = suffixStart;
				m_column = suffixCol;
			}
			// 匹配成功，Token 的 value 会通过 currentText() 包含整个 "42_i32"
		}

		return makeToken(isFloat ? TokenType::Float : TokenType::Integer, line, col);
	}

	Token Lexer::scanString(int line, int col)
	{
		// 此时 m_pos - 1 是起始的左引号 "

		while (peek() != '"' && !isAtEnd())
		{
			// 【核心改进】如果遇到换行符，说明字符串未闭合就换行了，这是非法格式
			if (peek() == '\n')
			{
				// 这里不消耗 '\n'，让外层循环或下一次扫描去处理行号更新
				// 返回 Unknown 类型，Parser 会根据这个 Token 报错
				return makeToken(TokenType::Unknown, line, col);
			}

			if (peek() == '\\')
			{
				advance(); // 跳过 '\'
				if (!isAtEnd())
				{
					// 如果转义的是换行符（有些语言支持这种续行符），
					// 但既然你决定不支持多行，这里也无需特殊处理，直接 advance
					advance();
				}
			}
			else
			{
				advance();
			}
		}

		if (isAtEnd())
		{
			// 错误：文件末尾了字符串还没结束
			return makeToken(TokenType::Unknown, line, col);
		}

		// 正常消耗掉右引号 "
		advance();

		return makeToken(TokenType::String, line, col);
	}

	Token Lexer::scanCharOrLifetime(int line, int col)
	{
		// 调用此函数时，m_pos 已经在 nextToken 中消耗了第一个 '\''
		// 当前 start_pos 指向 '\''

		// 场景 A: 字符字面量 (例如 'a' 或 '\n')
		if (peek() == '\\' || (peek() != '\'' && peek(1) == '\''))
		{
			if (peek() == '\\')
			{
				advance(); // 消耗 '\'
			}
			advance(); // 消耗字符本身 (如 'a' 或 'n')

			if (peek() == '\'')
			{
				advance(); // 消耗闭合的 '\''
				return makeToken(TokenType::Char, line, col);
			}
			// 如果没有闭合引号，则回退到场景 B 尝试生命周期，或者直接报错
		}

		// 场景 B: 生命周期 (例如 'a, 'static)
		// 逻辑：如果 '\'' 后面跟着的是标识符允许的字符
		if (isAlpha(peek()))
		{
			while (isAlphaNumeric(peek()))
			{
				advance();
			}
			return makeToken(TokenType::Lifetime, line, col);
		}

		// 场景 C: 单独的 Tick 符号或错误
		// 如果只需要识别单独的 ' (TICK)，可以返回对应的 Token
		return makeToken(TokenType::Tick, line, col);
	}

	Token Lexer::nextToken()
	{
		// 1. 跳过所有空白和注释，直到遇到有效字符或文件末尾
		skipWhitespace();

		// 2. 捕捉当前 Token 的起始信息（用于 makeToken 和错误定位）
		start_pos = m_pos;
		int line = m_line;
		int col = m_column;

		// 3. 检查是否结束
		if (isAtEnd())
		{
			return makeToken(TokenType::EndOfFile, line, col);
		}

		// 4. 消耗第一个字符进行分发
		char c = advance();

		// 标识符与关键字
		if (isAlpha(c))
			return scanIdentifierOrKeyword(line, col);

		// 数字字面量
		if (isDigit(c))
			return scanNumber(line, col);

		// 字符串字面量
		if (c == '"')
			return scanString(line, col);

		// 字符字面量或生命周期
		if (c == '\'')
			return scanCharOrLifetime(line, col);

		// 5. 符号与操作符处理
		switch (c)
		{
			// 单字符符号
		case '(':
			return makeToken(TokenType::LParen, line, col);
		case ')':
			return makeToken(TokenType::RParen, line, col);
		case '{':
			return makeToken(TokenType::LBrace, line, col);
		case '}':
			return makeToken(TokenType::RBrace, line, col);
		case '[':
			return makeToken(TokenType::LBracket, line, col);
		case ']':
			return makeToken(TokenType::RBracket, line, col);
		case ',':
			return makeToken(TokenType::Comma, line, col);
		case ';':
			return makeToken(TokenType::Semicolon, line, col);
		case '?':
			return makeToken(TokenType::Question, line, col);
		case '~':
			return makeToken(TokenType::Tilde, line, col);

			// 复杂符号逻辑
		case '.':
			if (peek() == '.' && peek(1) == '.')
			{
				advance();
				advance();
				return makeToken(TokenType::Ellipsis, line, col);
			}
			return makeToken(TokenType::Dot, line, col);

		case ':':
			if (match(':'))
				return makeToken(TokenType::ColonColon, line, col);
			return makeToken(TokenType::Colon, line, col);

		case '+':
			if (match('='))
				return makeToken(TokenType::PlusEqual, line, col);
			if (match('+'))
				return makeToken(TokenType::PlusPlus, line, col);
			return makeToken(TokenType::Plus, line, col);

		case '-':
			if (match('='))
				return makeToken(TokenType::MinusEqual, line, col);
			if (match('-'))
				return makeToken(TokenType::MinusMinus, line, col);
			return makeToken(TokenType::Minus, line, col);

		case '*':
			return match('=') ? makeToken(TokenType::StarEqual, line, col) : makeToken(TokenType::Star, line, col);
		case '/':
			return match('=') ? makeToken(TokenType::SlashEqual, line, col) : makeToken(TokenType::Slash, line, col);
		case '%':
			return match('=') ? makeToken(TokenType::PercentEqual, line, col) : makeToken(TokenType::Percent, line, col);

		case '=':
			if (match('='))
				return makeToken(TokenType::EqualEqual, line, col);
			if (match('>'))
				return makeToken(TokenType::FatArrow, line, col);
			return makeToken(TokenType::Equal, line, col);

		case '!':
			return match('=') ? makeToken(TokenType::NotEqual, line, col) : makeToken(TokenType::Not, line, col);

		case '>':
			if (match('>'))
			{
				if (match('='))
					return makeToken(TokenType::ShiftRightEqual, line, col);
				return makeToken(TokenType::ShiftRight, line, col);
			}
			if (match('='))
				return makeToken(TokenType::GreaterEqual, line, col);
			return makeToken(TokenType::Greater, line, col);

		case '<':
			if (match('<'))
			{
				if (match('='))
					return makeToken(TokenType::ShiftLeftEqual, line, col);
				return makeToken(TokenType::ShiftLeft, line, col);
			}
			if (match('='))
				return makeToken(TokenType::LessEqual, line, col);
			return makeToken(TokenType::Less, line, col);

		case '&':
			if (match('='))
				return makeToken(TokenType::AmpersandEqual, line, col);
			if (match('&'))
				return makeToken(TokenType::AndAnd, line, col);
			return makeToken(TokenType::Ampersand, line, col);

		case '|':
			if (match('='))
				return makeToken(TokenType::PipeEqual, line, col);
			if (match('|'))
				return makeToken(TokenType::OrOr, line, col);
			return makeToken(TokenType::Pipe, line, col);

		case '^':
			if (match('='))
				return makeToken(TokenType::CaretEqual, line, col);
			return makeToken(TokenType::Caret, line, col);

		default:
			// 遇到无法识别的字符
			return makeToken(TokenType::Unknown, line, col);
		}
	}

	Token Lexer::peekToken()
	{
		// 1. 保存当前 Lexer 的所有状态
		size_t old_pos = m_pos;
		size_t old_start_pos = start_pos; // 必须保存这个，否则切片会乱
		int old_line = m_line;
		int old_column = m_column;

		// 2. 预读下一个 Token
		Token token = nextToken();

		// 3. 彻底恢复状态
		m_pos = old_pos;
		start_pos = old_start_pos;
		m_line = old_line;
		m_column = old_column;

		return token;
	}

} // namespace chtholly