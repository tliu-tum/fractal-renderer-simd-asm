CC = gcc
CFLAGS = -g -Wall -Wextra -O2
LDFLAGS = -lm

#CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer -O0 -g
#LDFLAGS += -fsanitize=address,undefined

SRCS_C = main.c bmp.c impl_c.c 
SRCS_ASM = impl_asm.S


OBJS = $(SRCS_C:.c=.o) $(SRCS_ASM:.S=.o)

TARGET = main

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


%.o: %.S
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)