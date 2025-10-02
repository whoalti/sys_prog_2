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

 

class LexerFiniteAutomaton {
private:
    
    enum class State {
        START,
        IN_IDENTIFIER_LOCAL, 
        IN_CONSTANT,
        SAW_ZERO, 
        IN_NUMBER_INT, 
        IN_NUMBER_FLOAT, 
        IN_HEX_NUMBER,
        SAW_AT, 
        IN_INSTANCE_VAR, 
        SAW_DOUBLE_AT, 
        IN_CLASS_VAR,
        SAW_DOLLAR, 
        IN_GLOBAL_VAR, 
        SAW_COLON, 
        IN_SYMBOL,
        IN_STRING, 
        IN_COMMENT, 
        SAW_DOT, 
        IN_RANGE,
        SAW_EQUALS, 
        SAW_PLUS,
        SAW_MINUS, 
        SAW_STAR, 
        SAW_SLASH, 
        SAW_PIPE 
    };

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
    char peek() { return isAtEnd() ? '\0' : source[current]; }
    char peekNext() { return (current + 1 >= source.length()) ? '\0' : source[current + 1]; }
    
    Token makeToken(TokenType type) { return {type, source.substr(start, current - start), line}; }
    
    Token makeIdentifierToken() {
        std::string lexeme = source.substr(start, current - start);
        if (isupper(lexeme[0])) { return {TokenType::CONSTANT, lexeme, line}; }
        if (rubyKeywords.count(lexeme)) { return {TokenType::KEYWORD, lexeme, line}; }
        return {TokenType::IDENTIFIER_LOCAL, lexeme, line};
    }

public:
    LexerFiniteAutomaton(std::string source) : source(std::move(source)) {}

