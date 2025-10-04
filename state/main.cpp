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

struct Transition {
    std::string fromState;
    char character;
    std::string toState;
};

struct Token {
    TokenType type;
    std::string lexeme; 
    int line;           
    std::vector<Transition> transitions;
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

    std::string stateToString(State state) {
        static const std::unordered_map<State, std::string> stateMap = {
            {State::START, "START"},
            {State::IN_IDENTIFIER_LOCAL, "IN_IDENTIFIER_LOCAL"},
            {State::IN_CONSTANT, "IN_CONSTANT"},
            {State::SAW_ZERO, "SAW_ZERO"},
            {State::IN_NUMBER_INT, "IN_NUMBER_INT"},
            {State::IN_NUMBER_FLOAT, "IN_NUMBER_FLOAT"},
            {State::IN_HEX_NUMBER, "IN_HEX_NUMBER"},
            {State::SAW_AT, "SAW_AT"},
            {State::IN_INSTANCE_VAR, "IN_INSTANCE_VAR"},
            {State::SAW_DOUBLE_AT, "SAW_DOUBLE_AT"},
            {State::IN_CLASS_VAR, "IN_CLASS_VAR"},
            {State::SAW_DOLLAR, "SAW_DOLLAR"},
            {State::IN_GLOBAL_VAR, "IN_GLOBAL_VAR"},
            {State::SAW_COLON, "SAW_COLON"},
            {State::IN_SYMBOL, "IN_SYMBOL"},
            {State::IN_STRING, "IN_STRING"},
            {State::IN_COMMENT, "IN_COMMENT"},
            {State::SAW_DOT, "SAW_DOT"},
            {State::IN_RANGE, "IN_RANGE"},
            {State::SAW_EQUALS, "SAW_EQUALS"},
            {State::SAW_PLUS, "SAW_PLUS"},
            {State::SAW_MINUS, "SAW_MINUS"},
            {State::SAW_STAR, "SAW_STAR"},
            {State::SAW_SLASH, "SAW_SLASH"},
            {State::SAW_PIPE, "SAW_PIPE"}
        };
        auto it = stateMap.find(state);
        if (it != stateMap.end()) {
            return it->second;
        }
        return "UNKNOWN_STATE";
    }

    bool isAtEnd() { return current >= source.length(); }
    char advance() { return source[current++]; }
    char peek() { return isAtEnd() ? '\0' : source[current]; }
    char peekNext() { return (current + 1 >= source.length()) ? '\0' : source[current + 1]; }
    
    Token makeToken(TokenType type, const std::vector<Transition>& transitions) { return {type, source.substr(start, current - start), line, transitions}; }
    
