# Location of the 'EmbUnit' sources
EMBUNIT_DIR = $(CURDIR)/../../../../../../embunit/src

DATE:=$(shell date +"%y%m%d-%H%M%S")

#------------------------------------------------------------------------------
# What needs to be defined for EmbUnit:
# - EMBUNIT_DIR (location of the EmbUnit sources)
# - TARGET (executable name)
# - OBJS += (list of objs)
# - The '.pre' and '.post' rules
# - LDFLAGS & CFLAGS if required
#------------------------------------------------------------------------------

CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++

# Comment/uncomment the following line to disable/enable debugging
DEBUG = y
ifeq ($(DEBUG),y)
  EXTRA_CFLAGS  += -O0 -DDEBUG -g -ggdb
  EXTRA_LDFLAGS +=
else
  EXTRA_CFLAGS  += -O2
  EXTRA_LDFLAGS +=
endif

EXTRA_CFLAGS += -DUNITTEST -Wall -Werror -Wno-nonnull

BOARD ?= $(shell uname -m)
TYPE ?= release

STACK_DIR =../../../../..

EXTRA_CFLAGS += -I $(CURDIR) \
                -I $(CURDIR)/.. \
                -I $(CURDIR)/../../../../.. \
                -I $(STACK_DIR)/../cohda/kernel/include \
                -I $(STACK_DIR)/include \
                -I $(STACK_DIR)/v2x-lib/include \
                -I $(STACK_DIR)/v2x-lib/include/asn1 \
                -I $(EMBUNIT_DIR) \
                -DD_LOCAL=D_WARN \
                -Wall -Werror

CFLAGS += $(EXTRA_CFLAGS)
CXXFLAGS += $(CFLAGS)

LDFLAGS += -lhmi -ltm -lta -ltc -lspatmap
LDFLAGS += -litsfac -litsnet -litsasn
LDFLAGS += -lviicsec -laerolinkPKI
LDFLAGS += -lext -lplat -lldm -lclv -lpos -lpcap -lJudy -lgps_static -lconfig
LDFLAGS += -lLLC -L $(STACK_DIR)/../cohda/app/llc
LDFLAGS += -lm -lpthread -lrt -lnl-3 -lnl-genl-3 -ldl -lstdc++ $(EXTRA_LDFLAGS)

V2XLIB_DIR=$(STACK_DIR)/v2x-lib/lib/$(BOARD)/
LOCAL_RPATH := -Wl,--disable-new-dtags -Wl,--disable-new-dtags -Wl,-rpath,$(V2XLIB_DIR)
LDFLAGS += $(LOCAL_RPATH) -L$(V2XLIB_DIR)

TARGET  = test-Ntrip

SRCS    = $(wildcard $(TARGET)*.c)  $(wildcard ../*.c)  $(wildcard ../gen/*.c)
CXXSRCS = $(wildcard $(TARGET)*.cc) $(wildcard ../*.cc) $(wildcard ../gen/*.cc)

OBJS = $(SRCS:.c=.o)
OBJS += $(CXXSRCS:.cc=.o)

# Include the 'EmbUnit' unit test framework
# (The included makefile requires the above to be set)
include $(EMBUNIT_DIR)/App.mk

VALGRIND = valgrind \
	--leak-check=full \
	--show-reachable=yes \
	--num-callers=50 \
	--track-origins=yes \
	--suppressions=valgrind.supp \
	--error-exitcode=99 \
	--

# Any actions to perform before bulding the modules e.g. copying files
.pre:
	@# Put commands here between here...
	$(MAKE) -C $(STACK_DIR) BOARD=$(BOARD) TYPE=$(TYPE)
	$(MAKE) --directory ../../.. ntrip-gen
	@echo "ProtocolMode = 1 # NA" > test.conf
	@# ... and here
	@touch .pre
	@rm -rf .post

# Any actions to perform after bulding (called on 'make clean')
.post:
	@# Put commands here between here...
	@rm -rf *.gcov *.gcda *.gcno
	@rm -f test.conf
	@# ... and here
	@touch .post
	@rm -rf .pre
	@rm -rf .post

lib:
	$(MAKE) -C $(STACK_DIR) BOARD=$(BOARD) TYPE=$(TYPE)

ddd: $(TARGET)
	ddd ./$(TARGET)

# Any actions to in order to test the application
check: $(TARGET)
	@# Put your commands to prepare for the testing between here...
	@# None
	@# ... and here

	sudo rm -f $@.ok
	( $(VALGRIND) ./$(TARGET) && touch $@.ok ) | tee $(TARGET)-$(DATE).log
	rm $@.ok
	xsltproc -o $(TARGET)-$(DATE)-xunit.xml $(EMBUNIT_DIR)/embunit_to_junit.xslt $(TARGET)-$(DATE).log
	@sleep 2

	@# Put your commands to cleanup after the testing between here...
	@# None
	@# ... and here

