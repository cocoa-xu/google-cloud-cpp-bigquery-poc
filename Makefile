.DEFAULT_GLOBAL := build

BUILD_DIR ?= $(shell pwd)/build
INSTALL_PREFIX ?= $(shell pwd)/install
CMAKE_CXX_STANDARD ?= 14

THRID_PARTY_DIR = $(shell pwd)/third_party

ABSEIL_CPP_VERSION = lts_2024_01_16
ABSEIL_CPP_GIT_REPO = https://github.com/abseil/abseil-cpp.git
ABSEIL_CPP_SRC_DIR = $(THRID_PARTY_DIR)/abseil-cpp
ABSEIL_CPP_BUILD_DIR = $(BUILD_DIR)/abseil-cpp
ABSEIL_CPP_CONFIG_CMAKE = $(INSTALL_PREFIX)/lib/cmake/absl/abslConfig.cmake

GRPC_VERSION = v1.62.1
GRPC_GIT_REPO = https://github.com/grpc/grpc.git
GRPC_SRC_DIR = $(THRID_PARTY_DIR)/grpc
GRPC_BUILD_DIR = $(BUILD_DIR)/grpc
GRPC_CONFIG_CMAKE = $(INSTALL_PREFIX)/lib/cmake/grpc/gRPCConfig.cmake

OPENSSL_VERSION = 3.2.1
OPENSSL_TARBALL = $(THRID_PARTY_DIR)/openssl-$(OPENSSL_VERSION).tar.gz
OPENSSL_SRC_DIR = $(THRID_PARTY_DIR)/openssl-$(OPENSSL_VERSION)
OPENSSL_INSTALL_PREFIX = $(INSTALL_PREFIX)/openssl

GOOGLE_CLOUD_CPP_VERSION = v2.22.0
GOOGLE_CLOUD_CPP_GIT_REPO = https://github.com/googleapis/google-cloud-cpp.git
GOOGLE_CLOUD_CPP_SRC_DIR = $(THRID_PARTY_DIR)/google-cloud-cpp
GOOGLE_CLOUD_CPP_BUILD_DIR = $(BUILD_DIR)/google-cloud-cpp
GOOGLE_CLOUD_CPP_BIGQUERY_CONFIG_CMAKE = $(INSTALL_PREFIX)/lib/cmake/google_cloud_cpp_bigquery/google_cloud_cpp_bigquery-config.cmake

UNAME_S = $(shell uname -s)
ifneq ($(UNAME_S),Darwin)
NPROC = $(shell sysctl -n hw.ncpu)
else
NPROC = $(shell nproc)
endif

build: build-deps
	@ cmake -S . -B "$(BUILD_DIR)/poc" \
		-D CMAKE_BUILD_TYPE=Release \
		-D CMAKE_INSTALL_PREFIX="$(INSTALL_PREFIX)" \
		-D CMAKE_CXX_STANDARD=$(CMAKE_CXX_STANDARD) \
		-D BIGQUERY_INSTALL_DIR="$(INSTALL_PREFIX)"
	@ cmake --build "$(BUILD_DIR)/poc" --config Release -j

build-deps: $(ABSEIL_CPP_CONFIG_CMAKE) $(GRPC_CONFIG_CMAKE) install-openssl $(GOOGLE_CLOUD_CPP_BIGQUERY_CONFIG_CMAKE)
	@ echo > /dev/null

fetch-abseil-cpp:
	@ if [ ! -d "$(ABSEIL_CPP_SRC_DIR)" ]; then \
		git clone --depth=1 --branch $(ABSEIL_CPP_VERSION) $(ABSEIL_CPP_GIT_REPO) "$(ABSEIL_CPP_SRC_DIR)" ; \
	fi

config-abseil-cpp: fetch-abseil-cpp
	@ if [ ! -f "$(ABSEIL_CPP_CONFIG_CMAKE)" ]; then \
		cd "$(ABSEIL_CPP_SRC_DIR)" && \
		cmake -S . -B "$(ABSEIL_CPP_BUILD_DIR)" \
			-D CMAKE_BUILD_TYPE=Release \
			-D CMAKE_INSTALL_PREFIX="$(INSTALL_PREFIX)" \
			-D CMAKE_CXX_STANDARD=$(CMAKE_CXX_STANDARD) ; \
	fi

install-abseil-cpp: config-abseil-cpp
	@ if [ ! -f "$(ABSEIL_CPP_CONFIG_CMAKE)" ]; then \
		cmake --build "$(ABSEIL_CPP_BUILD_DIR)" --config Release --target install -j ; \
	fi

$(ABSEIL_CPP_CONFIG_CMAKE): install-abseil-cpp
	@ echo > /dev/null

