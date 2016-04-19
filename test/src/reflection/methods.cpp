#include "../catch.hpp"
#include "reflection.hpp"
#include <map>

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

  class JsonReflectable {
  public:
      void reflect(Reflection &refl) {
        XYZ_REFLECT(refl, element);
      }

      xyz::json::Element element;
  };

  class MethodReflectable {
  public:
      void reflect(Reflection &refl) {
        XYZ_REFLECT(refl, methVal1);
        XYZ_REFLECT(refl, methVal2);
        XYZ_REFLECT_METHOD(refl, MethodReflectable, noArgs);
        XYZ_REFLECT_METHOD(refl, MethodReflectable, method);
        XYZ_REFLECT_METHOD(refl, MethodReflectable, method2);
        XYZ_REFLECT_METHOD(refl, MethodReflectable, echo);
      }

      String noArgs() { return "no args"; }
      void method(int arg) { methVal1 = arg; }
      void method2(int arg1, const String &arg2) { methVal1 = arg1; methVal2 = arg2; }
      int echo(int arg) { return arg; }
      int methVal1;
      String methVal2;
  };

  class CompositeMethodReflectable {
  public:
      void reflect(Reflection &refl) {
        XYZ_REFLECT_METHOD(refl, CompositeMethodReflectable, compositeArg);
      }

      String compositeArg(std::map<String, String> arg) { return arg["text"]; }
  };

  class ComplexMethodReflectable {
  public:
      void reflect(Reflection &refl) {
        XYZ_REFLECT_METHOD(refl, ComplexMethodReflectable, complexArg);
      }

      String complexArg(BasicReflectable arg) { return arg.text; }
  };

  class SimpleClassWithMethod {
  public:
      String complexArg(BasicReflectable arg) { return arg.text; }
  };

  class DerivedComplexMethodReflectable: public SimpleClassWithMethod {
  public:
      void reflect(Reflection &refl) {
        XYZ_REFLECT_METHOD(refl, SimpleClassWithMethod, complexArg);
      }
  };
}

TEST_CASE("Method reflection (call)", "[core] [reflection]") {
  // Given:
  MethodReflectable reflectable;
  reflectable.methVal1 = 0;

  // When:
  Array args;
  args.push_back(Element(5.0f));
  ReflectionCaller caller("method", args);
  reflectable.reflect(caller);

  // Then:
  REQUIRE(caller.found);
  REQUIRE(caller.result.isNull());
  REQUIRE(reflectable.methVal1 == 5);
}

TEST_CASE("Method reflection multiple parameters (call)", "[core] [reflection]") {
  // Given:
  MethodReflectable reflectable;
  reflectable.methVal1 = 0;
  reflectable.methVal2 = "";

  // When:
  Array args;
  args.push_back(Element(10.0f));
  args.push_back(Element("Sea shells she sells."));
  ReflectionCaller caller("method2", args);
  reflectable.reflect(caller);

  // Then:
  REQUIRE(caller.found);
  REQUIRE(caller.result.isNull());
  REQUIRE(reflectable.methVal1 == 10);
  REQUIRE(reflectable.methVal2 == "Sea shells she sells.");
}

TEST_CASE("Method reflection no parameters (call)", "[core] [reflection]") {
  // Given:
  MethodReflectable reflectable;

  // When:
  Array args;
  ReflectionCaller caller("noArgs", args);
  reflectable.reflect(caller);

  // Then:
  REQUIRE(caller.found);
  REQUIRE(caller.result.isString());
  REQUIRE(caller.result.str() == "no args");
}

TEST_CASE("Method reflection with return value (call)", "[core] [reflection]") {
  // Given:
  MethodReflectable reflectable;

  // When:
  Array args;
  args.push_back(Element(11.0f));
  ReflectionCaller caller("echo", args);
  reflectable.reflect(caller);

  // Then:
  REQUIRE(caller.found);
  REQUIRE(caller.result.isNumber());
  REQUIRE(caller.result.number() == 11);
}

