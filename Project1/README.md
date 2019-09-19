To COMPILE:
	run the command 'make' or 'make all'
This will build both the server executable and client executable

To RUN the server:
After compiling using 'make' run './server <Port #>' where <Port #> is the desired port to be listening on.
If the server gives the error message 'bind() failed' then the specified port was occupied or otherwise
unavailable. Rerun the server using a different port or wait until the port becomes available. The server
is operational once the message "Waiting for connection..." appears

To RUN the client:
After compiling using 'make' run './client [-p] <URL or IP> <Port #>' where -p is an optional parameter
which will display the RTT for connecting to the specified server, <URL or IP> is the URL or IP of the 
page you are trying to access, and <Port #> is the port used to access the server. In the case of general
internet traffic use port 80, if trying to contact the running server executable use the same port as
the server's initial arguments. On completion, client will write a file called recieved.html, which
contains the response body from the server

To REMOVE executables:
	run the command 'make clean'
This will remove both the server and client executable


NOTES ABOUT BEHAVIOR:
I have manually made it so that several different specified paths lead to the required TMDG file. The Paths
"/", "/TMDG.html", "/index/html", and "" all lead to TMDG.html. THIS IS BY DESIGN.

There is strange behavior on the server when accessing it from a browser. Becuase there is such high latency
the browser is sometimes unable to recieve the 404 page from the server. This is due to the server closing
the socket once the 404 response is sent, and the browser not being able to reach this message before the
socket is closed. This is not an issue when the client accesses the server, which handles everything normally
and as expected. If I had more time on this assignment I could figure out a solution to this, but I thought
I might as well be honest about it. In addition to this the server sometimes indicates "send() failed" while
then still returning a page. There are instances where the server seems to double-run the TCP function,
failing the first time and succeeding the second time. I really don't know what's going on with that.

Because the assignment instructions were not quite all-comprehensive I have assumed a few things and
taken a few liberties. Notably:
If my server receives a request that is NOT a GET request it will send a 400 response
Only the status line is included in the server's response
The server only sends HTTP/1.1 responses
There are instances in the server code where it attempts to send a response but contains no error checking
	on the send function. This is because the standard error handling procedure would be to close
	the socket and return from the TCP function, however this is also the procedure for a send success,
	so error checking would be redundant.
The server can only send 200, 400, and 404 responses.
