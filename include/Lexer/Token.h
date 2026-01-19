#ifndef CHTHOLLY_TOKEN_H
#define CHTHOLLY_TOKEN_H

#include <string_view>

namespace chtholly
{

	enum class TokenType
	{
		EndOfFile, // EOF
		Unknown,

		Identifier, // 标识符
		Lifetime,   // 生命周期
		Integer,    // 整数
		Float,      // 浮点数
		String,     // 字符串
		Char,       // 字符

		Fn,
		Let,
		Mut,
		Class,
		Struct,
		Enum,
		If,
		Else,
		Switch,
		Case,
		While,
		For,
		Do,
		Return,
		Import,
		Package,
		Use,
		Pub,
		As,
		Break,
		Continue,
		Fallthrough,
		Default,
		Void,
		Bool,
		Self,
		CapitalSelf,
		True,
		False,
		Nullptr,
		Extern,
		Unsafe,
		Malloc,
		Alloca,
		Free,
		Sizeof,
		Alignof,
		Offsetof,
		Align,
		Packed,

		I8,
		I16,
		I32,
		I64,
		U8,
		U16,
		U32,
		U64,
		F32,
		F64,

		Plus,         // +
		Minus,        // -
		Star,         // *
		Slash,        // /
		Percent,      // %
		Question,     // ?
		Equal,        // =
		EqualEqual,   // ==
		NotEqual,     // !=
		Greater,      // >
		GreaterEqual, // >=
		Less,         // <
		LessEqual,    // <=
		Not,          // !
		Ampersand,    // &
		Pipe,         // |
		Caret,        // ^
		Tilde,        // ~
		Tick,         // '

		PlusEqual,       // +=
		MinusEqual,      // -=
		StarEqual,       // *=
		SlashEqual,      // /=
		PercentEqual,    // %=
		AmpersandEqual,  // &=
		PipeEqual,       // |=
		CaretEqual,      // ^=
		ShiftLeftEqual,  // <<=
		ShiftRightEqual, // >>=

		AndAnd,     // &&
		OrOr,       // ||
		PlusPlus,   // ++
		MinusMinus, // --
		ShiftLeft,  // <<
		ShiftRight, // >>
		ColonColon, // ::
		FatArrow,   // =>

		LParen,    // (
		RParen,    // )
		LBrace,    // {
		RBrace,    // }
		LBracket,  // [
		RBracket,  // ]
		Comma,     // ,
		Semicolon, // ;
		Colon,     // :
		Dot,       // .
		Ellipsis,  // ...
		Underscore // _
	};

	struct Token
	{
		TokenType type;
		std::string_view value;
		int line;
		int column;
	};