TEST_CASE("Method reflection with composite argument (call)", "[core] [reflection]") {
  // Given:
  CompositeMethodReflectable reflectable;

  // When:
  Object arg;
  arg["text"] = "hello";
  Array args;
  args.push_back(arg);
  ReflectionCaller caller("compositeArg", args);
  reflectable.reflect(caller);

  // Then:
  REQUIRE(caller.found);
  REQUIRE(caller.result.isString());
  REQUIRE(caller.result.str() == "hello");
}

TEST_CASE("Method reflection with complex argument (call)", "[core] [reflection]") {
  // Given:
  ComplexMethodReflectable reflectable;

  // When:
  Object arg;
  arg["text"] = "hello";
  Array args;
  args.push_back(arg);
  ReflectionCaller caller("complexArg", args);
  reflectable.reflect(caller);

  // Then:
  REQUIRE(caller.found);
  REQUIRE(caller.result.isString());
  REQUIRE(caller.result.str() == "hello");
}

TEST_CASE("Inherited method reflection with complex argument (call)", "[core] [reflection]") {
  // Given:
  DerivedComplexMethodReflectable reflectable;

  // When:
  Object arg;
  arg["text"] = "hello";
  Array args;
  args.push_back(arg);
  ReflectionCaller caller("complexArg", args);
  reflectable.reflect(caller);

  // Then:
  REQUIRE(caller.found);
  REQUIRE(caller.result.isString());
  REQUIRE(caller.result.str() == "hello");
}

TEST_CASE("Method reflection (sink)", "[core] [reflection]") {
  // Given:
  MethodReflectable reflectable;

  // When:
  ReflectionSink sink;
  sink.methods = true;
  reflectable.reflect(sink);

  // Then:
  REQUIRE(sink.sink.isObject());
  REQUIRE(sink.sink.object().size() == 4);
  REQUIRE(sink.sink.object()["method"].isArray());
  REQUIRE(sink.sink.object()["method"].array().size() == 2);
  REQUIRE(sink.sink.object()["method"].array()[0] == Element("func"));
  REQUIRE(sink.sink.object()["method"].array()[1] == Element("NUMBER"));
  REQUIRE(sink.sink.object()["method2"].isArray());
  REQUIRE(sink.sink.object()["method2"].array().size() == 3);
  REQUIRE(sink.sink.object()["method2"].array()[0] == Element("func"));
  REQUIRE(sink.sink.object()["method2"].array()[1] == Element("NUMBER"));
  REQUIRE(sink.sink.object()["method2"].array()[2] == Element("STRING"));
  REQUIRE(sink.sink.object()["noArgs"].isArray());
  REQUIRE(sink.sink.object()["noArgs"].array().size() == 1);
  REQUIRE(sink.sink.object()["noArgs"].array()[0] == Element("func"));
  REQUIRE(sink.sink.object()["echo"].isArray());
  REQUIRE(sink.sink.object()["echo"].array().size() == 2);
  REQUIRE(sink.sink.object()["echo"].array()[0] == Element("func"));
  REQUIRE(sink.sink.object()["echo"].array()[1] == Element("NUMBER"));
}

TEST_CASE("Reflect and ignore method (sink)", "[core] [reflection]") {
  // Given:
  MethodReflectable reflectable;

  // When:
  ReflectionSink sink;
  reflectable.reflect(sink);

  // Then:
  REQUIRE(sink.sink.isObject());
  REQUIRE(sink.sink.object().size() == 2);
  REQUIRE(sink.sink.object()["methVal1"].isNumber());
  REQUIRE(sink.sink.object()["methVal2"].isString());
}

TEST_CASE("Reflect and ignore method (source)", "[core] [reflection]") {
  // Given:
  MethodReflectable reflectable;

  // When:
  ReflectionSource source;
  reflectable.reflect(source);

  // Then:
  // No exception, passed.
}
