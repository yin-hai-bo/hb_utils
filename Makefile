.PHONY: all lib test clean

src_dir := ./src
bin_dir := ./bin
test_dir := ./test

lib_target := $(bin_dir)/libhbutils.a
test_exe := $(bin_dir)/test

impl_src := \
	checksum.cpp \
	tb_rate_limiter.cpp
impl_src := $(addprefix $(src_dir)/, $(impl_src))
impl_obj := $(impl_src:.cpp=.o)

test_src := \
	test_checksum.cpp \
	test_tb_rate_limiter.cpp
test_src := $(addprefix $(test_dir)/, $(test_src))
test_obj := $(test_src:.cpp=.o)

CXXFLAGS := \
	--std=c++11 -Werror -Wfatal-errors

all: lib test

lib: $(lib_target)

$(lib_target): $(impl_obj)
	$(RM) $@
	@mkdir -p $(bin_dir)
	$(AR) -rs $@ $^

test: $(test_exe)
	$^

$(test_exe): $(test_obj) $(lib_target)
	g++ -o $@ $^ \
		-l gtest -l gtest_main

%.o: %.cpp
	g++ -c $(CXXFLAGS) -I $(src_dir) -O2 -o $@ $^

clean:
	$(RM) $(test_obj) $(impl_obj) $(test_exe) $(lib_target)
