/*
_ _    _           ______   _______        
| |  | |    /\    /___  /   |  ____|| |    
| |__| |   /  \      / /    | |__   | |       Hazel - The Fast, Expressive * Elegant Programming Language
|  __  |  / /\ \    / /     |  __|  | |       Languages: C, C++, and Assembly
| |  | | / ____ \  / /___   | |____ | |____   https://github.com/HazelLang/Hazel/
|_|_ |_|/_/    \_\/_______\ |______|_\______|

Licensed under the MIT License <http://opensource.org/licenses/MIT>
SPDX-License-Identifier: MIT
Copyright (c) 2021 Jason Dsouza <http://github.com/jasmcaus>
*/

#include <Hazel/Compiler/Lexer/Lexer.h>

/*
    Lexer:
    In lexer phase, the source code is decomposed into its simplest "tokens". 

    Newlines are converted to newline tokens
*/

/*
 * INSPIRATION: https://github.com/JHG777000/marshmallow/blob/master/src/marshmallow_parser.c
 * INSPIRATION: https://github.com/k-mrm/maxc/blob/master/src/compiler/lexer.c

*/

// 
// ================ Lexer Stuff ================
// 
// Constructor 
Lexer* lexer_init(const char* buffer) {
    Lexer* lexer = calloc(1, sizeof(Lexer));
    lexer->buffer__ = buffer; 
    lexer->buffer_capacity__ = strlen(buffer);
    lexer->offset__ = 0;
    LEXER_LOCATION_INIT;
    return lexer;
}

// Returns the current character in the Lexical Buffer and advances to the next element.
// It does this by incrementing the buffer offset.
inline char lexer_next(Lexer* lexer) {
    LEXER_INCREMENT_COLNO;
    return (char)lexer->buffer__[lexer->offset__++];
}

// Returns the previous `n` elements in the Lexical buffer.
// This is non-destructive -- the buffer offset is not updated.
static inline char lexer_prev(Lexer* lexer, UInt32 n) {
    if(lexer->offset__ - n >= 0) {
        return (char)lexer->buffer__[lexer->offset__ - n];
    } else  {
        return 0; // TOK_ILLEGAL
    }
}

// "Look ahead" `n` characters in the Lexical buffer.
// It _does not_ increment the buffer offset.
inline char lexer_peek(Lexer* lexer, UInt32 n) {
    if(lexer->offset__ + n <= lexer->buffer_capacity__) {
        return (char)lexer->buffer__[lexer->offset__ + n];
    } else {
        return 0; // corresponds to TOK_ILLEGAL
    }
}

// lexer_peek_curr() returns the current element in the Lexical Buffer
inline char lexer_peek_curr(Lexer* lexer) {
    return (char)lexer->buffer__[lexer->offset__];
}

// 
// ================ Lexer Location ================
// 
inline void lexer_set_lineno(Lexer* lexer, UInt32 lineno) { lexer->location__.lineno__ = lineno; }
inline void lexer_set_colno(Lexer* lexer, UInt32 colno) { lexer->location__.colno__ = colno; }
inline void lexer_set_fname(Lexer* lexer, const char* fname) { lexer->location__.fname__ = fname; }


// 
// ================ Useful Functions used by the Lexer ================
// 
// Set token
inline void lexer_set_token(Lexer* lexer, Token token) { lexer->token__ = token; }
// Set token type
inline void lexer_set_token_type(Lexer* lexer, TokenType tok_type) { lexer->token__.type__ = tok_type; }
// Set token value 
inline void lexer_set_token_value(Lexer* lexer, const char* value) { lexer->token__.value__ = value; }
// Set token bytes 
inline void lexer_set_token_bytes(Lexer* lexer, UInt32 bytes) { lexer->token__.tok_bytes__ = bytes; }

// Check if the current Lexer state is at EOF
inline bool lexer_is_EOF(Lexer* lexer) { return lexer->offset__ >= lexer->buffer_capacity__; }
// Extract a Token 
inline Token lexer_extract_token(Lexer* lexer) { return lexer->token__; }

// Finalize a Token
inline void lexer_finalize_token(Lexer* lexer, TokenType __tok) {
	lexer->token__.type__ = __tok; 
	lexer->token__.location__.fname__ = lexer->location__.fname__;
}

