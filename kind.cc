#include "kind.h"
#include <sstream>
using std::string;
using std::istringstream;

// Use the annonymous namespace to prevent external linking
namespace {
	// Template function to convert from a string to some Type
	// Primarily used for string to integer conversions
	template <typename T>
		T fromString(const string& s, bool hex = false){
			istringstream iss(s);
			T n;
			if(hex)
				iss >> std::hex;
			if(iss >> n)
				return n;
			else
				throw string("ERROR: Type not convertible from string.");
		}
	// Strings representing the Kind of Token for printing
	const string kindStrings[] ={
		"ID",          // Opcode or identifier (e.g. label use without colon)
		"NUM",         // Decimal integer
		"LPAREN",      // (
		"RPAREN",      // )
		"LBRACE",      // {
		"RBRACE",      // }
		"RETURN",      // return
		"IF",          // if
		"ELSE",        // else
		"WHILE",       // while
		"PRINTLN",     // println
		"WAIN",        // wain
		"BECOMES",     // =
		"INT",         // int
		"EQ",          // ==
		"NE",          // !=
		"LT",          // <
		"GT",          // >
		"LE",          // <=
		"GE",          // >=
		"PLUS",        // +
		"MINUS",       // -
		"STAR",        // *
		"SLASH",       // /
		"PCT",         // %
		"COMMA",       // Comma
		"SEMI",        // ;
		"NEW",         // new
		"DELETE",      // delete
		"LBRACK",      // [
		"RBRACK",      // ]
		"AMP",         // &
		"NULLp",       // NULLp
		"WHITESPACE",  // Whitespace
		"ERR"          // Bad/Invalid Token

	};
}

ASM::Token::Token(ASM::Kind kind, string lexeme)
	: kind(kind), lexeme(lexeme){}

	// Cannot convert Tokens that do not represent numbers
	// to an Integer
	int ASM::Token::toInt() const{
		throw "ERROR: attempt to convert non-integer token: " + lexeme;
	}

string ASM::Token::toString() const{
	return ::kindStrings[kind];
}

string ASM::Token::getLexeme() const{
	return lexeme;
}

string ASM::Token::getLine() const {
	string line = "";
	if(getKind() == ASM::NUM) {
		std::ostringstream oss;
		oss<<toInt();
		line = toString() + " " + oss.str();
	}
	else if (toString() == "NULLp")
		line = "NULL " + getLexeme();
	else
		line = toString() + " " + getLexeme();
	return line;
}

ASM::Kind ASM::Token::getKind() const{
	return kind;
}

ASM::IntToken::IntToken(ASM::Kind kind, string lexeme)
	: Token(kind,lexeme){}

	// Ensure that the IntToken is a valid Integer for conversion
	// Arguably, we could do this check in the constructor
	int ASM::IntToken::toInt() const{
		long long l = ::fromString<long long>(lexeme);
			if(l > 2147483647LL)
				throw "ERROR: constant out of range: "  + lexeme;
		return l;
	}

//ASM::HexIntToken::HexIntToken(ASM::Kind kind, string lexeme)
//: Token(kind,lexeme){}
//
//// Ensure that the HexIntToken is a valid Integer for conversion
//// Arguably, we could do this check in the constructor
//int ASM::HexIntToken::toInt() const{
//  unsigned long long ul = ::fromString<unsigned long long>(lexeme,true);
//  if(ul > 0xffffffffLL)
//      throw "ERROR: constant out of range: " + lexeme;
//  return ul;
//}
//
//ASM::RegisterToken::RegisterToken(Kind kind, string lexeme)
//: Token(kind,lexeme){}
//
//// Ensure that the RegisterToken is a valid integer for conversion
//// Arguably, we could do this check in the constructor
//int ASM::RegisterToken::toInt() const{
//  unsigned long long ul = ::fromString<unsigned long long>(lexeme.substr(1));
//  if(ul > 31)
//      throw "ERROR: constant out of range: " + lexeme;
//  return ul;
//}

// Print the pertinent information to the ostream
std::ostream& ASM::operator<<(std::ostream& out, const ASM::Token& t){
	// Could use a dynamic cast here but it's "cheaper" to check the Kind
//	if(t.getKind() == ASM::INT || t.getKind() == ASM::HEXINT
//			|| t.getKind() == ASM::REGISTER)
//		out << t.toInt();
	if(t.getKind() == ASM::NUM) 
		out << t.toString() << " " << t.toInt();
	else if (t.toString() == "NULLp")
		out << "NULL " << t.getLexeme();
	else
		out << t.toString() << " " << t.getLexeme();
	return out;
}

// Make a new Token depending on the Kind provided
// Integral Kinds correspond to the appropriate Token type
ASM::Token* ASM::Token::makeToken(ASM::Kind kind, string lexeme){
	switch(kind){
		case NUM:
			return new IntToken(kind,lexeme);
		default:
			return new Token(kind,lexeme);
	}
}
