BIN_NAME = pthread_attr_demo
CC		 = clang
C_FLAGS  = -O3
L_FLAGS  = -lpthread -lm
C_SRC 	 = pthread_attr_demo.c
all:
	$(CC) $(C_SRC) $(C_FLAGS) $(L_FLAGS) -o ./$(BIN_NAME)
	./$(BIN_NAME)

clean:
	rm -rf ./$(BIN_NAME)