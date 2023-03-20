#pragma once

#include <stdint.h>
#include <algorithm>

#include "common.h"

namespace t
{
    namespace ast
    {
        namespace type
        {
            enum Type : uint8_t
            {
                Statement,
                Expression,
                Program,
                Scope,
            };
            enum ExpressionType : uint8_t
            {
                NumericLiteral,
                StringLiteral,
                Identifier,
                Binary,
                Call,
                Unary,
                FunctionDeclaration,
                VariableDeclaration,
                Assignment,
            };
            enum VariableType : uint8_t
            {
                String,
                i8, i16, i32, i64,
                u8, u16, u32, u64,
                char_,
                bool_,
                vector,
                auto_
            };
        };

        using type::Type;

        class Statement;

        using StatementList = std::vector< Statement >;

        class Program
        {
        public:
            Program() = default;

            Program( StatementList&& body ):
                body( std::move( body ) ) {}

            void print() const;

            ~Program() = default;
        private:
            StatementList body;
        };
        
        class Expression
        {
        public:
            virtual void print() const = 0;
            virtual ~Expression() = default;

            template< typename T >
            bool is() const { return dynamic_cast< const T* >( this ) != nullptr; }
            template< typename T >
            T* as() { return dynamic_cast< T* >( this ); }
        protected:
            static inline uint8_t numOfTabs = 0;
            static void printTabs()
            {
                for ( uint8_t i = 0; i < numOfTabs; i++ )
                {
                    std::cout << "   ";
                }
            }
        };

        class Statement
        {
        public:
            Statement():
                kind( Type::Statement ) {}

            Statement( ast::Program* ptr ):
                kind( Type::Program ), ptr( ptr ) {}

            Statement( ast::Expression* ptr ):
                kind( Type::Expression ), ptr( ptr ) {}

            Statement( StatementList* ptr ):
                kind ( Type::Scope ), ptr( ptr ) {}

            Statement( const Statement& stmt ) = delete;

            Statement( Statement&& stmt ) noexcept
            {
                kind = stmt.kind;
                ptr = stmt.ptr;
                stmt.ptr = nullptr;
            }

            template< typename T >
            T* release() { T* p = static_cast< T* >( ptr ); ptr = nullptr; return p; }

            Statement& operator=( const Statement& stmt ) = delete;

            Statement& operator=( Statement&& stmt ) noexcept
            {
                delete ptr;
                ptr = stmt.ptr;
                stmt.ptr = nullptr;
                kind = stmt.kind;
                return *this;
            }

            Type getKind() const { return kind; }
            ~Statement() { delete ptr; }

            template< Type T >
            bool is() const { return T == kind; }
            template< Type T >
            bool isNot() const { return !is< T >(); }
            template< typename T >
            T* as() { return static_cast< T* >( ptr ); }
            void print() const
            {
                switch ( kind )
                {
                case Type::Expression:
                    return static_cast< Expression* >( ptr )->print();
                case Type::Program:
                    return static_cast< Program* >( ptr )->print();
                case Type::Scope:
                    auto& stlist = *static_cast< StatementList* >( ptr );
                    std::for_each( stlist.cbegin(), stlist.cend(),
                        []( const Statement& stmt ){ stmt.print(); });
                    return;
                }
            }
        private:
            Type kind;
            void* ptr = nullptr;
        };

        void Program::print() const
        {
            for ( const auto& elem : body )
            {
                elem.print();
            }
        }

        class AssignmentExpression : public Expression
        {
        public:
            AssignmentExpression( std::unique_ptr< Expression >&& lhs, std::unique_ptr< Expression >&& rhs ):
                lhs( std::move( lhs ) ), rhs( std::move( rhs ) ) {}

            virtual void print() const override
            {
                numOfTabs++;
                printTabs();
                std::cout << "Assignment expression:\n";
                numOfTabs++;
                printTabs();
                std::cout << "lhs:\n";
                lhs.get()->print();
                printTabs();
                std::cout << "rhs:\n";
                rhs.get()->print();
                numOfTabs--;
                std::cout << '\n';
                numOfTabs--;
            }
        private:
            std::unique_ptr< Expression > lhs = nullptr;
            std::unique_ptr< Expression > rhs = nullptr;
        };

