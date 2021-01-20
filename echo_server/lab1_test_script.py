# -*- coding: utf-8 -*-
import sys
import time
import pexpect
import random
import string
import threading


class Server(threading.Thread):
    def __init__(self, prog_path, port_number):
        super(Server, self).__init__()
        self.prog_path = prog_path
        self.controler = None
        self.port_number = port_number

    def run(self):
        self.controler = pexpect.spawn("python3 %s s %d" % (self.prog_path, self.port_number))
        print("Server Port:", self.port_number)
        while True:
            try:
                self.controler.expect("\n\r\t\n\n", timeout=2)
            except pexpect.TIMEOUT:
                continue
            except pexpect.EOF:
                break

    def close(self):
        self.controler.sendcontrol('c')


def check_binary_valid(prog_path):
    return 1
    try:
        client = pexpect.spawn("python3 %s" % prog_path)
    except pexpect.ExceptionPexpect:
        return 0
    try:
        client.expect("Usage: python3 myprog.py c <port> <address> or python3 myprog.py s <port>", timeout=2)
        client.close()
        return 1
    except (pexpect.TIMEOUT, pexpect.EOF) as e:
        client.close()
        return 0


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('Usage: python3 lab1_test_script.py myprog_path')
    else:
        prog_path = sys.argv[1]
        
        if not check_binary_valid(prog_path):
            print('Please input a valid path of your binary program ')
        else:
            messages = [
                'test case 1',
                'test case 2:  COMPSCI 356 i\f\\\'\"\as an undergraduate course in computer science teaching the fundamentals of computer networks. We will cover the technologies supporting the Internet, from Ethernet and WiFi through the routing protocols that govern the flow of traffic, and the web technologies that	generate most of it.Topics:   The topics we will study include but are not limited to: how to achieve reliable communications over unreliable channels, how to find a good path through a network, how to share networ',
                [],
                "EOF"
            ]
            port_number = random.randint(1024, 65535)
            server = Server(prog_path, port_number)

            server.start()
            time.sleep(2)

            def send_message(client_handle, message):
                if message != 'EOF':
                    client_handle.sendline(message)
                else:
                    client_handle.sendcontrol('d')
                if message != 'EOF':
                    try:
                        client_handle.expect("Enter message:", timeout=4)
                        new_msg = client_handle.before.decode("utf-8") 
                        response = new_msg.replace("\r\n", "\n")[:-1]
                        reply = response.split("\n")[-1]
                        if message == reply:
                            return 1
                        else:
                            return 0
                    except (pexpect.TIMEOUT, pexpect.EOF) as e:
                        return 0
                else:
                    try:
                        client_handle.expect('Enter message:', timeout=4)
                        return 0
                    except pexpect.TIMEOUT:
                        return 0
                    except pexpect.EOF:
                        return 1

            def start_client(client_number):
                client = pexpect.spawn("python3 %s c localhost %d" % (prog_path,port_number))
                print("client connect to", port_number)
                try:
                    i = client.expect(["Enter message:", 'Connection refused'], timeout=4)
                    if i == 1:
                        print('Client%d: Connection refused.' % client_number)
                        return 0
                except (pexpect.TIMEOUT, pexpect.EOF) as e:
                        print(e)
                        print('Client%d: cannot connect to the server.' % client_number)
                        return 0
                results = [1, 1, 1, 1]
                for i in range(len(messages)):
                    message_list = [messages[i]]
                    if i == 2:
                        message_list = []
                        for j in range(20):
                            message_list.append(''.join(random.SystemRandom().choice(string.ascii_uppercase + string.digits) for _ in range(512)))
                    for message in message_list:
                        result = send_message(client, message)
                        if result == 0:
                            results[i] = 0
                            break
                for i in range(4):
                    if i == 0:
                        print('Client%d:  Testing short word:' % client_number,end=" ")
                    elif i == 1:
                        print('          Testing long sentence:', end=" ")
                    elif i == 2:
                        print('          Testing multiple sentences:', end=" ")
                    else:
                        print('          Testing EOF:', end=" ")
                    if results[i] == 0:
                        print('\033[1;31;40mFAILED\033[0m')
                    else:
                        print('\033[1;32;40mPASSED\033[0m')
                client.close()
                if sum(results) == 4:
                    return 1
                else:
                    return 0

            if start_client(1):
                start_client(2)

            server.close()

