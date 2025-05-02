CC=gcc
LIBS=-lpthread
BUILD_DIR=$(PWD)/output
OBJ=net_tap.o
dir:
	mkdir -p $(BUILD_DIR)
$(OBJ):
	$(CC) -c net_tap.c -o $(BUILD_DIR)/$@
net_tap:
	$(CC) $(BUILD_DIR)/$@.o -o $(BUILD_DIR)/$@.elf $(LIBS)
all: dir $(OBJ) net_tap
clean:
	rm -r $(BUILD_DIR)