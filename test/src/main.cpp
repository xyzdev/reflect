#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("Test CATCH", "[CATCH]") {
  REQUIRE(1 == 1);
  REQUIRE(1 != 0);
}
