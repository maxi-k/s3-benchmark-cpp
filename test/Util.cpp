//
// Created by maxi on 17.06.20.
//
#include <s3benchmark/Util.hpp>
#include <gtest/gtest.h>

namespace s3benchmark {

    namespace predicate {
       TEST(find_next, finds_one) {
           char needle[] = "test";
           char haystack[] = "this is a test which is cool"; // test end at pos 14
           char* haystack_ptr = haystack;
           bool found = find_next(&haystack_ptr, haystack + sizeof(haystack) - 1, needle, needle + sizeof(needle) - 1);

           ASSERT_TRUE(found);
           ASSERT_EQ(haystack + 14, haystack_ptr);
           ASSERT_STREQ(" which is cool", haystack_ptr);
       }

       TEST(find_next, finds_at_beginning) {
           char needle[] = "test";
           char haystack[] = "test is a test which is cool"; // first test end at pos 4
           char* haystack_ptr = haystack;
           bool found = find_next(&haystack_ptr, haystack + sizeof(haystack) - 1, needle, needle + sizeof(needle) - 1);

           ASSERT_TRUE(found);
           ASSERT_EQ(haystack + 4, haystack_ptr);
           ASSERT_STREQ(" is a test which is cool", haystack_ptr);
       }

       TEST(find_next, finds_at_end) {
           char needle[] = "test";
           char haystack[] = "this is a example test";
           char* haystack_ptr = haystack;
           bool found = find_next(&haystack_ptr, haystack + sizeof(haystack) - 1, needle, needle + sizeof(needle) - 1);

           ASSERT_TRUE(found);
           ASSERT_EQ(haystack + sizeof(haystack) - 1, haystack_ptr);
           ASSERT_STREQ("", haystack_ptr);
       }

       TEST(find_next, no_segfault) {
           char needle[] = "test";
           char haystack[] = "this is a example tes";
           char* haystack_ptr = haystack;
           bool found = find_next(&haystack_ptr, haystack + sizeof(haystack) - 1, needle, needle + sizeof(needle) - 1);

           ASSERT_FALSE(found);
           ASSERT_EQ(haystack + sizeof(haystack) - 1, haystack_ptr);
       }
}  // namespace predicate

namespace http {

    TEST(skim_http_data, finds_matches) {
        std::string data = "ajhfiayfkj \r\nContent-Length: 15\r\n"
                           + std::string(12, 'a')  + "\r\n\r\n" + std::string("l\0l", 3)
                           + "\r\nContent-Length: 20\r\n"
                           + std::string(5, 'b') + "\r\n";
            auto &&[next_pos, n_matches] = skim_http_data(data.data(), data.length(), 0);

            ASSERT_EQ(2, n_matches);
            ASSERT_EQ(17, next_pos);
    }

}  // namespace http

}  // namespace s3benchmark
