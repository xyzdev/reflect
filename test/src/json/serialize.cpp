#include "../catch.hpp"
#include "json.hpp"

using namespace xyz::json;

TEST_CASE("Serialize primitives", "[core] [json]") {
  REQUIRE(serialize(Element(Element::NULL_VALUE), true) == "null");;
  REQUIRE(serialize(Element(Element::NULL_VALUE), false) == "null");
  REQUIRE(serialize(Element(Number(5.0)), true) == "5");
  REQUIRE(serialize(Element(Number(5.0)), false) == "5");
  REQUIRE(serialize(Element(Number(-5.5)), true) == "-5.5");
  REQUIRE(serialize(Element(Number(-5.5)), false) == "-5.5");
  REQUIRE(serialize(Element("<\" \\>"), true) == "\"<\\\" \\\\>\"");
  REQUIRE(serialize(Element("<\" \\>"), false) == "\"<\\\" \\\\>\"");
  REQUIRE(serialize(Element("\xc4"), false) == "\"\\u00c4\"");
  REQUIRE(serialize(Element(true), true) == "true");
  REQUIRE(serialize(Element(true), false) == "true");
  REQUIRE(serialize(Element(false), true) == "false");
  REQUIRE(serialize(Element(false), false) == "false");
}

TEST_CASE("Serialize array", "[core] [json]") {
  Array ar;
  ar.push_back(Element(Element::NULL_VALUE));
  ar.push_back(Element("str"));
  ar.push_back(Element(Object()));
  ar.push_back(Element(false));
  ar.push_back(Element(Number(5.0)));
  Element el(ar);

  REQUIRE(serialize(el, true) == "[\n  null,\n  \"str\",\n  {},\n  false,\n  5\n]");
  REQUIRE(serialize(el, false) == "[ null, \"str\", {}, false, 5 ]");
}

TEST_CASE("Serialize object", "[core] [json]") {
  Object obj;
  obj["str"] = "foo";
  Element el(obj);

  REQUIRE(serialize(el, true) == "{\n  \"str\": \"foo\"\n}");
  REQUIRE(serialize(el, false) == "{ \"str\": \"foo\" }");

  obj.clear();
  obj["bool"] = true;
  el = Element(obj);

  REQUIRE(serialize(el, true) == "{\n  \"bool\": true\n}");
  REQUIRE(serialize(el, false) == "{ \"bool\": true }");

  obj.clear();
  obj["arr"] = Array();
  el = Element(obj);

  REQUIRE(serialize(el, true) == "{\n  \"arr\": []\n}");
  REQUIRE(serialize(el, false) == "{ \"arr\": [] }");
}

// TODO: Negative tests.

TEST_CASE("Deserialize empty", "[core] [json]") {
  REQUIRE(serialize(deserialize("{}")) == "{}");
  REQUIRE(serialize(deserialize("[]")) == "[]");
  REQUIRE(serialize(deserialize("{ }")) == "{}");
  REQUIRE(serialize(deserialize("[ ]")) == "[]");
  REQUIRE(serialize(deserialize("{  }")) == "{}");
  REQUIRE(serialize(deserialize("[  ]")) == "[]");
  REQUIRE(serialize(deserialize("\"\"")) == "\"\"");
}

TEST_CASE("Deserialize ignore whitespace", "[core] [json]") {
  REQUIRE(serialize(deserialize(" {}")) == "{}");
  REQUIRE(serialize(deserialize("  {}")) == "{}");
  REQUIRE(serialize(deserialize("{} ")) == "{}");
  REQUIRE(serialize(deserialize("{}  ")) == "{}");
  REQUIRE(serialize(deserialize("  {    \"\"    :    [    ]  }  ")) == "{ \"\": [] }");
  REQUIRE(serialize(deserialize("{\"\":[]}")) == "{ \"\": [] }");
  REQUIRE(serialize(deserialize("  [ 1,2, 3  , 4] ")) == "[ 1, 2, 3, 4 ]");
}

TEST_CASE("Deserialize object", "[core] [json]") {
  REQUIRE(serialize(deserialize("{\"a\":1}")) == "{ \"a\": 1 }");
  REQUIRE(serialize(deserialize("{\"a\":[1]}")) == "{ \"a\": [ 1 ] }");
}

TEST_CASE("Deserialize string", "[core] [json]") {
  REQUIRE(deserialize("\"<\\\" \\\\>\"") == Element("<\" \\>"));
  REQUIRE(deserialize("\"\\u0000\"") == Element(String(1, '\0')));
  REQUIRE(deserialize("\"\\u00c4\"") == Element(String(1, '\xc4')));
  REQUIRE(deserialize("\"\\u00C4\"") == Element(String(1, '\xc4')));
  REQUIRE(deserialize("\"\xc4\"") == Element(String(1, '\xc4')));

  try {
    deserialize("\"\\u0100\"");

    FAIL("Expected exception on multi-byte escape sequence");
  }
  catch (SyntaxError e) {
    REQUIRE(String(e.msg) == "Escape sequence above Latin-1 not implemented.");
    REQUIRE(e.line == 1);
  }

  try {
    deserialize(String("\"") + String(1, '\x80') + String("\""));

    FAIL("Expected exception on control character in string");
  }
  catch (SyntaxError e) {
    REQUIRE(String(e.msg) == "Control character in string.");
    REQUIRE(e.line == 1);
  }

  try {
    deserialize(String("\"\n\""));

    FAIL("Expected exception on newline in string");
  }
  catch (SyntaxError e) {
    REQUIRE(String(e.msg) == "Control character in string.");
    REQUIRE(e.line == 1);
  }

  try {
    deserialize(String("\"\r\""));

    FAIL("Expected exception on carriage return in string");
  }
  catch (SyntaxError e) {
    REQUIRE(String(e.msg) == "Control character in string.");
    REQUIRE(e.line == 1);
  }
}

TEST_CASE("Deserialize number", "[core] [json]") {
  REQUIRE(deserialize("0") == Element(Number(0)));
  REQUIRE(deserialize("0.0") == Element(Number(0)));
  REQUIRE(deserialize("0.1") == Element(Number(0.1)));
  REQUIRE(deserialize("10") == Element(Number(10)));
  REQUIRE(deserialize("-0") == Element(Number(0)));
  REQUIRE(deserialize("-0.0") == Element(Number(0)));
  REQUIRE(deserialize("-123.0") == Element(Number(-123)));
  REQUIRE(deserialize("12e-1") == Element(Number(1.2)));
}

TEST_CASE("Deserialize line comment", "[core] [json]") {
  Array ar;
  ar.push_back(Element(Element::NULL_VALUE));
  REQUIRE(deserialize("// x\n [ null//\n]//") == Element(ar));
}
