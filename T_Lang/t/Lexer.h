#pragma once

#include "common.h"

#include <set>
#include <unordered_map>

namespace t
{
    namespace lexer
    {
        class TokenType
        {
        public:
            enum Type : uint8_t
            {
                // Binary Operators

                // '='
                Equals,
                // '=='
                EqualsEquals,
                // '!='
                NotEquals,
                // '>'
                GreaterThan,
                // '<'
                LessThan,
                // '<<'
                ShiftLeft,
                // '>>'
                ShiftRight,
                // '+'
                Plus,
                // '-'
                Minus,
                // '/'
                Divide,
                // '*'
                Multiply,
                // '**'
                Exponent,
                // '%'
                Modulus,
                // '&'
                AND,
                // '&&'
                ANDAND,
                // '|'
                OR,
                // '||'
                OROR,
                // '.'
                Dot,
                // '::'
                ColonColon,

                // Unary Operators

                // '--'
                MinusMinus,
                // '!'
                Not,
                // '++'
                PlusPlus,

                // Other

                // '->'
                Pointer,
                // '~'
                Reference, 
                // "..."
                string_literal,
                // '...'
                char_literal,
                // 'true' | 'false'
                bool_literal,
                // ';'
                Semicolon,
                // ':'
                Colon,
                // ','
                Comma,
                // < 0-9 >:end
                integer_literal,
                // < (-0)-(-9) >:end
                negative_integer_literal,
                // < 0-9 >:end . <0-9>:end
                float_literal,
                // < function name, variable name >
                Identifier,
                // < for, while, bool, auto, ... >
                KeyWord,
                for_,
                while_,
                public_,
                private_,
                protected_,
                cast_,
                return_,
                null_,
                in_,
                if_,
                constexpr_,
                namespace_,

                // '('
                OParen,
                // ')'
                CParen,
                // '{'
                OCurlyBrace,
                // '}'
                CCurlyBrace,
                // 'mutable'
                mutable_,

                class_,

                ClassType,

                PrimitiveType,

                EOF_
            };
            constexpr TokenType( Type type ):
                m_type( type ) {}
            constexpr TokenType(): m_type( EOF_ ) {};
            bool operator==( const TokenType rhs ) const { return m_type == rhs.m_type; }
            bool operator!=( const TokenType rhs ) const { return !( *this == rhs ); }
            bool operator<( const TokenType rhs ) const { return m_type < rhs.m_type; }
            bool operator>( const TokenType rhs ) const { return m_type > rhs.m_type; }
            bool operator<=( const TokenType rhs ) const { return m_type <= rhs.m_type; }
            bool operator>=( const TokenType rhs ) const { return m_type >= rhs.m_type; }

            bool operator==( const Type rhs ) const { return m_type == rhs; }
            bool operator!=( const Type rhs ) const { return !( this->m_type == rhs ); }
            bool operator<( const Type rhs ) const { return m_type < rhs; }
            bool operator>( const Type rhs ) const { return m_type > rhs; }
            bool operator<=( const Type rhs ) const { return m_type <= rhs; }
            bool operator>=( const Type rhs ) const { return m_type >= rhs; }

            constexpr operator Type() const { return m_type; }

            explicit operator bool() const = delete;

            bool isBinaryOperator() const { return m_type < MinusMinus; }
            bool isUnaryOperator() const { return m_type <= PlusPlus && m_type >= MinusMinus; }
            bool isAccessSpecifier() const { return m_type == public_ || m_type == protected_ || m_type == private_; }
        private:
            Type m_type;
        };

        const std::unordered_map< std::string, TokenType > KEYWORDS
        {
            // Class words
            {"class", TokenType::class_}, {"private", TokenType::private_}, {"public", TokenType::public_}, {"protected", TokenType::protected_},
            // Generic
            {"mutable", TokenType::mutable_},
            {"cast", TokenType::cast_},
            {"return", TokenType::return_},
            {"for", TokenType::for_}, {"while", TokenType::while_}, {"in", TokenType::in_}, {"if", TokenType::if_},
            {"null", TokenType::null_},
            {"namespace", TokenType::namespace_},
        };

        const std::set< std::string > DEFAULT_TYPES
        {
            "auto",
            "char",
            "int8", "int16", "int32", "int64",
            "uint8", "uint16", "uint32", "uint64",
            "float", "double", "bool", "String",
            "void"
        };

        struct Token
        {
            Token( std::string&& value, TokenType type ):
                value( value ), type( type ) {}
            std::string value;
            TokenType type;
            bool isMultParseLevel() const { return type == TokenType::Multiply || type == TokenType::Divide || type == TokenType::Modulus; }
            bool isDefaultType() const { return DEFAULT_TYPES.find( value ) != DEFAULT_TYPES.cend(); }
            bool isRefOrPtr() const { return type == TokenType::Reference || type == TokenType::Pointer; }
            bool isBooleanOperator() const { return type == TokenType::EqualsEquals || type == TokenType::NotEquals; }
        };
    }