    std::vector<Token> analyze() {
        std::vector<Token> tokens;
        while (!isAtEnd()) {
            Token token = scanNextToken();
            if (token.type == TokenType::END_OF_FILE) break;
            tokens.push_back(token);
        }
        tokens.push_back({TokenType::END_OF_FILE, "", line});
        return tokens;
    }

private:
    Token scanNextToken() {
        start = current;
        State currentState = State::START;
        
        
        while(isspace(peek())) {
            if (peek() == '\n') line++;
            advance();
        }
        start = current;

        if (isAtEnd()) return makeToken(TokenType::END_OF_FILE);

        char c = advance(); 
        
        if (islower(c) || c == '_') { currentState = State::IN_IDENTIFIER_LOCAL; }
        else if (isupper(c)) { currentState = State::IN_CONSTANT; }
        else if (c == '0') { currentState = State::SAW_ZERO; }
        else if (isdigit(c)) { currentState = State::IN_NUMBER_INT; }
        else if (c == '@') { currentState = State::SAW_AT; }
        else if (c == '$') { currentState = State::SAW_DOLLAR; }
        else if (c == ':') { currentState = State::SAW_COLON; }
        else if (c == '#') { currentState = State::IN_COMMENT; }
        else if (c == '"') { currentState = State::IN_STRING; }
        else if (c == '.') { currentState = State::SAW_DOT; }
        else if (c == '=') { currentState = State::SAW_EQUALS; } 
        else if (c == '+') { currentState = State::SAW_PLUS; }
        else if (c == '-') { currentState = State::SAW_MINUS; }
        else if (c == '*') { currentState = State::SAW_STAR; }
        else if (c == '/') { currentState = State::SAW_SLASH; }
        else if (c == '|') { currentState = State::SAW_PIPE; }
        else if (strchr("()[]{},;", c)) { return makeToken(TokenType::SEPARATOR); }
        else if (strchr("<>!", c)) { return makeToken(TokenType::OPERATOR); }
        else { return makeToken(TokenType::UNKNOWN); }
        
        while (true) {
            char p = peek();
            switch (currentState) {
                case State::IN_IDENTIFIER_LOCAL: case State::IN_CONSTANT:
                case State::IN_INSTANCE_VAR: case State::IN_CLASS_VAR:
                case State::IN_GLOBAL_VAR: case State::IN_SYMBOL:
                    if (!isalnum(p) && p != '_') return makeIdentifierTokenByType(currentState);
                    advance();
                    break;

                case State::SAW_ZERO:
                    if (p == 'x' || p == 'X') { advance(); currentState = State::IN_HEX_NUMBER; }
                    else if (p == '.' && isdigit(peekNext())) { advance(); currentState = State::IN_NUMBER_FLOAT; }
                    else if (isdigit(p)) { currentState = State::IN_NUMBER_INT; }
                    else return makeToken(TokenType::NUMBER_INT);
                    break;

                case State::IN_NUMBER_INT:
                    if (p == '.' && isdigit(peekNext())) { advance(); currentState = State::IN_NUMBER_FLOAT; }
                    else if (!isdigit(p)) return makeToken(TokenType::NUMBER_INT);
                    advance();
                    break;
                    
                case State::IN_NUMBER_FLOAT:
                    if (!isdigit(p)) return makeToken(TokenType::NUMBER_FLOAT);
                    advance();
                    break;

                case State::IN_HEX_NUMBER:
                    if (!isxdigit(p)) {
                        if (current - start <= 2) return makeToken(TokenType::UNKNOWN);
                        return makeToken(TokenType::NUMBER_HEX);
                    }
                    advance();
                    break;

                case State::SAW_AT:
                    if (p == '@') { advance(); currentState = State::SAW_DOUBLE_AT; }
                    else if (isalpha(p) || p == '_') { advance(); currentState = State::IN_INSTANCE_VAR; }
                    else return makeToken(TokenType::OPERATOR);
                    break;
                
                case State::SAW_DOUBLE_AT:
                    if (isalpha(p) || p == '_') { advance(); currentState = State::IN_CLASS_VAR; }
                    else return makeToken(TokenType::UNKNOWN);
                    break;

                case State::SAW_DOLLAR:
                    if (isalpha(p) || p == '_') { advance(); currentState = State::IN_GLOBAL_VAR; }
                    else return makeToken(TokenType::OPERATOR);
                    break;

                case State::SAW_COLON:
                    if (isalpha(p) || p == '_') { advance(); currentState = State::IN_SYMBOL; }
                    else return makeToken(TokenType::OPERATOR);
                    break;
                
                case State::IN_COMMENT:
                    if (p == '\n' || p == '\0') return makeToken(TokenType::COMMENT);
                    advance();
                    break;
                
                case State::IN_STRING:
                    if (p == '"') { advance(); return makeToken(TokenType::STRING_LITERAL); }
                    else if (p == '\0') return makeToken(TokenType::UNKNOWN);
                    advance();
                    break;
                
                case State::SAW_DOT:
                    if (p == '.') { advance(); currentState = State::IN_RANGE; }
                    else return makeToken(TokenType::OPERATOR);
                    break;

                case State::IN_RANGE:
                    if (p == '.') { advance(); return makeToken(TokenType::RANGE_EXCLUSIVE); }
                    else return makeToken(TokenType::RANGE_INCLUSIVE);
                    break;

                case State::SAW_EQUALS:
                    if (p == '=' || p == '>') { advance(); }
                    return makeToken(TokenType::OPERATOR);

                case State::SAW_PLUS:
                    if (p == '=') { advance(); }
                    return makeToken(TokenType::OPERATOR);

                case State::SAW_MINUS:
                    if (p == '=') { advance(); }
                    return makeToken(TokenType::OPERATOR);

                case State::SAW_STAR:
                    if (p == '=') { advance(); }
                    return makeToken(TokenType::OPERATOR);

                case State::SAW_SLASH:
                    if (p == '=') { advance(); }
                    return makeToken(TokenType::OPERATOR);
                
                case State::SAW_PIPE:
                    if (p == '|') { advance(); }
                    return makeToken(TokenType::OPERATOR);
                default:
                    return makeToken(TokenType::UNKNOWN);
            }
        }
    }

    Token makeIdentifierTokenByType(State s) {
        switch (s) {
            case State::IN_IDENTIFIER_LOCAL: return makeIdentifierToken();
            case State::IN_CONSTANT: return makeToken(TokenType::CONSTANT);
            case State::IN_INSTANCE_VAR: return makeToken(TokenType::IDENTIFIER_INSTANCE);
            case State::IN_CLASS_VAR: return makeToken(TokenType::IDENTIFIER_CLASS);
            case State::IN_GLOBAL_VAR: return makeToken(TokenType::IDENTIFIER_GLOBAL);
            case State::IN_SYMBOL: return makeToken(TokenType::SYMBOL);
            default: return makeToken(TokenType::UNKNOWN);
        }
    }
};

void printTokens(const std::vector<Token>& tokens) {
    for (const auto& token : tokens) {;
        std::cout << "Line " << token.line << ":\t"
                  << "< " << token.lexeme << " >"
                  << "\t -> " << tokenTypeToString(token.type)
                  << std::endl;
    }
}

int main() {
    std::string filename;
    std::cout << "Enter the name of the file to read from: ";
    std::cin >> filename;

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Unable to open " << filename << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string ruby_code = buffer.str();

    std::cout << "--- Analyzing Ruby Code (Finite Automaton) ---" << std::endl;
    std::cout << ruby_code << std::endl;
    std::cout << "--------------------------" << std::endl;

    LexerFiniteAutomaton lexer(ruby_code);
    std::vector<Token> tokens = lexer.analyze();
    printTokens(tokens);

    return 0;
}