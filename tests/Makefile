.PHONY: all build run clean

CXX ?= g++
CXXFLAGS ?= -std=c++17 -I../include
# Track the headers each object was built from. Without this an edit to include/ leaves the old
# objects in place and the run reports the previous binary's result.
DEPFLAGS := -MMD -MP
LDFLAGS ?= -lcrypto
LDFLAGS += -lsodium

TARGET := carbon_tests
SRCS := $(filter-out vm_bigint_parity_main.cpp token_schemas_json_tests.cpp,$(wildcard *.cpp))
BUILD_DIR ?= build
BUILD_STAMP := $(BUILD_DIR)/.dir
OBJS := $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(SRCS))
TARGET_PATH := $(BUILD_DIR)/$(TARGET)
VM_BIGINT_PARITY_TARGET := $(BUILD_DIR)/vm_bigint_parity_tests
VM_BIGINT_PARITY_SRCS := test_common.cpp vm_bigint_parity_tests.cpp vm_bigint_parity_main.cpp
VM_BIGINT_PARITY_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/vm_bigint_parity_%.o,$(VM_BIGINT_PARITY_SRCS))
# The schemas JSON tests build against the RapidJSON copy this repository tracks, so the suite needs no system package.
SCHEMAS_JSON_TARGET := $(BUILD_DIR)/token_schemas_json_tests
SCHEMAS_JSON_SRCS := test_common.cpp token_schemas_json_tests.cpp
SCHEMAS_JSON_OBJS := $(patsubst %.cpp,$(BUILD_DIR)/schemas_json_%.o,$(SCHEMAS_JSON_SRCS))

all: build

build: $(TARGET_PATH) $(VM_BIGINT_PARITY_TARGET) $(SCHEMAS_JSON_TARGET)

$(TARGET_PATH): $(OBJS) | $(BUILD_STAMP)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET_PATH) $(LDFLAGS)

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_STAMP)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -c $< -o $@

$(VM_BIGINT_PARITY_TARGET): $(VM_BIGINT_PARITY_OBJS) | $(BUILD_STAMP)
	$(CXX) $(CXXFLAGS) $(VM_BIGINT_PARITY_OBJS) -o $(VM_BIGINT_PARITY_TARGET) $(LDFLAGS)

$(BUILD_DIR)/vm_bigint_parity_%.o: %.cpp | $(BUILD_STAMP)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -DPHANTASMA_EXCEPTION_ENABLE -c $< -o $@

$(SCHEMAS_JSON_TARGET): $(SCHEMAS_JSON_OBJS) | $(BUILD_STAMP)
	$(CXX) $(CXXFLAGS) $(SCHEMAS_JSON_OBJS) -o $(SCHEMAS_JSON_TARGET) $(LDFLAGS)

$(BUILD_DIR)/schemas_json_%.o: %.cpp | $(BUILD_STAMP)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) -DPHANTASMA_RAPIDJSON -isystem "../samples/Basic Examples/CurlRapidjsonSample" -c $< -o $@

$(BUILD_STAMP):
	mkdir -p $(BUILD_DIR)
	touch $(BUILD_STAMP)

run: $(TARGET_PATH) $(VM_BIGINT_PARITY_TARGET) $(SCHEMAS_JSON_TARGET)
	./$(TARGET_PATH)
	./$(VM_BIGINT_PARITY_TARGET)
	./$(SCHEMAS_JSON_TARGET)

run-vm-bigint-parity: $(VM_BIGINT_PARITY_TARGET)
	./$(VM_BIGINT_PARITY_TARGET)

clean:
	rm -rf $(BUILD_DIR)

-include $(OBJS:.o=.d) $(VM_BIGINT_PARITY_OBJS:.o=.d) $(SCHEMAS_JSON_OBJS:.o=.d)
