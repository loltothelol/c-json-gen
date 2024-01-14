EXECNAME:=c-json-gen
SDIR:=src
BDIR:=build
ODIR:=$(BDIR)/obj
CC:=gcc
CFLAGS:=-g -Wshadow -pedantic-errors
LIBS:=-ljson -lm
DEP:=$(wildcard $(SDIR)/*.h)
SRC:=$(wildcard $(SDIR)/*.c)
OBJ:=$(patsubst %.c,$(ODIR)/%.o,$(SRC))

$(BDIR)/$(EXECNAME): $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

$(BDIR)/$(EXECNAME).o: $(OBJ)
	@mkdir -p $(dir $@)
	ld -relocatable -o $@ $^

$(ODIR)/%.o: %.c $(DEP)
	@mkdir -p $(dir $@)
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: test clean

test: $(BDIR)/$(EXECNAME)
	./$< openapi.json

clean:
	@rm -rf $(BDIR)
