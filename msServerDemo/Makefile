CPWD := $(shell pwd -P)
TOP := $(dir $(CPWD))

include ../makefile.func

PROG = msServerDemo

SRC := ./main.c
SRC += ./dlmmap.c
SRC += ./dlserver.c
SRC += ./dlserver_cmd.c

INC := ./
INC += ../mmalloc
INC += ../include

vpath %.c $(call sh-sort, -n, $(sort $(dir $(SRC))))
IPATH := $(addprefix -I, $(call sh-sort, -n, $(sort $(INC))))

# token for building tags
ALL_SRCDIR := $(call sh-sort, -n, $(sort $(dir $(SRC)) $(INC)))

CFLAGS += $(IPATH)
CFLAGS += -g

LDLIBS = -lrt -lmmalloc
LDFLAGS = -L../lib

OBJD := $(CPWD)/out
OBJ := $(patsubst %.c, $(OBJD)/%.o, $(notdir $(SRC)))

$(OBJD)/%.o : %.c
	@mkdir -p $(OBJD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(PROG) : $(OBJ)
	@echo Building $(PROG)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)
	@echo "#!/bin/sh" > ./start_server.sh
	@echo "LD_LIBRARY_PATH=../lib/:$$LD_LIBRARY_PATH ./msServerDemo" >> start_server.sh
	@chmod a+x ./start_server.sh

include ../makefile.tags

clean:
	@rm -rf $(OBJD)
	@rm -f $(PROG)
	@rm -f start_server.sh

info:
	@echo SRC: $(SRC)
	@echo INC: $(INC)
	@echo OBJ: $(OBJ)
	@echo VPATH: $(VPATH)
	@echo IPATH: $(IPATH)