    using TokenList = std::vector< lexer::Token >;

    std::set< std::string > user_defined_classnames;

    class Lexer
    {
    public:
        using TokenType = lexer::TokenType;
        Lexer( const std::string& text ):
            srctext( text ), LENGTH( srctext.length() ) {}
        Lexer( std::string&& text ):
            srctext( std::move( text ) ) {}
        TokenList tokenize()
        {
            LENGTH = srctext.length();
            
            for ( ; i < LENGTH; ++i )
            {
                if ( isSkippable() )
                    continue;
                switch ( srctext[ i ] )
                {
                case ';':
                    handleSingleCharacter( TokenType::Semicolon );
                    continue;
                case ',':
                    handleSingleCharacter( TokenType::Comma );
                    continue;
                case '(':
                    handleSingleCharacter( TokenType::OParen );
                    continue;
                case ')':
                    handleSingleCharacter( TokenType::CParen );
                    continue;
                case '{':
                    handleSingleCharacter( TokenType::OCurlyBrace );
                    continue;
                case '}':
                    handleSingleCharacter( TokenType::CCurlyBrace );
                    continue;
                case '<':
                {
                    nextCharacterIsSame() ? handleDoubleCharacter( TokenType::ShiftLeft ) : handleSingleCharacter( TokenType::LessThan );
                    continue;
                }
                case '>':
                {
                    nextCharacterIsSame() ? handleDoubleCharacter( TokenType::ShiftRight ) : handleSingleCharacter( TokenType::GreaterThan );
                    continue;
                }
                case '+':
                    nextCharacterIsSame() ? handleDoubleCharacter( TokenType::PlusPlus ) : handleSingleCharacter( TokenType::Plus );
                    continue;
                case '~':
                    handleSingleCharacter( TokenType::Reference );
                    continue;
                case '-':
                    if ( nextCharacterIs( '>' ) )
                    {
                        handleDoubleCharacter( TokenType::Pointer );
                        continue;
                    }
                    if ( lastType.isBinaryOperator() || lastType == TokenType::Equals || lastType == TokenType::OParen || lastType == TokenType::Comma )
                    {
                        i++;
                        buildNumber< true >();
                        continue;
                    }
                    nextCharacterIsSame() ? handleDoubleCharacter( TokenType::MinusMinus ) : handleSingleCharacter( TokenType::Minus );
                    continue;
                case '*':
                {
                    nextCharacterIsSame() ? handleDoubleCharacter( TokenType::Exponent ) : handleSingleCharacter( TokenType::Multiply );
                    continue;
                }
                case '/':
                    if ( nextCharacterIsSame() )
                    {
                        auto c = srctext[ i ];
                        while ( i < LENGTH && c != '\n' )
                        {
                            c = srctext[ ++i ];
                        }
                        continue;
                    }
                    handleSingleCharacter( TokenType::Divide );
                    continue;
                case '%':
                    handleSingleCharacter( TokenType::Modulus );
                    continue;
                case ':':
                {
                    nextCharacterIsSame() ? handleDoubleCharacter( TokenType::ColonColon ) : handleSingleCharacter( TokenType::Colon );
                    continue;
                }
                case '&':
                {
                    nextCharacterIsSame() ? handleDoubleCharacter( TokenType::ANDAND ) : handleSingleCharacter( TokenType::AND );
                    continue;
                }
                case '|':
                {
                    nextCharacterIsSame() ? handleDoubleCharacter( TokenType::OROR ) : handleSingleCharacter( TokenType::OR );
                    continue;
                }
                case '=':
                {
                    nextCharacterIsSame() ? handleDoubleCharacter( TokenType::EqualsEquals ) : handleSingleCharacter( TokenType::Equals );
                    continue;
                }
                case '!':
                    if ( nextCharacterIs( '=' ) )
                    {
                        handleDoubleCharacter( TokenType::NotEquals );
                        continue;
                    }
                    handleSingleCharacter( TokenType::Not );
                    continue;
                case '.':
                {
                    if ( i+1 < LENGTH )
                    {
                        i++;
                        if ( isInt() )
                        {
                            i--;
                            buildNumber();
                            continue;
                        }
                        i--;
                    }
                    handleSingleCharacter( TokenType::Dot );
                    continue;
                }
                case '\"':
                {
                    buildString();
                    continue;
                }
                case '\'':
                    buildChar();
                    continue;
                default:
                {
                    if ( isInt() )
                    {
                        buildNumber();
                        continue;
                    }
                    if ( isAlpha() )
                    {
                        buildIdentifier();
                        continue;
                    }
                    std::cout << "Unrecognized character found in source: " + srctext[ i ] << '\n';
                    return TokenList();
                }
                }               
            }
            tokens.push_back( lexer::Token( "", TokenType::EOF_ ) );
            return tokens;
        }
    private:
        TokenType lastType;
        void handleDoubleCharacter( lexer::TokenType type )
        {
            tokens.push_back( lexer::Token( srctext.substr( i++, 2 ), lastType = type ) );
        }
        void handleSingleCharacter( lexer::TokenType type )
        {
            tokens.push_back( lexer::Token( srctext.substr( i, 1 ), lastType = type ) );
        }
        void buildString()
        {
            std::string str = "";

            if ( i < LENGTH && srctext[ i+1 ] == '\"' )
            {
                tokens.push_back( lexer::Token( "", lastType = TokenType::string_literal ) );
                return;
            }

            while ( ++i < LENGTH && srctext[ i ] != '\"' )
            {
                const auto c = srctext[ i ];
                if ( c == '\n' || c == '\r' )
                    throw std::runtime_error( "invalid string literal!" );
                str += c;
            }
            if ( i < LENGTH )
            {
                if ( srctext[ i ] != '\"' )
                    throw std::runtime_error( "invalid string literal!" );
            }
            
            tokens.push_back( lexer::Token( std::move( str ), lastType = TokenType::string_literal ) );
        }
        void buildChar()
        {
            std::string ch = srctext.substr( ++i, 1 );
            if ( srctext[ i ] == '\\' )
            {
                ch += srctext[ i ];
            }
            i++;
            tokens.push_back( lexer::Token( std::move( ch ), lastType = TokenType::char_literal ) );
        }
        template< bool isNegative = false >
        void buildNumber()
        {
            std::string num = isNegative ? "-" : "";
            while ( i < LENGTH && isInt() )
                num += srctext[ i++ ];

            if ( i < LENGTH && srctext[ i ] != '.' )
            {
                if constexpr ( isNegative )
                {
                    tokens.push_back( lexer::Token( std::move( num ), lastType = TokenType::negative_integer_literal ) );
                }
                else
                {
                    tokens.push_back( lexer::Token( std::move( num ), lastType = TokenType::integer_literal ) );
                }
                i--;
                return;
            }

            num += srctext[ i++ ];

            while ( i < LENGTH && isInt() )
            {
                num += srctext[ i++ ];
            }
            i--;
            tokens.push_back( lexer::Token( std::move( num ), lastType = TokenType::float_literal ) );
        }
        std::string parseIdentifier()
        {
            std::string id = "";
            while ( i < LENGTH && ( isInt() || isAlpha() || srctext[i] == '_' ) )
                id += srctext[i++];
            i--;
            return id;
        }
        void buildIdentifier()
        {
            auto id { parseIdentifier() };

            if ( id == "false" || id == "true" )
            {
                tokens.push_back( lexer::Token( std::move( id ), lastType = TokenType::bool_literal ) );
                return;
            }

            if ( isKeyWord( id ) )
            {
                lastType = lexer::KEYWORDS.at( id );

                tokens.push_back( lexer::Token( std::move( id ), lastType ) );
                return;
            }

            if ( isDefaultType( id ) )
            {
                if ( id == "String" )
                {
                    tokens.push_back( lexer::Token( std::move( id ), lastType = TokenType::ClassType ) );
                    return;
                }
                tokens.push_back( lexer::Token( std::move( id ), lastType = TokenType::PrimitiveType ) );
                return;
            }

            if ( lastType == TokenType::class_ )
            {
                user_defined_classnames.insert( id );
                goto PushBackToken;
            }

            if ( isUserDefinedClass( id ) )
            {
                PushBackToken:
                tokens.push_back( lexer::Token( std::move( id ), lastType = TokenType::ClassType ) );
                return;
            }

            tokens.push_back( lexer::Token( std::move( id ), lastType = TokenType::Identifier ) );
        }

        bool isKeyWord( const std::string& str ) const { return lexer::KEYWORDS.find( str ) != lexer::KEYWORDS.cend(); }
        bool isDefaultType( const std::string& str ) const { return lexer::DEFAULT_TYPES.find( str ) != lexer::DEFAULT_TYPES.cend(); }
        bool isUserDefinedClass( const std::string& str ) const { return user_defined_classnames.find( str ) != user_defined_classnames.cend(); }

        std::string srctext;
        TokenList tokens;
        size_t i = 0;
        size_t LENGTH = 0;

        inline bool nextCharacterIsSame() { return srctext.length() > i+1 && srctext[ i ] == srctext[ i+1 ]; }
        inline bool nextCharacterIs( char c ) { return srctext.length() > i+1 && srctext[ i+1 ] == c; }
        inline bool isSkippable() { return srctext[ i ] == ' ' || srctext[ i ] == '\t' || srctext[ i ] == '\n' || srctext[ i ] == '\r'; }
        inline bool isInt() { return srctext[ i ] >= '0' && srctext[ i ] <= '9'; }
        inline bool isAlpha() { return toupper( srctext[ i ] ) != tolower( srctext[ i ] ); }
    };
}
