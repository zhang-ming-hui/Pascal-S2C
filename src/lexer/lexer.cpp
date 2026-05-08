#include "lexer/lexer.h"

#include <array>
#include <cctype>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace pascal_s2c {

namespace {

class KeywordTrie {
public:
    KeywordTrie() {
        nodes_.push_back(Node{});

        insert("program", TokenKind::Program);
        insert("const", TokenKind::Const);
        insert("var", TokenKind::Var);
        insert("type", TokenKind::Type);
        insert("record", TokenKind::Record);
        insert("array", TokenKind::Array);
        insert("of", TokenKind::Of);
        insert("begin", TokenKind::Begin);
        insert("end", TokenKind::End);
        insert("function", TokenKind::Function);
        insert("procedure", TokenKind::Procedure);
        insert("integer", TokenKind::Integer);
        insert("real", TokenKind::Real);
        insert("boolean", TokenKind::Boolean);
        insert("char", TokenKind::Char);
        insert("if", TokenKind::If);
        insert("then", TokenKind::Then);
        insert("else", TokenKind::Else);
        insert("while", TokenKind::While);
        insert("do", TokenKind::Do);
        insert("for", TokenKind::For);
        insert("to", TokenKind::To);
        insert("downto", TokenKind::Downto);
        insert("case", TokenKind::Case);
        insert("repeat", TokenKind::Repeat);
        insert("until", TokenKind::Until);
        insert("break", TokenKind::Break);
        insert("read", TokenKind::Read);
        insert("write", TokenKind::Write);
        insert("div", TokenKind::Div);
        insert("mod", TokenKind::Mod);
        insert("and", TokenKind::And);
        insert("or", TokenKind::Or);
        insert("not", TokenKind::Not);
        insert("true", TokenKind::True);
        insert("false", TokenKind::False);
    }

    std::optional<TokenKind> find(std::string_view word) const {
        int nodeIndex = 0;
        for (char ch : word) {
            const int nextIndex = childIndex(nodeIndex, ch);
            if (nextIndex < 0) {
                return std::nullopt;
            }
            nodeIndex = nextIndex;
        }
        return nodes_[static_cast<std::size_t>(nodeIndex)].kind;
    }

private:
    static constexpr int kAlphabetSize = 26;

    struct Node {
        std::array<int, kAlphabetSize> children{};
        std::optional<TokenKind> kind;

        Node() {
            children.fill(-1);
        }
    };

    static int charToIndex(char ch) {
        return (ch >= 'a' && ch <= 'z') ? (ch - 'a') : -1;
    }

    void insert(std::string_view word, TokenKind kind) {
        int nodeIndex = 0;
        for (char ch : word) {
            const int edge = charToIndex(ch);
            if (edge < 0) {
                continue;
            }

            const std::size_t currentIndex = static_cast<std::size_t>(nodeIndex);
            int child = nodes_[currentIndex].children[edge];
            if (child < 0) {
                child = static_cast<int>(nodes_.size());
                nodes_[currentIndex].children[edge] = child;
                nodes_.push_back(Node{});
            }
            nodeIndex = child;
        }
        nodes_[static_cast<std::size_t>(nodeIndex)].kind = kind;
    }

    int childIndex(int nodeIndex, char ch) const {
        const int edge = charToIndex(ch);
        if (edge < 0) {
            return -1;
        }
        return nodes_[static_cast<std::size_t>(nodeIndex)].children[edge];
    }

    std::vector<Node> nodes_;
};

const KeywordTrie& keywordTrie() {
    static const KeywordTrie trie;
    return trie;
}


class LexerScanner {
public:
    explicit LexerScanner(const std::string& source) : source_(source) {}

    TokenList scan() {
        TokenList tokens;

        while (!isAtEnd()) {
            if (std::optional<Token> error = skipWhitespaceAndComments()) {
                tokens.push_back(std::move(*error));
                continue;
            }
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

    std::optional<Token> skipWhitespaceAndComments() {
        while (!isAtEnd()) {
            const char ch = peek();
            if (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n') {
                advance();
                continue;
            }
            if (ch == '{') {
                if (std::optional<Token> error = skipBraceComment()) {
                    return error;
                }
                continue;
            }
            if (ch == '/' && peekNext() == '/') {
                skipLineComment();
                continue;
            }
            if (ch == '(' && peekNext() == '*') {
                if (std::optional<Token> error = skipParenComment()) {
                    return error;
                }
                continue;
            }
            break;
        }
        return std::nullopt;
    }

    std::optional<Token> skipBraceComment() {
        const SourceLocation start = location_;
        std::string lexeme;
        advance();
        lexeme.push_back('{');
        while (!isAtEnd() && peek() != '}') {
            lexeme.push_back(advance());
        }
        if (isAtEnd()) {
            return Token{TokenKind::Error, lexeme, start, "unterminated comment"};
        }
        lexeme.push_back(advance());
        return std::nullopt;
    }

    void skipLineComment() {
        advance();
        advance();
        while (!isAtEnd() && peek() != '\n') {
            advance();
        }
    }

    std::optional<Token> skipParenComment() {
        const SourceLocation start = location_;
        std::string lexeme;
        advance();
        lexeme.push_back('(');
        advance();
        lexeme.push_back('*');
        while (!isAtEnd()) {
            if (peek() == '*' && peekNext() == ')') {
                lexeme.push_back(advance());
                lexeme.push_back(advance());
                return std::nullopt;
            }
            lexeme.push_back(advance());
        }
        return Token{TokenKind::Error, lexeme, start, "unterminated comment"};
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

        const std::optional<TokenKind> kind = keywordTrie().find(lowered);
        if (kind.has_value()) {
            return Token{*kind, lexeme, start};
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
                return Token{TokenKind::Error, lexeme, start, "unterminated character literal"};
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

        return Token{TokenKind::Error, lexeme, start, "unterminated character literal"};
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
            return Token{TokenKind::Error, std::string(1, ch), start, std::string("unexpected character: ") + ch};
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
