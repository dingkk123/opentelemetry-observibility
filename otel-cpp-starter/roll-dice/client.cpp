#include <curl/curl.h>

#include <array>
#include <iostream>
#include <string>

namespace {

struct MenuChoice {
  const char* number;
  const char* item_id;
  const char* display_name;
};

constexpr std::array<MenuChoice, 3> kMenuChoices = {{
    {"1", "coffee", "Coffee"},
    {"2", "latte", "Latte"},
    {"3", "dessert", "Dessert"},
}};

std::size_t DiscardResponse(char*, std::size_t size, std::size_t count,
                            void*) {
  return size * count;
}

const MenuChoice* FindChoice(const std::string& input) {
  for (const auto& choice : kMenuChoices) {
    if (input == choice.number) {
      return &choice;
    }
  }
  return nullptr;
}

bool SendOrder(const MenuChoice& choice) {
  CURL* curl = curl_easy_init();
  if (curl == nullptr) {
    std::cerr << "Unable to initialize the HTTP client.\n";
    return false;
  }

  const std::string url =
      "http://localhost:8080/order/" + std::string(choice.item_id);
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, DiscardResponse);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

  const CURLcode result = curl_easy_perform(curl);
  long status_code = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
  curl_easy_cleanup(curl);

  if (result != CURLE_OK) {
    std::cerr << "Order failed: " << curl_easy_strerror(result) << '\n';
    return false;
  }
  if (status_code != 200) {
    std::cerr << "Order failed with HTTP status " << status_code << ".\n";
    return false;
  }

  std::cout << choice.display_name << " complete\n";
  return true;
}

void PrintMenu() {
  std::cout << "\n=== Coffee Shop Menu ===\n"
            << "1. Coffee\n"
            << "2. Latte\n"
            << "3. Dessert\n"
            << "0. Exit\n"
            << "Select an item: ";
}

}  // namespace

int main() {
  if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
    std::cerr << "Unable to initialize libcurl.\n";
    return 1;
  }

  std::string input;
  while (true) {
    PrintMenu();
    if (!std::getline(std::cin, input) || input == "0") {
      break;
    }

    const MenuChoice* choice = FindChoice(input);
    if (choice == nullptr) {
      std::cout << "Invalid selection.\n";
      continue;
    }

    SendOrder(*choice);
  }

  curl_global_cleanup();
  return 0;
}