	inline std::string_view tokenTypeToString(TokenType type)
	{
		switch (type)
		{
		case TokenType::EndOfFile:
			return "EOF";
		case TokenType::Identifier:
			return "Identifier";
		case TokenType::Lifetime:
			return "Lifetime";
		case TokenType::Integer:
			return "Integer";
		case TokenType::Float:
			return "Float";
		case TokenType::String:
			return "String";
		case TokenType::Char:
			return "Char";

			// 关键字
		case TokenType::Fn:
			return "fn";
		case TokenType::Let:
			return "let";
		case TokenType::Mut:
			return "mut";
		case TokenType::Class:
			return "class";
		case TokenType::Struct:
			return "struct";
		case TokenType::Enum:
			return "enum";
		case TokenType::If:
			return "if";
		case TokenType::Else:
			return "else";
		case TokenType::Switch:
			return "switch";
		case TokenType::Case:
			return "case";
		case TokenType::While:
			return "while";
		case TokenType::For:
			return "for";
		case TokenType::Do:
			return "do";
		case TokenType::Return:
			return "return";
		case TokenType::Import:
			return "import";
		case TokenType::Package:
			return "package";
		case TokenType::Use:
			return "use";
		case TokenType::Pub:
			return "pub";
		case TokenType::As:
			return "as";
		case TokenType::Break:
			return "break";
		case TokenType::Continue:
			return "continue";
		case TokenType::Fallthrough:
			return "fallthrough";
		case TokenType::Default:
			return "default";
		case TokenType::Void:
			return "void";
		case TokenType::Bool:
			return "bool";
		case TokenType::Self:
			return "self";
		case TokenType::CapitalSelf:
			return "Self";
		case TokenType::True:
			return "true";
		case TokenType::False:
			return "false";
		case TokenType::Nullptr:
			return "nullptr";
		case TokenType::Extern:
			return "extern";
		case TokenType::Unsafe:
			return "unsafe";
		case TokenType::Malloc:
			return "malloc";
		case TokenType::Alloca:
			return "alloca";
		case TokenType::Free:
			return "free";
		case TokenType::Sizeof:
			return "sizeof";
		case TokenType::Alignof:
			return "alignof";
		case TokenType::Offsetof:
			return "offsetof";
		case TokenType::Align:
			return "align";
		case TokenType::Packed:
			return "packed";

			// 内置类型关键字
		case TokenType::I8:
			return "i8";
		case TokenType::I16:
			return "i16";
		case TokenType::I32:
			return "i32";
		case TokenType::I64:
			return "i64";
		case TokenType::U8:
			return "u8";
		case TokenType::U16:
			return "u16";
		case TokenType::U32:
			return "u32";
		case TokenType::U64:
			return "u64";
		case TokenType::F32:
			return "f32";
		case TokenType::F64:
			return "f64";

			// 运算符
		case TokenType::Plus:
			return "+";
		case TokenType::Minus:
			return "-";
		case TokenType::Star:
			return "*";
		case TokenType::Slash:
			return "/";
		case TokenType::Percent:
			return "%";
		case TokenType::Question:
			return "?";
		case TokenType::Equal:
			return "=";
		case TokenType::EqualEqual:
			return "==";
		case TokenType::NotEqual:
			return "!=";
		case TokenType::Greater:
			return ">";
		case TokenType::GreaterEqual:
			return ">=";
		case TokenType::Less:
			return "<";
		case TokenType::LessEqual:
			return "<=";
		case TokenType::Not:
			return "!";
		case TokenType::Ampersand:
			return "&";
		case TokenType::Pipe:
			return "|";
		case TokenType::Caret:
			return "^";
		case TokenType::Tilde:
			return "~";
		case TokenType::Tick:
			return "'";

			// 复合运算符
		case TokenType::PlusEqual:
			return "+=";
		case TokenType::MinusEqual:
			return "-=";
		case TokenType::StarEqual:
			return "*=";
		case TokenType::SlashEqual:
			return "/=";
		case TokenType::PercentEqual:
			return "%=";
		case TokenType::AmpersandEqual:
			return "&=";
		case TokenType::PipeEqual:
			return "|=";
		case TokenType::CaretEqual:
			return "^=";
		case TokenType::ShiftLeftEqual:
			return "<<=";
		case TokenType::ShiftRightEqual:
			return ">>=";
		case TokenType::AndAnd:
			return "&&";
		case TokenType::OrOr:
			return "||";
		case TokenType::PlusPlus:
			return "++";
		case TokenType::MinusMinus:
			return "--";
		case TokenType::ShiftLeft:
			return "<<";
		case TokenType::ShiftRight:
			return ">>";
		case TokenType::ColonColon:
			return "::";
		case TokenType::FatArrow:
			return "=>";

			// 分隔符
		case TokenType::LParen:
			return "(";
		case TokenType::RParen:
			return ")";
		case TokenType::LBrace:
			return "{";
		case TokenType::RBrace:
			return "}";
		case TokenType::LBracket:
			return "[";
		case TokenType::RBracket:
			return "]";
		case TokenType::Comma:
			return ",";
		case TokenType::Semicolon:
			return ";";
		case TokenType::Colon:
			return ":";
		case TokenType::Dot:
			return ".";
		case TokenType::Ellipsis:
			return "...";
		case TokenType::Underscore:
			return "Underscore";

		default:
			return "Unknown";
		}
	}
}

#endif // CHTHOLLY_TOKEN_H