        class TypeName : public Expression
        {
        public:
            TypeName( std::string&& name, const bool isMutable = false, bool isRef = false, bool isPtr = false ):
                name( std::move( name ) ),
                isMutable( isMutable ) 
                {
                    if ( isRef && isPtr == true )
                        throw std::runtime_error( "cannot be both reference and ptr" );
                    if ( isRef )
                        ptr_or_ref = "~";
                    if ( isPtr )
                        ptr_or_ref = "->";
                }
            virtual void print() const override
            {
                numOfTabs++;
                printTabs();
                std::cout << ( isMutable ? "mutable " : "" ) << name << ptr_or_ref << '\n';
                numOfTabs--;
            }
        private:
            std::string name;
            const bool isMutable = false;
            std::string ptr_or_ref = "";
        };

        class Identifier : public Expression
        {
        public:
            Identifier( std::string&& symbol ):
                symbol( std::move( symbol ) ) {}
            Identifier( Identifier&& id ) noexcept:
                symbol( std::move( id.symbol ) ) {}

            virtual void print() const override
            {
                numOfTabs++;
                printTabs();
                std::cout << symbol << '\n';
                numOfTabs--;
            }

            void setSymbol( std::string&& sym ) { symbol = std::move( sym ); }

            const std::string& getSymbol() const { return symbol; }
        private:
            std::string symbol;
        };

        class VariableDeclaration : public Expression
        {
        public:
            VariableDeclaration( bool isMutable, TypeName&& type, std::string&& identifier ):
                identifier( std::move( identifier ) ), isMutable( isMutable ),
                type( std::move( type ) ) {}

            VariableDeclaration( bool isMutable, TypeName&& type, std::string&& identifier, std::unique_ptr< Expression >&& value ):
                identifier( std::move( identifier ) ), isMutable( isMutable ),
                type( std::move( type ) ), value( std::move( value ) ) {}

            VariableDeclaration( VariableDeclaration&& var ) noexcept:
                identifier( std::move( var.identifier ) ), isMutable( var.isMutable ),
                type( std::move( var.type ) ), value( std::move( var.value ) ) {}

            virtual void print() const override
            {
                numOfTabs++;
                printTabs();
                std::cout << "Variable Declaration:\n";
                numOfTabs++;
                printTabs();
                std::cout << "Type:\n";
                type.print();
                printTabs();
                std::cout << "Identifier:\n";
                identifier.print();
                printTabs();
                std::cout << "Value:\n";
                if ( value )
                    value->print();
                else
                {
                    numOfTabs++;
                    printTabs();
                    std::cout << "null\n";
                    numOfTabs--;
                }
                numOfTabs--;
                numOfTabs--;
            }

            bool isMutableVar() const { return isMutable; }
            const Identifier& getIdentifier() const { return identifier; }
            const TypeName& getType() const { return type; }
            const Expression* getValue() const { return value.get(); }
        private:
            bool isMutable = false;
            Identifier identifier;
            TypeName type;
            std::unique_ptr< Expression > value = nullptr;
        };

        class BinaryExpression : public Expression
        {
        public:
            BinaryExpression( std::unique_ptr< Expression >&& lhs, std::string&& op, std::unique_ptr< Expression >&& rhs ):
                op( std::move( op ) ),
                lhs( std::move( lhs ) ),
                rhs( std::move( rhs ) ) {}

            virtual void print() const override
            {
                numOfTabs++;
                printTabs();
                std::cout << "Binary expression:\n";
                numOfTabs++;
                printTabs();
                std::cout << "lhs:\n";
                lhs.get()->print();
                printTabs();
                std::cout << "operator: " << op << '\n';
                printTabs();
                std::cout << "rhs:\n";
                rhs.get()->print();
                numOfTabs--;
                std::cout << '\n';
                numOfTabs--;
            }
        private:
            std::unique_ptr< Expression > lhs;
            std::unique_ptr< Expression > rhs;
            std::string op;
        };

