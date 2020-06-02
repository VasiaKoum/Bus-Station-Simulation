obj = mystation bus station-manager comptroller
all: $(obj)
$(obj): %: %.c
	$(CC) $(CFLAGS) -o $@ operations.c  $< -pthread
run:
	./mystation -l config.csv -e YES -c NO
val:
	valgrind --trace-children=yes -v --leak-check=full --show-leak-kinds=all ./mystation -l config.csv -e YES
clean:
	rm -f mystation bus station-manager comptroller logfile
