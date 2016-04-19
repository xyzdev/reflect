#include "../catch.hpp"
#include "reflection.hpp"

using xyz::core::TypeId;
using xyz::core::type_id;

namespace {

  struct XType {
      TypeId getType() {
        return type_id<XType>();
      }
  };

  struct YType {
      TypeId getType() {
        return type_id<YType>();
      }
  };

}

TEST_CASE("Static type id", "[core] [type_id]") {
  REQUIRE(type_id<XType>() != 0);
  REQUIRE(type_id<YType>() != 0);

  REQUIRE(type_id<XType>() == type_id<XType>());
  REQUIRE(type_id<YType>() == type_id<YType>());
  REQUIRE(type_id<XType>() != type_id<YType>());

  XType x;
  YType y;

  REQUIRE(x.getType() == type_id<XType>());
  REQUIRE(y.getType() == type_id<YType>());
  REQUIRE(x.getType() != type_id<YType>());
  REQUIRE(y.getType() != type_id<XType>());

  REQUIRE(x.getType() == x.getType());
  REQUIRE(y.getType() == y.getType());
  REQUIRE(x.getType() != y.getType());
}