    Token makeIdentifierToken(const std::vector<Transition>& transitions) {
        std::string lexeme = source.substr(start, current - start);
        if (isupper(lexeme[0])) { return {TokenType::CONSTANT, lexeme, line, transitions}; }
        if (rubyKeywords.count(lexeme)) { return {TokenType::KEYWORD, lexeme, line, transitions}; }
        return {TokenType::IDENTIFIER_LOCAL, lexeme, line, transitions};
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
        tokens.push_back({TokenType::END_OF_FILE, "", line, {}});
        return tokens;
    }

private:
    Token scanNextToken() {
        start = current;
        State currentState = State::START;
        std::vector<Transition> transitions;
        
        
        while(isspace(peek())) {
            if (peek() == '\n') line++;
            advance();
        }
        start = current;

        if (isAtEnd()) return makeToken(TokenType::END_OF_FILE, transitions);

        char c = advance(); 
        State prevState = currentState;
        
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
        else if (strchr("()[]{},;", c)) {
            transitions.push_back({stateToString(prevState), c, "SEPARATOR"});
            return makeToken(TokenType::SEPARATOR, transitions);
        }
        else if (strchr("<>!", c)) {
            transitions.push_back({stateToString(prevState), c, "OPERATOR"});
            return makeToken(TokenType::OPERATOR, transitions);
        }
        else {
            transitions.push_back({stateToString(prevState), c, "UNKNOWN"});
            return makeToken(TokenType::UNKNOWN, transitions);
        }
        
        transitions.push_back({stateToString(prevState), c, stateToString(currentState)});

        while (true) {
            char p = peek();
            prevState = currentState;

            switch (currentState) {
                case State::IN_IDENTIFIER_LOCAL: case State::IN_CONSTANT:
                case State::IN_INSTANCE_VAR: case State::IN_CLASS_VAR:
                case State::IN_GLOBAL_VAR: case State::IN_SYMBOL:
                    if (!isalnum(p) && p != '_') return makeIdentifierTokenByType(currentState, transitions);
                    advance();
                    transitions.push_back({stateToString(prevState), p, stateToString(currentState)});
                    break;

                case State::SAW_ZERO:
                    if (p == 'x' || p == 'X') { advance(); currentState = State::IN_HEX_NUMBER; }
                    else if (p == '.' && isdigit(peekNext())) { advance(); currentState = State::IN_NUMBER_FLOAT; }
                    else if (isdigit(p)) { currentState = State::IN_NUMBER_INT; }
                    else return makeToken(TokenType::NUMBER_INT, transitions);
                    transitions.push_back({stateToString(prevState), p, stateToString(currentState)});
                    break;

                case State::IN_NUMBER_INT:
                    if (p == '.' && isdigit(peekNext())) { advance(); currentState = State::IN_NUMBER_FLOAT; }
                    else if (!isdigit(p)) return makeToken(TokenType::NUMBER_INT, transitions);
                    else {
                        advance();
                        transitions.push_back({stateToString(prevState), p, stateToString(currentState)});
                    }
                    break;
                    
                case State::IN_NUMBER_FLOAT:
                    if (!isdigit(p)) return makeToken(TokenType::NUMBER_FLOAT, transitions);
                    advance();
                    transitions.push_back({stateToString(prevState), p, stateToString(currentState)});
                    break;

                case State::IN_HEX_NUMBER:
                    if (!isxdigit(p)) {
                        if (current - start <= 2) return makeToken(TokenType::UNKNOWN, transitions);
                        return makeToken(TokenType::NUMBER_HEX, transitions);
                    }
                    advance();
                    transitions.push_back({stateToString(prevState), p, stateToString(currentState)});
                    break;

                case State::SAW_AT:
                    if (p == '@') { advance(); currentState = State::SAW_DOUBLE_AT; }
                    else if (isalpha(p) || p == '_') { advance(); currentState = State::IN_INSTANCE_VAR; }
                    else return makeToken(TokenType::OPERATOR, transitions);
                    transitions.push_back({stateToString(prevState), p, stateToString(currentState)});
                    break;
                
                case State::SAW_DOUBLE_AT:
                    if (isalpha(p) || p == '_') { advance(); currentState = State::IN_CLASS_VAR; }
                    else return makeToken(TokenType::UNKNOWN, transitions);
                    transitions.push_back({stateToString(prevState), p, stateToString(currentState)});
                    break;

                case State::SAW_DOLLAR:
                    if (isalpha(p) || p == '_') { advance(); currentState = State::IN_GLOBAL_VAR; }
                    else return makeToken(TokenType::OPERATOR, transitions);
                    transitions.push_back({stateToString(prevState), p, stateToString(currentState)});
                    break;

                case State::SAW_COLON:
                    if (isalpha(p) || p == '_') { advance(); currentState = State::IN_SYMBOL; }
                    else return makeToken(TokenType::OPERATOR, transitions);
                    transitions.push_back({stateToString(prevState), p, stateToString(currentState)});
                    break;
                
                case State::IN_COMMENT:
                    if (p == '\n' || p == '\0') return makeToken(TokenType::COMMENT, transitions);
                    advance();
                    transitions.push_back({stateToString(prevState), p, stateToString(currentState)});
                    break;
                
                case State::IN_STRING:
                    if (p == '"') {
                        advance();
                        transitions.push_back({stateToString(prevState), p, "STRING_LITERAL"});
                        return makeToken(TokenType::STRING_LITERAL, transitions);
                    }
                    else if (p == '\0') return makeToken(TokenType::UNKNOWN, transitions);
                    advance();
                    transitions.push_back({stateToString(prevState), p, stateToString(currentState)});
                    break;
                
                case State::SAW_DOT:
                    if (p == '.') { advance(); currentState = State::IN_RANGE; }
                    else return makeToken(TokenType::OPERATOR, transitions);
                    transitions.push_back({stateToString(prevState), p, stateToString(currentState)});
                    break;

                case State::IN_RANGE:
                    if (p == '.') {
                        advance();
                        transitions.push_back({stateToString(prevState), p, "RANGE_EXCLUSIVE"});
                        return makeToken(TokenType::RANGE_EXCLUSIVE, transitions);
                    }
                    else {
                        transitions.push_back({stateToString(prevState), p, "RANGE_INCLUSIVE"});
                        return makeToken(TokenType::RANGE_INCLUSIVE, transitions);
                    }
                    break;

                case State::SAW_EQUALS:
                    if (p == '=' || p == '>') { advance(); }
                    transitions.push_back({stateToString(prevState), p, "OPERATOR"});
                    return makeToken(TokenType::OPERATOR, transitions);

                case State::SAW_PLUS:
                    if (p == '=') { advance(); }
                    transitions.push_back({stateToString(prevState), p, "OPERATOR"});
                    return makeToken(TokenType::OPERATOR, transitions);

                case State::SAW_MINUS:
                    if (p == '=') { advance(); }
                    transitions.push_back({stateToString(prevState), p, "OPERATOR"});
                    return makeToken(TokenType::OPERATOR, transitions);

                case State::SAW_STAR:
                    if (p == '=') { advance(); }
                    transitions.push_back({stateToString(prevState), p, "OPERATOR"});
                    return makeToken(TokenType::OPERATOR, transitions);

                case State::SAW_SLASH:
                    if (p == '=') { advance(); }
                    transitions.push_back({stateToString(prevState), p, "OPERATOR"});
                    return makeToken(TokenType::OPERATOR, transitions);
                
                case State::SAW_PIPE:
                    if (p == '|') { advance(); }
                    transitions.push_back({stateToString(prevState), p, "OPERATOR"});
                    return makeToken(TokenType::OPERATOR, transitions);
                default:
                    return makeToken(TokenType::UNKNOWN, transitions);
            }
        }
    }

