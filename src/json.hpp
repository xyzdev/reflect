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
#ifndef XYZDEV_JSON_HPP
#define XYZDEV_JSON_HPP

#include <vector>
#include <map>
#include <string>
#include <exception>

namespace xyz {
  namespace json {

    class Element;

    typedef std::string String;
    typedef std::map<String, Element> Object;
    typedef std::vector<Element> Array;
    typedef bool Boolean;
    typedef double Number;

    class Element {
    public:
      enum Type { NULL_VALUE = 0, OBJECT, ARRAY, STRING, NUMBER, BOOLEAN };

      Element():type(NULL_VALUE) {}
      Element(const Element &r):type(NULL_VALUE) { *this = r; }
      Element(Type type):type(type) {}
      Element(const Object &object):type(OBJECT),_object(object) {}
      Element(const Array &array):type(ARRAY),_array(array) {}
      Element(bool boolean):type(BOOLEAN),_boolean(boolean) {}
      Element(Number number):type(NUMBER),_number(number) {}
      Element(const String &str):type(STRING),_string(str) {}
      Element(const char *str):type(STRING),_string(str) {}

      Type getType() const;
      const char *getTypeName() const;

      bool empty() const {
        return isNull() ||
               (isBoolean() && !boolean()) ||
               (isNumber() && !number()) ||
               (_object.empty() && _array.empty() && _string.empty());
      }

      bool isPrimitive() const { return type != OBJECT && type != ARRAY; }

      bool isNull() const { return type == NULL_VALUE; }
      bool isObject() const { return type == OBJECT; }
      bool isArray() const { return type == ARRAY; }
      bool isString() const { return type == STRING; }
      bool isNumber() const { return type == NUMBER; }
      bool isBoolean() const { return type == BOOLEAN; }

      Object &object();
      Array &array();
      String &str();
      Boolean &boolean();
      Number &number();

      const Object &object() const;
      const Array &array() const;
      const String &str() const;
      const Boolean &boolean() const;
      const Number &number() const;

      String toString() const;

      void swap(Element &r);

      Element &operator =(const Element &r);

      bool operator ==(const Element &r) const;

    protected:
      Type type;
      Object _object;
      Array _array;
      String _string;
      Number _number;
      Boolean _boolean;
    };

    class TypeError: public std::exception {
    public:
      TypeError()
        :msg("TypeError"),
         expected(Element::NULL_VALUE) {}

      TypeError(const char *msg)
        :msg(msg),
         expected(Element::NULL_VALUE) {}

      TypeError(Element::Type expected)
        :msg(nullptr),
         expected(expected) {}

      const char *what() const throw() {
        if(msg) return msg;

        if(expected >= 0 && expected <= 5) {
          static const char *typeNames[] = {
            "TypeError: Expected NULL",
            "TypeError: Expected OBJECT",
            "TypeError: Expected ARRAY",
            "TypeError: Expected STRING",
            "TypeError: Expected NUMBER",
            "TypeError: Expected BOOLEAN"
          };
          return typeNames[expected];
        }

        return "TypeError: Expected illegal type";
      }

      const char * const msg;
      const Element::Type expected;
    };

    class SyntaxError: public std::exception {
    public:
        SyntaxError(const char *msg, int line, char chr):
          msg(msg),line(line),chr(chr)
        {}

        const char *what() const throw() {
          return msg;
        }

        const char *msg;
        int line;
        char chr;
    };

    String serialize(const Element &node, bool indent = false);
    std::ostream &serialize(std::ostream &stream, const Element &node, bool indent = false);

    Element deserialize(const String &str);
    std::istream &deserialize(std::istream &stream, Element &element);
  }
}

#endif
