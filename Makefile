BIGQUERY_VERSION ?= 2.22.0
PRECOMPILED_BIGQUERY_BASE_URL = https://github.com/cocoa-xu/google-cloud-cpp-bigquery/releases/download/v$(BIGQUERY_VERSION)

BUILD_DIR ?= $(shell pwd)/build
CACHE_DIR ?= $(shell pwd)/cache
INSTALL_PREFIX ?= $(shell pwd)/install
CMAKE_CXX_STANDARD = 17

ifndef NPROC
UNAME_S = $(shell uname -s)
ifneq ($(UNAME_S),Darwin)
NPROC = $(shell sysctl -n hw.ncpu)
else
NPROC = $(shell nproc)
endif
endif

UNAME_M = $(shell uname -m)
ifneq ($(UNAME_S),Darwin)
PRECOMPILED_BIGQUERY_TARBALL_FILENAME = bigquery-$(BIGQUERY_VERSION)-$(UNAME_M)-linux-gnu.tar.gz
else
ifeq ($(UNAME_M),x86_64)
PRECOMPILED_BIGQUERY_TARBALL_FILENAME = bigquery-$(BIGQUERY_VERSION)-$(UNAME_M)-apple-darwin.tar.gz
else
PRECOMPILED_BIGQUERY_TARBALL_FILENAME = bigquery-$(BIGQUERY_VERSION)-aarch64-apple-darwin.tar.gz
endif
endif
PRECOMPILED_BIGQUERY_URL = $(PRECOMPILED_BIGQUERY_BASE_URL)/$(PRECOMPILED_BIGQUERY_TARBALL_FILENAME)
PRECOMPILED_BIGQUERY_TARBALL = $(CACHE_DIR)/$(PRECOMPILED_BIGQUERY_TARBALL_FILENAME)
PRECOMPILED_BIGQUERY_ARCHIVE = $(INSTALL_PREFIX)/lib/libgoogle_cloud_cpp_bigquery.a

build: precompiled-bigquery
	@ cmake -S . -B "$(BUILD_DIR)" \
		-D CMAKE_BUILD_TYPE=Release \
		-D CMAKE_INSTALL_PREFIX="$(INSTALL_PREFIX)" \
		-D CMAKE_CXX_STANDARD=$(CMAKE_CXX_STANDARD) \
		-D BIGQUERY_INSTALL_DIR="$(INSTALL_PREFIX)"
	@ cmake --build "$(BUILD_DIR)" --config Release -j

$(CACHE_DIR):
	@ mkdir -p "$(CACHE_DIR)"

$(INSTALL_PREFIX):
	@ mkdir -p "$(INSTALL_PREFIX)"

fetch-precompiled-bigquery: $(CACHE_DIR) $(INSTALL_PREFIX)
	@ if [ ! -f "$(PRECOMPILED_BIGQUERY_TARBALL)" ]; then \
		echo "Downloading precompiled BigQuery client library from $(PRECOMPILED_BIGQUERY_URL) ..." && \
		curl -fSL "$(PRECOMPILED_BIGQUERY_URL)" -o "$(PRECOMPILED_BIGQUERY_TARBALL)" ; \
	fi

unarchive-precompiled-bigquery: fetch-precompiled-bigquery
	@ if [ ! -f "$(PRECOMPILED_BIGQUERY_ARCHIVE)" ]; then \
		tar -xzf "$(PRECOMPILED_BIGQUERY_TARBALL)" -C "$(INSTALL_PREFIX)" ; \
	fi

precompiled-bigquery: unarchive-precompiled-bigquery
	@ echo > /dev/null

clean:
	@ rm -rf "$(BUILD_DIR)"