        template< bool isPre = false >
        class UnaryExpression : public Expression
        {
        public:
            UnaryExpression( std::string&& op, std::unique_ptr< Expression >&& rhs ):
                op( std::move( op ) ),
                expr( std::move( expr ) ) {}

            virtual void print() const override
            {
                numOfTabs++;
                printTabs();
                std::cout << "Unary expression:\n";
                numOfTabs++;
                printTabs();
                std::cout << "Expression:\n";
                if ( expr )
                    expr.get()->print();
                printTabs();
                std::cout << "operator: " << op << ( isPre ? " (pre)" : " (post)" ) << '\n';
                numOfTabs--;
                std::cout << '\n';
                numOfTabs--;
            }
        private:
            std::unique_ptr< Expression > expr;
            std::string op;
        };

        template< typename T, typename = std::enable_if_t< std::is_arithmetic_v< T > > >
        class NumericLiteral : public Expression
        {
        public:
            NumericLiteral( T value ):
                value( value ) {}

            virtual void print() const override
            {
                numOfTabs++;
                printTabs();
                if constexpr ( std::is_floating_point_v< T > )
                    std::cout << "Floating Point ";
                else
                        std::cout << "Integer ";

                std::cout << "Numeric Literal:\n";
                numOfTabs++;
                printTabs();
                std::cout << "Value: " << value << '\n';
                numOfTabs--;
                numOfTabs--;
            }
        private:
            T value = 0;
        };

        class StringLiteral : public Expression
        {
        public:
            StringLiteral( std::string&& value ):
                value( std::move( value ) ) {}

            virtual void print() const override
            {
                numOfTabs++;
                printTabs();
                std::cout << "String Literal:\n";
                numOfTabs++;
                printTabs();
                std::cout << "Value: " << value << '\n';
                numOfTabs--;
                numOfTabs--;
            }
        private:
            std::string value;
        };

        class CharacterLiteral : public Expression
        {
        public:
            CharacterLiteral( std::string&& value ):
                value( std::move( value ) ) {}

            virtual void print() const override
            {
                numOfTabs++;
                printTabs();
                std::cout << "Character Literal:\n";
                numOfTabs++;
                printTabs();
                std::cout << "Value: " << value << '\n';
                numOfTabs--;
                numOfTabs--;
            }
        private:
            std::string value;
        };

        class BoolLiteral : public Expression
        {
        public:
            BoolLiteral( bool value ):
                value( value ) {}
            BoolLiteral( std::string& boolInString ):
                value( boolInString == "true" ?
                        true :
                        boolInString == "false" ?
                        false :
                        throw std::runtime_error( "bad bool literal" )
                    ) {}
            virtual void print() const
            {
                numOfTabs++;
                printTabs();
                std::cout << "BoolLiteral:\n";
                numOfTabs++;
                printTabs();
                std::cout << ( value ? "true" : "false" ) << '\n';
                numOfTabs--;
                numOfTabs--;
            }
        private:
            bool value;
        };

        class Parameter : public Expression
        {
        public:
            Parameter( bool isMutable, std::string&& type, bool isRef, bool isPtr, Identifier&& name ):
                type( std::move( type ), isMutable, isRef, isPtr ),
                name( std::move( name ) ) {}
            Parameter( std::string&& type, Identifier&& name ):
                type( std::move( type ), false, false, false ),
                name( std::move( name ) ) {}
            Parameter( std::string&& type, bool isRef, bool isPtr, Identifier&& name ):
                type( std::move( type ), false, isRef, isPtr ),
                name( std::move( name ) ) {}
            Parameter( std::string&& type, bool isMutable, Identifier&& name ):
                type( std::move( type ), isMutable, false, false ),
                name( std::move( name ) ) {}
            Parameter( Parameter&& param ) noexcept:
                type( std::move( param.type ) ),
                name( std::move( param.name ) ) {}
            virtual ~Parameter() = default;
            virtual void print() const override
            {
                numOfTabs++;
                printTabs();
                std::cout << "Parameter:\n";
                type.print();
                name.print();
                numOfTabs--;
            }
            const TypeName& getTypeName() const { return type; }
            const Identifier& getIdentifier() const { return name; }
        private:
            TypeName type;
            Identifier name;
        };

