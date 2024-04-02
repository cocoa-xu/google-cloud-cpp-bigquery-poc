cmake_minimum_required(VERSION 3.10...3.24)
project(google-cloud-cpp-bigquery-poc CXX)

if(NOT DEFINED CMAKE_BUILD_TYPE OR "${CMAKE_BUILD_TYPE}" STREQUAL "")
    set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT DEFINED BIGQUERY_INSTALL_PREFIX OR "${BIGQUERY_INSTALL_PREFIX}" STREQUAL "")
    set(BIGQUERY_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install")
endif()

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()

set(BIGQUERY_VERSION_WITH_HASH "2.22.0")
set(BIGQUERY_HASH_X86_64_APPLE_DARWIN "5dec87b4829a3553927dada827144f11909c37deb2c3e8994d2944e739ac4b35")
set(BIGQUERY_HASH_AARCH64_APPLE_DARWIN "fe8bcb8d0d712caa0f80a60720c9eef629d9f04e0f422a3a2f103b9eaa6b0f12")
set(BIGQUERY_HASH_X86_64_LINUX_GNU "d842947d55e808320163f42a031056b6fb18381f41c14ed207f3a8268459f955")
set(BIGQUERY_HASH_X86_64_WINDOWS_MSVC "c59b8116dd6df8bb04d324198eb4792911adc122966858d7f563db7175d999c3")

if(NOT DEFINED BIGQUERY_VERSION OR "${BIGQUERY_VERSION}" STREQUAL "")
    set(BIGQUERY_VERSION "${BIGQUERY_VERSION_WITH_HASH}")
endif()
set(PRECOMPILED_BIGQUERY_BASE_URL "https://github.com/cocoa-xu/google-cloud-cpp-bigquery/releases/download/v${BIGQUERY_VERSION}")

if(UNIX)
    if(APPLE)
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
            set(PRECOMPILED_BIGQUERY_TARBALL_FILENAME "bigquery-${BIGQUERY_VERSION}-x86_64-apple-darwin.tar.gz")
            if(BIGQUERY_VERSION STREQUAL BIGQUERY_VERSION_WITH_HASH)
                set(PRECOMPILED_BIGQUERY_HASH "${BIGQUERY_HASH_X86_64_APPLE_DARWIN}")
            endif()
        elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
            set(PRECOMPILED_BIGQUERY_TARBALL_FILENAME "bigquery-${BIGQUERY_VERSION}-aarch64-apple-darwin.tar.gz")
            if(BIGQUERY_VERSION STREQUAL BIGQUERY_VERSION_WITH_HASH)
                set(PRECOMPILED_BIGQUERY_HASH "${BIGQUERY_HASH_AARCH64_APPLE_DARWIN}")
            endif()
        else()
            message(FATAL "Unsupported CPU architecture: ${CMAKE_SYSTEM_PROCESSOR}")
        endif()
    else()
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
            set(PRECOMPILED_BIGQUERY_TARBALL_FILENAME "bigquery-${BIGQUERY_VERSION}-x86_64-linux-gnu.tar.gz")
            if(BIGQUERY_VERSION STREQUAL BIGQUERY_VERSION_WITH_HASH)
                set(PRECOMPILED_BIGQUERY_HASH "${BIGQUERY_HASH_X86_64_LINUX_GNU}")
            endif()
        else()
            message(FATAL "Unsupported CPU architecture: ${CMAKE_SYSTEM_PROCESSOR}")
        endif()
    endif()
elseif(WIN32)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(PRECOMPILED_BIGQUERY_TARBALL_FILENAME "bigquery-${BIGQUERY_VERSION}-x86_64-windows-msvc.tar.gz")
        if(BIGQUERY_VERSION STREQUAL BIGQUERY_VERSION_WITH_HASH)
            set(PRECOMPILED_BIGQUERY_HASH "${BIGQUERY_HASH_X86_64_WINDOWS_MSVC}")
        endif()
    else()
        message(FATAL "Unsupported CPU architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
else()
    message(FATAL "Unsupported operating system: ${CMAKE_SYSTEM_NAME}")
endif()
set(PRECOMPILED_BIGQUERY_URL "${PRECOMPILED_BIGQUERY_BASE_URL}/${PRECOMPILED_BIGQUERY_TARBALL_FILENAME}")
message(STATUS "Using precompiled bigquery binaries from ${PRECOMPILED_BIGQUERY_URL}")

set(CACHE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cache")
set(PRECOMPILED_BIGQUERY_TARBALL "${CACHE_DIR}/${PRECOMPILED_BIGQUERY_TARBALL_FILENAME}")
if(UNIX)
    set(PRECOMPILED_BIGQUERY_ARCHIVE "${CMAKE_INSTALL_PREFIX}/lib/libgoogle_cloud_cpp_bigquery.a")
elseif(WIN32)
    set(PRECOMPILED_BIGQUERY_ARCHIVE "${CMAKE_INSTALL_PREFIX}/lib/google_cloud_cpp_bigquery.lib")
else()
    message(FATAL "Unsupported operating system: ${CMAKE_SYSTEM_NAME}")
endif()

if(NOT EXISTS "${PRECOMPILED_BIGQUERY_ARCHIVE}")
    if(NOT EXISTS "${PRECOMPILED_BIGQUERY_TARBALL}")
        if(DEFINED PRECOMPILED_BIGQUERY_HASH)
            file(DOWNLOAD 
                "${PRECOMPILED_BIGQUERY_URL}"
                "${PRECOMPILED_BIGQUERY_TARBALL}"
                INACTIVITY_TIMEOUT 300
                SHOW_PROGRESS TRUE
                EXPECTED_HASH SHA256=${PRECOMPILED_BIGQUERY_HASH}
            )
        else()
            file(DOWNLOAD 
                "${PRECOMPILED_BIGQUERY_URL}"
                "${PRECOMPILED_BIGQUERY_TARBALL}"
                INACTIVITY_TIMEOUT 300
                SHOW_PROGRESS TRUE
            )
        endif()
    endif()

    if(UNIX)
        execute_process(
            COMMAND bash -c "mkdir -p \"${BIGQUERY_INSTALL_PREFIX}\" && tar xzf \"${PRECOMPILED_BIGQUERY_TARBALL}\" -C \"${BIGQUERY_INSTALL_PREFIX}\""
            RESULT_VARIABLE STATUS
        )
    elseif(WIN32)
        execute_process(
            COMMAND powershell -command "New-Item -ItemType Directory -Path \"${BIGQUERY_INSTALL_PREFIX}\" -Force -ErrorAction SilentlyContinue && tar xzf \"${PRECOMPILED_BIGQUERY_TARBALL}\" -C \"${BIGQUERY_INSTALL_PREFIX}\""
            RESULT_VARIABLE STATUS
        )
    else()
        message(FATAL "Unsupported operating system: ${CMAKE_SYSTEM_NAME}")
    endif()
    if(STATUS STREQUAL "0")
        message(STATUS "Precompiled bigquery binaries extracted to ${BIGQUERY_INSTALL_PREFIX}")
    else()
        message(FATAL_ERROR "Failed to extract bigquery binaries from ${PRECOMPILED_BIGQUERY_TARBALL} to ${BIGQUERY_INSTALL_PREFIX}")
    endif()
endif()

link_directories("${BIGQUERY_INSTALL_PREFIX}/lib" "${BIGQUERY_INSTALL_PREFIX}/lib64")
include_directories("${BIGQUERY_INSTALL_PREFIX}/include")
add_executable(poc main.cpp)
set(LIBRARIES
    absl_bad_any_cast_impl
    absl_bad_optional_access
    absl_bad_variant_access
    absl_base
    absl_city
    absl_civil_time
    absl_cord
    absl_cord_internal
    absl_cordz_functions
    absl_cordz_handle
    absl_cordz_info
    absl_cordz_sample_token
    absl_crc32c
    absl_crc_cord_state
    absl_crc_cpu_detect
    absl_crc_internal
    absl_debugging_internal
    absl_demangle_internal
    absl_die_if_null
    absl_examine_stack
    absl_exponential_biased
    absl_failure_signal_handler
    absl_flags_commandlineflag
    absl_flags_commandlineflag_internal
    absl_flags_config
    absl_flags_internal
    absl_flags_marshalling
    absl_flags_parse
    absl_flags_private_handle_accessor
    absl_flags_program_name
    absl_flags_reflection
    absl_flags_usage
    absl_flags_usage_internal
    absl_graphcycles_internal
    absl_hash
    absl_hashtablez_sampler
    absl_int128
    absl_kernel_timeout_internal
    absl_leak_check
    absl_log_entry
    absl_log_flags
    absl_log_globals
    absl_log_initialize
    absl_log_internal_check_op
    absl_log_internal_conditions
    absl_log_internal_fnmatch
    absl_log_internal_format
    absl_log_internal_globals
    absl_log_internal_log_sink_set
    absl_log_internal_message
    absl_log_internal_nullguard
    absl_log_internal_proto
    absl_log_severity
    absl_log_sink
    absl_low_level_hash
    absl_malloc_internal
    absl_periodic_sampler
    absl_random_distributions
    absl_random_internal_distribution_test_util
    absl_random_internal_platform
    absl_random_internal_pool_urbg
    absl_random_internal_randen
    absl_random_internal_randen_hwaes
    absl_random_internal_randen_hwaes_impl
    absl_random_internal_randen_slow
    absl_random_internal_seed_material
    absl_random_seed_gen_exception
    absl_random_seed_sequences
    absl_raw_hash_set
    absl_raw_logging_internal
    absl_scoped_set_env
    absl_spinlock_wait
    absl_stacktrace
    absl_status
    absl_statusor
    absl_str_format_internal
    absl_strerror
    absl_string_view
    absl_strings
    absl_strings_internal
    absl_symbolize
    absl_synchronization
    absl_throw_delegate
    absl_time
    absl_time_zone
    absl_vlog_config_internal
    address_sorting
    cares
    crypto
    google_cloud_cpp_api_annotations_protos
    google_cloud_cpp_api_auth_protos
    google_cloud_cpp_api_backend_protos
    google_cloud_cpp_api_billing_protos
    google_cloud_cpp_api_client_protos
    google_cloud_cpp_api_config_change_protos
    google_cloud_cpp_api_consumer_protos
    google_cloud_cpp_api_context_protos
    google_cloud_cpp_api_control_protos
    google_cloud_cpp_api_distribution_protos
    google_cloud_cpp_api_documentation_protos
    google_cloud_cpp_api_endpoint_protos
    google_cloud_cpp_api_error_reason_protos
    google_cloud_cpp_api_field_behavior_protos
    google_cloud_cpp_api_field_info_protos
    google_cloud_cpp_api_http_protos
    google_cloud_cpp_api_httpbody_protos
    google_cloud_cpp_api_label_protos
    google_cloud_cpp_api_launch_stage_protos
    google_cloud_cpp_api_log_protos
    google_cloud_cpp_api_logging_protos
    google_cloud_cpp_api_metric_protos
    google_cloud_cpp_api_monitored_resource_protos
    google_cloud_cpp_api_monitoring_protos
    google_cloud_cpp_api_policy_protos
    google_cloud_cpp_api_quota_protos
    google_cloud_cpp_api_resource_protos
    google_cloud_cpp_api_routing_protos
    google_cloud_cpp_api_service_protos
    google_cloud_cpp_api_source_info_protos
    google_cloud_cpp_api_system_parameter_protos
    google_cloud_cpp_api_usage_protos
    google_cloud_cpp_api_visibility_protos
    google_cloud_cpp_bigquery
    google_cloud_cpp_bigquery_protos
    google_cloud_cpp_cloud_common_common_protos
    google_cloud_cpp_cloud_extended_operations_protos
    google_cloud_cpp_cloud_orgpolicy_v1_orgpolicy_protos
    google_cloud_cpp_common
    google_cloud_cpp_grpc_utils
    google_cloud_cpp_iam_credentials_v1_common_protos
    google_cloud_cpp_iam_credentials_v1_iamcredentials_protos
    google_cloud_cpp_iam_v1_iam_policy_protos
    google_cloud_cpp_iam_v1_options_protos
    google_cloud_cpp_iam_v1_policy_protos
    google_cloud_cpp_longrunning_operations_protos
    google_cloud_cpp_rpc_code_protos
    google_cloud_cpp_rpc_context_attribute_context_protos
    google_cloud_cpp_rpc_error_details_protos
    google_cloud_cpp_rpc_status_protos
    google_cloud_cpp_type_calendar_period_protos
    google_cloud_cpp_type_color_protos
    google_cloud_cpp_type_date_protos
    google_cloud_cpp_type_datetime_protos
    google_cloud_cpp_type_dayofweek_protos
    google_cloud_cpp_type_decimal_protos
    google_cloud_cpp_type_expr_protos
    google_cloud_cpp_type_fraction_protos
    google_cloud_cpp_type_interval_protos
    google_cloud_cpp_type_latlng_protos
    google_cloud_cpp_type_localized_text_protos
    google_cloud_cpp_type_money_protos
    google_cloud_cpp_type_month_protos
    google_cloud_cpp_type_phone_number_protos
    google_cloud_cpp_type_postal_address_protos
    google_cloud_cpp_type_quaternion_protos
    google_cloud_cpp_type_timeofday_protos
    gpr
    grpc
    grpc++
    grpc++_alts
    grpc++_error_details
    grpc++_reflection
    grpc++_unsecure
    grpc_authorization_provider
    grpc_plugin_support
    grpc_unsecure
    grpcpp_channelz
    re2
    ssl
    upb_base_lib
    upb_json_lib
    upb_mem_lib
    upb_message_lib
    upb_textformat_lib
    utf8_range
    utf8_range_lib
    utf8_validity
)
if(UNIX)
    list(APPEND LIBRARIES 
        protobuf
        protobuf-lite
        protoc
        resolv
        z
    )
    if(APPLE)
        list(APPEND LIBRARIES "-framework CoreFoundation")
        set_target_properties(poc PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
    endif()
elseif(WIN32)
    list(APPEND LIBRARIES 
        libprotobuf
        libprotobuf-lite
        libprotoc
        zlibstatic
        ucrtd
    )
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
endif()

target_link_libraries(poc PUBLIC ${LIBRARIES})
install(TARGETS poc)
