# Import socket module
from socket import * 
import sys # In order to terminate the program


class Server:
    def __init__(self, port_number):
        self.port_number = port_number
    def run(self):
        # Add your code here
        serverSocket = socket(AF_INET,SOCK_STREAM)
        serverSocket.bind(('',self.port_number))
        serverSocket.listen(1)
        print('The server is ready to receive')
        while True:
            print('here')
            connectionSocket, addr = serverSocket.accept()
            while True:
                print('hey')
                sentence = connectionSocket.recv(1024).decode()
                if sentence =='':
                    print("except")
                    connectionSocket.close()
                    break
                
                #if sys.getsizeof(sentence) <= 512:
                connectionSocket.send(sentence.encode())
                #else:
                #    connectionSocket.send("sentence too long".encode())
                
                
            #connectionSocket.close()
            
            


class Client:
    def __init__(self, server_port, server_ip):
        self.server_ip = server_ip
        self.server_port = server_port
    
    def run(self):
        # Create server socket
        client = socket(AF_INET, SOCK_STREAM)
        client.connect((self.server_ip, self.server_port))

        try:
            msg = input('Enter message: \n')
            while msg.strip(): # strip removes trailing whitespace
                # send message to the echo server
                client.send(msg.encode())
                # receive the reply from the echo server 
                reply = client.recv(1024).decode()
                print("Server reply:\n%s" % reply)
                msg = input('Enter message: \n')
        except EOFError:
            pass

        client.close()  # close the connection


if __name__ == '__main__':
    if len(sys.argv) < 3 or (sys.argv[1] == 'c' and len(sys.argv) < 4):
        print('Usage: myprog c <port> <address> or myprog s <port>')
    elif not sys.argv[2].isdigit() or (int(sys.argv[2]) < 1024 or int(sys.argv[2]) > 65535):
        print('port number should be larger than 1023 and less than 65536')
    elif sys.argv[1] == 's':
        server = Server(int(sys.argv[2]))
        server.run()
    elif sys.argv[1] == 'c':
        client = Client(int(sys.argv[2]), sys.argv[3])
        client.run()
    else:
        print('unkonwn commend type %s' % sys.argv[1])
