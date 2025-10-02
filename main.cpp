#include <iostream>
#include <string>
#include <vector>
#include <cctype>
#include <unordered_set>
#include <unordered_map>
#include <fstream>
#include <sstream>

enum class TokenType {
    NUMBER_INT,
    NUMBER_FLOAT,
    NUMBER_HEX,

    STRING_LITERAL,

    IDENTIFIER_LOCAL,    
    IDENTIFIER_INSTANCE, 
    IDENTIFIER_CLASS,    
    IDENTIFIER_GLOBAL,   
    CONSTANT,            

    SYMBOL,              
    KEYWORD,
    OPERATOR,
    SEPARATOR,           
    COMMENT,
    
    RANGE_INCLUSIVE,     
    RANGE_EXCLUSIVE,     

    UNKNOWN,             
    END_OF_FILE          
};

struct Token {
    TokenType type;
    std::string lexeme; 
    int line;           
};

std::string tokenTypeToString(TokenType type) {
    static const std::unordered_map<TokenType, std::string> typeMap = {
        {TokenType::NUMBER_INT, "NUMBER_INT"},
        {TokenType::NUMBER_FLOAT, "NUMBER_FLOAT"},
        {TokenType::NUMBER_HEX, "NUMBER_HEX"},
        {TokenType::STRING_LITERAL, "STRING_LITERAL"},
        {TokenType::IDENTIFIER_LOCAL, "IDENTIFIER_LOCAL"},
        {TokenType::IDENTIFIER_INSTANCE, "IDENTIFIER_INSTANCE"},
        {TokenType::IDENTIFIER_CLASS, "IDENTIFIER_CLASS"},
        {TokenType::IDENTIFIER_GLOBAL, "IDENTIFIER_GLOBAL"},
        {TokenType::CONSTANT, "CONSTANT"},
        {TokenType::SYMBOL, "SYMBOL"},
        {TokenType::KEYWORD, "KEYWORD"},
        {TokenType::OPERATOR, "OPERATOR"},
        {TokenType::SEPARATOR, "SEPARATOR"},
        {TokenType::COMMENT, "COMMENT"},
        {TokenType::RANGE_INCLUSIVE, "RANGE_INCLUSIVE"},
        {TokenType::RANGE_EXCLUSIVE, "RANGE_EXCLUSIVE"},
        {TokenType::UNKNOWN, "UNKNOWN"},
        {TokenType::END_OF_FILE, "END_OF_FILE"}
    };
    auto it = typeMap.find(type);
    if (it != typeMap.end()) {
        return it->second;
    }
    return "INVALID_TOKEN_TYPE";
}


class Lexer {
public:
    Lexer(std::string source) : source(std::move(source)), current(0), line(1) {}

    std::vector<Token> analyze() {
        std::vector<Token> tokens;
        while (!isAtEnd()) {
            start = current;
            char c = advance();
            scanToken(c, tokens);
        }
        tokens.push_back({TokenType::END_OF_FILE, "", line});
        return tokens;
    }

private:
    std::string source;
    int start = 0;
    int current = 0;
    int line = 1;

    const std::unordered_set<std::string> rubyKeywords = {
        "alias", "and", "begin", "break", "case", "class", "def", "defined?", 
        "do", "else", "elsif", "end", "ensure", "false", "for", "if", "in", 
        "module", "next", "nil", "not", "or", "redo", "rescue", "retry", 
        "return", "self", "super", "then", "true", "undef", "unless", 
        "until", "when", "while", "yield"
    };

    bool isAtEnd() { return current >= source.length(); }
    char advance() { return source[current++]; }
    char peek() { if (isAtEnd()) return '\0'; return source[current]; }
    char peekNext() { if (current + 1 >= source.length()) return '\0'; return source[current + 1]; }
    bool match(char expected) {
        if (isAtEnd() || source[current] != expected) return false;
        current++;
        return true;
    }
    
    void addToken(TokenType type, std::vector<Token>& tokens) {
        std::string lexeme = source.substr(start, current - start);
        tokens.push_back({type, lexeme, line});
    }

