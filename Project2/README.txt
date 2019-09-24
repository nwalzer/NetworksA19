Simulates reliable data transfer between two processes

TO COMPILE:
	run the command 'make' or 'make all'

TO RUN:
	After compiling run the command './p2' and enter your input parameters when prompted

TO CLEAN:
	run the command 'make clean'

NOTE: Be wary about setting the error-related input parameters too high all at once. It is possible for the 
	program to take too long to send packets successfully when these parameters are too high, leading
	to the AEntity to begin dropping packets as its buffer fills, and leading to infinite timerinterrupts.
	The highest this has been tested to is with inputs 100, 0.75, 0.75, 0.75, 5000, 1, 1, 0 with a 100%
	success rate. There was a notable failure with all error inputs being set to 0.99 and the packet delay
	being set to 1000, and arguably completely unreasonable set of parameters.
	
	Sometimes after running the program you will see a lot (sometimes hundreds) of the word PuTTY over and
	over and over. I have no idea why this happens nor do I know how to stop it, however the program still
	executed as normal and results are still displayed (if you can scroll up far enough to find them). It
	also may take up to 5 minutes for the program to stop execution, sorry
