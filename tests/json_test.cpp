#include "nlohmann/json.hpp"

#include <iostream>

using json = nlohmann::json;

int main() {
  // 1. Create a JSON object
  json j;
  j["name"] = "Test";
  j["status"] = "Success";
  j["code"] = 200;
  j["features"] = {"parsing", "serialization", "stl_integration"};

  // 2. Serialize to string and print
  std::cout << "Serialized JSON:\n" << j.dump(4) << "\n\n";

  // 3. Parse a raw string back into JSON
  std::string raw_input = R"({"library": "nlohmann/json", "working": true})";
  json parsed = json::parse(raw_input);

  // 4. Verify values
  if (parsed["working"] == true) {
    std::cout << "SUCCESS: " << parsed["library"] << " is working properly!\n";
  }

  return 0;
}