fetch-grpc:
	@ if [ ! -d "$(GRPC_SRC_DIR)" ]; then \
		git clone $(GRPC_GIT_REPO) "$(GRPC_SRC_DIR)" ; \
	fi

config-grpc: fetch-grpc
	@ if [ ! -f "$(GRPC_CONFIG_CMAKE)" ]; then \
		cd "$(GRPC_SRC_DIR)" && \
		git submodule update --init && \
		cmake -S . -B "$(GRPC_BUILD_DIR)" \
			-D CMAKE_BUILD_TYPE=Release \
			-D CMAKE_INSTALL_PREFIX="$(INSTALL_PREFIX)" \
			-D CMAKE_CXX_STANDARD=$(CMAKE_CXX_STANDARD)	; \
	fi

install-grpc: config-grpc
	@ if [ ! -f "$(GRPC_CONFIG_CMAKE)" ]; then \
		cmake --build "$(GRPC_BUILD_DIR)" --config Release --target install -j ; \
	fi

$(GRPC_CONFIG_CMAKE): install-grpc
	@ echo > /dev/null

download-openssl:
	@ if [ ! -f "$(OPENSSL_TARBALL)" ]; then \
		curl -fSL "https://www.openssl.org/source/openssl-$(OPENSSL_VERSION).tar.gz" -o "$(OPENSSL_TARBALL)" ; \
	fi

unarchive-openssl: download-openssl
	@ if [ ! -d "$(OPENSSL_SRC_DIR)" ]; then \
		mkdir -p "$(OPENSSL_SRC_DIR)" && \
		tar -xzf "$(OPENSSL_TARBALL)" -C "$(OPENSSL_SRC_DIR)" --strip-components=1 ; \
	fi

install-openssl: unarchive-openssl
	@ if [ ! -f "$(OPENSSL_INSTALL_PREFIX)/include/openssl/opensslconf.h" ]; then \
		cd "$(OPENSSL_SRC_DIR)" && \
		./config --prefix="$(OPENSSL_INSTALL_PREFIX)" --openssldir="$(OPENSSL_INSTALL_PREFIX)" no-tests no-shared && \
		make -j$(NPROC) && \
		make -j$(NPROC) install_sw && \
		make -j$(NPROC) install_ssldirs ; \
	fi

fetch-google-cloud-cpp:
	@ if [ ! -d "$(GOOGLE_CLOUD_CPP_SRC_DIR)" ]; then \
		git clone --depth=1 --branch $(GOOGLE_CLOUD_CPP_VERSION) $(GOOGLE_CLOUD_CPP_GIT_REPO) "$(GOOGLE_CLOUD_CPP_SRC_DIR)" ; \
	fi

config-google-cloud-cpp: fetch-google-cloud-cpp
	@ if [ ! -f "$(GOOGLE_CLOUD_CPP_BIGQUERY_CONFIG_CMAKE)" ]; then \
		cd "$(GOOGLE_CLOUD_CPP_SRC_DIR)" && \
		cmake -S . -B "$(GOOGLE_CLOUD_CPP_BUILD_DIR)" \
			-D CMAKE_BUILD_TYPE=Release \
			-D CMAKE_INSTALL_PREFIX="$(INSTALL_PREFIX)" \
			-D CMAKE_CXX_STANDARD=$(CMAKE_CXX_STANDARD)	\
			-D CMAKE_PREFIX_PATH="$(INSTALL_PREFIX)/lib/cmake" \
			-D OPENSSL_ROOT_DIR="$(INSTALL_PREFIX)/openssl" \
			-D BUILD_TESTING=OFF \
			-D GOOGLE_CLOUD_CPP_ENABLE_EXAMPLES=OFF \
			-D GOOGLE_CLOUD_CPP_ENABLE=bigquery ; \
	fi

install-google-cloud-cpp: config-google-cloud-cpp
	@ if [ ! -f "$(GOOGLE_CLOUD_CPP_BIGQUERY_CONFIG_CMAKE)" ]; then \
		cmake --build "$(GOOGLE_CLOUD_CPP_BUILD_DIR)" --config Release --target install -j ; \
	fi

$(GOOGLE_CLOUD_CPP_BIGQUERY_CONFIG_CMAKE): install-google-cloud-cpp
	@ echo > /dev/null

clean-abseil-cpp:
	rm -rf "$(ABSEIL_CPP_BUILD_DIR)"

clean-grpc:
	rm -rf "$(GRPC_BUILD_DIR)"

clean-openssl:
	@ cd "$(OPENSSL_SRC_DIR)"
	make clean

clean-google-cloud-cpp:
	rm -rf "$(GOOGLE_CLOUD_CPP_BUILD_DIR)"

clean: clean-abseil-cpp clean-grpc clean-openssl clean-google-cloud-cpp
	@ echo > /dev/null