    void scanToken(char c, std::vector<Token>& tokens) {
        switch (c) {
            case '(': case ')': case '[': case ']': case '{': case '}': case ',': case ';':
                addToken(TokenType::SEPARATOR, tokens);
                break;

            case '!': addToken(match('=') ? TokenType::OPERATOR : TokenType::OPERATOR, tokens); break;
            case '=': addToken(match('=') || match('>') ? TokenType::OPERATOR : TokenType::OPERATOR, tokens); break;
            case '<': addToken(match('=') ? TokenType::OPERATOR : TokenType::OPERATOR, tokens); break;
            case '>': addToken(match('=') ? TokenType::OPERATOR : TokenType::OPERATOR, tokens); break;
            case '+': addToken(match('=') ? TokenType::OPERATOR : TokenType::OPERATOR, tokens); break;
            case '*': addToken(match('*') || match('=') ? TokenType::OPERATOR : TokenType::OPERATOR, tokens); break;
            case '/': addToken(match('=') ? TokenType::OPERATOR : TokenType::OPERATOR, tokens); break;
            case '|': addToken(match('|') ? TokenType::OPERATOR : TokenType::OPERATOR, tokens); break;

            case '#':
                while (peek() != '\n' && !isAtEnd()) advance();
                addToken(TokenType::COMMENT, tokens);
                break;

            case '.':
                if (match('.')) {
                    addToken(match('.') ? TokenType::RANGE_EXCLUSIVE : TokenType::RANGE_INCLUSIVE, tokens);
                } else {
                     addToken(TokenType::OPERATOR, tokens);
                }
                break;
            
            case ':':
                if (isalpha(peek()) || peek() == '_') {
                    advance(); 
                    while (isalnum(peek()) || peek() == '_') advance();
                    addToken(TokenType::SYMBOL, tokens);
                } else {
                    addToken(TokenType::OPERATOR, tokens);
                }
                break;
            
            case '"': case '\'':
                scanString(c, tokens);
                break;

            case ' ': case '\r': case '\t': break;
            case '\n': line++; break;

            default:
                if (isdigit(c)) {
                    if (c == '0' && (peek() == 'x' || peek() == 'X')) {
                        advance(); 
                        while (isxdigit(peek())) {
                            advance();
                        }
                        addToken(TokenType::NUMBER_HEX, tokens);
                    } else { 
                        while (isdigit(peek())) {
                            advance();
                        }
                        if (peek() == '.' && isdigit(peekNext())) {
                            advance(); 
                            while (isdigit(peek())) {
                                advance();
                            }
                            addToken(TokenType::NUMBER_FLOAT, tokens);
                        } else {
                            addToken(TokenType::NUMBER_INT, tokens);
                        }
                    }
                } else if (isalpha(c) || c == '_') {
                    scanIdentifier(tokens);
                } else if (c == '@' || c == '$') {
                    scanPrefixedIdentifier(tokens);
                }
                else {
                    addToken(TokenType::UNKNOWN, tokens);
                }
                break;
        }
    }

    void scanString(char quote_type, std::vector<Token>& tokens) {
        while (peek() != quote_type && !isAtEnd()) {
            if (peek() == '\n') line++;
            if (peek() == '\\' && peekNext() == quote_type) {
                advance();
            }
            advance();
        }
        if (isAtEnd()) {
            addToken(TokenType::UNKNOWN, tokens); 
            return;
        }
        advance(); 
        addToken(TokenType::STRING_LITERAL, tokens);
    }
    
    void scanIdentifier(std::vector<Token>& tokens) {
        current--;
        
        while (isalnum(peek()) || peek() == '_') {
            advance();
        }
        std::string lexeme = source.substr(start, current - start);
        if (isupper(lexeme[0])) {
            addToken(TokenType::CONSTANT, tokens);
        } else if (rubyKeywords.count(lexeme)) {
            addToken(TokenType::KEYWORD, tokens);
        } else {
            addToken(TokenType::IDENTIFIER_LOCAL, tokens);
        }
    }
    
    void scanPrefixedIdentifier(std::vector<Token>& tokens) {
        char prefix = source[start];
        TokenType type;
        if (prefix == '$') {
            type = TokenType::IDENTIFIER_GLOBAL;
        } else if (prefix == '@') {
            type = match('@') ? TokenType::IDENTIFIER_CLASS : TokenType::IDENTIFIER_INSTANCE;
        } else {
            addToken(TokenType::UNKNOWN, tokens); 
            return;
        }
        
        if (isalpha(peek()) || peek() == '_') {
             while (isalnum(peek()) || peek() == '_') advance();
             addToken(type, tokens);
        } else {
             addToken(TokenType::OPERATOR, tokens);
        }
    }
};


void printTokens(const std::vector<Token>& tokens) {
    for (const auto& token : tokens) {
        std::cout << "Line " << token.line << ":\t"
                  << "< " << token.lexeme << " >"
                  << "\t -> " << tokenTypeToString(token.type)
                  << std::endl;
    }
}


int main() {
    std::ifstream file("ruby_code.txt");
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open ruby_code.txt" << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string ruby_code = buffer.str();

    std::cout << "--- Analyzing Ruby Code (Regular solution) ---" << std::endl;
    std::cout << ruby_code << std::endl;
    std::cout << "--------------------------" << std::endl;

    Lexer lexer(ruby_code);
    std::vector<Token> tokens = lexer.analyze();
    printTokens(tokens);

    return 0;
}