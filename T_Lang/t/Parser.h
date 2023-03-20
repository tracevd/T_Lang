#pragma once

#include <type_traits>

#include "AST.h"
#include "Lexer.h"

namespace t
{
    class Parser
    {
    public:
        Parser( const TokenList& tokens ):
            tokens( tokens ) {}
        Parser( std::vector< lexer::Token >&& tokens ):
            tokens( std::move( tokens ) ) {}
        ast::Program produceAST()
        {
            ast::StatementList body;
            while ( not_eof() )
                body.push_back( parseStatement() );

            return ast::Program( std::move( body ) );
        }
    private:
        std::pair< bool, bool > eatIfRefOrPtr()
        {
            const auto tkty = peek().type;
            if ( tkty == TokenType::Reference )
            {
                eat();
                return { true, false };
            }
            if ( tkty == TokenType::Pointer )
            {
                eat();
                return { false, true };
            }
            return { false, false };
        }

        bool eatIfMutable()
        {
            auto const isMutable { peek().type == TokenType::mutable_ };
            if ( isMutable )
                eat();
            return isMutable;
        }

        std::unique_ptr< ast::Expression > makeExpression( ast::Expression* expr )
        {
            return std::unique_ptr< ast::Expression >( expr );
        }
        using Token = lexer::Token;
        using TokenType = lexer::TokenType;
        TokenList tokens;
        size_t i = 0;

        Token peek() const { return tokens[ i ]; }
        
        Token eat()  { return tokens[ i++ ]; }

        Token peekNext() const { return tokens[ i+1 ]; }

        Token peekTo( size_t i ) const { if ( i >= tokens.size() ) return tokens.back(); return tokens[i]; }
        
        bool not_eof() const { return tokens[ i ].type != TokenType::EOF_; }

        Token expect( TokenType type, const std::string& err )
        {
            const auto tk = eat();
            if ( tk.type != type )
            {
                throw std::runtime_error("Unexpected token type\n" + err );
            }
            return tk;
        }

        Token expect( TokenType type1, TokenType type2, const std::string& err )
        {
            const auto tk = eat();
            if ( tk.type != type1 && tk.type != type2 )
            {
                throw std::runtime_error("Unexpected token type\n" + err );
            }
            return tk;
        }
        
        template< bool AllowDeclarations = true >
        ast::Statement parseStatement()
        {
            switch ( peek().type )
            {
            case TokenType::if_:
                return parseIfStatement();
            case TokenType::namespace_:
                if constexpr ( AllowDeclarations )
                    return parseNameSpaceDeclaration();
                else throw std::runtime_error( "cannot create namespace inside of if statement" );
            case TokenType::Identifier:
                return handleIdentifier();
            case TokenType::PrimitiveType:
            case TokenType::ClassType:
                return handleType();
            case TokenType::mutable_:
                return handleMutable();
            case TokenType::class_:
                if constexpr ( AllowDeclarations )
                    return parseClassDefinition();
                else throw std::runtime_error( "cannot create class inside of if statement" );
            default:
                return parseExpression().release();
            }
        }

#define NOT_VALID_IF_CONDITION !dynamic_cast<ast::BinaryExpression*>(condition.get()) &&\
    !dynamic_cast<ast::BoolLiteral*>(condition.get()) &&\
    !dynamic_cast<ast::NumericLiteral<int64_t>*>(condition.get()) &&\
    !dynamic_cast<ast::NumericLiteral<uint64_t>*>(condition.get()) &&\
    !dynamic_cast<ast::NumericLiteral<double>*>(condition.get())

        ast::Statement parseIfStatement()
        {
            eat();

            expect( TokenType::OParen, "expected opening paren to start if statement" );

            auto condition = parseExpression< false >();

            if ( NOT_VALID_IF_CONDITION )
            {
                throw std::runtime_error( "invalid if condition" );
            }

            expect( TokenType::CParen, "expected closing paren after condition" );

            ast::StatementList stmts;

            const auto multipleStatments = peek().type == TokenType::OCurlyBrace;

            if ( multipleStatments )
            {
                eat();

                while ( peek().type != TokenType::CCurlyBrace )
                {
                    stmts.push_back( parseExpression().release() );
                }

                expect( TokenType::CCurlyBrace, "expected closing brace of if statement body" );
            }
            else
            {
                stmts.push_back( parseStatement< false >() );
            }

            return new ast::IfStatement( std::move( condition ), std::move( stmts ) );
        }
#undef NOT_VALID_IF_CONDITION

