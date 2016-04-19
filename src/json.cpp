/*

Copyright (c) 2016 xyzdev.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
#include "json.hpp"

#include <stack>
#include <sstream>
#include <cmath>
#include <iomanip>

namespace xyz {
  namespace json {

    Element::Type Element::getType() const {
      return type;
    }

    const char *Element::getTypeName() const {
      if(type < 0 || type > 5) {
        throw TypeError("TypeError: Invalid type.");
      }
      static const char *names[] = {"NULL", "OBJECT", "ARRAY", "STRING", "NUMBER", "BOOLEAN"};
      return names[type];
    }

    Object &Element::object() {
      if(type != OBJECT) throw TypeError(OBJECT);
      return _object;
    }

    Array &Element::array() {
      if(type != ARRAY) throw TypeError(ARRAY);
      return _array;
    }

    String &Element::str() {
      if(type != STRING) throw TypeError(STRING);
      return _string;
    }

    Number &Element::number() {
      if(type != NUMBER) throw TypeError(NUMBER);
      return _number;
    }

    Boolean &Element::boolean() {
      if(type != BOOLEAN) throw TypeError(BOOLEAN);
      return _boolean;
    }

    const Object &Element::object() const {
      if(type != OBJECT) throw TypeError(OBJECT);
      return _object;
    }

    const Array &Element::array() const {
      if(type != ARRAY) throw TypeError(ARRAY);
      return _array;
    }

    const String &Element::str() const {
      if(type != STRING) throw TypeError(STRING);
      return _string;
    }

    const Number &Element::number() const {
      if(type != NUMBER) throw TypeError(NUMBER);
      return _number;
    }

    const Boolean &Element::boolean() const {
      if(type != BOOLEAN) throw TypeError(BOOLEAN);
      return _boolean;
    }

    String Element::toString() const {
      if(isString()) {
        return '"' + str() + '"';
      }
      if(isNumber()) {
        std::ostringstream os;
        os << number();
        return os.str();
      }
      if(isNull()) {
        return "null";
      }
      if(isBoolean()) {
        return boolean() ? "true" : "false";
      }
      if(isArray()) {
        std::ostringstream os;
        os << "ARRAY [" << array().size() << "]";
        return os.str();
      }
      if(isObject()) {
        std::ostringstream os;
        os << "OBJECT [" << object().size() << "]";
        return os.str();
      }

      throw TypeError("TypeError: Bug: Illegal enum value");
    }

    Element &Element::operator =(const Element &r) {
      if(this == &r) {
        return *this;
      }

      if(r.type == OBJECT) {
        if(type == OBJECT) {
          // In case r is child of _object.
          Object tmp(r._object);
          _object.swap(tmp);
        } else {
          _object = r._object;
        }
        _array.clear();
        _string.clear();
      }
      else if(r.type == ARRAY) {
        if(type == ARRAY) {
          // In case r is child of _array.
          Array tmp(r._array);
          _array.swap(tmp);
        } else {
          _array = r._array;
        }
        _object.clear();
        _string.clear();
      }
      else if(r.type != NULL_VALUE) {
	_string = r._string;
	_number = r._number;
	_boolean = r._boolean;
        _object.clear();
        _array.clear();
      }
      else {
        _object.clear();
        _array.clear();
        _string.clear();
      }

      type = r.type;

      return *this;
    }

    bool Element::operator ==(const Element &r) const {
      if(type != r.type && !(isNumber() && r.isNumber())) {
	return false;
      }
      if(type == NULL_VALUE) {
        return true;
      }
      if(type == OBJECT) {
	return object() == r.object();
      }
      if(type == ARRAY) {
	return array() == r.array();
      }
      if(type == NUMBER) {
	return _number == r._number;
      }
      if(type == BOOLEAN) {
        return _boolean == r._boolean;
      }
      return str() == r.str();
    }

    void Element::swap(Element &r) {
      if(this == &r) {
        return;
      }

      if(r.type == OBJECT || type == OBJECT) {
	_object.swap(r._object);
      }

      if(r.type == ARRAY || type == ARRAY) {
	_array.swap(r._array);
      }

      if(r.type == STRING || type == STRING) {
	_string.swap(r._string);
      }

      bool b = _boolean;
      _boolean = r._boolean;
      r._boolean = b;

      Number n = _number;
      _number = r._number;
      r._number = n;

      Type t = type;
      type = r.type;
      r.type = t;
    }

    String::value_type parseHexChar(std::istream &stream, int &line) {
      String buf = "    ";
      if(!stream.read(&(buf[0]), 4)) {
        throw SyntaxError("Unexpected end of file while reading character escape sequence.", line, buf[0]);
      }

      std::size_t pos = 0;
      int val = -1;

      try {
        val = std::stoi(buf, &pos, 16);
      } catch(std::invalid_argument &e) {
      } catch(std::out_of_range &e) {
      }

      if(val < 0 || pos != 4) {
        throw SyntaxError("Invalid character escape sequence.", line, buf[0]);
      }

      if(val > 0xff) {
        throw SyntaxError("Escape sequence above Latin-1 not implemented.", line, buf[0]);
      }

      return String::value_type(val);
    }

    void parseString(std::istream &stream, String &str, int &line) {
      // Read string content up to and including terminating quote.
      // Opening quote must have been previously consumed from stream.

      std::ostringstream os;
      char in;
      bool escaped = false;

      while(stream.read(&in, 1)) {
        if(escaped) {
          if(in == '\\') os << '\\';
          else if(in == '"') os << '"';
          else if(in == 'n') os << '\n';
          else if(in == 'r') os << '\r';
          else if(in == 't') os << '\t';
          else if(in == 'f') os << '\f';
          else if(in == 'b') os << '\b';
          else if(in == '/') os << '/';
          else if(in == 'u') {
            os << parseHexChar(stream, line);
          }
          else {
            throw SyntaxError("Illegal string escape sequence", line, in);
          }
          escaped = false;
        }
        else {
          if(in == '"') {
            str = os.str();
            return;
          }
          if(in == '\\') {
            escaped = true;
          }
          else if((in >= '\x00' && in <= '\x1f') || in == '\x7f' || (in >= '\x80' && in <= '\x9f')) {
            throw SyntaxError("Control character in string.", line, in);
          }
          else {
            os << in;
          }
        }
      }

      throw SyntaxError("Unexpected end of file while parsing string.", line, in);
    }

    char parseNumber(std::istream &stream, char first, Number &num, int &line) {
      // Read number from stream.
      // This function is more permissive than the json standard, e.g. allowing leading '+'.
      // The first character must have been consumed from stream and passed as "first".
      // This function may consume a character past the end of the number,
      // in which case it is returned, otherwise ' ' is returned.

      std::ostringstream os;
      os << first;

      char in;
      char extra = ' ';

      while(stream.read(&in, 1)) {
        if((in < '0' || in > '9') && in != '-' && in != '+' && in != '.' && in != 'e' && in != 'E') {
          extra = in;
          break;
        }
        os << in;
      }

      try {
        std::size_t read;
        num = std::stod(os.str(), &read);
        if(read != os.str().length()) {
          throw SyntaxError("Illegal number format.", line, first);
        }
      } catch(std::invalid_argument &e) {
        throw SyntaxError("Failed to parse number", line, first);
      } catch(std::out_of_range &e) {
        throw SyntaxError("Failed to parse number, value out of range", line, first);
      }

      return extra;
    }

    void parseNull(std::istream &stream, int &line) {
      // Read and verify the primitive "null".
      // The first character must have been consumed and verified to be 'n'.
      String v = "n   ";
      if(!stream.read(&v[1], 3) || v != "null") {
        throw SyntaxError("Expected \"null\"", line, v[1]);
      }
    }

    void parseFalse(std::istream &stream, int &line) {
      // Read and verify the primitive "false".
      // The first character must have been consumed and verified to be 'f'.
      String v = "f    ";
      if(!stream.read(&v[1], 4) || v != "false") {
        throw SyntaxError("Expected \"false\"", line, v[1]);
      }
    }

    void parseTrue(std::istream &stream, int &line) {
      // Read and verify the primitive "true".
      // The first character must have been consumed and verified to be 't'.
      String v = "t   ";
      if(!stream.read(&v[1], 3) || v != "true") {
        throw SyntaxError("Expected \"true\"", line, v[1]);
      }
    }

    char parsePrimitive(std::istream &stream, char first, Element &el, int &line) {
      // Read a primitive from stream (null, bool, number, string, not array or object).
      // The first character must have been consumed from stream and passed as "first".
      // This function may consume a character past the end of the primitive,
      // in which case it is returned, otherwise ' ' is returned.

      if(first == 'n') {
        parseNull(stream, line);
        el = Element(Element::NULL_VALUE);
      }
      else if(first == 't') {
        parseTrue(stream, line);
        el = Element(true);
      }
      else if(first == 'f') {
        parseFalse(stream, line);
        el = Element(false);
      }
      else if(first == '"') {
        el = Element(Element::STRING);
        parseString(stream, el.str(), line);
      }
      else if(first == '-' || (first >= '0' && first <= '9')) {
        el = Element(Element::NUMBER);
        return parseNumber(stream, first, el.number(), line);
      }
      else {
        throw SyntaxError("Primitive must be one of null, true, false, number or quoted string.", line, first);
      }

      return ' ';
    }

    void skipLineComment(std::istream &stream, int &line) {
      char in;
      if(!stream.read(&in, 1) || in != '/') {
        throw SyntaxError("Expected second '/' to begin line comment.", line, in);
      }

      while(stream.read(&in, 1)) {
        if(in == '\n' || in == '\r') {
          ++line;
          break;
        }
      }
    }

    std::istream &deserialize(std::istream &stream, Element &root)
    {
      enum State {
          S_PRE_ELEMENT,   // Read an element (root, array item or object value) or close parent array.
          S_PRE_KEY,       // Inside object, read quoted key name or close object.
          S_PRE_SEP,       // Inside object (after key), read ':' before value.
          S_POST_ELEMENT,  // After complete element (key+value if parent is obj). Read comma, close parent array/object or finish.
      };

      State state = S_PRE_ELEMENT;

      // Parents of any element in the stack must not be modified as a reallocation would be very bad.
      std::stack<Element*> nodes;
      nodes.push(&root);
      root = Element::NULL_VALUE;

      // S_PRE_KEY sets this variable to pass the key (for the following value) to S_PRE_ELEMENT.
      String key;

      int line = 1;
      char in;

      while(stream.read(&in, 1)) {
      redo:
        if(in == '/') {
          skipLineComment(stream, line);
          continue;
        }

        // TODO: Respect system's endl for line numbering.
        if(in == '\n' || in == '\r') {
          ++line;
        }

        if(in == ' ' || in == '\r' || in == '\n' || in == '\t') {
          continue;
        }

        switch(state) {

          case S_PRE_KEY: {
            if(in == '"') {
              parseString(stream, key, line);
              state = S_PRE_SEP;
            }
            else if(in == '}') {
              state = S_POST_ELEMENT;
              goto redo;
            }
            else {
              throw SyntaxError("Expected key or closing bracket.", line, in);
            }
          } break;

          case S_PRE_SEP: {
            if(in != ':') {
              throw SyntaxError("Expected ':' separating key and value.", line, in);
            }
            state = S_PRE_ELEMENT;
          } break;

          case S_PRE_ELEMENT: {

            if(nodes.top()->isNull()) {
              // Root element.
            }
            else if(nodes.top()->isArray()) {
              if(in == ']') {
                state = S_POST_ELEMENT;
                goto redo;
              }

              nodes.top()->array().push_back(Element());
              nodes.push(&(nodes.top()->array().back()));
            }
            else if(nodes.top()->isObject()) {
              nodes.top()->object()[key] = Element();
              nodes.push(&(nodes.top()->object()[key]));
            }

            if(in == '[') {
              *nodes.top() = Element(Element::ARRAY);
              state = S_PRE_ELEMENT;
            }
            else if(in == '{') {
              *nodes.top() = Element(Element::OBJECT);
              state = S_PRE_KEY;
            }
            else {
              *nodes.top() = Element(Element::NULL_VALUE);
              in = parsePrimitive(stream, in, *nodes.top(), line);
              nodes.pop();
              state = S_POST_ELEMENT;
              goto redo;
            }
          } break;

          case S_POST_ELEMENT: {
            if(nodes.empty()) {
              throw SyntaxError("Input after end.", line, in);
            }

            if(in == ',') {
              state = nodes.top()->isArray() ? S_PRE_ELEMENT : S_PRE_KEY;
            }
            else if(in == ']') {
              if(!nodes.top()->isArray()) {
                throw SyntaxError("Token ']' is illegal inside object.", line, in);
              }
              nodes.pop();
              state = S_POST_ELEMENT;
            }
            else if(in == '}') {
              if(!nodes.top()->isObject()) {
                throw SyntaxError("Token '}' is illegal inside array.", line, in);
              }
              nodes.pop();
              state = S_POST_ELEMENT;
            }
            else {
                throw SyntaxError("Expected ',' or closing bracket.", line, in);
            }
          } break;

        }
      }

      if(!(state == S_POST_ELEMENT && nodes.empty())) {
        throw SyntaxError("Unexpected end of file.", line, in);
      }

      return stream;
    }

    Element deserialize(const String &str)
    {
      std::istringstream is(str);
      Element el;
      deserialize(is, el);

      return el;
    }

    void serializeString(std::ostream &stream, const Element &node) {
      stream << '"';
      const String &str = node.str();
      for(String::const_iterator it = str.begin(); it != str.end(); ++it) {
          if(*it == '\\') stream << "\\\\";
          else if(*it == '"') stream << "\\\"";
          else if(*it == '\n') stream << "\\n";
          else if(*it == '\r') stream << "\\r";
          else if(*it == '\t') stream << "\\t";
          else if(*it == '\f') stream << "\\f";
          else if(*it == '\b') stream << "\\b";
          else if((*it >= '\x00' && *it <= '\x1f') ||
                  (*it == '\x7f') ||
                  (*it >= '\x80' && *it <= '\x9f') ||
                  (*it >= '\x80' && *it <= '\xff')) {
            // Print control and extended characters as unicode escape sequence.
            int code = reinterpret_cast<const unsigned char&>(*it);
            auto flags = stream.flags();
            stream << "\\u" << std::setfill('0') << std::setw(4) << std::hex << code;
            stream.flags(flags);
          }
          else stream << *it;
        }
      stream << '"';
    }

    std::ostream &serialize(std::ostream &stream, const Element &node, int indent)
    {
      bool compact = indent < 0;

      String thisIndent = String(String::size_type(compact ? 0 : indent), ' ');
      String nextIndent = String(String::size_type(compact ? 0 : (indent + 2)), ' ');

      if(node.getType() == Element::NULL_VALUE) {
        stream << "null";
      }
      else if(node.getType() == Element::BOOLEAN) {
        stream << (node.boolean() ? "true" : "false");
      }
      else if(node.getType() == Element::STRING) {
        serializeString(stream, node);
      }
      else if(node.getType() == Element::NUMBER) {
        const Number &num = node.number();
        if(std::isfinite(num)) stream << num;
        else stream << "null";
      }
      else if(node.getType() == Element::OBJECT) {
	if(node.object().empty()) {
	  stream << "{}";
	}
	else {
	  stream << '{';

	  if(compact) stream << ' ';
	  else stream << std::endl;

	  for(Object::const_iterator child = node.object().begin(); child != node.object().end(); ++child) {
	    stream << nextIndent << '"' << child->first << "\": ";

	    serialize(stream, child->second, compact ? -2 : indent + 2);

	    if(child->first != node.object().rbegin()->first) {
	      stream << ',';
	    }

	    if(compact) stream << ' ';
	    else stream << std::endl;
	  }

	  stream << thisIndent << '}';
	}
      }
      else if(node.getType() == Element::ARRAY) {
	if(node.array().empty()) {
	  stream << "[]";
	}
	else {
	  stream << '[';

          if(compact) stream << ' ';
          else stream << std::endl;

	  for(Array::const_iterator child = node.array().begin(); child != node.array().end(); ++child) {
            if(!compact) {
              stream << nextIndent;
            }

	    serialize(stream, *child, compact ? -2 : indent + 2);

	    if(&*child != &*(node.array().rbegin())) {
	      stream << ',';
	    }

            if(compact) stream << ' ';
            else stream << std::endl;
	  }

          if(!compact)  stream << thisIndent;
          stream << ']';
	}
      }

      if(indent == 0 || indent == -1) {
        stream << std::flush;
      }

      return stream;
    }

    std::ostream &serialize(std::ostream &stream, const Element &node, bool indent)
    {
      return serialize(stream, node, indent ? 0 : -1);
    }

    String serialize(const Element &node, bool indent) {
      std::ostringstream os;
      serialize(os, node, indent ? 0 : -1);
      return os.str();
    }

  }
}
