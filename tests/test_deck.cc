#include "deck.h"
#include "log.h"

#include "support/catch.hpp"

SCENARIO("deck can be manipulated safely", "[deck]") {
    GIVEN("an initial deck") {
        Deck d;

        REQUIRE(d.size() == NUM_CARDS);
        REQUIRE(d.discard.empty());
        REQUIRE(d.isConsistent());

        WHEN("deck is cloned") {
            auto clone = d.clone();
            THEN("the clone is identical") {
                REQUIRE(*clone == d);
                REQUIRE(clone->isConsistent());
            }
        }

        WHEN("cloned deck is shuffled") {
            auto clone = d.clone();
            clone->shuffle();
            THEN("the clone is no longer identical") {
                REQUIRE(*clone != d);
                REQUIRE(clone->isConsistent());
            }
        }

        WHEN("a character is drawn") {
            auto start = d.size();
            auto cstart = d.draw.characters.size();
            d.drawCharacter();
            THEN("the deck is smaller") {
                REQUIRE(d.size()+1 == start);
                REQUIRE(d.draw.characters.size()+1 == cstart);
                //REQUIRE(d.isConsistent()); <-- deck is missing cards
            }
        }

        WHEN("a character is discarded") {
            auto start = d.draw.characters.size();
            auto c = d.drawCharacter();
            d.discardCard(Card::Get(c));
            THEN("the deck is consistent") {
                REQUIRE(d.draw.characters.size()+1 == start);
                REQUIRE(d.discard.characters.size() == 1);
                REQUIRE(d.isConsistent());
            }
        }

        WHEN("deck is shuffled after discard") {
            auto start = d.draw.characters.size();
            auto c = d.drawCharacter();
            d.discardCard(Card::Get(c));
            d.shuffle();
            THEN("the deck is consistent") {
                REQUIRE(d.draw.size() == NUM_CARDS);
                REQUIRE(d.discard.size() == 0);
                REQUIRE(d.isConsistent());
            }
        }
    }
}
