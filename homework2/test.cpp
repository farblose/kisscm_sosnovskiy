#define BOOST_TEST_MODULE GitIdxParserTest
#include "GitIdxParser.hpp"
#include "GitPackParser.hpp"
#include <boost/test/included/unit_test.hpp>
#include <filesystem>

GitIdxParser test;

BOOST_AUTO_TEST_SUITE(GitIdxParserTestSuite)

BOOST_AUTO_TEST_CASE(TestFindUnixTimestamp_ValidInput) {
    std::string input = "Commit 1234567890";
    int result = test.find_unix_timestamp(input);
    BOOST_CHECK_EQUAL(result, 1234567890);
}

BOOST_AUTO_TEST_CASE(TestFindUnixTimestamp_InvalidInput) {
    std::string input = "Commit abcdefghij";
    int result = test.find_unix_timestamp(input);
    BOOST_CHECK_EQUAL(result, -1);
}

BOOST_AUTO_TEST_CASE(TestBytesToHex) {
    unsigned char bytes[] = {0xDE, 0xAD, 0xBE, 0xEF};
    std::string result = test.bytesToHex(bytes, 4);
    BOOST_CHECK_EQUAL(result, "deadbeef");
}

BOOST_AUTO_TEST_CASE(TestReadExactly_Success) {
    std::ofstream tempFile("test.bin", std::ios::binary);
    char buffer[] = "test";
    tempFile.write(buffer, 4);
    tempFile.close();

    std::ifstream file("test.bin", std::ios::binary);
    char readBuffer[4];
    bool result = test.readExactly(file, readBuffer, 4);

    BOOST_CHECK(result);
    BOOST_CHECK_EQUAL(std::string(readBuffer, 4), "test");

    file.close();
    std::filesystem::remove("test.bin");
}

BOOST_AUTO_TEST_CASE(TestReadExactly_Failure) {
    std::ofstream tempFile("test.bin", std::ios::binary);
    char buffer[] = "abc";
    tempFile.write(buffer, 3);
    tempFile.close();

    std::ifstream file("test.bin", std::ios::binary);
    char readBuffer[4];
    bool result = test.readExactly(file, readBuffer, 4);

    BOOST_CHECK(!result);

    file.close();
    std::filesystem::remove("test.bin");
}

BOOST_AUTO_TEST_CASE(TestParseFile_InvalidFile) {
    GitIdxParser parser;
    bool result = parser.parseFile("non_existent_file.idx");
    BOOST_CHECK(!result);
}

const std::string mockPackPath = "mock.pack";

struct MockGitPackParserFixture {
    MockGitPackParserFixture() : parser(mockPackPath) {}

    GitPackParser parser;
};

BOOST_FIXTURE_TEST_SUITE(GitPackParserTestSuite, MockGitPackParserFixture)

BOOST_AUTO_TEST_CASE(TestConstructor_InvalidPath) {
    BOOST_CHECK_THROW(GitPackParser("invalid_path.pack"), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(TestConstructor_ValidSignatureAndVersion) {
    BOOST_CHECK_NO_THROW(GitPackParser validParser(mockPackPath));
}

BOOST_AUTO_TEST_CASE(TestObjectTypeToString) {
    BOOST_CHECK_EQUAL(parser.objectTypeToString(GitObjectType::COMMIT), "commit");
    BOOST_CHECK_EQUAL(parser.objectTypeToString(GitObjectType::TREE), "tree");
    BOOST_CHECK_EQUAL(parser.objectTypeToString(GitObjectType::BLOB), "blob");
    BOOST_CHECK_EQUAL(parser.objectTypeToString(GitObjectType::TAG), "tag");
    BOOST_CHECK_EQUAL(parser.objectTypeToString(GitObjectType::OFS_DELTA), "ofs-delta");
    BOOST_CHECK_EQUAL(parser.objectTypeToString(GitObjectType::REF_DELTA), "ref-delta");
    BOOST_CHECK_EQUAL(parser.objectTypeToString(static_cast<GitObjectType>(255)), "unknown");
}

BOOST_AUTO_TEST_CASE(TestReadObjectAtOffset_ValidOffset) {
    uint32_t validOffset = 12;
    BOOST_CHECK_NO_THROW(parser.readObjectAtOffset(validOffset));
}

BOOST_AUTO_TEST_CASE(TestReadObjectAtOffset_InvalidOffset) {
    uint32_t invalidOffset = 0;
    BOOST_CHECK_THROW(parser.readObjectAtOffset(invalidOffset), std::runtime_error);
}

BOOST_AUTO_TEST_CASE(TestReadExactly_Success) {
    char buffer[4];
    BOOST_CHECK(parser.readExactly(buffer, 4));
}

BOOST_AUTO_TEST_CASE(TestReadExactly_Failure) {
    char buffer[1000];
    BOOST_CHECK(!parser.readExactly(buffer, 1000));
}

BOOST_AUTO_TEST_CASE(TestReadVariableLengthNumber_Valid) {
    int shift;
    BOOST_CHECK_NO_THROW(uint64_t result = parser.readVariableLengthNumber(shift));
}

BOOST_AUTO_TEST_SUITE_END()
}
