        ast::Statement parseNameSpaceDeclaration()
        {
            eat();

            auto nsp_name { expect( TokenType::Identifier, "namespace must have a name" ).value };

            expect( TokenType::OCurlyBrace, "expected obening bracket to namespace declaration" );

            ast::StatementList stmtList;

            while ( peek().type != TokenType::CCurlyBrace )
            {
                stmtList.push_back( parseStatement() );
            }

            expect( TokenType::CCurlyBrace, "expected closing bracket to end namespace declaration" );

            return new ast::NameSpaceDeclaration( std::move( nsp_name ), std::move( stmtList ) );
        }

        ast::Statement parseClassDefinition()
        {
            eat();

            auto typestr { expect( TokenType::ClassType, "Class type must follow class keyword" ).value };

            expect( TokenType::OCurlyBrace, "expected opening bracket to start class definition" );

            ast::FieldList fields;

            ast::MethodList methods;

            ast::AccessSpecifier currentspec = ast::Public;

            while ( peek().type != TokenType::CCurlyBrace )
            {
                auto tk = peek();

                size_t idx = 0;

                if ( tk.type == TokenType::mutable_ )
                {
                    idx++;
                    tk = peekNext();
                }
                else if ( tk.type.isAccessSpecifier() )
                {
                    eat();
                    switch ( tk.type )
                    {
                    case TokenType::public_:
                        currentspec = ast::Public;
                        break;
                    case TokenType::protected_:
                        currentspec = ast::Protected;
                        break;
                    case TokenType::private_:
                        currentspec = ast::Private;
                        break;
                    }
                    expect( TokenType::Colon, "expected colon after access specifier" );
                    tk = peek();
                }
                if ( tk.type != TokenType::ClassType && tk.type != TokenType::PrimitiveType )
                {
                    throw std::runtime_error( "Inner class definition requires type name" );
                }
                auto tk2 = peekTo( i+1+idx );
                if ( tk2.isRefOrPtr() )
                {
                    idx++;
                    tk2 = peekTo( i + 1 + idx );
                }
                auto tk3 = peekTo( i+2+idx );
                if ( tk3.type == TokenType::OParen )
                {
                    auto stmt = parseFunctionDeclaration();
                    auto func = stmt.as< ast::FunctionDeclaration >();
                    methods.push_back( ast::MethodDeclaration( std::move( *func ), currentspec ) );
                }
                else
                {
                    auto stmt = parseVariableDeclaration();
                    auto var = stmt.as< ast::VariableDeclaration >();
                    fields.push_back( ast::FieldDeclaration( std::move( *var ), currentspec ) );
                }
            }

            expect( TokenType::CCurlyBrace, "expected closing bracket to class definition" );

            ast::TypeName type { std::move( typestr ) };

            return new ast::ClassDeclaration( std::move( type ), std::move( fields ), std::move( methods ) );
        }

        ast::Statement handleType()
        {            
            auto tk = peekNext();

            size_t idx = 0;

            if ( tk.isRefOrPtr() )
            {
                tk = peekTo( i + 2 );
                ++idx;
            }
            if ( tk.type != TokenType::Identifier )
            {
                throw std::runtime_error( "Identifier expected after type" );
            }
            const auto tk2 = peekTo( i + 2 + idx );

            if ( tk2.type == TokenType::Equals )
            {
                return parseVariableDeclaration();
            }
            return parseFunctionDeclaration();
        }

        ast::Statement handleMutable()
        {
            const auto tk = peekNext();

            if ( tk.type != TokenType::ClassType && tk.type != TokenType::PrimitiveType )
                throw std::runtime_error( "expected type after 'mutable' keyword" );

            auto tk2 = peekTo( i+2 );

            size_t idx = 0;

            if ( tk2.isRefOrPtr() )
            {
                tk2 = peekTo( i+3 );
                idx++;
            }
            if ( tk2.type == TokenType::Equals )
            {
                return parseAssignmentExpression().release();
            }
            if ( tk2.type == TokenType::Identifier )
            {
                const auto tk3 = peekTo( i+3 + idx ).type;
                if ( tk3 == TokenType::Equals || tk3 == TokenType::Semicolon )
                {
                    return parseVariableDeclaration();
                }
                return parseFunctionDeclaration();
            }
            throw std::runtime_error( "unkown token found" );
        }

