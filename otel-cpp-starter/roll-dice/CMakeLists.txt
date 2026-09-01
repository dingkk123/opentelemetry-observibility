cmake_minimum_required(VERSION 3.25)

project(CoffeeShop)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(dice-server main.cpp)
add_executable(coffee-client client.cpp)

find_package(oatpp REQUIRED)
find_package(CURL REQUIRED)

find_package(
    opentelemetry-cpp
    CONFIG
    REQUIRED
    COMPONENTS
      api
      sdk
      exporters_otlp_http
)

target_link_libraries(
    dice-server
    PRIVATE
      oatpp::oatpp
      opentelemetry-cpp::api
      opentelemetry-cpp::trace
      opentelemetry-cpp::logs
      opentelemetry-cpp::metrics
      opentelemetry-cpp::resources
      opentelemetry-cpp::otlp_http_exporter
      opentelemetry-cpp::otlp_http_log_record_exporter
      opentelemetry-cpp::otlp_http_metric_exporter
)

target_link_libraries(
    coffee-client
    PRIVATE
      CURL::libcurl
)
