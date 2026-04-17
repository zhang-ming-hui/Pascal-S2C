#include "lexer/lexer.h"

#include <cctype>
#include <unordered_map>

#include "common/error.h"

namespace pascal_s2c {

namespace {

class LexerScanner {
public:
    explicit LexerScanner(const std::string& source) : source_(source) {}

    TokenList scan() {
        TokenList tokens;

        while (!isAtEnd()) {
            skipWhitespaceAndComments();
            if (isAtEnd()) {
                break;
            }

            const SourceLocation start = location_;
            const char current = peek();

            if (isIdentifierStart(current)) {
                tokens.push_back(scanIdentifierOrKeyword(start));
                continue;
            }

            if (std::isdigit(static_cast<unsigned char>(current)) != 0) {
                tokens.push_back(scanNumber(start));
                continue;
            }

            if (current == '\'') {
                tokens.push_back(scanQuotedLiteral(start));
                continue;
            }

            tokens.push_back(scanSymbol(start));
        }

        tokens.push_back(Token{TokenKind::EndOfFile, "", location_});
        return tokens;
    }

private:
    static bool isIdentifierStart(char ch) {
        return ch == '_' || std::isalpha(static_cast<unsigned char>(ch)) != 0;
    }

    static bool isIdentifierPart(char ch) {
        return ch == '_' || std::isalnum(static_cast<unsigned char>(ch)) != 0;
    }

    static char toLowerAscii(char ch) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }

    bool isAtEnd() const {
        return index_ >= source_.size();
    }

    char peek() const {
        return isAtEnd() ? '\0' : source_[index_];
    }

    char peekNext() const {
        return (index_ + 1) < source_.size() ? source_[index_ + 1] : '\0';
    }

    char advance() {
        const char ch = source_[index_++];
        if (ch == '\n') {
            location_.line += 1;
            location_.column = 1;
        } else {
            location_.column += 1;
        }
        return ch;
    }

    bool match(char expected) {
        if (peek() != expected) {
            return false;
        }
        advance();
        return true;
    }

    void skipWhitespaceAndComments() {
        while (!isAtEnd()) {
            const char ch = peek();
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                advance();
                continue;
            }
            if (ch == '{') {
                skipBraceComment();
                continue;
            }
            if (ch == '/' && peekNext() == '/') {
                skipLineComment();
                continue;
            }
            if (ch == '(' && peekNext() == '*') {
                skipParenComment();
                continue;
            }
            break;
        }
    }

    void skipBraceComment() {
        const SourceLocation start = location_;
        advance();
        while (!isAtEnd() && peek() != '}') {
            advance();
        }
        if (isAtEnd()) {
            throw CompilerError("lexer", "unterminated comment", start);
        }
        advance();
    }

    void skipLineComment() {
        advance();
        advance();
        while (!isAtEnd() && peek() != '\n') {
            advance();
        }
    }

    void skipParenComment() {
        const SourceLocation start = location_;
        advance();
        advance();
        while (!isAtEnd()) {
            if (peek() == '*' && peekNext() == ')') {
                advance();
                advance();
                return;
            }
            advance();
        }
        throw CompilerError("lexer", "unterminated comment", start);
    }

    Token scanIdentifierOrKeyword(SourceLocation start) {
        std::string lexeme;
        while (!isAtEnd() && isIdentifierPart(peek())) {
            lexeme.push_back(advance());
        }

        std::string lowered;
        lowered.reserve(lexeme.size());
        for (char ch : lexeme) {
            lowered.push_back(toLowerAscii(ch));
        }

        static const std::unordered_map<std::string, TokenKind> kKeywords = {
            {"program", TokenKind::Program},
            {"const", TokenKind::Const},
            {"var", TokenKind::Var},
            {"type", TokenKind::Type},
            {"record", TokenKind::Record},
            {"array", TokenKind::Array},
            {"of", TokenKind::Of},
            {"begin", TokenKind::Begin},
            {"end", TokenKind::End},
            {"function", TokenKind::Function},
            {"procedure", TokenKind::Procedure},
            {"integer", TokenKind::Integer},
            {"real", TokenKind::Real},
            {"boolean", TokenKind::Boolean},
            {"char", TokenKind::Char},
            {"if", TokenKind::If},
            {"then", TokenKind::Then},
            {"else", TokenKind::Else},
            {"while", TokenKind::While},
            {"do", TokenKind::Do},
            {"for", TokenKind::For},
            {"to", TokenKind::To},
            {"downto", TokenKind::Downto},
            {"case", TokenKind::Case},
            {"repeat", TokenKind::Repeat},
            {"until", TokenKind::Until},
            {"break", TokenKind::Break},
            {"read", TokenKind::Read},
            {"write", TokenKind::Write},
            {"div", TokenKind::Div},
            {"mod", TokenKind::Mod},
            {"and", TokenKind::And},
            {"or", TokenKind::Or},
            {"not", TokenKind::Not},
            {"true", TokenKind::True},
            {"false", TokenKind::False},
        };

        const auto it = kKeywords.find(lowered);
        if (it != kKeywords.end()) {
            return Token{it->second, lexeme, start};
        }
        return Token{TokenKind::Identifier, lowered, start};
    }