        ast::Statement handleIdentifier()
        {
            return parseAssignmentExpression().release();
        }

        ast::Statement parseFunctionDeclaration()
        {
            auto const isMutable = eatIfMutable();

            auto type = expect( TokenType::ClassType, TokenType::PrimitiveType, "function must have return type" );

            std::string f_rettype_str { std::move( type.value ) };

            const auto [ isRef, isPtr ] = eatIfRefOrPtr();
            
            ast::TypeName f_rettype { std::move( f_rettype_str ), isMutable, isRef, isPtr };

            ast::Identifier f_name = expect( TokenType::Identifier, "function must have name" ).value;

            expect( TokenType::OParen, "missing open paren to start parameter list" );

            ast::ParameterList f_p_list;
            f_p_list.reserve( 10 );

            // Generate param list
            while ( peek().type == TokenType::ClassType || peek().type == TokenType::PrimitiveType || peek().type == TokenType::mutable_ )
            {
                auto const isMutable = eatIfMutable();

                std::string p_type { eat().value };

                const auto [ isRef, isPtr ] = eatIfRefOrPtr();

                std::string p_name { expect( TokenType::Identifier, "parameters must have a type and name" ).value };
                
                const auto maybe_comma = peek();

                if ( maybe_comma.type == TokenType::Comma ) eat();
                else if ( maybe_comma.type != TokenType::CParen )
                    throw std::runtime_error( "invalid parameter list for function " + f_name.getSymbol() );

                f_p_list.push_back( ast::Parameter( isMutable, std::move( p_type ), isRef, isPtr, std::move( p_name ) ) );
            }

            f_p_list.shrink_to_fit();

            expect( TokenType::CParen, "missing closing paren of parameter list" );
            expect( TokenType::OCurlyBrace, "missing opening bracket of function body" );

            ast::StatementList f_body;
            f_body.reserve( 10 );

            // Generate function body
            while ( peek().type != TokenType::CCurlyBrace )
            {
                const auto tk = peek();
                if ( tk.value == "return" )
                {
                    f_body.push_back( parseReturnStatement() );
                    break;
                }
                f_body.push_back( parseStatement() );
            }

            f_body.shrink_to_fit();

            expect( TokenType::CCurlyBrace, "No matching closing bracket on function " + f_name.getSymbol() );

            return new ast::FunctionDeclaration( f_rettype, f_name, std::move( f_p_list ), std::move( f_body ) );
        }

        ast::Statement parseReturnStatement()
        {
            const auto ret = expect( TokenType::return_, "" );
            return new ast::ReturnStatement( parseStatement() );
        }

        ast::Statement parseVariableDeclaration()
        {
            const auto isMutable = eatIfMutable();

            std::string typestr = std::move( expect( TokenType::ClassType, TokenType::PrimitiveType, "expected type in variable declaration" ).value );

            const auto [ isRef, isPtr ] = eatIfRefOrPtr();

            ast::TypeName type { std::move( typestr ), isMutable, isRef, isPtr };

            std::string name = std::move( expect( TokenType::Identifier, "Expected an identifier for a variable" ).value );

            if ( peek().type == TokenType::Semicolon )
            {
                eat();

                return new ast::VariableDeclaration( false, std::move( type ), std::move( name ) );
            }

            expect( TokenType::Equals, "Expected an '=' after identifier." );
            auto expr = parseExpression();
            return new ast::VariableDeclaration( isMutable, std::move( type ), std::move( name ), std::move( expr ) );
        }
        
        template< bool TopCall = true >
        std::unique_ptr< ast::Expression > parseExpression()
        {
            return parseAssignmentExpression< TopCall >();
        }

        template< bool TopCall = true >
        std::unique_ptr< ast::Expression > parseAssignmentExpression()
        {
            if constexpr ( TopCall )
            {
                const auto next = peekNext().type;

                if ( next == TokenType::ClassType || next == TokenType::PrimitiveType || next == TokenType::Reference || next == TokenType::Pointer )
                {
                    return makeExpression( parseVariableDeclaration().release< ast::VariableDeclaration >() );
                }
            }

            auto left = parseBooleanExpression();

            if ( peek().type == TokenType::Equals )
            {
                eat();
                auto right = parseAssignmentExpression< false >();
                left = makeExpression( new ast::AssignmentExpression{ std::move( left ), std::move( right ) } );
            }

            if constexpr ( TopCall )
                expect( TokenType::Semicolon, "must end statement with semicolon" );

            return left;
        }

