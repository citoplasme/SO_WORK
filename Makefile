# All depends on write, which depends on work, which depends on work.c
all:	compile

# Command to compile
compile:	work.c
	gcc -o work work.c
	
# Cleans the executable
clean: 
	rm work