static inline bool isNewLine(Lexer* lexer, char c) {
    // Carriage Return: U+000D (UTF-8 in hex: 0D)
    // Line Feed: U+000A (UTF-8 in hex: 0A)
    // CR+LF: CR (U+000D) followed by LF (U+000A) (UTF-8 in hex: 0D0A)
    // UTF-8 cases https://en.wikipedia.org/wiki/Newline#Unicode:
    //      1. Next Line, U+0085 (UTF-8 in hex: C285) or (194)
    //      2. Line Separator, U+2028 (UTF-8 in hex: E280A8)

    // Line Feed 
    if(c == '\n') return true; 

    // CR+LF or CR
    if(c == '\r') {
        if(lexer_peek_curr(lexer) == '\n') { lexer_next(lexer); }
        return true; 
    }

    // // Next Line
    // if((c == 194) && (lexer_peek_curr(lexer) == 133)) {
    //     lexer_next(lexer); 
    //     return true;
    // }
    
    // // Line Separator
    // if((c == 226) && (lexer_peek_curr(lexer) == 128) && 168) {
    //     lexer_next(lexer); 
    //     lexer_next(lexer); 
    //     return true; 
    // }

    // will add more at some point in the future 
    return false; 
}

static inline bool isComment(char c1, char c2) { return isSlashComment(c1, c2) || isHashComment(c1) || isHashComment(c2); }
static inline bool isSlashComment(char c1, char c2) { return (c1 == '/' && (c2 == '*' || c2 == '/')); }
static inline bool isHashComment(char c) { return c == '#';}
static inline bool isSemicolon(char c) { return c == ';'; }
static inline bool isString(char c) { return (c == '"' || c == '\'');}
// Returns true if [c] is the beginning of a Macro (In Hazel, macros begin with the `@` sign)
static inline bool isMacro(char c) { return c == '@';}
// Returns true if [c] is a valid (non-initial) identifier
static inline bool isIdentifier(char c) { return isAlpha(c) || isDigit(c) || c == '_'; }

static inline bool isBuiltinOperator(char c) {
    // Parenthesis
    // { } [ ] ( )
    // Punctuation
    // . ; : ? ,
    // Operators
    // + - * / < > ! = | * ^ % ~
    return ((c == '+') || (c == '-') || (c == '*') || (c == '/') || (c == '<') || (c == '>') || (c == '!') || 
            (c == '=') || (c == '|') || (c == '*') || (c == '^') || (c == '%') || (c == '~') || (c == '.') || 
            (c == ';') || (c == ':') || (c == '?') || (c == ',') || (c == '{') || (c == '}') || (c == '[') || 
            (c == ']') || (c == '(') || (c == ')') );
}

// Lexing Errors
TokenType lexer_error(Lexer* lexer, const char* message) {
    if(!lexer_is_EOF(lexer)) {
        LEXER_INCREMENT_TOK_LENGTH;
        LEXER_INCREMENT_OFFSET;
    }
    lexer_finalize_token(lexer, TOK_ILLEGAL);
    lexer_set_token_value(lexer, message);
    lexer_set_token_bytes(lexer, strlen(message));
    return TOK_ILLEGAL; 
}

TokenType lexer_lex_comment(Lexer* lexer) {
    // # Hello 
    bool isLineComment = lexer_peek(lexer, 1) == '#';

    LEXER_RESET_TOKEN;
    LEXER_INCREMENT_OFFSET;

    char c = lexer_next(lexer);

    while(!lexer_is_EOF(lexer)) {
        if(isLineComment) {
            if(isNewLine(lexer, c)) {
                printf("%c", c);
                LEXER_INCREMENT_LINENO;
                break;
            }
        }
    }

}

