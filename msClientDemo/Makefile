CPWD := $(shell pwd -P)
TOP := $(dir $(CPWD))

PROG = msClientDemo
include ../makefile.func

SRC := ./main.c
SRC += ./dlclient.c
SRC += ./dlclient_cmd.c

INC := ./
INC += ../include

vpath %.c $(call sh-sort, -n, $(sort $(dir $(SRC))))
IPATH := $(addprefix -I, $(call sh-sort, -n, $(sort $(INC))))

# token for building tags
ALL_SRCDIR := $(call sh-sort, -n, $(sort $(dir $(SRC)) $(INC)))

CFLAGS += $(IPATH)
CFLAGS += -g

LDLIBS = -lrt

OBJD := $(CPWD)/out
OBJ := $(patsubst %.c, $(OBJD)/%.o, $(notdir $(SRC)))

$(OBJD)/%.o : %.c 
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(PROG) : $(OBJ)
	@echo Building $(PROG)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)
	@echo "#!/bin/sh" > ./start_client.sh
	@echo "./$(PROG)" >> ./start_client.sh
	@chmod a+x ./start_client.sh

include ../makefile.tags

clean:
	@rm -rf $(OBJD)
	@rm -f $(PROG)
	@rm -f start_client.sh

info:
	@echo SRC: $(SRC)
	@echo INC: $(INC)
	@echo OBJ: $(OBJ)