    Token makeIdentifierTokenByType(State s, const std::vector<Transition>& transitions) {
        switch (s) {
            case State::IN_IDENTIFIER_LOCAL: return makeIdentifierToken(transitions);
            case State::IN_CONSTANT: return makeToken(TokenType::CONSTANT, transitions);
            case State::IN_INSTANCE_VAR: return makeToken(TokenType::IDENTIFIER_INSTANCE, transitions);
            case State::IN_CLASS_VAR: return makeToken(TokenType::IDENTIFIER_CLASS, transitions);
            case State::IN_GLOBAL_VAR: return makeToken(TokenType::IDENTIFIER_GLOBAL, transitions);
            case State::IN_SYMBOL: return makeToken(TokenType::SYMBOL, transitions);
            default: return makeToken(TokenType::UNKNOWN, transitions);
        }
    }
};

void printTokens(const std::vector<Token>& tokens) {
    for (const auto& token : tokens) {;
        std::cout << "Line " << token.line << ":\t"
                  << "< " << token.lexeme << " >"
                  << "\t -> " << tokenTypeToString(token.type)
                  << std::endl;
        if (!token.transitions.empty()) {
            std::cout << "  Transitions:" << std::endl;
            for (const auto& t : token.transitions) {
                std::cout << "    " << t.fromState << " --'" << t.character << "'--> " << t.toState << std::endl;
            }
        }
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
