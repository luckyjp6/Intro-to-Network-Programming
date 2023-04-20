all: server client
	python3 submit.py ./server ./client

server: server.cpp my_define.h
	g++ server.cpp -static -o server
client: client.cpp my_define.h
	g++ client.cpp -static -o client
 
test: server client my_define.h
	# sudo ./server ./recv_case 3 172.30.207.255
	# sudo ./client ./test_case 3 172.30.207.255