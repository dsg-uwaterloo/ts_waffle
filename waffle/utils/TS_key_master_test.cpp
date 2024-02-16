//
// Created by Peter Pan on 2/11/2024.
//

#include <cassert>
#include <iostream>
#include "TS_key_master.h"
#include "TS_value_master.h"
#include "utils.h"
static void test_keyGenerationDecomposition() {
    ItemIdGenerator item_id_generator;
    std::string item_id = item_id_generator.generate_item_ids(1)[0];
    std::string user_id = "user_id";
    std::string timestamp = std::to_string(UNIX_TIMESTAMP::current_time());
    KeyGenerationDecomposition keyGenerationDecomposition(item_id, user_id, timestamp);
    std::cout << keyGenerationDecomposition.key << std::endl;
    assert(keyGenerationDecomposition.item_id == item_id);
    assert(keyGenerationDecomposition.user_id.substr(keyGenerationDecomposition.user_id.length() - user_id.length(), user_id.length()) == user_id);
    assert(keyGenerationDecomposition.timestamp == timestamp);
    KeyGenerationDecomposition keyGenerationDecomposition2(keyGenerationDecomposition.key);
    assert(keyGenerationDecomposition2.item_id == item_id);
    assert(keyGenerationDecomposition.user_id.substr(keyGenerationDecomposition.user_id.length() - user_id.length(), user_id.length()) == user_id);
    assert(keyGenerationDecomposition2.timestamp == timestamp);
    KeyGenerationDecomposition keyGenerationDecomposition3(item_id, user_id);
    assert(keyGenerationDecomposition3.item_id == item_id);
    assert(keyGenerationDecomposition.user_id.substr(keyGenerationDecomposition.user_id.length() - user_id.length(), user_id.length()) == user_id);
    assert(keyGenerationDecomposition3.timestamp == keyGenerationDecomposition3.current_time());
    std::cout << DataType::get_data_type(keyGenerationDecomposition3.key) << std::endl;
}

static void test_ItemIdGenerator() {
    ItemIdGenerator item_id_generator;
    std::vector<std::string> item_ids1 = item_id_generator.generate_item_ids(10);
    std::vector<std::string> item_ids2 = item_id_generator.generate_item_ids(10);
    for (int i = 0; i < item_ids1.size() ; i++) {
        std::cout << item_ids1[i] << std::endl;
        std::cout << item_ids2[i] << std::endl;
        assert(item_ids1[i] != item_ids2[i]);
    }
}

int main() {
    test_keyGenerationDecomposition();
    test_ItemIdGenerator();
    return 0;
}