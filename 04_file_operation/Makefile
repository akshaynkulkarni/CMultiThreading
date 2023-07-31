BIN_NAME = file_operation_demo
CC		 = gcc
C_FLAGS  = -O3
L_FLAGS  = -lpthread -lm
C_SRC 	 = file_operation_demo.c
all:
	$(CC) $(C_SRC) $(C_FLAGS) $(L_FLAGS) -o ./$(BIN_NAME)
	./$(BIN_NAME)

clean:
	rm -rf ./$(BIN_NAME)