        template< bool isLoneCall = false >
        std::unique_ptr< ast::Expression > parseFunctionCall()
        {
            auto f_name { expect( TokenType::Identifier, "Expected function name" ).value };

            expect( TokenType::OParen, "Function call must have open paren" );

            auto param = peek();

            ast::StatementList params;

            while ( param.type != TokenType::CParen )
            {
                params.push_back( parseAdditiveExpression().release() );
                const auto tkty = peek().type;
                if ( tkty == TokenType::Comma )
                {
                    eat();
                }
                param = peek();
            }

            expect( TokenType::CParen, "Expected closing paren to end function call" );

            if constexpr ( isLoneCall )
            {
                expect( TokenType::Semicolon, "Expected semicolon to end statement" );
            }

            return makeExpression( new ast::FunctionCall( std::move( f_name ), std::move( params ) ) );
        }

        // Recursive descent

        std::unique_ptr< ast::Expression > parseBooleanExpression()
        {
            auto left = parseAdditiveExpression();

            while ( peek().type == TokenType::EqualsEquals || peek().type == TokenType::NotEquals )
            {
                auto op{ eat().value };
                auto right = parseAdditiveExpression();
                left = makeExpression( new ast::BinaryExpression( std::move( left ), std::move( op ), std::move( right ) ) );
            }
            return left;
        }

        std::unique_ptr< ast::Expression > parseAdditiveExpression()
        {
            auto left = parseMultiplicativeExpression();

            while ( peek().value == "+" || peek().value == "-" )
            {
                auto op { eat().value };
                auto right = parseMultiplicativeExpression();
                left = makeExpression( new ast::BinaryExpression( std::move( left ), std::move( op ), std::move( right ) ) );
            }
            return left;
        }

        std::unique_ptr< ast::Expression > parseMultiplicativeExpression() 
        {
            auto left = parseExponentialExpression();

            while ( peek().isMultParseLevel() )
            {
                auto op { eat().value };
                auto right = parseExponentialExpression();
                left = makeExpression( new ast::BinaryExpression( std::move( left ), std::move( op ), std::move( right ) ) );
            }
            return left;
        }

        std::unique_ptr< ast::Expression > parseExponentialExpression() 
        {
            auto left = parseDotExpression();

            while ( peek().type == TokenType::Exponent )
            {
                auto op { eat().value };
                auto right = parseDotExpression();
                left = makeExpression( new ast::BinaryExpression( std::move( left ), std::move( op ), std::move( right ) ) );
            }
            return left;
        }

        std::unique_ptr< ast::Expression > parseDotExpression()
        {
            auto left = parsePrimaryExpression();

            while ( peek().type == TokenType::Dot )
            {
                auto op{ eat().value };
                auto right = parsePrimaryExpression();
                left = makeExpression( new ast::BinaryExpression( std::move( left ), std::move( op ), std::move( right ) ) );
            }
            return left;
        }
        
        std::unique_ptr< ast::Expression > parsePrimaryExpression() 
        {
            const auto tk = peek().type;
            
            switch ( tk )
            {
                case TokenType::Identifier:
                {
                    if ( peekNext().type == TokenType::OParen )
                    {
                        return parseFunctionCall();
                    }
                    return makeExpression( new ast::Identifier( eat().value ) );
                }
                case TokenType::negative_integer_literal:
                    return makeExpression( new ast::NumericLiteral< int64_t >( atoll( eat().value.c_str() ) ) );
                case TokenType::integer_literal:
                    return makeExpression( new ast::NumericLiteral< uint64_t >( atoll( eat().value.c_str() ) ) );
                case TokenType::float_literal:
                    return makeExpression( new ast::NumericLiteral< double >( atof( eat().value.c_str() ) ) );
                case TokenType::string_literal:
                    return makeExpression( new ast::StringLiteral( eat().value ) );
                case TokenType::char_literal:
                    return makeExpression( new ast::CharacterLiteral( eat().value ) );
                case TokenType::bool_literal:
                    return makeExpression( new ast::BoolLiteral( eat().value == "true" ) );
                case TokenType::OParen:
                {
                    eat();
                    auto value = parseExpression< false >();
                    expect( TokenType::CParen, "No closing paren!" );
                    return value;
                }

                default:
                    throw std::runtime_error( "Unexpected token found during parsing!" );
            }
        }
    };
}