TokenType lexer_lex_operator(Lexer* lexer) {
    LEXER_RESET_TOKEN;
    LEXER_INCREMENT_TOK_LENGTH;

    // Do not change the declaration order of _next_ and _curr_
    char next = lexer_next(lexer); 
    char curr = lexer_peek_curr(lexer); 
    TokenType token = TOK_ILLEGAL; 

    switch(next) {
        // '='
        case '=':
            // '=='
            if(curr == '=') {
                LEXER_INCREMENT_OFFSET;
                LEXER_INCREMENT_TOK_LENGTH;

                // 
                // Uncomment the following ONLY if Hazel ends up supporting '==='
                // curr = lexer_peek_curr(lexer); 
                // // '===' 
                // if(curr == '=') {
                //     INCREMENT_OFFSET;
                //     INCREMENT_TOKENLENGTH;
                //     token = EQUALS_EQUALS_EQUALS;
                // } else {
                //     token = EQUALS_EQUALS;
                // }

                token = EQUALS_EQUALS; 
            } 
            
            // '=>'
            if(curr == '>') {
                LEXER_INCREMENT_OFFSET;
                LEXER_INCREMENT_TOK_LENGTH;
                token = EQUALS_ARROW;
            } else {
                token = EQUALS;
            }
            break; 
        
        // '+'
        case '+':
            // The following might be removed at some point. 
            // '++' serves no purpose for us since we don't support pointer arithmetic
            // 
            // '++'
            if(curr == '+') {
                LEXER_INCREMENT_OFFSET;
                LEXER_INCREMENT_TOK_LENGTH;
                token = PLUS_PLUS;
            }

            // '+='
            if(curr == '=') {
                LEXER_INCREMENT_OFFSET; 
                LEXER_INCREMENT_TOK_LENGTH;

                token = PLUS_EQUALS;
            } else {
                token = PLUS;
            }
            break; 
        
        // '-'
        case '-':
            // The following might be removed at some point. 
            // '--' serves no purpose for us since we don't support pointer arithmetic
            // 
            // '--'
            if(curr == '-') {
                LEXER_INCREMENT_OFFSET;
                LEXER_INCREMENT_TOK_LENGTH;
                token = MINUS_MINUS;
            }

            // '->'
            if(curr == '>') {
                LEXER_INCREMENT_OFFSET;
                LEXER_INCREMENT_TOK_LENGTH;
                token = RARROW;
            }

            // '-='
            if(curr == '=') {
                LEXER_INCREMENT_OFFSET; 
                LEXER_INCREMENT_TOK_LENGTH;

                token = MINUS_EQUALS;
            } else {
                token = MINUS;
            }
            break; 

        // '*'
        case '*':
            // '**'
            if(curr == '*') {
                LEXER_INCREMENT_OFFSET;
                LEXER_INCREMENT_TOK_LENGTH;
                token = MULT_MULT;
            }

            // '*='
            if(curr == '=') {
                LEXER_INCREMENT_OFFSET; 
                LEXER_INCREMENT_TOK_LENGTH;

                token = MULT_EQUALS;
            } else {
                token = MULT;
            }
            break;
        
        // '/'
        case '/':
            // '//'
            if(curr == '/') {
                LEXER_INCREMENT_OFFSET;
                LEXER_INCREMENT_TOK_LENGTH;
                token = SLASH_SLASH;
            }

            // '/='
            if(curr == '=') {
                LEXER_INCREMENT_OFFSET; 
                LEXER_INCREMENT_TOK_LENGTH;

                token = SLASH_EQUALS;
            } else {
                token = SLASH;
            }
            break;
        
        // '!'
        case '!':
            // '!='
            if(curr == '=') {
                LEXER_INCREMENT_OFFSET; 
                LEXER_INCREMENT_TOK_LENGTH;

                token = EXCLAMATION_EQUALS;
            } else {
                token = EXCLAMATION;
            }
            break;
        
        // '%'
        case '%':
            // '%%'
            if(curr == '%') {
                LEXER_INCREMENT_OFFSET;
                LEXER_INCREMENT_TOK_LENGTH;
                token = MOD_MOD;
            }

            // '%='
            if(curr == '=') {
                LEXER_INCREMENT_OFFSET; 
                LEXER_INCREMENT_TOK_LENGTH;

                token = MOD_EQUALS;
            } else {
                token = MOD;
            }
            break;
        
        // '&'
        case '&':
            // '&&'
            if(curr == '&') {
                LEXER_INCREMENT_OFFSET;
                LEXER_INCREMENT_TOK_LENGTH;
                token = AND_AND;
            }

            // '&^'
            if(curr == '^') {
                LEXER_INCREMENT_OFFSET;
                LEXER_INCREMENT_TOK_LENGTH;
                token = AND_NOT;
            }

            // '&='
            if(curr == '=') {
                LEXER_INCREMENT_OFFSET; 
                LEXER_INCREMENT_TOK_LENGTH;

                token = AND_EQUALS;
            } else {
                token = AND;
            }
            break;
        
        // '|'
        case '|':
            // '||'
            if(curr == '|') {
                LEXER_INCREMENT_OFFSET;
                LEXER_INCREMENT_TOK_LENGTH;
                token = OR_OR;
            }

            // '|='
            if(curr == '=') {
                LEXER_INCREMENT_OFFSET; 
                LEXER_INCREMENT_TOK_LENGTH;

                token = OR_EQUALS;
            } else {
                token = OR;
            }
            break;
        
        // '^'
        case '^':
            // '^='
            if(curr == '=') {
                LEXER_INCREMENT_OFFSET; 
                LEXER_INCREMENT_TOK_LENGTH;

                token = XOR_EQUALS;
            } else {
                token = XOR;
            }
            break;
        
        // '?'
        case '?':
            token = QUESTION;
            break;

        // '<'
        case '<':
            // '<='
            if(curr == '=') {
                LEXER_INCREMENT_OFFSET; 
                LEXER_INCREMENT_TOK_LENGTH;

                token = LESS_THAN_OR_EQUAL_TO;
            }

            // '<-'
            else if(curr == '-') {
                LEXER_INCREMENT_OFFSET; 
                LEXER_INCREMENT_TOK_LENGTH;

                token = LARROW;
            }

            // '<<'
            else if(curr == '<') {
                LEXER_INCREMENT_OFFSET;
                LEXER_INCREMENT_TOK_LENGTH;

                // '<<='
                curr = lexer_peek_curr(lexer);
                if(curr == '=') {
                    LEXER_INCREMENT_OFFSET;
                    LEXER_INCREMENT_TOK_LENGTH;
                    token = LBITSHIFT_EQUALS;
                } else {
                    token = LBITSHIFT;
                }
            } else {
                token = LESS_THAN;
            }
            break;
        
        // '>'
        case '>':
            // '>='
            if(curr == '=') {
                LEXER_INCREMENT_OFFSET; 
                LEXER_INCREMENT_TOK_LENGTH;

                token = GREATER_THAN_OR_EQUAL_TO;
            } 

            // '>>'
            else if(curr == '>') {
                LEXER_INCREMENT_OFFSET;
                LEXER_INCREMENT_TOK_LENGTH;

                // '>>='
                curr = lexer_peek_curr(lexer);
                if(curr == '=') {
                    LEXER_INCREMENT_OFFSET;
                    LEXER_INCREMENT_TOK_LENGTH;
                    token = RBITSHIFT_EQUALS;
                } else {
                    token = RBITSHIFT;
                }
            } else {
                token = GREATER_THAN;
            }
            break;  
        
        // '~'
        case '~':
            // '~='
            if(curr == '=') {
                LEXER_INCREMENT_OFFSET; 
                LEXER_INCREMENT_TOK_LENGTH;

                token = TILDA_EQUALS;
            } else {
                token = TILDA;
            }
            break;
        
        default: 
            // printf("LexerError -> Unrecognized Token (%c)", next);
            printf("LexerError -> Unrecognized Token");
            abort();
    }

    lexer_finalize_token(lexer, token);
    // LEXER_DEBUG("Found operator: %s", token_toString(token));
    return token; 
} 


