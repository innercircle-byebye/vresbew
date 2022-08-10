#include "webserv/config/Tokenizer.hpp"

namespace ft {

std::vector<std::string> Tokenizer::parse(const std::string &file) {
  std::vector<std::string> tokens;
  Token currentToken;

  //std::cout << "--------------- file start-------------" << std::endl;
  //std::cout << file << std::endl;
  //std::cout << "--------------- file end -------------" << std::endl;

  // 현재 토큰 초기화
  currentToken.m_type = WHITESPACE;

  std::string::const_iterator it;

  for (it = file.begin(); it != file.end(); it++) {
    switch (*it) {
      case ';':
        if (currentToken.m_type == COMMENT) {
          currentToken.m_text.append(1, *it);
          endToken(currentToken, tokens);
          break;
        } else if (currentToken.m_type == IDENTIFIER) {
          endToken(currentToken, tokens);
        }
          currentToken.m_type = OPERATOR;
          currentToken.m_text.append(1, *it);
          endToken(currentToken, tokens);
        break;

      case '"':
        // to keep quotes inside
        // if not need, simply delete the block
        if (currentToken.m_type == SINGLE_QUOTE_LITERAL) {
          currentToken.m_text.append(1, *it);
          continue;
        } else if (currentToken.m_type != DOUBLE_QUOTE_LITERAL) {
          currentToken.m_type = DOUBLE_QUOTE_LITERAL;
        } else if (currentToken.m_type == DOUBLE_QUOTE_LITERAL) {
          endToken(currentToken, tokens);
        }
        break;

      case '\'':
        // to keep quotes inside
        // if not need, simply delete the block
        if (currentToken.m_type == DOUBLE_QUOTE_LITERAL) {
          currentToken.m_text.append(1, *it);
          continue;
        } else if (currentToken.m_type != SINGLE_QUOTE_LITERAL) {
          currentToken.m_type = SINGLE_QUOTE_LITERAL;
        } else if (currentToken.m_type == SINGLE_QUOTE_LITERAL) {
          endToken(currentToken, tokens);
        }
        break;

      case ' ':
      case '\t':
        if (currentToken.m_type == SINGLE_QUOTE_LITERAL ||
            currentToken.m_type == DOUBLE_QUOTE_LITERAL ||
            currentToken.m_type == COMMENT) {
          currentToken.m_text.append(1, *it);
        } else {
          endToken(currentToken, tokens);
        }
        break;


      case '\n':
        endToken(currentToken, tokens);
        break;


      case '#':
        currentToken.m_type = COMMENT;


      default:
        if (currentToken.m_type == WHITESPACE) {
          endToken(currentToken, tokens);
          currentToken.m_type = IDENTIFIER;
          currentToken.m_text.append(1, *it);

        } else {
          currentToken.m_text.append(1, *it);
        }
        break;
    }
  }
  endToken(currentToken, tokens);

  return (tokens);
}

void Tokenizer::endToken(Token &token, std::vector<std::string> &tokens) {
  if (token.m_type == COMMENT) {
    std::cout << "Ignoring comment " + token.m_text << std::endl;
  } else if (token.m_type != WHITESPACE) {
    tokens.push_back(token.m_text);
  }

  token.m_type = WHITESPACE;
  token.m_text.erase();
}

}  // namespace ft
