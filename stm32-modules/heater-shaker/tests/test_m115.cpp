#include <array>

#include "catch2/catch.hpp"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#include "heater-shaker/gcodes.hpp"
#pragma GCC diagnostic pop

SCENARIO("GetVersion (M115) response works", "[gcode][parse][m115]") {
    GIVEN("a response buffer large enough for the formatted response") {
        std::string buffer(64, 'c');
        WHEN("filling response") {
            auto written = gcode::GetVersion::write_response_into(
                buffer.begin(), buffer.end(), "hello", "world");
            THEN("the response should be written in full") {
                std::string ok = "M115 FW:hello HW:world OK\n";
                REQUIRE_THAT(buffer, Catch::Matchers::StartsWith(ok));
                REQUIRE(written == buffer.begin() + ok.size());
                std::string suffix(buffer.size() - ok.size(), 'c');
                REQUIRE_THAT(buffer, Catch::Matchers::EndsWith(suffix));
            }
        }
    }

    GIVEN("a response buffer not large enough for the formatted response") {
        std::string buffer(32, 'c');
        WHEN("filling response") {
            auto written = gcode::GetVersion::write_response_into(
                buffer.begin(), buffer.begin() + 16, "hello", "world");
            THEN("the response should write only up to the available space") {
                std::string response = "M115 FW:hello HWcccccccccccccccc";
                REQUIRE_THAT(buffer, Catch::Matchers::Equals(response));
                REQUIRE(written == buffer.begin() + 16);
            }
        }
    }
}
