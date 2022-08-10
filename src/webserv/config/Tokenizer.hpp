/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Tokenizer.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: kycho <kycho@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2021/06/18 19:18:46 by sucho             #+#    #+#             */
/*   Updated: 2021/07/06 15:14:04 by kycho            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <iostream>
#include <string>
#include <vector>

namespace ft {

enum TokenType {
  WHITESPACE,
  STRING_ESCAPE_SEQUENCE,
  SINGLE_QUOTE_LITERAL,
  DOUBLE_QUOTE_LITERAL,
  IDENTIFIER,
  OPERATOR,
  COMMENT
};

class Token {
 public:
  enum TokenType m_type;
  std::string m_text;
};


class Tokenizer {
 public:
  std::vector<std::string> parse(const std::string &file);

 private:
  void endToken(Token &token, std::vector<std::string> &tokens);
};

}  // namespace ft

#endif
