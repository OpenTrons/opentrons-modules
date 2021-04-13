#include <array>
#include <cstring>

#include "catch2/catch.hpp"
#include "heater-shaker/errors.hpp"

SCENARIO("testing error writing") {
    GIVEN("a buffer long enough for an error") {
        auto buffer = std::string(64, 'c');
        WHEN("writing an error into it") {
            auto written = errors::write_into<errors::USBTXBufOverrun>(
                buffer.data(), buffer.size());
            THEN("the error is written into the buffer") {
                REQUIRE_THAT(buffer,
                             Catch::Matchers::StartsWith(
                                 errors::USBTXBufOverrun::errorstring()));
                AND_THEN("the length was appropriately returned") {
                    REQUIRE(written ==
                            strlen(errors::USBTXBufOverrun::errorstring()));
                }
            }
        }
    }
    GIVEN("a buffer too small for an error") {
        auto buffer = std::string(2, 'c');
        WHEN("trying to write an error into it") {
            auto written = errors::write_into<errors::InternalQueueFull>(
                buffer.data(), buffer.size());
            THEN(
                "the error written into only the space available in the "
                "buffer") {
                REQUIRE_THAT(buffer,
                             Catch::Matchers::Equals(std::string("ER")));
                AND_THEN("the amount written is 0") { REQUIRE(written == 2); }
            }
        }
    }
}
