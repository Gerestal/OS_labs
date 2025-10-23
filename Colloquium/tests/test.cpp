#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "Header_functions.h"

TEST_CASE("Testing fact_mod function") {
    SUBCASE("Base case") {
        CHECK(Tasks::fact_mod(0) == 1);
    }

    SUBCASE("Small numbers") {
        CHECK(Tasks::fact_mod(1) == 1);
        CHECK(Tasks::fact_mod(2) == 2);
        CHECK(Tasks::fact_mod(3) == 6);
        CHECK(Tasks::fact_mod(4) == 24);
        CHECK(Tasks::fact_mod(5) == 120);
    }

    SUBCASE("Modulo operation") {
        unsigned long long result = Tasks::fact_mod(50);
        CHECK(result <= ULLONG_MAX);
    }
}

TEST_CASE("Testing first_factorials function") {
    SUBCASE("n = 0") {
        auto result = Tasks::first_factorials(0);
        CHECK(result.empty());
    }

    SUBCASE("n = 1") {
        auto result = Tasks::first_factorials(1);
        REQUIRE(result.size() == 1);
        CHECK(result[0] == 1);
    }

    SUBCASE("n = 5") {
        auto result = Tasks::first_factorials(5);
        REQUIRE(result.size() == 5);
        CHECK(result[0] == 1);  
        CHECK(result[1] == 2);  
        CHECK(result[2] == 6);  
        CHECK(result[3] == 24); 
        CHECK(result[4] == 120); 
    }

    SUBCASE("Correct order") {
        auto result = Tasks::first_factorials(3);
        REQUIRE(result.size() == 3);
        CHECK(result[0] == 1);
        CHECK(result[1] == 2);
        CHECK(result[2] == 6);
    }
}

TEST_CASE("Testing remove_duplicates function") {
    SUBCASE("Empty vector") {
        std::vector<int> empty;
        auto result = Tasks::remove_duplicates(empty);
        CHECK(result.empty());
    }

    SUBCASE("No duplicates") {
        std::vector<int> v = { 1, 2, 3, 4, 5 };
        auto result = Tasks::remove_duplicates(v);
        CHECK(result.size() == 5);
        CHECK(result.find(1) != result.end());
        CHECK(result.find(2) != result.end());
        CHECK(result.find(3) != result.end());
        CHECK(result.find(4) != result.end());
        CHECK(result.find(5) != result.end());
    }

    SUBCASE("With duplicates") {
        std::vector<int> v = { 1, 2, 2, 3, 3, 3, 4, 4, 4, 4 };
        auto result = Tasks::remove_duplicates(v);
        CHECK(result.size() == 4);
        CHECK(result.find(1) != result.end());
        CHECK(result.find(2) != result.end());
        CHECK(result.find(3) != result.end());
        CHECK(result.find(4) != result.end());
    }

    SUBCASE("All same elements") {
        std::vector<int> v = { 5, 5, 5, 5, 5 };
        auto result = Tasks::remove_duplicates(v);
        CHECK(result.size() == 1);
        CHECK(*result.begin() == 5);
    }

    SUBCASE("Negative numbers") {
        std::vector<int> v = { -1, -2, -1, -3, -2 };
        auto result = Tasks::remove_duplicates(v);
        CHECK(result.size() == 3);
        CHECK(result.find(-1) != result.end());
        CHECK(result.find(-2) != result.end());
        CHECK(result.find(-3) != result.end());
    }
}

TEST_CASE("Testing linked list functions") {
    Tasks list;

    SUBCASE("push function") {
        list.push(1);
        list.push(2);
        list.push(3);

        auto vec = list.listToVector();
        REQUIRE(vec.size() == 3);
        CHECK(vec[0] == 1);
        CHECK(vec[1] == 2);
        CHECK(vec[2] == 3);
    }

    SUBCASE("reverseList function") {
        list.push(1);
        list.push(2);
        list.push(3);
        list.push(4);

        list.reverseList();
        auto vec = list.listToVector();

        REQUIRE(vec.size() == 4);
        CHECK(vec[0] == 4);
        CHECK(vec[1] == 3);
        CHECK(vec[2] == 2);
        CHECK(vec[3] == 1);
    }

    SUBCASE("reverseList with one element") {
        list.push(42);
        list.reverseList();
        auto vec = list.listToVector();

        REQUIRE(vec.size() == 1);
        CHECK(vec[0] == 42);
    }

    SUBCASE("reverseList with empty list") {
        list.reverseList();
        auto vec = list.listToVector();
        CHECK(vec.empty());
    }

    SUBCASE("listToVector function") {
        CHECK(list.listToVector().empty());

        list.push(10);
        list.push(20);
        list.push(30);

        auto vec = list.listToVector();
        REQUIRE(vec.size() == 3);
        CHECK(vec[0] == 10);
        CHECK(vec[1] == 20);
        CHECK(vec[2] == 30);
    }

    SUBCASE("Multiple operations") {
        
        list.push(1);
        list.push(2);
        list.reverseList();
        list.push(3);

        auto vec = list.listToVector();
        REQUIRE(vec.size() == 3);
        CHECK(vec[0] == 2);
        CHECK(vec[1] == 1);
        CHECK(vec[2] == 3);
    }
}

TEST_CASE("Edge cases and stress tests") {
    SUBCASE("Large n for first_factorials") {
        auto result = Tasks::first_factorials(10);
        CHECK(result.size() == 10);
       
        for (size_t i = 0; i < result.size(); ++i) {
            CHECK(result[i] > 0); 
        }
    }

    SUBCASE("remove_duplicates with large vector") {
        std::vector<int> large_vec;
        for (int i = 0; i < 1000; ++i) {
            large_vec.push_back(i % 100); 
        }
        auto result = Tasks::remove_duplicates(large_vec);
        CHECK(result.size() == 100); 
    }
}