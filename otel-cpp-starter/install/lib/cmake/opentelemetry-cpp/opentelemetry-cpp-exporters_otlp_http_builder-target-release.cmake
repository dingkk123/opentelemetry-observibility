#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "opentelemetry-cpp::otlp_http_exporter_builder" for configuration "Release"
set_property(TARGET opentelemetry-cpp::otlp_http_exporter_builder APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(opentelemetry-cpp::otlp_http_exporter_builder PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopentelemetry_exporter_otlp_http_builder.a"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::otlp_http_exporter_builder )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::otlp_http_exporter_builder "${_IMPORT_PREFIX}/lib/libopentelemetry_exporter_otlp_http_builder.a" )

# Import target "opentelemetry-cpp::otlp_http_log_record_exporter_builder" for configuration "Release"
set_property(TARGET opentelemetry-cpp::otlp_http_log_record_exporter_builder APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(opentelemetry-cpp::otlp_http_log_record_exporter_builder PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopentelemetry_exporter_otlp_http_log_builder.a"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::otlp_http_log_record_exporter_builder )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::otlp_http_log_record_exporter_builder "${_IMPORT_PREFIX}/lib/libopentelemetry_exporter_otlp_http_log_builder.a" )

# Import target "opentelemetry-cpp::otlp_http_metric_exporter_builder" for configuration "Release"
set_property(TARGET opentelemetry-cpp::otlp_http_metric_exporter_builder APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(opentelemetry-cpp::otlp_http_metric_exporter_builder PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libopentelemetry_exporter_otlp_http_metric_builder.a"
  )

list(APPEND _cmake_import_check_targets opentelemetry-cpp::otlp_http_metric_exporter_builder )
list(APPEND _cmake_import_check_files_for_opentelemetry-cpp::otlp_http_metric_exporter_builder "${_IMPORT_PREFIX}/lib/libopentelemetry_exporter_otlp_http_metric_builder.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
