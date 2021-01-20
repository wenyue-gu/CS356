# Import socket module
from socket import * 
import sys # In order to terminate the program


class Server:
    def __init__(self, port_number):
        self.port_number = port_number
    def run(self):

        # Create server socket
        
        # Add your code here
        
        # Set up a new connection from the client
        while True:
            print('The server is ready to receive')
            
            # Server should be up and running and listening to the incoming connections
            
            # Add your code here

        serverSocket.close()  
        sys.exit()#Terminate the program after sending the corresponding data


class Client:
    def __init__(self, server_ip, server_port):
        self.server_ip = server_ip
        self.server_port = server_port
    
    def run(self):
        
        # Create server socket
        
        # Add your code here

        # Get input with function input()

        # Add your code here
        # Hint:
        # try:
        #     from_client = input("Enter message: \n")
        # ...
        # ...


        while from_client.lower().strip() != 'bye':
            # send and receive message

            # Add your code here

            print (from_server)   # show in terminal
            
            # Get input again

            # Add your code here


        client.close()  # close the connection

if __name__ == '__main__':
    if len(sys.argv) <3:
        print('Usage: python3 myprog.py c <address> <port> or python3 myprog.py s <port>')
    elif sys.argv[1]!="s" and sys.argv[1]!="c":
        print('Usage: python3 myprog.py c <address> <port> or python3 myprog.py s <port>')
    elif(sys.argv[1]=="s"):
        server = Server(int(sys.argv[2]))
        server.run()
    else:
        client = Client(sys.argv[2],int(sys.argv[3]))
        client.run()
