CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -fpermissive
INCLUDES := -I../Basic\ Examples/CurlRapidjsonSample
LIBCURL ?= $(shell ldconfig -p | awk '/libcurl\.so\./ {print $$NF; exit}')
TARGET := wallet

# Carbon needs libsodium: its address hashing and its random bytes come from there.
SODIUM_CFLAGS := $(shell pkg-config --cflags libsodium)
SODIUM_LIBS := $(shell pkg-config --libs libsodium)

ifeq ($(LIBCURL),)
LIBS := -lcurl -lcrypto -lpthread $(SODIUM_LIBS)
else
LIBS := $(LIBCURL) -lcrypto -lpthread $(SODIUM_LIBS)
endif

all: $(TARGET)

$(TARGET): program.cpp
	$(CXX) $(CXXFLAGS) $(SODIUM_CFLAGS) $(INCLUDES) program.cpp $(LIBS) -o $(TARGET)

.PHONY: clean
clean:
	rm -f $(TARGET)
