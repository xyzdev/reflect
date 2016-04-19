#include "../catch.hpp"
#include "reflection.hpp"
#include <map>
#include <vector>
#include <list>

using namespace xyz::json;
using xyz::json::String;
using xyz::core::Reflection;
using xyz::core::ReflectionSink;
using xyz::core::ReflectionSource;
using xyz::core::ReflectionCaller;

namespace {
  class BasicReflectable {
  public:
    void reflect(Reflection &refl) {
      XYZ_REFLECT(refl, integer);
      XYZ_REFLECT(refl, nonsigned);
      XYZ_REFLECT(refl, boolean);
      XYZ_REFLECT(refl, floating);
      XYZ_REFLECT(refl, floatinger);
      XYZ_REFLECT(refl, text);
    }

    int integer;
    unsigned nonsigned;
    bool boolean;
    float floating;
    double floatinger;
    String text;
  };

  class CompositeReflectable {
  public:
    void reflect(Reflection &refl) {
      XYZ_REFLECT(refl, map);
      XYZ_REFLECT(refl, vector);
      XYZ_REFLECT(refl, list);
    }

    std::map<String, String> map;
    std::vector<int> vector;
    std::list<float> list;
  };

  class PropertyReflectable {
  public:
    void reflect(Reflection &refl) {
      xyz::core::reflect_property(refl, *this, &PropertyReflectable::getValue, &PropertyReflectable::setValue, "value");
    }

    void setValue(int v) {
      value = v;
    }

    int getValue() {
      return value;
    }

    int value;
  };

  class ComplexReflectable {
  public:
    void reflect(Reflection &refl) {
      XYZ_REFLECT(refl, basic);
    }

    BasicReflectable basic;
  };

  class JsonReflectable {
  public:
    void reflect(Reflection &refl) {
      XYZ_REFLECT(refl, element);
    }

    xyz::json::Element element;
  };
}

TEST_CASE("Basic reflection (sink)", "[core] [reflection]") {
  // Given:
  BasicReflectable expected;
  expected.integer = -10;
  expected.nonsigned = 53467342;
  expected.boolean = false;
  expected.floating = 3.1415f;
  expected.floatinger = 3.141592654;
  expected.text = "This text should be reflected!";

  // When:
  ReflectionSink sink;
  expected.reflect(sink);

  // Then:
  REQUIRE(sink.sink.isObject());

  Object actual = sink.sink.object();
  REQUIRE(actual["integer"].number() == expected.integer);
  REQUIRE(actual["nonsigned"].number() == expected.nonsigned);
  REQUIRE(actual["boolean"].boolean() == expected.boolean);
  REQUIRE(actual["floating"].number() == expected.floating);
  REQUIRE(actual["floatinger"].number() == expected.floatinger);
  REQUIRE(actual["text"].str() == expected.text);
}

TEST_CASE("Basic reflection (source)", "[core] [reflection]") {
  // Given:
  Object expected;
  expected["integer"] = Number(12958);
  expected["nonsigned"] = Number(7612958);
  expected["boolean"] = Boolean(true);
  expected["floating"] = Number(43246.6654f);
  expected["floatinger"] = Number(475.535723235467436731);
  expected["text"] = "A nice little string w/ some [characters']\"}";

  // When:
  BasicReflectable actual;
  ReflectionSource source(expected);
  actual.reflect(source);

  // Then:
  REQUIRE(expected["integer"].number() == actual.integer);
  REQUIRE(expected["nonsigned"].number() == actual.nonsigned);
  REQUIRE(expected["boolean"].boolean() == actual.boolean);
  REQUIRE(expected["floating"].number() == actual.floating);
  REQUIRE(expected["floatinger"].number() == actual.floatinger);
  REQUIRE(expected["text"].str() == actual.text);
}

TEST_CASE("Basic reflection (bidirectional)", "[core] [reflection]") {
  // Given:
  BasicReflectable expected;
  expected.integer = 10;
  expected.nonsigned = 53467342;
  expected.boolean = false;
  expected.floating = 3.1415f;
  expected.floatinger = 3.141592654;
  expected.text = "This text should be reflected!";

  // When:
  ReflectionSink sink;
  expected.reflect(sink);

  BasicReflectable actual;
  ReflectionSource source(sink.sink);
  actual.reflect(source);

  // Then:
  REQUIRE(actual.integer == expected.integer);
  REQUIRE(actual.nonsigned == expected.nonsigned);
  REQUIRE(actual.boolean == expected.boolean);
  REQUIRE(actual.floating == expected.floating);
  REQUIRE(actual.floatinger == expected.floatinger);
  REQUIRE(actual.text == expected.text);
}

TEST_CASE("Composite reflection (sink)", "[core] [reflection]") {
  // Given:
  CompositeReflectable expected;
  expected.map["one"] = "a";
  expected.map["two"] = "b";
  expected.vector.push_back(1);
  expected.vector.push_back(2);
  expected.list.push_back(1000.5);
  expected.list.push_back(2000.5);

  // When:
  ReflectionSink sink;
  expected.reflect(sink);

  // Then:
  REQUIRE(sink.sink.isObject());

  Object actual = sink.sink.object();
  REQUIRE(actual["map"].isObject());
  REQUIRE(actual["vector"].isArray());
  REQUIRE(actual["list"].isArray());

  REQUIRE(actual["map"].object().size() == 2);
  REQUIRE(actual["vector"].array().size() == 2);
  REQUIRE(actual["list"].array().size() == 2);

  REQUIRE(actual["map"].object()["one"].str() == expected.map["one"]);
  REQUIRE(actual["map"].object()["two"].str() == expected.map["two"]);
  REQUIRE(actual["vector"].array()[0].number() == expected.vector[0]);
  REQUIRE(actual["vector"].array()[1].number() == expected.vector[1]);
  REQUIRE(actual["list"].array()[0].number() == expected.list.front());
  REQUIRE(actual["list"].array()[1].number() == expected.list.back());
}

