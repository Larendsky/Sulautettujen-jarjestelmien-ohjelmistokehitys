#include <gtest/gtest.h>
#include "TimeParser.h"

// 1. Perustesti: Oikea aika (korjattu nollasta oikeaksi sekuntimääräksi)
TEST(TimeParserTest, TestCaseCorrectTime) {
    char time_test[] = "141205";
    // 14 tuntia, 12 minuuttia ja 5 sekuntia = 51125 sekuntia
    ASSERT_EQ(time_parse(time_test), 51125);
}

// 2. Lisätesti: Kellonaika täsmälleen keskiyöllä
TEST(TimeParserTest, TestCaseMidnight) {
    char time_test[] = "000000";
    // Nolla sekuntia (jos koodisi sallii nollan, muuten laita tähän TIME_ZERO_ERROR)
    ASSERT_EQ(time_parse(time_test), 0);
}

// 3. Lisätesti: Liian suuret tunnit (yli 23)
TEST(TimeParserTest, TestCaseHoursTooHigh) {
    char time_test[] = "240000";
    ASSERT_EQ(time_parse(time_test), TIME_VALUE_ERROR);
}

// 4. Lisätesti: Liian suuret minuutit (yli 59)
TEST(TimeParserTest, TestCaseMinutesTooHigh) {
    char time_test[] = "126000";
    ASSERT_EQ(time_parse(time_test), TIME_VALUE_ERROR);
}

// 5. Lisätesti: Väärän pituinen merkkijono (alle 6 merkkiä)
TEST(TimeParserTest, TestCaseTooShort) {
    char time_test[] = "12345";
    ASSERT_EQ(time_parse(time_test), TIME_LEN_ERROR);
}

// 6. Lisätesti: Kirjaimia numeroiden seassa
TEST(TimeParserTest, TestCaseInvalidCharacters) {
    char time_test[] = "12A456";
    ASSERT_EQ(time_parse(time_test), TIME_DIGIT_ERROR);
}