        using ParameterList = std::vector< Parameter >;

        class FunctionDeclaration : public Expression
        {
        public:
            FunctionDeclaration( TypeName& retType, Identifier& name, ParameterList&& params, StatementList&& stmts ):
                returnType( std::move( retType ) ),
                name( std::move( name ) ),
                paramList( std::move( params ) ),
                body( std::move( stmts ) ) {}
            FunctionDeclaration( FunctionDeclaration&& func ) noexcept:
                returnType( std::move( func.returnType ) ),
                name( std::move( func.name ) ),
                paramList( std::move( func.paramList ) ),
                body( std::move( func.body ) ) {}

            virtual void print() const override
            {
                numOfTabs++;
                printTabs();
                std::cout << "Function Declaration:\n";
                numOfTabs++;
                printTabs();
                std::cout << "Name:\n";
                name.print();
                printTabs();
                std::cout << "Returns:\n";
                returnType.print();
                printTabs();
                std::cout << "Parameters:\n";
                if ( paramList.empty() ) { numOfTabs++; printTabs(); std::cout << "null\n"; numOfTabs--; }
                else for ( size_t i = 0; i < paramList.size(); i++ )
                {
                    paramList[ i ].print();
                }
                printTabs();
                std::cout << "Body:\n";
                if ( body.empty() ) { numOfTabs++; printTabs(); std::cout << "null\n"; numOfTabs--; }
                else for ( const auto& stmt : body )
                {
                    stmt.print();
                }

                numOfTabs--;

                numOfTabs--;
            }
            const TypeName& getReturnType() const { return returnType; }
            const Identifier& getName() const { return name; }
            const ParameterList& getParamList() const { return paramList; }
            const StatementList& getBody() const { return body; }
        private:
            TypeName returnType;
            Identifier name;
            ParameterList paramList;
            StatementList body;
        };

        enum AccessSpecifier : uint8_t
        {
            Public,
            Private,
            Protected,
        };

        std::string specToStr( AccessSpecifier spec )
        {
            switch ( spec )
            {
            case Public:
                return "public";
            case Private:
                return "private";
            case Protected:
                return "protected";
            }
        }

        struct FieldDeclaration : public Expression
        {
            VariableDeclaration var;
            const AccessSpecifier spec;
            FieldDeclaration( VariableDeclaration&& var, AccessSpecifier spec ):
                var( std::move( var ) ),
                spec( spec ) {}
            FieldDeclaration( FieldDeclaration&& field ) noexcept:
                var( std::move( field.var ) ),
                spec( field.spec ) {}

            virtual void print() const override
            {
                numOfTabs++;
                printTabs();
                std::cout << "Field Declaration: (" << specToStr( spec ) << ")\n";

                var.getType().print();
                var.getIdentifier().print();
                if ( var.getValue() )
                    var.getValue()->print();

                numOfTabs--;
            }
        };

        struct MethodDeclaration : public Expression
        {
            FunctionDeclaration func;
            const AccessSpecifier spec;
            MethodDeclaration( FunctionDeclaration&& func, AccessSpecifier spec ):
                func( std::move( func ) ),
                spec( spec ) {}
            MethodDeclaration( MethodDeclaration&& meth ):
                func( std::move( meth.func ) ),
                spec( meth.spec ) {}

            virtual void print() const override
            {
                numOfTabs++;
                printTabs();
                std::cout << "Method Declaration: (" << specToStr( spec ) << ")\n";
                numOfTabs++;
                printTabs();
                std::cout << "Name:\n";
                func.getName().print();
                printTabs();
                std::cout << "Returns:\n";
                func.getReturnType().print();
                printTabs();
                std::cout << "Parameters:\n";
                if ( func.getParamList().empty() ) { numOfTabs++; printTabs(); std::cout << "null\n"; numOfTabs--; }
                else for ( const auto& param : func.getParamList() )
                {
                    param.print();
                }
                printTabs();
                std::cout << "Body:\n";
                if ( func.getBody().empty() ) { numOfTabs++; printTabs(); std::cout << "null\n"; numOfTabs--; }
                else for ( const auto& stmt : func.getBody() )
                {
                    stmt.print();
                }

                numOfTabs--;

                numOfTabs--;
            }
        };