TokenType lexer_lex_separator(Lexer* lexer) {
    LEXER_RESET_TOKEN;
    LEXER_INCREMENT_TOK_LENGTH;

    // Do not change the declaration order of _next_ and _curr_
    char next = lexer_next(lexer); 
    char curr = lexer_peek_curr(lexer); 
    TokenType token = TOK_ILLEGAL; 

    switch(next) {
        // '.'
        case '.':
            // '..'
            if(curr == '.') {
                LEXER_INCREMENT_OFFSET;
                LEXER_INCREMENT_TOK_LENGTH;

                // '...'
                curr = lexer_peek_curr(lexer);
                if(curr == '.') {
                    LEXER_INCREMENT_OFFSET;
                    LEXER_INCREMENT_TOK_LENGTH;
                    token = ELLIPSIS;
                } else {
                    token = DDOT;
                }
            } else {
                token = DOT;
            }
            break;  
    
        case ':': token = COLON; break; 
        case ';': token = SEMICOLON; break; 
        case ',': token = COMMA; break; 
        case '\\': token = BACKSLASH; break; 
        default: 
            printf("LexerError -> Unrecognized Token");
            abort();
    }

    lexer_finalize_token(lexer, token);
    // LEXER_DEBUG("Found separator: %s", token_toString(token));
    return token; 
}


TokenType lexer_lex_delimiter(Lexer* lexer) {
    LEXER_RESET_TOKEN;
    LEXER_INCREMENT_TOK_LENGTH;

    // Do not change the declaration order of _next_ and _curr_
    char next = lexer_next(lexer); 
    char curr = lexer_peek_curr(lexer); 
    TokenType token = TOK_ILLEGAL; 

    switch(next) {       
        case '[': token = LSQUAREBRACK; break; 
        case ';': token = RSQUAREBRACK; break; 
        case '{': token = LBRACE; break; 
        case '}': token = RBRACE; break; 
        case '(': token = LPAREN; break; 
        case ')': token = RPAREN; break; 
        default:  
            printf("LexerError -> Unrecognized Token"); 
            abort();
    }
        
    lexer_finalize_token(lexer, token);
    // LEXER_DEBUG("Found delimiter: %s", token_toString(token));
    return token; 
}


TokenType lexer_lex_macro(Lexer* lexer) {
    LEXER_RESET_TOKEN;
    LEXER_INCREMENT_TOK_LENGTH;
    TokenType token = TOK_ILLEGAL;

    char curr = lexer_peek_curr(lexer); 
    if(curr == '@')
        token = MACRO;

    lexer_finalize_token(lexer, token);
    return token;
}