TEST_CASE("Composite reflection (source)", "[core] [reflection]") {
  // Given:
  Object expected;
  expected["map"] = Object();
  expected["map"].object()["hello"] = "world";
  expected["map"].object()["foo"] = "bar";
  expected["vector"] = Array();
  expected["vector"].array().push_back(Number(42));
  expected["vector"].array().push_back(Number(666));
  expected["list"] = Array();
  expected["list"].array().push_back(Number(42));
  expected["list"].array().push_back(Number(666));

  // When:
  CompositeReflectable actual;
  ReflectionSource source(expected);
  actual.reflect(source);

  // Then:
  REQUIRE(actual.map.size() == 2);
  REQUIRE(actual.vector.size() == 2);
  REQUIRE(actual.list.size() == 2);

  REQUIRE(expected["map"].object()["hello"].str() == actual.map["hello"]);
  REQUIRE(expected["map"].object()["foo"].str() == actual.map["foo"]);
  REQUIRE(expected["vector"].array()[0].number() == actual.vector[0]);
  REQUIRE(expected["vector"].array()[1].number() == actual.vector[1]);
  REQUIRE(expected["list"].array()[0].number() == actual.list.front());
  REQUIRE(expected["list"].array()[1].number() == actual.list.back());
}

TEST_CASE("Composite reflection (bidirectional)", "[core] [reflection]") {
  // Given:
  CompositeReflectable expected;
  expected.map["one"] = "a";
  expected.map["two"] = "b";
  expected.vector.push_back(1);
  expected.vector.push_back(2);
  expected.list.push_back(1000.5);
  expected.list.push_back(2000.5);

  // When:
  ReflectionSink sink;
  expected.reflect(sink);

  CompositeReflectable actual;
  ReflectionSource source(sink.sink);
  actual.reflect(source);

  // Then:
  REQUIRE(actual.map.size() == expected.map.size());
  REQUIRE(actual.vector.size() == expected.vector.size());
  REQUIRE(actual.list.size() == expected.list.size());

  REQUIRE(expected.map["hello"] == actual.map["hello"]);
  REQUIRE(expected.map["foo"] == actual.map["foo"]);
  REQUIRE(expected.vector[0] == actual.vector[0]);
  REQUIRE(expected.vector[1] == actual.vector[1]);
  REQUIRE(expected.list.front() == actual.list.front());
  REQUIRE(expected.list.back() == actual.list.back());
}

TEST_CASE("Property reflection (sink)", "[core] [reflection]") {
  // Given:
  PropertyReflectable expected;
  expected.value = -10;

  // When:
  ReflectionSink sink;
  expected.reflect(sink);

  // Then:
  REQUIRE(sink.sink.isObject());

  Object actual = sink.sink.object();
  REQUIRE(actual["value"].number() == expected.value);
}

TEST_CASE("Property reflection (source)", "[core] [reflection]") {
  // Given:
  Object expected;
  expected["value"] = Number(65654);

  // When:
  PropertyReflectable actual;
  ReflectionSource source(expected);
  actual.reflect(source);

  // Then:
  REQUIRE(expected["value"].number() == actual.value);
}

TEST_CASE("Property reflection (bidirectional)", "[core] [reflection]") {
  // Given:
  PropertyReflectable expected;
  expected.value = 6575;

  // When:
  ReflectionSink sink;
  expected.reflect(sink);

  PropertyReflectable actual;
  ReflectionSource source(sink.sink);
  actual.reflect(source);

  // Then:
  REQUIRE(actual.value == expected.value);
}

TEST_CASE("Complex (recursive) reflection (sink)", "[core] [reflection]") {
  // Given:
  ComplexReflectable expected;
  expected.basic.integer = -10;
  expected.basic.nonsigned = 53467342;
  expected.basic.boolean = false;
  expected.basic.floating = 3.1415f;
  expected.basic.floatinger = 3.141592654;
  expected.basic.text = "This text should be reflected!";

  // When:
  ReflectionSink sink;
  expected.reflect(sink);

  // Then:
  REQUIRE(sink.sink.isObject());
  REQUIRE(sink.sink.object().size() == 1);
  REQUIRE(sink.sink.object()["basic"].isObject());

  Object &actual = sink.sink.object()["basic"].object();
  REQUIRE(actual["integer"].number() == expected.basic.integer);
  REQUIRE(actual["nonsigned"].number() == expected.basic.nonsigned);
  REQUIRE(actual["boolean"].boolean() == expected.basic.boolean);
  REQUIRE(actual["floating"].number() == expected.basic.floating);
  REQUIRE(actual["floatinger"].number() == expected.basic.floatinger);
  REQUIRE(actual["text"].str() == expected.basic.text);
}

TEST_CASE("Json reflection (bidirectional)", "[core] [reflection]") {
  // Given:
  JsonReflectable expected;
  expected.element = Object();
  expected.element.object()["str"] = "xyz";
  expected.element.object()["arr"] = Array();
  expected.element.object()["arr"].array().push_back(Number(1));
  expected.element.object()["arr"].array().push_back(Number(2));

  // When:
  ReflectionSink sink;
  expected.reflect(sink);

  // Then:
  REQUIRE(sink.sink.getType() == Element::OBJECT);
  REQUIRE(sink.sink.object().size() == 1);
  REQUIRE(sink.sink.object()["element"] == expected.element);

  // When:
  JsonReflectable actual;
  ReflectionSource source(sink.sink);
  actual.reflect(source);

  // Then:
  REQUIRE(actual.element == expected.element);
}