    Token scanNumber(SourceLocation start) {
        std::string lexeme;
        while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek())) != 0) {
            lexeme.push_back(advance());
        }

        bool isReal = false;
        if (peek() == '.' && peekNext() != '.' &&
            std::isdigit(static_cast<unsigned char>(peekNext())) != 0) {
            isReal = true;
            lexeme.push_back(advance());
            while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek())) != 0) {
                lexeme.push_back(advance());
            }
        }

        return Token{isReal ? TokenKind::RealLiteral : TokenKind::IntegerLiteral, lexeme, start};
    }

    Token scanQuotedLiteral(SourceLocation start) {
        std::string lexeme;
        lexeme.push_back(advance());

        int charCount = 0;
        while (!isAtEnd()) {
            const char ch = advance();
            lexeme.push_back(ch);

            if (ch == '\n' || ch == '\r') {
                throw CompilerError("lexer", "unterminated character literal", start);
            }

            if (ch == '\'') {
                if (peek() == '\'') {
                    lexeme.push_back(advance());
                    ++charCount;
                    continue;
                }
                if (charCount == 1 && lexeme.size() == 3) {
                    return Token{TokenKind::CharLiteral, lexeme, start};
                }
                return Token{TokenKind::StringLiteral, lexeme, start};
            }

            ++charCount;
        }

        throw CompilerError("lexer", "unterminated character literal", start);
    }

    Token scanSymbol(SourceLocation start) {
        const char ch = advance();
        switch (ch) {
        case ':':
            if (match('=')) {
                return Token{TokenKind::Assign, ":=", start};
            }
            return Token{TokenKind::Colon, ":", start};
        case '<':
            if (match('=')) {
                return Token{TokenKind::LessEqual, "<=", start};
            }
            if (match('>')) {
                return Token{TokenKind::NotEqual, "<>", start};
            }
            return Token{TokenKind::Less, "<", start};
        case '>':
            if (match('=')) {
                return Token{TokenKind::GreaterEqual, ">=", start};
            }
            return Token{TokenKind::Greater, ">", start};
        case '.':
            if (match('.')) {
                return Token{TokenKind::Range, "..", start};
            }
            return Token{TokenKind::Dot, ".", start};
        case '=':
            return Token{TokenKind::Equal, "=", start};
        case '+':
            return Token{TokenKind::Plus, "+", start};
        case '-':
            return Token{TokenKind::Minus, "-", start};
        case '*':
            return Token{TokenKind::Star, "*", start};
        case '/':
            return Token{TokenKind::Slash, "/", start};
        case ',':
            return Token{TokenKind::Comma, ",", start};
        case ';':
            return Token{TokenKind::Semicolon, ";", start};
        case '(':
            return Token{TokenKind::LParen, "(", start};
        case ')':
            return Token{TokenKind::RParen, ")", start};
        case '[':
            return Token{TokenKind::LBracket, "[", start};
        case ']':
            return Token{TokenKind::RBracket, "]", start};
        default:
            throw CompilerError("lexer", std::string("unexpected character: ") + ch, start);
        }
    }

    const std::string& source_;
    std::size_t index_ = 0;
    SourceLocation location_{1, 1};
};

}  // namespace

TokenList Lexer::tokenize(const std::string& source) const {
    LexerScanner scanner(source);
    return scanner.scan();
}

}  // namespace pascal_s2c