        using FieldList = std::vector< FieldDeclaration >;
        using MethodList = std::vector< MethodDeclaration >;

        class ClassDeclaration : public Expression
        {
        public:
            ClassDeclaration( TypeName&& type, FieldList&& fields, MethodList&& methods ):
                type( std::move( type ) ),
                fields( std::move( fields ) ),
                methods( std::move( methods ) ) {}

            virtual void print() const override
            {
                numOfTabs++;
                printTabs();
                std::cout << "Class Definition:\n";
                type.print();
                for ( const auto& f : fields )
                    f.print();
                for ( const auto& m : methods )
                    m.print();
                numOfTabs--;
            }
        private:
            TypeName type;
            FieldList fields;
            MethodList methods;
        };

        class FunctionCall : public Expression
        {
        public:
            FunctionCall( Identifier&& id, StatementList&& params ):
                name( std::move( id ) ),
                parameters( std::move( params ) ) {}

            virtual void print() const override
            {
                numOfTabs++;
                printTabs();
                std::cout << "Function Call:\n";
                numOfTabs++;
                printTabs();
                std::cout << "Name:\n";
                numOfTabs++;
                printTabs();
                std::cout << name.getSymbol() << '\n';
                numOfTabs--;
                printTabs();
                std::cout << "Parameters:\n";
                if ( parameters.empty() ){ numOfTabs++; printTabs(); std::cout << "null\n"; numOfTabs--; }
                else for ( const auto& param : parameters )
                {
                    param.print();
                }

                numOfTabs--;

                numOfTabs--;
            }
        private:
            Identifier name;
            StatementList parameters;
        };

        class ReturnStatement : public Expression
        {
        public:
            ReturnStatement( Statement&& stmt ):
                stmt( std::move( stmt ) ) {}

            virtual void print() const override
            {
                numOfTabs++;
                printTabs();
                std::cout << "Return Statement:\n";
                stmt.print();
                numOfTabs--;
            }
        private:
            Statement stmt;
        };

        class NameSpaceDeclaration : public Expression
        {
        public:
            NameSpaceDeclaration( std::string&& name, StatementList&& body ):
                name( std::move( name ) ),
                body( std::move( body ) ) {}
            NameSpaceDeclaration( Identifier&& name, StatementList&& body ):
                name( std::move( name ) ),
                body( std::move( body ) ) {}
            virtual void print() const
            {
                numOfTabs++;
                printTabs();
                std::cout << "Namespace Declaration:\n";
                numOfTabs++;
                printTabs();
                std::cout << "Name:\n";
                name.print();
                printTabs();
                std::cout << "Body:\n";
                if ( body.empty() ) { numOfTabs++; printTabs(); std::cout << "null\n"; numOfTabs--; }
                else for ( const auto& stmt : body )
                {
                    stmt.print();
                }
                numOfTabs--;
                numOfTabs--;
            }
        private:
            Identifier name;
            StatementList body;
        };

        class IfStatement : public Expression
        {
        public:
            IfStatement( std::unique_ptr< Expression >&& condition, StatementList&& body ):
                condition( std::move( condition ) ),
                body( std::move( body ) ) {}
            virtual void print() const
            {
                numOfTabs++;
                printTabs();
                std::cout << "If Statement:\n";
                numOfTabs++;
                printTabs();
                std::cout << "Condition:\n";
                condition->print();
                printTabs();
                std::cout << "Body:\n";
                if ( body.empty() ) { numOfTabs++; printTabs(); std::cout << "null\n"; numOfTabs--; }
                else for ( const auto& stmt : body )
                {
                    stmt.print();
                }
                numOfTabs--;
                numOfTabs--;
            }
        private:
            std::unique_ptr< Expression > condition;
            StatementList body;
        };
    }
}