/*
	This file is part of solidity.

	solidity is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	solidity is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with solidity.  If not, see <http://www.gnu.org/licenses/>.
*/
// SPDX-License-Identifier: GPL-3.0
#include <libsolidity/lsp/SemanticTokensBuilder.h>

#include <liblangutil/CharStream.h>
#include <liblangutil/SourceLocation.h>

using namespace std;
using namespace solidity::langutil;
using namespace solidity::frontend;

namespace solidity::lsp
{

Json::Value SemanticTokensBuilder::build(SourceUnit const& _sourceUnit, CharStream const& _charStream)
{
	reset(&_charStream);
	_sourceUnit.accept(*this);
	return m_encodedTokens;
}

void SemanticTokensBuilder::reset(CharStream const* _charStream)
{
	m_encodedTokens = Json::arrayValue;
	m_charStream = _charStream;
	m_lastLine = 0;
	m_lastStartChar = 0;
}

void SemanticTokensBuilder::encode(
	SourceLocation const& _sourceLocation,
	SemanticTokenType _tokenType,
	SemanticTokenModifiers _modifiers
)
{
	/*
	https://microsoft.github.io/language-server-protocol/specifications/specification-3-17/#textDocument_semanticTokens

	// Step-1: Absolute positions
	{ line: 2, startChar:  5, length: 3, tokenType: 0, tokenModifiers: 3 },
	{ line: 2, startChar: 10, length: 4, tokenType: 1, tokenModifiers: 0 },
	{ line: 5, startChar:  2, length: 7, tokenType: 2, tokenModifiers: 0 }

	// Step-2: Relative positions as intermediate step
	{ deltaLine: 2, deltaStartChar: 5, length: 3, tokenType: 0, tokenModifiers: 3 },
	{ deltaLine: 0, deltaStartChar: 5, length: 4, tokenType: 1, tokenModifiers: 0 },
	{ deltaLine: 3, deltaStartChar: 2, length: 7, tokenType: 2, tokenModifiers: 0 }

	// Step-3: final array result
	// 1st token,  2nd token,  3rd token
	[  2,5,3,0,3,  0,5,4,1,0,  3,2,7,2,0 ]

	So traverse through the AST and assign each leaf a token 5-tuple.
	*/

	// solAssert(_sourceLocation.isValid());
	if (!_sourceLocation.isValid())
		return;

	auto const [line, startChar] = m_charStream->translatePositionToLineColumn(_sourceLocation.start);
	auto const length = _sourceLocation.end - _sourceLocation.start;

	if (_tokenType == SemanticTokenType::Parameter)
	{
		fmt::print("type param: {}:{}+{}\n", line, startChar, length);
	}

	m_encodedTokens.append(line - m_lastLine);
	if (line == m_lastLine)
		m_encodedTokens.append(startChar - m_lastStartChar);
	else
		m_encodedTokens.append(startChar);
	m_encodedTokens.append(length);
	m_encodedTokens.append(static_cast<int>(_tokenType));
	m_encodedTokens.append(static_cast<int>(_modifiers));

	m_lastLine = line;
	m_lastStartChar = startChar;
}

bool SemanticTokensBuilder::visit(frontend::ContractDefinition const& _node)
{
	encode(_node.nameLocation(), SemanticTokenType::Class);
	return true;
}

bool SemanticTokensBuilder::visit(frontend::EnumDefinition const& _node)
{
	encode(_node.nameLocation(), SemanticTokenType::Enum);
	return true;
}

bool SemanticTokensBuilder::visit(frontend::EnumValue const& _node)
{
	encode(_node.nameLocation(), SemanticTokenType::EnumMember);
	return true;
}

bool SemanticTokensBuilder::visit(frontend::FunctionDefinition const& _node)
{
	encode(_node.nameLocation(), SemanticTokenType::Function);
	return true;
}

void SemanticTokensBuilder::endVisit(frontend::Literal const& _literal)
{
	encode(_literal.location(), SemanticTokenType::Number);
}

void SemanticTokensBuilder::endVisit(frontend::Identifier const& _identifier)
{
	encode(_identifier.location(), SemanticTokenType::Variable);
}

void SemanticTokensBuilder::endVisit(frontend::IdentifierPath const& _identifierPath)
{
	if (dynamic_cast<EnumDefinition const*>(_identifierPath.annotation().referencedDeclaration))
		encode(_identifierPath.location(), SemanticTokenType::Enum);
	else
		encode(_identifierPath.location(), SemanticTokenType::Variable);
}

bool SemanticTokensBuilder::visit(frontend::ParameterList const& _parameterList)
{
	for (ASTPointer<VariableDeclaration> const& parameter: _parameterList.parameters())
	{
		// SourceLocation typeLocation = parameter->location();
		// typeLocation.end = parameter->nameLocation().start - 1;
		// encode(typeLocation, SemanticTokenType::TypeParameter);
		// encode(parameter->nameLocation(), SemanticTokenType::Parameter);
		encode(parameter->location(), SemanticTokenType::Parameter);
	}
	return false; // do not descent into child nodes
}

void SemanticTokensBuilder::endVisit(PragmaDirective const& _pragma)
{
	encode(_pragma.location(), SemanticTokenType::Macro);
}

bool SemanticTokensBuilder::visit(frontend::VariableDeclaration const& _node)
{
	auto typeLocation = _node.location();
	typeLocation.end = _node.nameLocation().start - 1;
	encode(typeLocation, SemanticTokenType::Type);
	encode(_node.nameLocation(), SemanticTokenType::Variable);
	return true;
}

} // end namespace
